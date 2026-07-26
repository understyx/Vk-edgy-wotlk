#ifndef VK_LAYER_H
#define VK_LAYER_H

#include "vkroots.h"
#include <vector>
#include <memory>

// Forward-declare OverlayUI so we don't pull all RmlUi headers into every TU
namespace WoTLKGuiLayer {
class OverlayUI;
}

namespace WoTLKGuiLayer {

/**
 * @struct OverlayContext
 * @brief Holds all Vulkan resources and the RmlUi overlay manager.
 */
struct OverlayContext
{
    // Core Vulkan handles
    VkDevice     device      = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

    // Stored dispatch pointer (stable – backed by vkroots' global ObjectMap)
    const vkroots::VkDeviceDispatch* dispatch = nullptr;

    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    VkFormat       format    = VK_FORMAT_UNDEFINED;
    VkExtent2D     extent    = {};

    std::vector<VkImage> images;

    VkQueue  graphicsQueue       = VK_NULL_HANDLE;
    uint32_t graphicsQueueFamily = 0;

    // RmlUi overlay (null until swapchain + queue are both available)
    std::unique_ptr<OverlayUI> rmlOverlay;
    bool rmlInitialized = false;
};

extern OverlayContext gOverlay;

/**
 * @brief Helper function to check if a launch option was passed to the process.
 */
bool HasLaunchOption(const std::string& option);

/**
 * @class VkInstanceOverrides
 * @brief Intercepts Vulkan instance-level function calls
 */
class VkInstanceOverrides {
public:
    static VkResult CreateDevice(
        const vkroots::VkPhysicalDeviceDispatch& pDispatch, 
        VkPhysicalDevice physicalDevice, 
        const VkDeviceCreateInfo* pCreateInfo, 
        const VkAllocationCallbacks* pAllocator, 
        VkDevice* pDevice);
};

/**
 * @class VkDeviceOverrides
 * @brief Intercepts Vulkan device-level function calls
 */
class VkDeviceOverrides {
public:
    static VkResult CreateSwapchainKHR(
        const vkroots::VkDeviceDispatch& pDispatch,
        VkDevice device,
        const VkSwapchainCreateInfoKHR* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkSwapchainKHR* pSwapchain);

    static void DestroySwapchainKHR(
        const vkroots::VkDeviceDispatch& pDispatch, 
        VkDevice device, 
        VkSwapchainKHR swapchain, 
        const VkAllocationCallbacks* pAllocator);

    static VkResult GetSwapchainImagesKHR(
        const vkroots::VkDeviceDispatch& pDispatch,
        VkDevice device,
        VkSwapchainKHR swapchain,
        uint32_t* pSwapchainImageCount,
        VkImage* pSwapchainImages);

    static VkResult AcquireNextImageKHR(
        const vkroots::VkDeviceDispatch& pDispatch, 
        VkDevice device, 
        VkSwapchainKHR swapchain, 
        uint64_t timeout, 
        VkSemaphore semaphore, 
        VkFence fence, 
        uint32_t *pImageIndex);

    static VkResult AcquireNextImage2KHR(
        const vkroots::VkDeviceDispatch& pDispatch, 
        VkDevice device, 
        const VkAcquireNextImageInfoKHR *pAcquireInfo, 
        uint32_t *pImageIndex);

    static VkResult CreateImage(
        const vkroots::VkDeviceDispatch& pDispatch, 
        VkDevice device, 
        const VkImageCreateInfo* pCreateInfo, 
        const VkAllocationCallbacks* pAllocator, 
        VkImage* pImage);

    static VkResult QueuePresentKHR(
        const vkroots::VkQueueDispatch& pDispatch, 
        VkQueue queue, 
        const VkPresentInfoKHR* pPresentInfo);

    static VkResult QueueSubmit(
        const vkroots::VkQueueDispatch& pDispatch, 
        VkQueue queue, 
        uint32_t submitCount, 
        const VkSubmitInfo* pSubmits, 
        VkFence fence);

    static VkResult QueueSubmit2(
        const vkroots::VkQueueDispatch& pDispatch, 
        VkQueue queue, 
        uint32_t submitCount, 
        const VkSubmitInfo2* pSubmits,
        VkFence fence);
};

} // namespace WoTLKGuiLayer

#endif // VK_LAYER_H
