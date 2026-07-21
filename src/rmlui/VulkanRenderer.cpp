#include "VulkanRenderer.h"

#include <array>
#include <cstring>
#include <cstdio>
#include <fstream>
#include <vector>

namespace WoTLKGuiLayer {

// ============================================================================
// Module-local helpers
// ============================================================================

static std::vector<char> LoadSpv(const std::string& path)
{
    std::ifstream file(path, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        fprintf(stderr, "[RmlUi] Cannot open shader: %s\n", path.c_str());
        return {};
    }
    size_t size = static_cast<size_t>(file.tellg());
    std::vector<char> buf(size);
    file.seekg(0);
    file.read(buf.data(), static_cast<std::streamsize>(size));
    return buf;
}

/// Build a column-major 2-D orthographic projection for Vulkan NDC.
static void OrthoProjection(float w, float h, float out[16])
{
    // Maps x ∈ [0,w] → [-1,1], y ∈ [0,h] → [-1,1].
    // For Vulkan, Y-axis points downwards, so y=0 is top (-1), y=h is bottom (1).
    const float m[16] = {
        2.f / w,  0,        0,  0,
        0,        2.f / h,  0,  0,
        0,        0,        1,  0,
       -1,       -1,        0,  1,
    };
    memcpy(out, m, 64);
}

static constexpr float kIdentity[16] = {
    1, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, 1, 0,
    0, 0, 0, 1,
};

// ============================================================================
// Destructor / lifecycle
// ============================================================================

VulkanRenderer::~VulkanRenderer()
{
    if (m_initialized)
        Shutdown();
}

bool VulkanRenderer::Initialize(const InitInfo& info)
{
    m_device         = info.device;
    m_physicalDevice = info.physicalDevice;
    m_dispatch       = info.dispatch;
    m_shaderDir      = info.shaderDir;

    memcpy(m_push.transform, kIdentity, sizeof(kIdentity));

    if (CreateCommandPool(info.graphicsQueueFamilyIndex) != VK_SUCCESS) return false;
    if (CreateDescriptorSetLayout() != VK_SUCCESS)                      return false;
    if (CreatePipelineLayout() != VK_SUCCESS)                           return false;
    if (CreateSampler() != VK_SUCCESS)                                  return false;
    if (CreateDescriptorPool() != VK_SUCCESS)                           return false;
    if (CreateWhiteTextureResources() != VK_SUCCESS)                    return false;

    m_initialized = true;
    return true;
}

void VulkanRenderer::SetupWithQueue(VkQueue queue)
{
    m_uploadQueue = queue;
    UploadWhiteTexture();
    CreateTextureDescriptorSet(m_whiteView, m_whiteDescSet);
    m_whiteReady = true;

    // Pipeline creation is deferred here because it requires m_renderPass,
    // which is set by ResizeSwapchain(). If ResizeSwapchain() was called
    // before SetupWithQueue(), create the pipeline now.
    if (m_pipeline == VK_NULL_HANDLE && m_renderPass != VK_NULL_HANDLE) {
        if (CreatePipeline() != VK_SUCCESS)
            fprintf(stderr, "[RmlUi] Failed to create graphics pipeline\n");
    }
}

void VulkanRenderer::Shutdown()
{
    if (!m_device) return;
    m_dispatch->DeviceWaitIdle(m_device);

    DestroySwapchainResources();

    if (m_pipeline != VK_NULL_HANDLE) {
        m_dispatch->DestroyPipeline(m_device, m_pipeline, nullptr);
        m_pipeline = VK_NULL_HANDLE;
    }
    if (m_renderPass != VK_NULL_HANDLE) {
        m_dispatch->DestroyRenderPass(m_device, m_renderPass, nullptr);
        m_renderPass = VK_NULL_HANDLE;
    }

    if (m_whiteView != VK_NULL_HANDLE) {
        m_dispatch->DestroyImageView(m_device, m_whiteView, nullptr);
        m_whiteView = VK_NULL_HANDLE;
    }
    if (m_whiteImage != VK_NULL_HANDLE) {
        m_dispatch->DestroyImage(m_device, m_whiteImage, nullptr);
        m_whiteImage = VK_NULL_HANDLE;
    }
    if (m_whiteMemory != VK_NULL_HANDLE) {
        m_dispatch->FreeMemory(m_device, m_whiteMemory, nullptr);
        m_whiteMemory = VK_NULL_HANDLE;
    }
    // whiteDescSet freed with pool
    m_whiteDescSet = VK_NULL_HANDLE;

    if (m_descPool != VK_NULL_HANDLE) {
        m_dispatch->DestroyDescriptorPool(m_device, m_descPool, nullptr);
        m_descPool = VK_NULL_HANDLE;
    }
    if (m_sampler != VK_NULL_HANDLE) {
        m_dispatch->DestroySampler(m_device, m_sampler, nullptr);
        m_sampler = VK_NULL_HANDLE;
    }
    if (m_pipelineLayout != VK_NULL_HANDLE) {
        m_dispatch->DestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
        m_pipelineLayout = VK_NULL_HANDLE;
    }
    if (m_descSetLayout != VK_NULL_HANDLE) {
        m_dispatch->DestroyDescriptorSetLayout(m_device, m_descSetLayout, nullptr);
        m_descSetLayout = VK_NULL_HANDLE;
    }
    if (m_commandPool != VK_NULL_HANDLE) {
        m_dispatch->DestroyCommandPool(m_device, m_commandPool, nullptr);
        m_commandPool = VK_NULL_HANDLE;
    }

    m_initialized = false;
}

// ============================================================================
// Swapchain lifecycle
// ============================================================================

void VulkanRenderer::ResizeSwapchain(const std::vector<VkImage>& images,
                                      VkFormat                    format,
                                      VkExtent2D                  extent)
{
    DestroySwapchainResources();

    m_swapchainFormat = format;
    m_swapchainExtent = extent;

    // Recreate render pass when format changes
    if (m_renderPass != VK_NULL_HANDLE) {
        m_dispatch->DestroyRenderPass(m_device, m_renderPass, nullptr);
        m_renderPass = VK_NULL_HANDLE;
    }
    if (CreateRenderPass() != VK_SUCCESS) {
        fprintf(stderr, "[RmlUi] Failed to create render pass\n");
        return;
    }

    // Recreate pipeline (needs render pass)
    if (m_pipeline != VK_NULL_HANDLE) {
        m_dispatch->DestroyPipeline(m_device, m_pipeline, nullptr);
        m_pipeline = VK_NULL_HANDLE;
    }
    // Only create the pipeline once we have a queue (SetupWithQueue may not have been called yet)
    if (m_uploadQueue != VK_NULL_HANDLE) {
        if (CreatePipeline() != VK_SUCCESS)
            fprintf(stderr, "[RmlUi] Failed to create pipeline on resize\n");
    }

    m_frames.resize(images.size());

    // Allocate command buffers
    {
        VkCommandBufferAllocateInfo ai{};
        ai.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        ai.commandPool        = m_commandPool;
        ai.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        ai.commandBufferCount = static_cast<uint32_t>(images.size());

        std::vector<VkCommandBuffer> cmds(images.size());
        if (m_dispatch->AllocateCommandBuffers(m_device, &ai, cmds.data()) != VK_SUCCESS) {
            fprintf(stderr, "[RmlUi] Failed to allocate command buffers\n");
            return;
        }
        for (size_t i = 0; i < images.size(); ++i)
            m_frames[i].cmdBuf = cmds[i];
    }

    for (size_t i = 0; i < images.size(); ++i) {
        // Image view
        VkImageViewCreateInfo ivci{};
        ivci.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        ivci.image                           = images[i];
        ivci.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
        ivci.format                          = format;
        ivci.components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        ivci.components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        ivci.components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        ivci.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        ivci.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        ivci.subresourceRange.baseMipLevel   = 0;
        ivci.subresourceRange.levelCount     = 1;
        ivci.subresourceRange.baseArrayLayer = 0;
        ivci.subresourceRange.layerCount     = 1;

        if (m_dispatch->CreateImageView(m_device, &ivci, nullptr, &m_frames[i].imageView) != VK_SUCCESS)
            return;

        // Framebuffer
        VkFramebufferCreateInfo fci{};
        fci.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fci.renderPass      = m_renderPass;
        fci.attachmentCount = 1;
        fci.pAttachments    = &m_frames[i].imageView;
        fci.width           = extent.width;
        fci.height          = extent.height;
        fci.layers          = 1;

        if (m_dispatch->CreateFramebuffer(m_device, &fci, nullptr, &m_frames[i].framebuffer) != VK_SUCCESS)
            return;
    }
}

void VulkanRenderer::DestroySwapchainResources()
{
    if (!m_device || m_frames.empty()) return;

    // Free command buffers
    std::vector<VkCommandBuffer> cmds;
    for (auto& f : m_frames)
        if (f.cmdBuf) cmds.push_back(f.cmdBuf);

    if (!cmds.empty())
        m_dispatch->FreeCommandBuffers(m_device, m_commandPool,
                                       static_cast<uint32_t>(cmds.size()),
                                       cmds.data());

    for (auto& f : m_frames) {
        if (f.framebuffer)
            m_dispatch->DestroyFramebuffer(m_device, f.framebuffer, nullptr);
        if (f.imageView)
            m_dispatch->DestroyImageView(m_device, f.imageView, nullptr);
    }
    m_frames.clear();
    m_currentCmdBuf = VK_NULL_HANDLE;
}

// ============================================================================
// Per-frame control
// ============================================================================

void VulkanRenderer::SetCurrentImageIndex(uint32_t imageIndex)
{
    m_currentImage = imageIndex;
}

VkCommandBuffer VulkanRenderer::PrepareFrame()
{
    if (m_pipeline == VK_NULL_HANDLE || m_currentImage >= m_frames.size())
        return VK_NULL_HANDLE;

    FrameData& frame = m_frames[m_currentImage];
    m_currentCmdBuf  = frame.cmdBuf;

    m_dispatch->ResetCommandBuffer(m_currentCmdBuf, 0);

    VkCommandBufferBeginInfo bi{};
    bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    if (m_dispatch->BeginCommandBuffer(m_currentCmdBuf, &bi) != VK_SUCCESS)
        return VK_NULL_HANDLE;

    VkRenderPassBeginInfo rpbi{};
    rpbi.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpbi.renderPass        = m_renderPass;
    rpbi.framebuffer       = frame.framebuffer;
    rpbi.renderArea.offset = {0, 0};
    rpbi.renderArea.extent = m_swapchainExtent;
    // No clear value – loadOp is LOAD

    m_dispatch->CmdBeginRenderPass(m_currentCmdBuf, &rpbi, VK_SUBPASS_CONTENTS_INLINE);
    m_dispatch->CmdBindPipeline(m_currentCmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

    VkViewport vp{};
    vp.x        = 0;
    vp.y        = 0;
    vp.width    = static_cast<float>(m_swapchainExtent.width);
    vp.height   = static_cast<float>(m_swapchainExtent.height);
    vp.minDepth = 0.f;
    vp.maxDepth = 1.f;
    m_dispatch->CmdSetViewport(m_currentCmdBuf, 0, 1, &vp);

    VkRect2D scissor{{0, 0}, m_swapchainExtent};
    m_dispatch->CmdSetScissor(m_currentCmdBuf, 0, 1, &scissor);

    // Reset per-frame state
    m_scissorActive = false;
    OrthoProjection(static_cast<float>(m_swapchainExtent.width),
                    static_cast<float>(m_swapchainExtent.height),
                    m_push.transform);
    m_push.translate[0] = 0;
    m_push.translate[1] = 0;
    m_push.hasTexture   = 0;

    return m_currentCmdBuf;
}

void VulkanRenderer::SubmitFrame(VkQueue                         queue,
                                  const std::vector<VkSemaphore>& waitSemaphores,
                                  VkSemaphore                     signalSemaphore,
                                  VkFence                         fence)
{
    if (!m_currentCmdBuf) return;

    m_dispatch->CmdEndRenderPass(m_currentCmdBuf);
    m_dispatch->EndCommandBuffer(m_currentCmdBuf);

    std::vector<VkPipelineStageFlags> waitStages(
        waitSemaphores.size(), VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

    VkSubmitInfo si{};
    si.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    si.waitSemaphoreCount   = static_cast<uint32_t>(waitSemaphores.size());
    si.pWaitSemaphores      = waitSemaphores.empty() ? nullptr : waitSemaphores.data();
    si.pWaitDstStageMask    = waitStages.empty()     ? nullptr : waitStages.data();
    si.commandBufferCount   = 1;
    si.pCommandBuffers      = &m_currentCmdBuf;
    si.signalSemaphoreCount = (signalSemaphore != VK_NULL_HANDLE) ? 1u : 0u;
    si.pSignalSemaphores    = (signalSemaphore != VK_NULL_HANDLE) ? &signalSemaphore : nullptr;

    m_dispatch->QueueSubmit(queue, 1, &si, fence);
    m_currentCmdBuf = VK_NULL_HANDLE;
}

// ============================================================================
// Rml::RenderInterface – Geometry
// ============================================================================

Rml::CompiledGeometryHandle VulkanRenderer::CompileGeometry(
    Rml::Span<const Rml::Vertex> vertices,
    Rml::Span<const int>         indices)
{
    auto* geo = new GeometryBuffer();
    geo->indexCount = static_cast<uint32_t>(indices.size());

    VkDeviceSize vSize = vertices.size() * sizeof(Rml::Vertex);
    VkDeviceSize iSize = indices.size()  * sizeof(int);

    const VkMemoryPropertyFlags hostProps =
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    if (CreateDeviceBuffer(vSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, hostProps,
                           geo->vertexBuffer, geo->vertexMemory) != VK_SUCCESS ||
        UploadToBuffer(geo->vertexMemory, vertices.data(), vSize) != VK_SUCCESS) {
        if (geo->vertexBuffer) m_dispatch->DestroyBuffer(m_device, geo->vertexBuffer, nullptr);
        if (geo->vertexMemory) m_dispatch->FreeMemory(m_device, geo->vertexMemory, nullptr);
        delete geo;
        return 0;
    }

    if (CreateDeviceBuffer(iSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, hostProps,
                           geo->indexBuffer, geo->indexMemory) != VK_SUCCESS ||
        UploadToBuffer(geo->indexMemory, indices.data(), iSize) != VK_SUCCESS) {
        m_dispatch->DestroyBuffer(m_device, geo->vertexBuffer, nullptr);
        m_dispatch->FreeMemory(m_device,    geo->vertexMemory, nullptr);
        if (geo->indexBuffer) m_dispatch->DestroyBuffer(m_device, geo->indexBuffer, nullptr);
        if (geo->indexMemory) m_dispatch->FreeMemory(m_device, geo->indexMemory, nullptr);
        delete geo;
        return 0;
    }

    return reinterpret_cast<Rml::CompiledGeometryHandle>(geo);
}

void VulkanRenderer::RenderGeometry(Rml::CompiledGeometryHandle geometry,
                                     Rml::Vector2f               translation,
                                     Rml::TextureHandle          texture)
{
    if (!m_currentCmdBuf || !geometry) return;

    auto* geo = reinterpret_cast<GeometryBuffer*>(geometry);

    m_push.translate[0] = translation.x;
    m_push.translate[1] = translation.y;
    m_push.hasTexture   = (texture != 0) ? 1u : 0u;

    m_dispatch->CmdPushConstants(
        m_currentCmdBuf, m_pipelineLayout,
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        0, sizeof(PushConstants), &m_push);

    VkDescriptorSet descSet = m_whiteDescSet;
    if (texture != 0) {
        auto* tex = reinterpret_cast<TextureData*>(texture);
        descSet = tex->descSet;
    }

    m_dispatch->CmdBindDescriptorSets(
        m_currentCmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS,
        m_pipelineLayout, 0, 1, &descSet, 0, nullptr);

    VkDeviceSize offset = 0;
    m_dispatch->CmdBindVertexBuffers(m_currentCmdBuf, 0, 1, &geo->vertexBuffer, &offset);
    m_dispatch->CmdBindIndexBuffer(m_currentCmdBuf, geo->indexBuffer, 0, VK_INDEX_TYPE_UINT32);
    m_dispatch->CmdDrawIndexed(m_currentCmdBuf, geo->indexCount, 1, 0, 0, 0);
}

void VulkanRenderer::ReleaseGeometry(Rml::CompiledGeometryHandle geometry)
{
    if (!geometry) return;
    auto* geo = reinterpret_cast<GeometryBuffer*>(geometry);

    m_dispatch->DeviceWaitIdle(m_device);
    m_dispatch->DestroyBuffer(m_device, geo->vertexBuffer, nullptr);
    m_dispatch->FreeMemory(m_device,    geo->vertexMemory, nullptr);
    m_dispatch->DestroyBuffer(m_device, geo->indexBuffer,  nullptr);
    m_dispatch->FreeMemory(m_device,    geo->indexMemory,  nullptr);
    delete geo;
}

// ============================================================================
// Rml::RenderInterface – Textures
// ============================================================================

Rml::TextureHandle VulkanRenderer::LoadTexture(Rml::Vector2i&      /*texture_dimensions*/,
                                               const Rml::String&  /*source*/)
{
    // Returning 0 causes RmlUi to decode the file and call GenerateTexture.
    return 0;
}

Rml::TextureHandle VulkanRenderer::GenerateTexture(Rml::Span<const Rml::byte> source,
                                                    Rml::Vector2i              source_dimensions)
{
    if (source_dimensions.x <= 0 || source_dimensions.y <= 0) return 0;

    auto* tex = new TextureData();
    uint32_t w = static_cast<uint32_t>(source_dimensions.x);
    uint32_t h = static_cast<uint32_t>(source_dimensions.y);

    if (CreateDeviceImage(w, h, VK_FORMAT_R8G8B8A8_SRGB,
                          VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                          tex->image, tex->memory) != VK_SUCCESS) {
        delete tex;
        return 0;
    }

    if (UploadImageData(tex->image, w, h,
                        source.data(), static_cast<VkDeviceSize>(source.size())) != VK_SUCCESS) {
        m_dispatch->DestroyImage(m_device, tex->image, nullptr);
        m_dispatch->FreeMemory(m_device,   tex->memory, nullptr);
        delete tex;
        return 0;
    }

    VkImageViewCreateInfo ivci{};
    ivci.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    ivci.image                           = tex->image;
    ivci.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
    ivci.format                          = VK_FORMAT_R8G8B8A8_SRGB;
    ivci.components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    ivci.components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    ivci.components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    ivci.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    ivci.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    ivci.subresourceRange.baseMipLevel   = 0;
    ivci.subresourceRange.levelCount     = 1;
    ivci.subresourceRange.baseArrayLayer = 0;
    ivci.subresourceRange.layerCount     = 1;

    if (m_dispatch->CreateImageView(m_device, &ivci, nullptr, &tex->view) != VK_SUCCESS ||
        CreateTextureDescriptorSet(tex->view, tex->descSet) != VK_SUCCESS) {
        if (tex->view) m_dispatch->DestroyImageView(m_device, tex->view, nullptr);
        m_dispatch->DestroyImage(m_device, tex->image, nullptr);
        m_dispatch->FreeMemory(m_device,   tex->memory, nullptr);
        delete tex;
        return 0;
    }

    return reinterpret_cast<Rml::TextureHandle>(tex);
}

void VulkanRenderer::ReleaseTexture(Rml::TextureHandle texture_handle)
{
    if (!texture_handle) return;
    auto* tex = reinterpret_cast<TextureData*>(texture_handle);

    m_dispatch->DeviceWaitIdle(m_device);
    // descSet freed with pool
    m_dispatch->DestroyImageView(m_device, tex->view,   nullptr);
    m_dispatch->DestroyImage(m_device,     tex->image,  nullptr);
    m_dispatch->FreeMemory(m_device,       tex->memory, nullptr);
    delete tex;
}

// ============================================================================
// Rml::RenderInterface – Scissor & Transform
// ============================================================================

void VulkanRenderer::EnableScissorRegion(bool enable)
{
    m_scissorActive = enable;
    if (!m_currentCmdBuf) return;

    if (!enable) {
        VkRect2D full{{0, 0}, m_swapchainExtent};
        m_dispatch->CmdSetScissor(m_currentCmdBuf, 0, 1, &full);
    } else {
        // Re-apply the last region
        VkRect2D scissor{};
        scissor.offset.x      = m_scissorRegion.Left();
        scissor.offset.y      = m_scissorRegion.Top();
        scissor.extent.width  = static_cast<uint32_t>(m_scissorRegion.Width());
        scissor.extent.height = static_cast<uint32_t>(m_scissorRegion.Height());
        m_dispatch->CmdSetScissor(m_currentCmdBuf, 0, 1, &scissor);
    }
}

void VulkanRenderer::SetScissorRegion(Rml::Rectanglei region)
{
    m_scissorRegion = region;
    if (!m_currentCmdBuf || !m_scissorActive) return;

    VkRect2D scissor{};
    scissor.offset.x      = region.Left();
    scissor.offset.y      = region.Top();
    scissor.extent.width  = static_cast<uint32_t>(region.Width());
    scissor.extent.height = static_cast<uint32_t>(region.Height());
    m_dispatch->CmdSetScissor(m_currentCmdBuf, 0, 1, &scissor);
}

void VulkanRenderer::SetTransform(const Rml::Matrix4f* transform)
{
    float proj_m[16];
    OrthoProjection(static_cast<float>(m_swapchainExtent.width),
                    static_cast<float>(m_swapchainExtent.height),
                    proj_m);

    if (transform) {
        Rml::Matrix4f projection = Rml::Matrix4f::FromColumnMajor(proj_m);
        Rml::Matrix4f combined = projection * (*transform);
        memcpy(m_push.transform, combined.data(), 64);
    } else {
        // Restore the default orthographic projection
        memcpy(m_push.transform, proj_m, 64);
    }
}

// ============================================================================
// GPU resource helpers
// ============================================================================

uint32_t VulkanRenderer::FindMemoryType(uint32_t              type_filter,
                                         VkMemoryPropertyFlags props) const
{
    VkPhysicalDeviceMemoryProperties memProps{};
    m_dispatch->GetPhysicalDeviceMemoryProperties(m_physicalDevice, &memProps);

    for (uint32_t i = 0; i < memProps.memoryTypeCount; ++i) {
        if ((type_filter & (1u << i)) &&
            (memProps.memoryTypes[i].propertyFlags & props) == props)
            return i;
    }
    return UINT32_MAX;
}

VkResult VulkanRenderer::CreateDeviceBuffer(VkDeviceSize          size,
                                              VkBufferUsageFlags    usage,
                                              VkMemoryPropertyFlags props,
                                              VkBuffer&             out_buffer,
                                              VkDeviceMemory&       out_memory) const
{
    VkBufferCreateInfo bci{};
    bci.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bci.size        = size;
    bci.usage       = usage;
    bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkResult r = m_dispatch->CreateBuffer(m_device, &bci, nullptr, &out_buffer);
    if (r != VK_SUCCESS) return r;

    VkMemoryRequirements req{};
    m_dispatch->GetBufferMemoryRequirements(m_device, out_buffer, &req);

    VkMemoryAllocateInfo mai{};
    mai.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    mai.allocationSize  = req.size;
    mai.memoryTypeIndex = FindMemoryType(req.memoryTypeBits, props);
    if (mai.memoryTypeIndex == UINT32_MAX) {
        m_dispatch->DestroyBuffer(m_device, out_buffer, nullptr);
        out_buffer = VK_NULL_HANDLE;
        return VK_ERROR_OUT_OF_DEVICE_MEMORY;
    }

    r = m_dispatch->AllocateMemory(m_device, &mai, nullptr, &out_memory);
    if (r != VK_SUCCESS) {
        m_dispatch->DestroyBuffer(m_device, out_buffer, nullptr);
        out_buffer = VK_NULL_HANDLE;
        return r;
    }

    r = m_dispatch->BindBufferMemory(m_device, out_buffer, out_memory, 0);
    if (r != VK_SUCCESS) {
        m_dispatch->FreeMemory(m_device, out_memory, nullptr);
        m_dispatch->DestroyBuffer(m_device, out_buffer, nullptr);
        out_buffer = VK_NULL_HANDLE;
        out_memory = VK_NULL_HANDLE;
    }
    return r;
}

VkResult VulkanRenderer::UploadToBuffer(VkDeviceMemory memory,
                                         const void*    data,
                                         VkDeviceSize   size) const
{
    void* mapped = nullptr;
    VkResult r = m_dispatch->MapMemory(m_device, memory, 0, size, 0, &mapped);
    if (r != VK_SUCCESS) return r;
    memcpy(mapped, data, static_cast<size_t>(size));
    m_dispatch->UnmapMemory(m_device, memory);
    return VK_SUCCESS;
}

VkResult VulkanRenderer::CreateDeviceImage(uint32_t          width,
                                            uint32_t          height,
                                            VkFormat          format,
                                            VkImageUsageFlags usage,
                                            VkImage&          out_image,
                                            VkDeviceMemory&   out_memory) const
{
    VkImageCreateInfo ici{};
    ici.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ici.imageType     = VK_IMAGE_TYPE_2D;
    ici.format        = format;
    ici.extent        = {width, height, 1};
    ici.mipLevels     = 1;
    ici.arrayLayers   = 1;
    ici.samples       = VK_SAMPLE_COUNT_1_BIT;
    ici.tiling        = VK_IMAGE_TILING_OPTIMAL;
    ici.usage         = usage;
    ici.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
    ici.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VkResult r = m_dispatch->CreateImage(m_device, &ici, nullptr, &out_image);
    if (r != VK_SUCCESS) return r;

    VkMemoryRequirements req{};
    m_dispatch->GetImageMemoryRequirements(m_device, out_image, &req);

    VkMemoryAllocateInfo mai{};
    mai.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    mai.allocationSize  = req.size;
    mai.memoryTypeIndex = FindMemoryType(req.memoryTypeBits,
                                         VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (mai.memoryTypeIndex == UINT32_MAX) {
        m_dispatch->DestroyImage(m_device, out_image, nullptr);
        out_image = VK_NULL_HANDLE;
        return VK_ERROR_OUT_OF_DEVICE_MEMORY;
    }

    r = m_dispatch->AllocateMemory(m_device, &mai, nullptr, &out_memory);
    if (r != VK_SUCCESS) {
        m_dispatch->DestroyImage(m_device, out_image, nullptr);
        out_image = VK_NULL_HANDLE;
        return r;
    }

    r = m_dispatch->BindImageMemory(m_device, out_image, out_memory, 0);
    if (r != VK_SUCCESS) {
        m_dispatch->FreeMemory(m_device, out_memory, nullptr);
        m_dispatch->DestroyImage(m_device, out_image, nullptr);
        out_image  = VK_NULL_HANDLE;
        out_memory = VK_NULL_HANDLE;
    }
    return r;
}

VkResult VulkanRenderer::UploadImageData(VkImage      image,
                                          uint32_t     width,
                                          uint32_t     height,
                                          const void*  pixels,
                                          VkDeviceSize pixel_bytes)
{
    // Staging buffer
    VkBuffer       stagingBuf = VK_NULL_HANDLE;
    VkDeviceMemory stagingMem = VK_NULL_HANDLE;

    const VkMemoryPropertyFlags hostProps =
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    VkResult r = CreateDeviceBuffer(pixel_bytes, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                    hostProps, stagingBuf, stagingMem);
    if (r != VK_SUCCESS) return r;
    UploadToBuffer(stagingMem, pixels, pixel_bytes);

    // One-time command buffer
    VkCommandBufferAllocateInfo ai{};
    ai.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    ai.commandPool        = m_commandPool;
    ai.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    ai.commandBufferCount = 1;

    VkCommandBuffer cmd = VK_NULL_HANDLE;
    r = m_dispatch->AllocateCommandBuffers(m_device, &ai, &cmd);
    if (r != VK_SUCCESS) {
        m_dispatch->DestroyBuffer(m_device, stagingBuf, nullptr);
        m_dispatch->FreeMemory(m_device, stagingMem, nullptr);
        return r;
    }

    VkCommandBufferBeginInfo bi{};
    bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    m_dispatch->BeginCommandBuffer(cmd, &bi);

    auto barrier = [&](VkImageLayout oldLayout, VkImageLayout newLayout,
                       VkAccessFlags srcAccess, VkAccessFlags dstAccess,
                       VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage) {
        VkImageMemoryBarrier b{};
        b.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        b.oldLayout                       = oldLayout;
        b.newLayout                       = newLayout;
        b.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        b.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        b.image                           = image;
        b.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        b.subresourceRange.baseMipLevel   = 0;
        b.subresourceRange.levelCount     = 1;
        b.subresourceRange.baseArrayLayer = 0;
        b.subresourceRange.layerCount     = 1;
        b.srcAccessMask                   = srcAccess;
        b.dstAccessMask                   = dstAccess;
        m_dispatch->CmdPipelineBarrier(cmd, srcStage, dstStage,
                                       0, 0, nullptr, 0, nullptr, 1, &b);
    };

    barrier(VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            0,
            VK_ACCESS_TRANSFER_WRITE_BIT,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT);

    VkBufferImageCopy region{};
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.layerCount = 1;
    region.imageExtent                 = {width, height, 1};

    m_dispatch->CmdCopyBufferToImage(cmd, stagingBuf, image,
                                     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                     1, &region);

    barrier(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_ACCESS_TRANSFER_WRITE_BIT,
            VK_ACCESS_SHADER_READ_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

    m_dispatch->EndCommandBuffer(cmd);

    VkSubmitInfo si{};
    si.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    si.commandBufferCount = 1;
    si.pCommandBuffers    = &cmd;

    m_dispatch->QueueSubmit(m_uploadQueue, 1, &si, VK_NULL_HANDLE);
    m_dispatch->QueueWaitIdle(m_uploadQueue);

    m_dispatch->FreeCommandBuffers(m_device, m_commandPool, 1, &cmd);
    m_dispatch->DestroyBuffer(m_device, stagingBuf, nullptr);
    m_dispatch->FreeMemory(m_device,    stagingMem, nullptr);
    return VK_SUCCESS;
}

VkResult VulkanRenderer::CreateTextureDescriptorSet(VkImageView     view,
                                                     VkDescriptorSet& out_set) const
{
    VkDescriptorSetAllocateInfo ai{};
    ai.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    ai.descriptorPool     = m_descPool;
    ai.descriptorSetCount = 1;
    ai.pSetLayouts        = &m_descSetLayout;

    VkResult r = m_dispatch->AllocateDescriptorSets(m_device, &ai, &out_set);
    if (r != VK_SUCCESS) return r;

    VkDescriptorImageInfo imgInfo{};
    imgInfo.sampler     = m_sampler;
    imgInfo.imageView   = view;
    imgInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkWriteDescriptorSet write{};
    write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet          = out_set;
    write.dstBinding      = 0;
    write.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.descriptorCount = 1;
    write.pImageInfo      = &imgInfo;

    m_dispatch->UpdateDescriptorSets(m_device, 1, &write, 0, nullptr);
    return VK_SUCCESS;
}

VkResult VulkanRenderer::CreateShaderModule(const std::vector<char>& code,
                                             VkShaderModule&          out) const
{
    if (code.empty()) return VK_ERROR_INITIALIZATION_FAILED;

    VkShaderModuleCreateInfo ci{};
    ci.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    ci.codeSize = code.size();
    ci.pCode    = reinterpret_cast<const uint32_t*>(code.data());

    return m_dispatch->CreateShaderModule(m_device, &ci, nullptr, &out);
}

// ============================================================================
// Init helpers
// ============================================================================

VkResult VulkanRenderer::CreateCommandPool(uint32_t queue_family)
{
    VkCommandPoolCreateInfo ci{};
    ci.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    ci.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    ci.queueFamilyIndex = queue_family;
    return m_dispatch->CreateCommandPool(m_device, &ci, nullptr, &m_commandPool);
}

VkResult VulkanRenderer::CreateDescriptorSetLayout()
{
    VkDescriptorSetLayoutBinding b{};
    b.binding         = 0;
    b.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    b.descriptorCount = 1;
    b.stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo ci{};
    ci.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    ci.bindingCount = 1;
    ci.pBindings    = &b;

    return m_dispatch->CreateDescriptorSetLayout(m_device, &ci, nullptr, &m_descSetLayout);
}

VkResult VulkanRenderer::CreatePipelineLayout()
{
    VkPushConstantRange pcr{};
    pcr.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pcr.offset     = 0;
    pcr.size       = sizeof(PushConstants); // 80 bytes

    VkPipelineLayoutCreateInfo ci{};
    ci.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    ci.setLayoutCount         = 1;
    ci.pSetLayouts            = &m_descSetLayout;
    ci.pushConstantRangeCount = 1;
    ci.pPushConstantRanges    = &pcr;

    return m_dispatch->CreatePipelineLayout(m_device, &ci, nullptr, &m_pipelineLayout);
}

VkResult VulkanRenderer::CreateSampler()
{
    VkSamplerCreateInfo ci{};
    ci.sType        = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    ci.magFilter    = VK_FILTER_LINEAR;
    ci.minFilter    = VK_FILTER_LINEAR;
    ci.mipmapMode   = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    ci.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    ci.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    ci.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    ci.maxLod       = VK_LOD_CLAMP_NONE;

    return m_dispatch->CreateSampler(m_device, &ci, nullptr, &m_sampler);
}

VkResult VulkanRenderer::CreateDescriptorPool()
{
    VkDescriptorPoolSize ps{};
    ps.type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    ps.descriptorCount = 1024;

    VkDescriptorPoolCreateInfo ci{};
    ci.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    ci.maxSets       = 1024;
    ci.poolSizeCount = 1;
    ci.pPoolSizes    = &ps;

    return m_dispatch->CreateDescriptorPool(m_device, &ci, nullptr, &m_descPool);
}

VkResult VulkanRenderer::CreateRenderPass()
{
    // Overlay render pass: LOAD_OP preserves game content; uses PRESENT_SRC_KHR layout.
    VkAttachmentDescription att{};
    att.format         = m_swapchainFormat;
    att.samples        = VK_SAMPLE_COUNT_1_BIT;
    att.loadOp         = VK_ATTACHMENT_LOAD_OP_LOAD;
    att.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    att.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    att.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    att.initialLayout  = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    att.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference ref{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

    VkSubpassDescription sub{};
    sub.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
    sub.colorAttachmentCount = 1;
    sub.pColorAttachments    = &ref;

    VkSubpassDependency dep{};
    dep.srcSubpass    = VK_SUBPASS_EXTERNAL;
    dep.dstSubpass    = 0;
    dep.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dep.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dep.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo ci{};
    ci.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    ci.attachmentCount = 1;
    ci.pAttachments    = &att;
    ci.subpassCount    = 1;
    ci.pSubpasses      = &sub;
    ci.dependencyCount = 1;
    ci.pDependencies   = &dep;

    return m_dispatch->CreateRenderPass(m_device, &ci, nullptr, &m_renderPass);
}

VkResult VulkanRenderer::CreatePipeline()
{
    auto vertCode = LoadSpv(m_shaderDir + "/rmlui.vert.spv");
    auto fragCode = LoadSpv(m_shaderDir + "/rmlui.frag.spv");
    if (vertCode.empty() || fragCode.empty())
        return VK_ERROR_INITIALIZATION_FAILED;

    VkShaderModule vertMod = VK_NULL_HANDLE;
    VkShaderModule fragMod = VK_NULL_HANDLE;

    VkResult r = CreateShaderModule(vertCode, vertMod);
    if (r != VK_SUCCESS) return r;
    r = CreateShaderModule(fragCode, fragMod);
    if (r != VK_SUCCESS) {
        m_dispatch->DestroyShaderModule(m_device, vertMod, nullptr);
        return r;
    }

    // Vertex input: Rml::Vertex = { Vector2f pos, Colourb col, Vector2f uv }
    // Sizes: 8 + 4 + 8 = 20 bytes, no padding.
    VkVertexInputBindingDescription vib{};
    vib.binding   = 0;
    vib.stride    = 20;
    vib.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    std::array<VkVertexInputAttributeDescription, 3> vias{};
    vias[0] = {0, 0, VK_FORMAT_R32G32_SFLOAT,   0};  // position
    vias[1] = {1, 0, VK_FORMAT_R8G8B8A8_UNORM,  8};  // colour  (u8 → float [0,1])
    vias[2] = {2, 0, VK_FORMAT_R32G32_SFLOAT,  12};  // tex_coord

    VkPipelineVertexInputStateCreateInfo vi{};
    vi.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vi.vertexBindingDescriptionCount   = 1;
    vi.pVertexBindingDescriptions      = &vib;
    vi.vertexAttributeDescriptionCount = static_cast<uint32_t>(vias.size());
    vi.pVertexAttributeDescriptions    = vias.data();

    VkPipelineInputAssemblyStateCreateInfo ia{};
    ia.sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineViewportStateCreateInfo vpState{};
    vpState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vpState.viewportCount = 1;
    vpState.scissorCount  = 1;

    VkPipelineRasterizationStateCreateInfo rast{};
    rast.sType       = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rast.polygonMode = VK_POLYGON_MODE_FILL;
    rast.cullMode    = VK_CULL_MODE_NONE;
    rast.frontFace   = VK_FRONT_FACE_CLOCKWISE;
    rast.lineWidth   = 1.f;

    VkPipelineMultisampleStateCreateInfo ms{};
    ms.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState blendAtt{};
    blendAtt.blendEnable         = VK_TRUE;
    blendAtt.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    blendAtt.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blendAtt.colorBlendOp        = VK_BLEND_OP_ADD;
    blendAtt.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    blendAtt.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blendAtt.alphaBlendOp        = VK_BLEND_OP_ADD;
    blendAtt.colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                   VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo blend{};
    blend.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blend.attachmentCount = 1;
    blend.pAttachments    = &blendAtt;

    std::array<VkDynamicState, 2> dynStates{VK_DYNAMIC_STATE_VIEWPORT,
                                             VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dyn{};
    dyn.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dyn.dynamicStateCount = static_cast<uint32_t>(dynStates.size());
    dyn.pDynamicStates    = dynStates.data();

    VkPipelineDepthStencilStateCreateInfo ds{};
    ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

    std::array<VkPipelineShaderStageCreateInfo, 2> stages{};
    stages[0].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage  = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = vertMod;
    stages[0].pName  = "main";
    stages[1].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = fragMod;
    stages[1].pName  = "main";

    VkGraphicsPipelineCreateInfo ci{};
    ci.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    ci.stageCount          = static_cast<uint32_t>(stages.size());
    ci.pStages             = stages.data();
    ci.pVertexInputState   = &vi;
    ci.pInputAssemblyState = &ia;
    ci.pViewportState      = &vpState;
    ci.pRasterizationState = &rast;
    ci.pMultisampleState   = &ms;
    ci.pColorBlendState    = &blend;
    ci.pDynamicState       = &dyn;
    ci.pDepthStencilState  = &ds;
    ci.layout              = m_pipelineLayout;
    ci.renderPass          = m_renderPass;
    ci.subpass             = 0;

    r = m_dispatch->CreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &ci, nullptr, &m_pipeline);

    m_dispatch->DestroyShaderModule(m_device, vertMod, nullptr);
    m_dispatch->DestroyShaderModule(m_device, fragMod, nullptr);
    return r;
}

VkResult VulkanRenderer::CreateWhiteTextureResources()
{
    VkResult r = CreateDeviceImage(
        1, 1, VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        m_whiteImage, m_whiteMemory);
    if (r != VK_SUCCESS) return r;

    VkImageViewCreateInfo ivci{};
    ivci.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    ivci.image                           = m_whiteImage;
    ivci.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
    ivci.format                          = VK_FORMAT_R8G8B8A8_SRGB;
    ivci.components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    ivci.components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    ivci.components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    ivci.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    ivci.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    ivci.subresourceRange.baseMipLevel   = 0;
    ivci.subresourceRange.levelCount     = 1;
    ivci.subresourceRange.baseArrayLayer = 0;
    ivci.subresourceRange.layerCount     = 1;

    return m_dispatch->CreateImageView(m_device, &ivci, nullptr, &m_whiteView);
}

void VulkanRenderer::UploadWhiteTexture()
{
    if (!m_uploadQueue) return;
    const uint8_t white[4] = {255, 255, 255, 255};
    UploadImageData(m_whiteImage, 1, 1, white, 4);
}

} // namespace WoTLKGuiLayer
