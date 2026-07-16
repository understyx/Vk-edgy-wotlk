#include "OverlayUI.h"
#include <cstdio>
#include <cstring>

namespace WoTLKGuiLayer {

OverlayUI::~OverlayUI()
{
    if (m_ready)
        Shutdown();
}

bool OverlayUI::Initialize(const InitInfo& info)
{
    m_device   = info.device;
    m_dispatch = info.dispatch;

    // ---- RmlUi interfaces ----
    m_sysInterface  = std::make_unique<RmlSystemInterface>();
    m_fileInterface = std::make_unique<RmlFileInterface>(info.uiDir);

    m_renderer = std::make_unique<VulkanRenderer>();

    VulkanRenderer::InitInfo ri{};
    ri.device                    = info.device;
    ri.physicalDevice            = info.physicalDevice;
    ri.dispatch                  = info.dispatch;
    ri.graphicsQueueFamilyIndex  = info.graphicsQueueFamilyIndex;
    ri.shaderDir                 = info.shaderDir;

    if (!m_renderer->Initialize(ri)) {
        fprintf(stderr, "[RmlUi] VulkanRenderer::Initialize failed\n");
        return false;
    }

    // Provide the graphics queue so the renderer can upload textures
    m_renderer->SetupWithQueue(info.graphicsQueue);

    // ---- Bootstrap RmlUi ----
    Rml::SetSystemInterface(m_sysInterface.get());
    Rml::SetFileInterface(m_fileInterface.get());
    Rml::SetRenderInterface(m_renderer.get());

    if (!Rml::Initialise()) {
        fprintf(stderr, "[RmlUi] Rml::Initialise() failed\n");
        return false;
    }

    // Load default font (optional; overlay will render without text if unavailable)
    if (!info.fontPath.empty()) {
        if (!Rml::LoadFontFace(info.fontPath)) {
            fprintf(stderr, "[RmlUi] Warning: could not load font %s\n",
                    info.fontPath.c_str());
        }
    }

    m_ready = true;
    return true;
}

void OverlayUI::Shutdown()
{
    if (!m_ready) return;

    m_dispatch->DeviceWaitIdle(m_device);

    if (m_document) {
        m_document->Close();
        m_document = nullptr;
    }
    if (m_context) {
        Rml::RemoveContext(m_context->GetName());
        m_context = nullptr;
    }

    Rml::Shutdown();

    DestroySyncObjects();
    DestroySwapchainResources();

    if (m_renderer) {
        m_renderer->Shutdown();
        m_renderer.reset();
    }

    m_sysInterface.reset();
    m_fileInterface.reset();
    m_ready = false;
}

// ============================================================================
// Swapchain lifecycle
// ============================================================================

void OverlayUI::ResizeSwapchain(const std::vector<VkImage>& images,
                                 VkFormat                    format,
                                 VkExtent2D                  extent)
{
    if (!m_ready) return;

    m_extent = extent;
    m_renderer->ResizeSwapchain(images, format, extent);

    // Create or recreate the RmlUi context with the new dimensions
    if (m_document) {
        m_document->Close();
        m_document = nullptr;
    }
    if (m_context) {
        Rml::RemoveContext(m_context->GetName());
        m_context = nullptr;
    }

    m_context = Rml::CreateContext("overlay",
        Rml::Vector2i(static_cast<int>(extent.width),
                      static_cast<int>(extent.height)));
    if (!m_context) {
        fprintf(stderr, "[RmlUi] Failed to create RmlUi context\n");
        return;
    }

    // Load the overlay document
    m_document = m_context->LoadDocument("overlay.rml");
    if (!m_document) {
        fprintf(stderr, "[RmlUi] Failed to load overlay.rml\n");
        return;
    }
    m_document->Show();

    // Recreate sync objects for the new image count
    DestroySyncObjects();
    if (CreateSyncObjects(static_cast<uint32_t>(images.size())) != VK_SUCCESS) {
        fprintf(stderr, "[RmlUi] Failed to create sync objects\n");
    }
}

void OverlayUI::DestroySwapchainResources()
{
    if (m_renderer)
        m_renderer->DestroySwapchainResources();
    DestroySyncObjects();
}

// ============================================================================
// Per-frame render
// ============================================================================

VkSemaphore OverlayUI::Render(VkQueue                         queue,
                               uint32_t                        imageIndex,
                               const std::vector<VkSemaphore>& waitSems)
{
    if (!m_ready || !m_context || imageIndex >= m_overlaySemaphores.size())
        return VK_NULL_HANDLE;

    VkFence     fence     = m_fences[imageIndex];
    VkSemaphore signalSem = m_overlaySemaphores[imageIndex];

    // Wait for the previous use of this image's resources to finish
    m_dispatch->WaitForFences(m_device, 1, &fence, VK_TRUE, UINT64_MAX);
    m_dispatch->ResetFences(m_device, 1, &fence);

    // Update RmlUi (runs layout, animations, etc.)
    m_context->Update();

    // Prepare the Vulkan command buffer for this image
    m_renderer->SetCurrentImageIndex(imageIndex);
    VkCommandBuffer cmd = m_renderer->PrepareFrame();
    if (!cmd) return VK_NULL_HANDLE;

    // Render all RmlUi elements (calls CompileGeometry / RenderGeometry via our interface)
    m_context->Render();

    // End the render pass, end the command buffer, and submit with semaphore chain
    m_renderer->SubmitFrame(queue, waitSems, signalSem, fence);

    return signalSem;
}

// ============================================================================
// Sync object management
// ============================================================================

VkResult OverlayUI::CreateSyncObjects(uint32_t imageCount)
{
    m_overlaySemaphores.resize(imageCount, VK_NULL_HANDLE);
    m_fences.resize(imageCount, VK_NULL_HANDLE);

    VkSemaphoreCreateInfo sci{};
    sci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fci{};
    fci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fci.flags = VK_FENCE_CREATE_SIGNALED_BIT;  // Start signaled so first frame doesn't wait

    for (uint32_t i = 0; i < imageCount; ++i) {
        VkResult r = m_dispatch->CreateSemaphore(m_device, &sci, nullptr,
                                                 &m_overlaySemaphores[i]);
        if (r != VK_SUCCESS) return r;

        r = m_dispatch->CreateFence(m_device, &fci, nullptr, &m_fences[i]);
        if (r != VK_SUCCESS) return r;
    }
    return VK_SUCCESS;
}

void OverlayUI::DestroySyncObjects()
{
    for (auto sem : m_overlaySemaphores)
        if (sem) m_dispatch->DestroySemaphore(m_device, sem, nullptr);
    m_overlaySemaphores.clear();

    for (auto fence : m_fences)
        if (fence) m_dispatch->DestroyFence(m_device, fence, nullptr);
    m_fences.clear();
}

} // namespace WoTLKGuiLayer
