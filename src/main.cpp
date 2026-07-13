#include "vkroots.h"

#include <cstdio>

namespace WoTLKGuiLayer {

struct OverlayContext
{
    VkDevice device = VK_NULL_HANDLE;

    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    VkFormat format = VK_FORMAT_UNDEFINED;
    VkExtent2D extent;

    std::vector<VkImage> images;
    std::vector<VkImageView> imageViews;
    std::vector<VkFramebuffer> framebuffers;

    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;

    VkCommandPool commandPool = VK_NULL_HANDLE;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    uint32_t graphicsQueueFamily = 0;
    std::vector<VkCommandBuffer> commandBuffers;
} gOverlay;



  class VkInstanceOverrides {
public:
    static VkResult CreateDevice(
      const vkroots::VkDeviceDispatch& pDispatch, VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDevice* pDevice) 
    {
    VkResult result = pDispatch.CreateDevice(physicalDevice, pCreateInfo, pAllocator, pDevice);
    if (result == VK_SUCCESS)
    {
        gOverlay.device = *pDevice;
    }
    return result;
  }
  };
  class VkDeviceOverrides {
  public:

    static VkResult CreateSwapchainKHR(
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
    
    static void DestroySwapchainKHR(
        const vkroots::VkDeviceDispatch& pDispatch, VkDevice device, VkSwapchainKHR swapchain, const VkAllocationCallbacks* pAllocator)
        {
          return pDispatch.DestroySwapchainKHR(device,swapchain, pAllocator);
        }
    static VkResult GetSwapchainImagesKHR(
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
    static VkResult AcquireNextImageKHR(
            const vkroots::VkDeviceDispatch& pDispatch, VkDevice device, VkSwapchainKHR swapchain, uint64_t timeout, VkSemaphore semaphore, VkFence fence, uint32_t *pImageIndex)
            {
              return pDispatch.AcquireNextImageKHR(device, swapchain, timeout, semaphore, fence, pImageIndex);
            }
    static VkResult AcquireNextImage2KHR(
            const vkroots::VkDeviceDispatch& pDispatch, VkDevice device, const VkAcquireNextImageInfoKHR *pAcquireInfo, uint32_t *pImageIndex){
              return pDispatch.AcquireNextImage2KHR(device, pAcquireInfo, pImageIndex);
            }


    static VkResult CreateImage(
      const vkroots::VkDeviceDispatch& pDispatch, VkDevice device, const VkImageCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkImage* pImage) {
      printf("The app has made an image, I bet it's going to be frogtastically beautiful!\n");
      return pDispatch.CreateImage(device, pCreateInfo, pAllocator, pImage);
    }
 
  
  static VkResult QueuePresentKHR(
        const vkroots::VkQueueDispatch& pDispatch, VkQueue queue, const VkPresentInfoKHR* pPresentInfo) {
        printf("Frame Finished. Presenting image: %u\n", pPresentInfo->pImageIndices[0]);
        return pDispatch.QueuePresentKHR(queue, pPresentInfo);
      }

  static VkResult QueueSubmit(const vkroots::VkQueueDispatch& pDispatch, VkQueue queue, uint32_t submitCount, const VkSubmitInfo* pSubmits, VkFence fence){
                                  printf("nothing blew up vkQueueSubmit!\n");
  return pDispatch.QueueSubmit(queue,submitCount,pSubmits,fence);

  }

    static VkResult QueueSubmit2(const vkroots::VkQueueDispatch& pDispatch, VkQueue queue, uint32_t submitCount, const VkSubmitInfo2* pSubmits,VkFence fence){
                                    printf("nothing blew up vkQueueSubmit2!\n");
              return pDispatch.QueueSubmit2(queue,submitCount,pSubmits,fence);
  }
 };

}

VKROOTS_DEFINE_LAYER_INTERFACES(WoTLKGuiLayer::VkInstanceOverrides,
                                WoTLKGuiLayer::VkDeviceOverrides);