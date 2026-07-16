#include "vklayer/vk_layer.h"
#include "rmlui/OverlayUI.h"

#include <cstdio>
#include <cstring>
#include <dlfcn.h>
#include <string>
#include <vector>

namespace WoTLKGuiLayer {

// Global overlay context
OverlayContext gOverlay;

// ============================================================================
// Path helpers
// ============================================================================

/**
 * Returns the directory containing this shared library.
 * Used to locate shader SPIR-V and UI document files at runtime.
 */
static std::string GetLayerDir()
{
    Dl_info info;
    if (dladdr(reinterpret_cast<void*>(&GetLayerDir), &info) && info.dli_fname) {
        std::string path = info.dli_fname;
        auto pos = path.rfind('/');
        if (pos != std::string::npos)
            return path.substr(0, pos);
    }
    return ".";
}

// ============================================================================
// RmlUi initialisation helper
// ============================================================================

static void TryInitRmlUi(const vkroots::VkDeviceDispatch& dispatch)
{
    if (gOverlay.rmlInitialized) return;
    if (gOverlay.graphicsQueue == VK_NULL_HANDLE) return;
    if (gOverlay.images.empty()) return;

    const std::string layerDir  = GetLayerDir();
    const std::string shaderDir = layerDir + "/shaders";
    const std::string uiDir     = layerDir + "/ui";

    // Try several well-known font locations (Linux)
    const char* fontCandidates[] = {
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
        "/usr/share/fonts/truetype/freefont/FreeSans.ttf",
        nullptr,
    };
    std::string fontPath;
    for (int i = 0; fontCandidates[i]; ++i) {
        if (access(fontCandidates[i], R_OK) == 0) {
            fontPath = fontCandidates[i];
            break;
        }
    }

    gOverlay.rmlOverlay = std::make_unique<OverlayUI>();

    OverlayUI::InitInfo info{};
    info.device                   = gOverlay.device;
    info.physicalDevice           = gOverlay.physicalDevice;
    info.dispatch                 = &dispatch;
    info.graphicsQueueFamilyIndex = gOverlay.graphicsQueueFamily;
    info.graphicsQueue            = gOverlay.graphicsQueue;
    info.shaderDir                = shaderDir;
    info.uiDir                    = uiDir;
    info.fontPath                 = fontPath;

    if (!gOverlay.rmlOverlay->Initialize(info)) {
        fprintf(stderr, "[WoTLKLayer] RmlUi init failed – overlay disabled\n");
        gOverlay.rmlOverlay.reset();
        return;
    }

    gOverlay.rmlOverlay->ResizeSwapchain(
        gOverlay.images, gOverlay.format, gOverlay.extent);

    gOverlay.rmlInitialized = true;
    fprintf(stdout, "[WoTLKLayer] RmlUi overlay initialised (%ux%u)\n",
            gOverlay.extent.width, gOverlay.extent.height);
}

// ============================================================================
// VkInstanceOverrides
// ============================================================================

VkResult VkInstanceOverrides::CreateDevice(
    const vkroots::VkPhysicalDeviceDispatch& pDispatch,
    VkPhysicalDevice physicalDevice,
    const VkDeviceCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDevice* pDevice)
{
    VkResult result = pDispatch.CreateDevice(physicalDevice, pCreateInfo, pAllocator, pDevice);
    if (result == VK_SUCCESS) {
        gOverlay.device         = *pDevice;
        gOverlay.physicalDevice = physicalDevice;

        // Store a pointer to the stable device dispatch table
        gOverlay.dispatch = vkroots::LookupDispatch(*pDevice);

        // Find a graphics-capable queue family
        uint32_t qfCount = 0;
        pDispatch.GetPhysicalDeviceQueueFamilyProperties(physicalDevice, &qfCount, nullptr);
        std::vector<VkQueueFamilyProperties> qfProps(qfCount);
        pDispatch.GetPhysicalDeviceQueueFamilyProperties(
            physicalDevice, &qfCount, qfProps.data());

        gOverlay.graphicsQueueFamily = 0;
        for (uint32_t i = 0; i < qfCount; ++i) {
            if (qfProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                gOverlay.graphicsQueueFamily = i;
                break;
            }
        }

        // Retrieve the graphics queue
        if (gOverlay.dispatch) {
            gOverlay.dispatch->GetDeviceQueue(
                *pDevice, gOverlay.graphicsQueueFamily, 0, &gOverlay.graphicsQueue);
        }

        fprintf(stdout, "[WoTLKLayer] Device created (queue family %u)\n",
                gOverlay.graphicsQueueFamily);
    }
    return result;
}

// ============================================================================
// VkDeviceOverrides
// ============================================================================

VkResult VkDeviceOverrides::CreateSwapchainKHR(
    const vkroots::VkDeviceDispatch& pDispatch,
    VkDevice device,
    const VkSwapchainCreateInfoKHR* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkSwapchainKHR* pSwapchain)
{
    // Tear down any previous overlay resources when the swapchain is recreated
    if (gOverlay.rmlOverlay)
        gOverlay.rmlOverlay->DestroySwapchainResources();

    VkResult result = pDispatch.CreateSwapchainKHR(device, pCreateInfo, pAllocator, pSwapchain);
    if (result == VK_SUCCESS) {
        gOverlay.swapchain = *pSwapchain;
        gOverlay.extent    = pCreateInfo->imageExtent;
        gOverlay.format    = pCreateInfo->imageFormat;
        gOverlay.images.clear();

        // Reset init flag so we re-initialise with the new dimensions
        if (gOverlay.rmlInitialized) {
            gOverlay.rmlInitialized = false;
        }
    }
    return result;
}

void VkDeviceOverrides::DestroySwapchainKHR(
    const vkroots::VkDeviceDispatch& pDispatch,
    VkDevice device,
    VkSwapchainKHR swapchain,
    const VkAllocationCallbacks* pAllocator)
{
    if (gOverlay.rmlOverlay)
        gOverlay.rmlOverlay->DestroySwapchainResources();

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
        device, swapchain, pSwapchainImageCount, pSwapchainImages);

    if (result == VK_SUCCESS && pSwapchainImages) {
        gOverlay.images.assign(
            pSwapchainImages, pSwapchainImages + *pSwapchainImageCount);

        // Now we have device + queue + images – attempt RmlUi init
        TryInitRmlUi(pDispatch);
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
    uint32_t* pImageIndex)
{
    return pDispatch.AcquireNextImageKHR(device, swapchain, timeout,
                                         semaphore, fence, pImageIndex);
}

VkResult VkDeviceOverrides::AcquireNextImage2KHR(
    const vkroots::VkDeviceDispatch& pDispatch,
    VkDevice device,
    const VkAcquireNextImageInfoKHR* pAcquireInfo,
    uint32_t* pImageIndex)
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
    return pDispatch.CreateImage(device, pCreateInfo, pAllocator, pImage);
}

VkResult VkDeviceOverrides::QueuePresentKHR(
    const vkroots::VkQueueDispatch& pDispatch,
    VkQueue queue,
    const VkPresentInfoKHR* pPresentInfo)
{
    // Only handle single-swapchain presents for simplicity
    if (!gOverlay.rmlOverlay || !gOverlay.rmlOverlay->IsReady() ||
        pPresentInfo->swapchainCount != 1) {
        return pDispatch.QueuePresentKHR(queue, pPresentInfo);
    }

    uint32_t imageIndex = pPresentInfo->pImageIndices[0];

    // Collect the game's wait semaphores
    std::vector<VkSemaphore> waitSems(
        pPresentInfo->pWaitSemaphores,
        pPresentInfo->pWaitSemaphores + pPresentInfo->waitSemaphoreCount);

    // Render overlay; get the semaphore to wait on before present
    VkSemaphore overlaySem = gOverlay.rmlOverlay->Render(queue, imageIndex, waitSems);

    if (overlaySem == VK_NULL_HANDLE) {
        // Overlay render failed – fall through to normal present
        return pDispatch.QueuePresentKHR(queue, pPresentInfo);
    }

    // Replace wait semaphores with our overlay semaphore
    VkPresentInfoKHR modifiedPresent = *pPresentInfo;
    modifiedPresent.waitSemaphoreCount = 1;
    modifiedPresent.pWaitSemaphores    = &overlaySem;

    return pDispatch.QueuePresentKHR(queue, &modifiedPresent);
}

VkResult VkDeviceOverrides::QueueSubmit(
    const vkroots::VkQueueDispatch& pDispatch,
    VkQueue queue,
    uint32_t submitCount,
    const VkSubmitInfo* pSubmits,
    VkFence fence)
{
    return pDispatch.QueueSubmit(queue, submitCount, pSubmits, fence);
}

VkResult VkDeviceOverrides::QueueSubmit2(
    const vkroots::VkQueueDispatch& pDispatch,
    VkQueue queue,
    uint32_t submitCount,
    const VkSubmitInfo2* pSubmits,
    VkFence fence)
{
    return pDispatch.QueueSubmit2(queue, submitCount, pSubmits, fence);
}

} // namespace WoTLKGuiLayer

// Define the layer interfaces using vkroots macros
VKROOTS_DEFINE_LAYER_INTERFACES(WoTLKGuiLayer::VkInstanceOverrides,
                                 WoTLKGuiLayer::VkDeviceOverrides);
