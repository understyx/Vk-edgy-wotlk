#include "vulkan_shims.h"
#include "vkroots.h"
#include "layer_threads.h"

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

    LayerThreadManager threadManager;
} gOverlay;



  class VkInstanceOverrides {
public:
    static VkResult CreateDevice(
      const vkroots::VkPhysicalDeviceDispatch& pDispatch, VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDevice* pDevice)
    {
    VkResult result = pDispatch.CreateDevice(physicalDevice, pCreateInfo, pAllocator, pDevice);
    if (result == VK_SUCCESS)
    {
        gOverlay.device = *pDevice;
        // Start the triple-buffered game data, JS engine, and rendering threads
        gOverlay.threadManager.start();
    }
    return result;
  }
  };
  class VkDeviceOverrides {
  public:

    static void DestroyDevice(
        const vkroots::VkDeviceDispatch& pDispatch, VkDevice device, const VkAllocationCallbacks* pAllocator)
    {
        // Stop UI threads cleanly
        gOverlay.threadManager.stop();
        pDispatch.DestroyDevice(device, pAllocator);
    }

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

        // Simulating the 3rd thread rendering output blit/overlay
        UIData uiFrame;
        gOverlay.threadManager.getLatestUIFrame(uiFrame);
        if (uiFrame.uiFrameId > 0) {
            printf("[Vulkan Layer present] Compositing HTML/JS WebTexture onto swapchain image:\n");
            printf("  -> UI Frame #%u (from game frame #%u) | Size: %u x %u pixels | Buffer: %zu bytes\n",
                   uiFrame.uiFrameId, uiFrame.sourceGameFrameId,
                   uiFrame.webTexture.width, uiFrame.webTexture.height, uiFrame.webTexture.rgbaPixels.size());

            // Sample a few pixels to prove rendering has drawn on the texture!
            // Let's sample pixel at (35, 35) which is inside the green Player HP Bar:
            uint32_t sampleX = 35;
            uint32_t sampleY = 35;
            uint32_t offset = (sampleY * uiFrame.webTexture.width + sampleX) * 4;
            if (offset + 3 < uiFrame.webTexture.rgbaPixels.size()) {
                uint8_t r = uiFrame.webTexture.rgbaPixels[offset + 0];
                uint8_t g = uiFrame.webTexture.rgbaPixels[offset + 1];
                uint8_t b = uiFrame.webTexture.rgbaPixels[offset + 2];
                uint8_t a = uiFrame.webTexture.rgbaPixels[offset + 3];
                printf("  -> Pixel sample at HP Bar (%u, %u): RGBA(%u, %u, %u, %u)\n",
                       sampleX, sampleY, r, g, b, a);
            }

            for (const auto& element : uiFrame.elements) {
                printf("  -> Element '%s' [%s] at (%.1f, %.1f), size %.1f x %.1f, Text: '%s'\n",
                       element.id.c_str(), element.type.c_str(), element.screenX, element.screenY,
                       element.width, element.height, element.text.c_str());
            }
        }

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