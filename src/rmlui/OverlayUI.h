#pragma once

#include "SystemInterface.h"
#include "FileInterface.h"
#include "VulkanRenderer.h"

#include <RmlUi/Core.h>
#include <vulkan/vulkan.h>
#include "vkroots.h"

#include <memory>
#include <string>
#include <vector>

namespace WoTLKGuiLayer {

/**
 * @class OverlayUI
 * @brief Manages the RmlUi context, document, and per-frame rendering.
 *
 * OverlayUI owns the RmlUi interfaces (system, file, renderer) and orchestrates
 * the full per-frame flow:
 *   1. Update the RmlUi context with the current screen dimensions.
 *   2. Inject the Vulkan command buffer via VulkanRenderer::PrepareFrame().
 *   3. Call context->Render() to have RmlUi record draw commands.
 *   4. Submit the command buffer, chaining semaphores so the present queue
 *      waits for the overlay before displaying the frame.
 */
class OverlayUI {
public:
    OverlayUI() = default;
    ~OverlayUI();

    struct InitInfo {
        VkDevice                         device;
        VkPhysicalDevice                 physicalDevice;
        const vkroots::VkDeviceDispatch* dispatch;
        uint32_t                         graphicsQueueFamilyIndex;
        VkQueue                          graphicsQueue;
        std::string                      shaderDir;
        std::string                      uiDir;    // directory containing overlay.rml
        std::string                      fontPath; // path to a .ttf font file
    };

    bool Initialize(const InitInfo& info);
    void Shutdown();

    // Called once after the first swapchain is created (or on resize).
    void ResizeSwapchain(const std::vector<VkImage>& images,
                         VkFormat                    format,
                         VkExtent2D                  extent);
    void DestroySwapchainResources();

    /**
     * Render the UI overlay for one frame.
     *
     * @param queue       Queue to submit overlay work to.
     * @param imageIndex  Swapchain image index being presented.
     * @param waitSems    Semaphores the overlay must wait on (game's render finished).
     * @return            Semaphore the present queue should wait on.
     *                    Returns VK_NULL_HANDLE on failure (caller should present normally).
     */
    VkSemaphore Render(VkQueue                         queue,
                       uint32_t                        imageIndex,
                       const std::vector<VkSemaphore>& waitSems);

    bool IsReady() const { return m_ready; }

private:
    VkResult CreateSyncObjects(uint32_t imageCount);
    void     DestroySyncObjects();

    bool m_ready = false;

    std::unique_ptr<RmlSystemInterface> m_sysInterface;
    std::unique_ptr<RmlFileInterface>   m_fileInterface;
    std::unique_ptr<VulkanRenderer>     m_renderer;

    Rml::Context*        m_context  = nullptr;
    Rml::ElementDocument* m_document = nullptr;

    VkDevice                         m_device   = VK_NULL_HANDLE;
    const vkroots::VkDeviceDispatch* m_dispatch = nullptr;

    // Per-image synchronisation
    std::vector<VkSemaphore> m_overlaySemaphores;
    std::vector<VkFence>     m_fences;

    VkExtent2D m_extent = {};
};

} // namespace WoTLKGuiLayer
