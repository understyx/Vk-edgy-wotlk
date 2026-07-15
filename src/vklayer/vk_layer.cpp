#include "vklayer/vk_layer.h"
#include <cstdio>

namespace WoTLKGuiLayer {

// Global overlay context
OverlayContext gOverlay;

// ============================================================================
// VkInstanceOverrides implementation
// ============================================================================

VkResult VkInstanceOverrides::CreateDevice(
    const vkroots::VkDeviceDispatch& pDispatch, 
    VkPhysicalDevice physicalDevice, 
    const VkDeviceCreateInfo* pCreateInfo, 
    const VkAllocationCallbacks* pAllocator, 
    VkDevice* pDevice) 
{
    VkResult result = pDispatch.CreateDevice(physicalDevice, pCreateInfo, pAllocator, pDevice);
    if (result == VK_SUCCESS)
    {
        gOverlay.device = *pDevice;
    }
    return result;
}

// ============================================================================
// VkDeviceOverrides implementation
// ============================================================================

VkResult VkDeviceOverrides::CreateSwapchainKHR(
    const vkroots::VkDeviceDispatch& pDispatch,
    VkDevice device,
    const VkSwapchainCreateInfoKHR* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkSwapchainKHR* pSwapchain)
{
    VkResult result =
        pDispatch.CreateSwapchainKHR(
            device,
            pCreateInfo,
            pAllocator,
            pSwapchain);

    if (result == VK_SUCCESS)
    {
        gOverlay.swapchain = *pSwapchain;
        gOverlay.extent = pCreateInfo->imageExtent;
        gOverlay.format = pCreateInfo->imageFormat;
    }

    return result;
}

void VkDeviceOverrides::DestroySwapchainKHR(
    const vkroots::VkDeviceDispatch& pDispatch, 
    VkDevice device, 
    VkSwapchainKHR swapchain, 
    const VkAllocationCallbacks* pAllocator)
{
    return pDispatch.DestroySwapchainKHR(device, swapchain, pAllocator);
}

VkResult VkDeviceOverrides::GetSwapchainImagesKHR(
    const vkroots::VkDeviceDispatch& pDispatch,
    VkDevice device,
    VkSwapchainKHR swapchain,
    uint32_t* pSwapchainImageCount,
    VkImage* pSwapchainImages)
{
    VkResult result = pDispatch.GetSwapchainImagesKHR(
        device,
        swapchain,
        pSwapchainImageCount,
        pSwapchainImages);

    if (result == VK_SUCCESS && pSwapchainImages)
    {
        gOverlay.images.assign(
            pSwapchainImages,
            pSwapchainImages + *pSwapchainImageCount);
    }

    return result;
}

VkResult VkDeviceOverrides::AcquireNextImageKHR(
    const vkroots::VkDeviceDispatch& pDispatch, 
    VkDevice device, 
    VkSwapchainKHR swapchain, 
    uint64_t timeout, 
    VkSemaphore semaphore, 
    VkFence fence, 
    uint32_t *pImageIndex)
{
    return pDispatch.AcquireNextImageKHR(device, swapchain, timeout, semaphore, fence, pImageIndex);
}

VkResult VkDeviceOverrides::AcquireNextImage2KHR(
    const vkroots::VkDeviceDispatch& pDispatch, 
    VkDevice device, 
    const VkAcquireNextImageInfoKHR *pAcquireInfo, 
    uint32_t *pImageIndex)
{
    return pDispatch.AcquireNextImage2KHR(device, pAcquireInfo, pImageIndex);
}

VkResult VkDeviceOverrides::CreateImage(
    const vkroots::VkDeviceDispatch& pDispatch, 
    VkDevice device, 
    const VkImageCreateInfo* pCreateInfo, 
    const VkAllocationCallbacks* pAllocator, 
    VkImage* pImage)
{
    printf("The app has made an image, I bet it's going to be frogtastically beautiful!\n");
    return pDispatch.CreateImage(device, pCreateInfo, pAllocator, pImage);
}

VkResult VkDeviceOverrides::QueuePresentKHR(
    const vkroots::VkQueueDispatch& pDispatch, 
    VkQueue queue, 
    const VkPresentInfoKHR* pPresentInfo)
{
    printf("Frame Finished. Presenting image: %u\n", pPresentInfo->pImageIndices[0]);
    return pDispatch.QueuePresentKHR(queue, pPresentInfo);
}

VkResult VkDeviceOverrides::QueueSubmit(
    const vkroots::VkQueueDispatch& pDispatch, 
    VkQueue queue, 
    uint32_t submitCount, 
    const VkSubmitInfo* pSubmits, 
    VkFence fence)
{
    printf("nothing blew up vkQueueSubmit!\n");
    return pDispatch.QueueSubmit(queue, submitCount, pSubmits, fence);
}

VkResult VkDeviceOverrides::QueueSubmit2(
    const vkroots::VkQueueDispatch& pDispatch, 
    VkQueue queue, 
    uint32_t submitCount, 
    const VkSubmitInfo2* pSubmits,
    VkFence fence)
{
    printf("nothing blew up vkQueueSubmit2!\n");
    return pDispatch.QueueSubmit2(queue, submitCount, pSubmits, fence);
}

} // namespace WoTLKGuiLayer

// Define the layer interfaces using vkroots macros
VKROOTS_DEFINE_LAYER_INTERFACES(WoTLKGuiLayer::VkInstanceOverrides,
                                 WoTLKGuiLayer::VkDeviceOverrides);
