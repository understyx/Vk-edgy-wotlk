#include "vkroots.h"
#include "common/PointerSwapBuffer.hpp"
#include "game_data/GameDataQuery.hpp"
#include "ultralight_renderer/WebUIRenderingEngine.hpp"
#include "vulkan_renderer/VulkanOverlayRenderer.hpp"

#include <cstdio>
#include <vector>
#include <thread>
#include <atomic>

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

    // Multi-threaded Modern Pipeline components
    std::atomic<bool> pipelineRunning{false};
    std::thread gameThread;
    std::thread uiThread;
    std::thread renderThread;

    PointerSwapBuffer<GameData::TelemetryData> gameDataBuffer;
    PointerSwapBuffer<UltralightRenderer::WebTexture> uiTextureBuffer;

    void startPipeline() {
        if (pipelineRunning) return;
        pipelineRunning = true;
        gameThread = std::thread(GameData::gameDataThreadLoop, std::ref(pipelineRunning), std::ref(gameDataBuffer));
        uiThread = std::thread(UltralightRenderer::jsHtmlEngineThreadLoop, std::ref(pipelineRunning), std::ref(gameDataBuffer), std::ref(uiTextureBuffer));
        renderThread = std::thread(VulkanRenderer::renderingThreadLoop, std::ref(pipelineRunning), std::ref(uiTextureBuffer));
        printf("WoTLK 3-stage Modern UI Overlay pipeline successfully initiated.\n");
    }

    void stopPipeline() {
        if (!pipelineRunning) return;
        pipelineRunning = false;
        if (gameThread.joinable()) gameThread.join();
        if (uiThread.joinable()) uiThread.join();
        if (renderThread.joinable()) renderThread.join();
        printf("WoTLK 3-stage Modern UI Overlay pipeline stopped.\n");
    }
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
        // Start multi-threaded modern UI layout pipeline components synchronized with swapchain
        gOverlay.startPipeline();
    }

    return result;
}
    
    static void DestroySwapchainKHR(
        const vkroots::VkDeviceDispatch& pDispatch, VkDevice device, VkSwapchainKHR swapchain, const VkAllocationCallbacks* pAllocator)
        {
          gOverlay.stopPipeline();
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
        uint32_t imageIndex = pPresentInfo->pImageIndices[0];
        printf("Frame Finished. Presenting image: %u\n", imageIndex);

        // Retrieve the latest dynamic layout render texture from our HTML/CSS engine
        bool hasNewTexture = gOverlay.uiTextureBuffer.swapConsumer();
        if (hasNewTexture || gOverlay.uiTextureBuffer.getReadBuffer()->rgbaPixels.size() > 0) {
            const UltralightRenderer::WebTexture* uiTex = gOverlay.uiTextureBuffer.getReadBuffer();

            // To render the overlay, we would transition the swapchain image to VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            // bind our Vulkan render pass, pipeline, and descriptor sets (holding the uploaded UI texture),
            // record draw commands inside our command buffer to draw a full-screen quad,
            // and submit it to the graphics queue before presentation.
            //
            // Conceptually:
            // vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
            // vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, gOverlay.pipeline);
            // vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, gOverlay.pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
            // vkCmdDraw(cmd, 4, 1, 0, 0);
            // vkCmdEndRenderPass(cmd);

            (void)uiTex; // Mark as used for compiler
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
