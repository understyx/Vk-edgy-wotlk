#include "vkroots.h"
#include <cstdio>
#include <vector>
#include <array>

namespace WoTLKGuiLayer {

/*
 * NOTE: Multi-Swapchain Support Limitation.
 * The current design of this minimal Vulkan overlay layer assumes a single global
 * swapchain (tracked in gOverlay). If the target application opens multiple windows
 * or utilizes multiple swapchains/devices simultaneously, state conflicts will occur.
 * For a minimal overlay layer, this single-swapchain limitation is acceptable.
 */

// Embedded Vertex Shader SPIR-V code (triangle.vert)
constexpr uint32_t vert_shader_size = 1108;
alignas(4) constexpr uint8_t vert_shader_code[] = {
    0x03, 0x02, 0x23, 0x07, 0x00, 0x00, 0x01, 0x00, 0x0b, 0x00, 0x08, 0x00,
    0x29, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x00, 0x02, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x06, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x47, 0x4c, 0x53, 0x4c, 0x2e, 0x73, 0x74, 0x64, 0x2e, 0x34, 0x35, 0x30,
    0x00, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x6d, 0x61, 0x69, 0x6e, 0x00, 0x00, 0x00, 0x00,
    0x19, 0x00, 0x00, 0x00, 0x1d, 0x00, 0x00, 0x00, 0x03, 0x00, 0x03, 0x00,
    0x02, 0x00, 0x00, 0x00, 0xc2, 0x01, 0x00, 0x00, 0x05, 0x00, 0x04, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x6d, 0x61, 0x69, 0x6e, 0x00, 0x00, 0x00, 0x00,
    0x05, 0x00, 0x03, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x70, 0x6f, 0x73, 0x00,
    0x05, 0x00, 0x06, 0x00, 0x17, 0x00, 0x00, 0x00, 0x67, 0x6c, 0x5f, 0x50,
    0x65, 0x72, 0x56, 0x65, 0x72, 0x74, 0x65, 0x78, 0x00, 0x00, 0x00, 0x00,
    0x06, 0x00, 0x06, 0x00, 0x17, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x67, 0x6c, 0x5f, 0x50, 0x6f, 0x73, 0x69, 0x74, 0x69, 0x6f, 0x6e, 0x00,
    0x06, 0x00, 0x07, 0x00, 0x17, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x67, 0x6c, 0x5f, 0x50, 0x6f, 0x69, 0x6e, 0x74, 0x53, 0x69, 0x7a, 0x65,
    0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x07, 0x00, 0x17, 0x00, 0x00, 0x00,
    0x02, 0x00, 0x00, 0x00, 0x67, 0x6c, 0x5f, 0x43, 0x6c, 0x69, 0x70, 0x44,
    0x69, 0x73, 0x74, 0x61, 0x6e, 0x63, 0x65, 0x00, 0x06, 0x00, 0x07, 0x00,
    0x17, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x67, 0x6c, 0x5f, 0x43,
    0x75, 0x6c, 0x6c, 0x44, 0x69, 0x73, 0x74, 0x61, 0x6e, 0x63, 0x65, 0x00,
    0x05, 0x00, 0x03, 0x00, 0x19, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x05, 0x00, 0x06, 0x00, 0x1d, 0x00, 0x00, 0x00, 0x67, 0x6c, 0x5f, 0x56,
    0x65, 0x72, 0x74, 0x65, 0x78, 0x49, 0x6e, 0x64, 0x65, 0x78, 0x00, 0x00,
    0x47, 0x00, 0x03, 0x00, 0x17, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
    0x48, 0x00, 0x05, 0x00, 0x17, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x0b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x48, 0x00, 0x05, 0x00,
    0x17, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x48, 0x00, 0x05, 0x00, 0x17, 0x00, 0x00, 0x00,
    0x02, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
    0x48, 0x00, 0x05, 0x00, 0x17, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
    0x0b, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00,
    0x1d, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x00, 0x00, 0x2a, 0x00, 0x00, 0x00,
    0x13, 0x00, 0x02, 0x00, 0x02, 0x00, 0x00, 0x00, 0x21, 0x00, 0x03, 0x00,
    0x03, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x16, 0x00, 0x03, 0x00,
    0x06, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x17, 0x00, 0x04, 0x00,
    0x07, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
    0x15, 0x00, 0x04, 0x00, 0x08, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x2b, 0x00, 0x04, 0x00, 0x08, 0x00, 0x00, 0x00,
    0x09, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x04, 0x00,
    0x0a, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00,
    0x20, 0x00, 0x04, 0x00, 0x0b, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00,
    0x0a, 0x00, 0x00, 0x00, 0x2b, 0x00, 0x04, 0x00, 0x06, 0x00, 0x00, 0x00,
    0x0d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xbf, 0x2c, 0x00, 0x05, 0x00,
    0x07, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x00, 0x00, 0x0d, 0x00, 0x00, 0x00,
    0x0d, 0x00, 0x00, 0x00, 0x2b, 0x00, 0x04, 0x00, 0x06, 0x00, 0x00, 0x00,
    0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0x2c, 0x00, 0x05, 0x00,
    0x07, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x0d, 0x00, 0x00, 0x00,
    0x0f, 0x00, 0x00, 0x00, 0x2c, 0x00, 0x05, 0x00, 0x07, 0x00, 0x00, 0x00,
    0x11, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x0d, 0x00, 0x00, 0x00,
    0x2c, 0x00, 0x05, 0x00, 0x07, 0x00, 0x00, 0x00, 0x12, 0x00, 0x00, 0x00,
    0x0f, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x2c, 0x00, 0x07, 0x00,
    0x0a, 0x00, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x00, 0x00,
    0x10, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0x12, 0x00, 0x00, 0x00,
    0x17, 0x00, 0x04, 0x00, 0x14, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x2b, 0x00, 0x04, 0x00, 0x08, 0x00, 0x00, 0x00,
    0x15, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x04, 0x00,
    0x16, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x15, 0x00, 0x00, 0x00,
    0x1e, 0x00, 0x06, 0x00, 0x17, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00,
    0x06, 0x00, 0x00, 0x00, 0x16, 0x00, 0x00, 0x00, 0x16, 0x00, 0x00, 0x00,
    0x20, 0x00, 0x04, 0x00, 0x18, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
    0x17, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00, 0x18, 0x00, 0x00, 0x00,
    0x19, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x15, 0x00, 0x04, 0x00,
    0x1a, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x2b, 0x00, 0x04, 0x00, 0x1a, 0x00, 0x00, 0x00, 0x1b, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00, 0x1b, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00, 0x1c, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x1a, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00,
    0x1c, 0x00, 0x00, 0x00, 0x1d, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x20, 0x00, 0x04, 0x00, 0x1f, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00,
    0x07, 0x00, 0x00, 0x00, 0x2b, 0x00, 0x04, 0x00, 0x06, 0x00, 0x00, 0x00,
    0x22, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2b, 0x00, 0x04, 0x00,
    0x06, 0x00, 0x00, 0x00, 0x23, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3f,
    0x20, 0x00, 0x04, 0x00, 0x27, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
    0x14, 0x00, 0x00, 0x00, 0x36, 0x00, 0x05, 0x00, 0x02, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
    0xf8, 0x00, 0x02, 0x00, 0x05, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00,
    0x0b, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00,
    0x3e, 0x00, 0x03, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00,
    0x3d, 0x00, 0x04, 0x00, 0x1a, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00,
    0x1d, 0x00, 0x00, 0x00, 0x41, 0x00, 0x05, 0x00, 0x1f, 0x00, 0x00, 0x00,
    0x20, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00,
    0x3d, 0x00, 0x04, 0x00, 0x07, 0x00, 0x00, 0x00, 0x21, 0x00, 0x00, 0x00,
    0x20, 0x00, 0x00, 0x00, 0x51, 0x00, 0x05, 0x00, 0x06, 0x00, 0x00, 0x00,
    0x24, 0x00, 0x00, 0x00, 0x21, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x51, 0x00, 0x05, 0x00, 0x06, 0x00, 0x00, 0x00, 0x25, 0x00, 0x00, 0x00,
    0x21, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x50, 0x00, 0x07, 0x00,
    0x14, 0x00, 0x00, 0x00, 0x26, 0x00, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00,
    0x25, 0x00, 0x00, 0x00, 0x22, 0x00, 0x00, 0x00, 0x23, 0x00, 0x00, 0x00,
    0x41, 0x00, 0x05, 0x00, 0x27, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00,
    0x19, 0x00, 0x00, 0x00, 0x1b, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x03, 0x00,
    0x28, 0x00, 0x00, 0x00, 0x26, 0x00, 0x00, 0x00, 0xfd, 0x00, 0x01, 0x00,
    0x38, 0x00, 0x01, 0x00,
};

// Embedded Fragment Shader SPIR-V code (triangle.frag)
constexpr uint32_t frag_shader_size = 352;
alignas(4) constexpr uint8_t frag_shader_code[] = {
    0x03, 0x02, 0x23, 0x07, 0x00, 0x00, 0x01, 0x00, 0x0b, 0x00, 0x08, 0x00,
    0x0d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x00, 0x02, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x06, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x47, 0x4c, 0x53, 0x4c, 0x2e, 0x73, 0x74, 0x64, 0x2e, 0x34, 0x35, 0x30,
    0x00, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x06, 0x00, 0x04, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x6d, 0x61, 0x69, 0x6e, 0x00, 0x00, 0x00, 0x00,
    0x09, 0x00, 0x00, 0x00, 0x10, 0x00, 0x03, 0x00, 0x04, 0x00, 0x00, 0x00,
    0x07, 0x00, 0x00, 0x00, 0x03, 0x00, 0x03, 0x00, 0x02, 0x00, 0x00, 0x00,
    0xc2, 0x01, 0x00, 0x00, 0x05, 0x00, 0x04, 0x00, 0x04, 0x00, 0x00, 0x00,
    0x6d, 0x61, 0x69, 0x6e, 0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x05, 0x00,
    0x09, 0x00, 0x00, 0x00, 0x6f, 0x75, 0x74, 0x43, 0x6f, 0x6c, 0x6f, 0x72,
    0x00, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00, 0x09, 0x00, 0x00, 0x00,
    0x1e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x13, 0x00, 0x02, 0x00,
    0x02, 0x00, 0x00, 0x00, 0x21, 0x00, 0x03, 0x00, 0x03, 0x00, 0x00, 0x00,
    0x02, 0x00, 0x00, 0x00, 0x16, 0x00, 0x03, 0x00, 0x06, 0x00, 0x00, 0x00,
    0x20, 0x00, 0x00, 0x00, 0x17, 0x00, 0x04, 0x00, 0x07, 0x00, 0x00, 0x00,
    0x06, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00,
    0x08, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00,
    0x3b, 0x00, 0x04, 0x00, 0x08, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00,
    0x03, 0x00, 0x00, 0x00, 0x2b, 0x00, 0x04, 0x00, 0x06, 0x00, 0x00, 0x00,
    0x0a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3f, 0x2b, 0x00, 0x04, 0x00,
    0x06, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x2c, 0x00, 0x07, 0x00, 0x07, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00,
    0x0a, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x00, 0x00,
    0x0a, 0x00, 0x00, 0x00, 0x36, 0x00, 0x05, 0x00, 0x02, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
    0xf8, 0x00, 0x02, 0x00, 0x05, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x03, 0x00,
    0x09, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0xfd, 0x00, 0x01, 0x00,
    0x38, 0x00, 0x01, 0x00,
};

struct OverlayContext
{
    VkDevice device = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    VkFormat format = VK_FORMAT_UNDEFINED;
    VkExtent2D extent{};

    std::vector<VkImage> images;
    std::vector<VkImageView> imageViews;
    std::vector<VkFramebuffer> framebuffers;

    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;

    VkCommandPool commandPool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> commandBuffers;

    std::vector<VkSemaphore> overlaySemaphores;

    uint32_t graphicsQueueFamily = 0xFFFFFFFF;
    VkQueue graphicsQueue = VK_NULL_HANDLE;

    bool initialized = false;
} gOverlay;

void CleanupOverlayResources(const vkroots::VkDeviceDispatch& pDispatch)
{
    if (gOverlay.device == VK_NULL_HANDLE) return;

    printf("[Overlay] Waiting for device idle before cleanup...\n");
    pDispatch.DeviceWaitIdle(gOverlay.device);

    for (auto framebuffer : gOverlay.framebuffers) {
        if (framebuffer != VK_NULL_HANDLE) {
            pDispatch.DestroyFramebuffer(gOverlay.device, framebuffer, nullptr);
        }
    }
    gOverlay.framebuffers.clear();

    for (auto imageView : gOverlay.imageViews) {
        if (imageView != VK_NULL_HANDLE) {
            pDispatch.DestroyImageView(gOverlay.device, imageView, nullptr);
        }
    }
    gOverlay.imageViews.clear();

    if (gOverlay.pipeline != VK_NULL_HANDLE) {
        pDispatch.DestroyPipeline(gOverlay.device, gOverlay.pipeline, nullptr);
        gOverlay.pipeline = VK_NULL_HANDLE;
    }

    if (gOverlay.pipelineLayout != VK_NULL_HANDLE) {
        pDispatch.DestroyPipelineLayout(gOverlay.device, gOverlay.pipelineLayout, nullptr);
        gOverlay.pipelineLayout = VK_NULL_HANDLE;
    }

    if (gOverlay.renderPass != VK_NULL_HANDLE) {
        pDispatch.DestroyRenderPass(gOverlay.device, gOverlay.renderPass, nullptr);
        gOverlay.renderPass = VK_NULL_HANDLE;
    }

    if (gOverlay.commandPool != VK_NULL_HANDLE) {
        if (!gOverlay.commandBuffers.empty()) {
            pDispatch.FreeCommandBuffers(gOverlay.device, gOverlay.commandPool, static_cast<uint32_t>(gOverlay.commandBuffers.size()), gOverlay.commandBuffers.data());
            gOverlay.commandBuffers.clear();
        }
        pDispatch.DestroyCommandPool(gOverlay.device, gOverlay.commandPool, nullptr);
        gOverlay.commandPool = VK_NULL_HANDLE;
    }

    for (auto semaphore : gOverlay.overlaySemaphores) {
        if (semaphore != VK_NULL_HANDLE) {
            pDispatch.DestroySemaphore(gOverlay.device, semaphore, nullptr);
        }
    }
    gOverlay.overlaySemaphores.clear();

    gOverlay.images.clear();
    gOverlay.initialized = false;
    printf("[Overlay] Resources cleaned up successfully.\n");
}

class VkInstanceOverrides {
public:
    static VkResult CreateDevice(
        const vkroots::VkPhysicalDeviceDispatch& pDispatch,
        VkPhysicalDevice physicalDevice,
        const VkDeviceCreateInfo* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkDevice* pDevice)
    {
        VkResult result = pDispatch.CreateDevice(physicalDevice, pCreateInfo, pAllocator, pDevice);
        if (result == VK_SUCCESS)
        {
            printf("[Overlay] Device created successfully.\n");
            gOverlay.device = *pDevice;
            gOverlay.physicalDevice = physicalDevice;

            // Find graphics queue family from device creation info or check physical device
            gOverlay.graphicsQueueFamily = 0xFFFFFFFF;
            for (uint32_t i = 0; i < pCreateInfo->queueCreateInfoCount; ++i) {
                // Technically, any queue that is requested can be used, but we check if physical device supports graphics on it.
                uint32_t queueFamilyIndex = pCreateInfo->pQueueCreateInfos[i].queueFamilyIndex;
                uint32_t propCount = 0;
                pDispatch.GetPhysicalDeviceQueueFamilyProperties(physicalDevice, &propCount, nullptr);
                std::vector<VkQueueFamilyProperties> props(propCount);
                pDispatch.GetPhysicalDeviceQueueFamilyProperties(physicalDevice, &propCount, props.data());

                if (queueFamilyIndex < propCount && (props[queueFamilyIndex].queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
                    gOverlay.graphicsQueueFamily = queueFamilyIndex;
                    break;
                }
            }

            if (gOverlay.graphicsQueueFamily == 0xFFFFFFFF) {
                printf("[Overlay] Warning: Could not find graphics queue family in CreateDevice info.\n");
                // Default to 0 as fallback
                gOverlay.graphicsQueueFamily = 0;
            } else {
                printf("[Overlay] Selected Graphics Queue Family Index: %u\n", gOverlay.graphicsQueueFamily);
            }
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
        // Clean up old resources if any
        CleanupOverlayResources(pDispatch);

        VkResult result = pDispatch.CreateSwapchainKHR(device, pCreateInfo, pAllocator, pSwapchain);
        if (result == VK_SUCCESS)
        {
            printf("[Overlay] Swapchain created/recreated: %p, format: %d, extent: %dx%d\n",
                   (void*)*pSwapchain, pCreateInfo->imageFormat, pCreateInfo->imageExtent.width, pCreateInfo->imageExtent.height);
            gOverlay.swapchain = *pSwapchain;
            gOverlay.extent = pCreateInfo->imageExtent;
            gOverlay.format = pCreateInfo->imageFormat;
        }
        return result;
    }

    static void DestroySwapchainKHR(
        const vkroots::VkDeviceDispatch& pDispatch,
        VkDevice device,
        VkSwapchainKHR swapchain,
        const VkAllocationCallbacks* pAllocator)
    {
        CleanupOverlayResources(pDispatch);
        pDispatch.DestroySwapchainKHR(device, swapchain, pAllocator);
    }

    static VkResult GetSwapchainImagesKHR(
        const vkroots::VkDeviceDispatch& pDispatch,
        VkDevice device,
        VkSwapchainKHR swapchain,
        uint32_t* pSwapchainImageCount,
        VkImage* pSwapchainImages)
    {
        VkResult result = pDispatch.GetSwapchainImagesKHR(device, swapchain, pSwapchainImageCount, pSwapchainImages);

        if (result == VK_SUCCESS && pSwapchainImages)
        {
            gOverlay.images.assign(pSwapchainImages, pSwapchainImages + *pSwapchainImageCount);
            printf("[Overlay] Found %u swapchain images.\n", *pSwapchainImageCount);

            if (gOverlay.initialized) {
                return result;
            }

            // --- 1. Create Image Views ---
            gOverlay.imageViews.resize(gOverlay.images.size());
            for (size_t i = 0; i < gOverlay.images.size(); ++i) {
                VkImageViewCreateInfo viewInfo{};
                viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                viewInfo.image = gOverlay.images[i];
                viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
                viewInfo.format = gOverlay.format;
                viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
                viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
                viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
                viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
                viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                viewInfo.subresourceRange.baseMipLevel = 0;
                viewInfo.subresourceRange.levelCount = 1;
                viewInfo.subresourceRange.baseArrayLayer = 0;
                viewInfo.subresourceRange.layerCount = 1;

                if (pDispatch.CreateImageView(device, &viewInfo, nullptr, &gOverlay.imageViews[i]) != VK_SUCCESS) {
                    printf("[Overlay] Failed to create ImageView %zu\n", i);
                    return VK_ERROR_INITIALIZATION_FAILED;
                }
            }

            // --- 2. Create RenderPass (LOAD_OP_LOAD so we draw on top!) ---
            VkAttachmentDescription colorAttachment{};
            colorAttachment.format = gOverlay.format;
            colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
            colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD; // Load existing pixels
            colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // Save overlay
            colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            colorAttachment.initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

            VkAttachmentReference colorAttachmentRef{};
            colorAttachmentRef.attachment = 0;
            colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            VkSubpassDescription subpass{};
            subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpass.colorAttachmentCount = 1;
            subpass.pColorAttachments = &colorAttachmentRef;

            VkSubpassDependency dependency{};
            dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
            dependency.dstSubpass = 0;
            dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

            VkRenderPassCreateInfo renderPassInfo{};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            renderPassInfo.attachmentCount = 1;
            renderPassInfo.pAttachments = &colorAttachment;
            renderPassInfo.subpassCount = 1;
            renderPassInfo.pSubpasses = &subpass;
            renderPassInfo.dependencyCount = 1;
            renderPassInfo.pDependencies = &dependency;

            if (pDispatch.CreateRenderPass(device, &renderPassInfo, nullptr, &gOverlay.renderPass) != VK_SUCCESS) {
                printf("[Overlay] Failed to create RenderPass\n");
                return VK_ERROR_INITIALIZATION_FAILED;
            }

            // --- 3. Create Shader Modules ---
            VkShaderModuleCreateInfo vertCreateInfo{};
            vertCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            vertCreateInfo.codeSize = vert_shader_size;
            vertCreateInfo.pCode = reinterpret_cast<const uint32_t*>(vert_shader_code);
            VkShaderModule vertModule;
            if (pDispatch.CreateShaderModule(device, &vertCreateInfo, nullptr, &vertModule) != VK_SUCCESS) {
                printf("[Overlay] Failed to create vertex shader module\n");
                return VK_ERROR_INITIALIZATION_FAILED;
            }

            VkShaderModuleCreateInfo fragCreateInfo{};
            fragCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            fragCreateInfo.codeSize = frag_shader_size;
            fragCreateInfo.pCode = reinterpret_cast<const uint32_t*>(frag_shader_code);
            VkShaderModule fragModule;
            if (pDispatch.CreateShaderModule(device, &fragCreateInfo, nullptr, &fragModule) != VK_SUCCESS) {
                printf("[Overlay] Failed to create fragment shader module\n");
                pDispatch.DestroyShaderModule(device, vertModule, nullptr);
                return VK_ERROR_INITIALIZATION_FAILED;
            }

            VkPipelineShaderStageCreateInfo shaderStages[2]{};
            shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
            shaderStages[0].module = vertModule;
            shaderStages[0].pName = "main";

            shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
            shaderStages[1].module = fragModule;
            shaderStages[1].pName = "main";

            // --- 4. Create Pipeline ---
            VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
            vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

            VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
            inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
            inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP; // Draws our square
            inputAssembly.primitiveRestartEnable = VK_FALSE;

            VkViewport viewport{};
            viewport.x = 0.0f;
            viewport.y = 0.0f;
            viewport.width = static_cast<float>(gOverlay.extent.width);
            viewport.height = static_cast<float>(gOverlay.extent.height);
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;

            VkRect2D scissor{};
            scissor.offset = {0, 0};
            scissor.extent = gOverlay.extent;

            VkPipelineViewportStateCreateInfo viewportState{};
            viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
            viewportState.viewportCount = 1;
            viewportState.pViewports = &viewport;
            viewportState.scissorCount = 1;
            viewportState.pScissors = &scissor;

            VkPipelineRasterizationStateCreateInfo rasterizer{};
            rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
            rasterizer.depthClampEnable = VK_FALSE;
            rasterizer.rasterizerDiscardEnable = VK_FALSE;
            rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
            rasterizer.lineWidth = 1.0f;
            rasterizer.cullMode = VK_CULL_MODE_NONE; // No culling to be absolutely safe
            rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
            rasterizer.depthBiasEnable = VK_FALSE;

            VkPipelineMultisampleStateCreateInfo multisampling{};
            multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
            multisampling.sampleShadingEnable = VK_FALSE;
            multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

            VkPipelineColorBlendAttachmentState colorBlendAttachment{};
            colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
            colorBlendAttachment.blendEnable = VK_FALSE; // Draw solid red square on top

            VkPipelineColorBlendStateCreateInfo colorBlending{};
            colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
            colorBlending.logicOpEnable = VK_FALSE;
            colorBlending.attachmentCount = 1;
            colorBlending.pAttachments = &colorBlendAttachment;

            VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
            pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

            if (pDispatch.CreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &gOverlay.pipelineLayout) != VK_SUCCESS) {
                printf("[Overlay] Failed to create PipelineLayout\n");
                pDispatch.DestroyShaderModule(device, vertModule, nullptr);
                pDispatch.DestroyShaderModule(device, fragModule, nullptr);
                return VK_ERROR_INITIALIZATION_FAILED;
            }

            VkGraphicsPipelineCreateInfo pipelineInfo{};
            pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
            pipelineInfo.stageCount = 2;
            pipelineInfo.pStages = shaderStages;
            pipelineInfo.pVertexInputState = &vertexInputInfo;
            pipelineInfo.pInputAssemblyState = &inputAssembly;
            pipelineInfo.pViewportState = &viewportState;
            pipelineInfo.pRasterizationState = &rasterizer;
            pipelineInfo.pMultisampleState = &multisampling;
            pipelineInfo.pColorBlendState = &colorBlending;
            pipelineInfo.layout = gOverlay.pipelineLayout;
            pipelineInfo.renderPass = gOverlay.renderPass;
            pipelineInfo.subpass = 0;

            if (pDispatch.CreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &gOverlay.pipeline) != VK_SUCCESS) {
                printf("[Overlay] Failed to create GraphicsPipeline\n");
                pDispatch.DestroyShaderModule(device, vertModule, nullptr);
                pDispatch.DestroyShaderModule(device, fragModule, nullptr);
                return VK_ERROR_INITIALIZATION_FAILED;
            }

            pDispatch.DestroyShaderModule(device, vertModule, nullptr);
            pDispatch.DestroyShaderModule(device, fragModule, nullptr);

            // --- 5. Create Framebuffers ---
            gOverlay.framebuffers.resize(gOverlay.imageViews.size());
            for (size_t i = 0; i < gOverlay.imageViews.size(); i++) {
                VkFramebufferCreateInfo framebufferInfo{};
                framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
                framebufferInfo.renderPass = gOverlay.renderPass;
                framebufferInfo.attachmentCount = 1;
                framebufferInfo.pAttachments = &gOverlay.imageViews[i];
                framebufferInfo.width = gOverlay.extent.width;
                framebufferInfo.height = gOverlay.extent.height;
                framebufferInfo.layers = 1;

                if (pDispatch.CreateFramebuffer(device, &framebufferInfo, nullptr, &gOverlay.framebuffers[i]) != VK_SUCCESS) {
                    printf("[Overlay] Failed to create Framebuffer %zu\n", i);
                    return VK_ERROR_INITIALIZATION_FAILED;
                }
            }

            // --- 6. Create CommandPool and CommandBuffers ---
            VkCommandPoolCreateInfo poolInfo{};
            poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            poolInfo.queueFamilyIndex = gOverlay.graphicsQueueFamily;

            if (pDispatch.CreateCommandPool(device, &poolInfo, nullptr, &gOverlay.commandPool) != VK_SUCCESS) {
                printf("[Overlay] Failed to create CommandPool\n");
                return VK_ERROR_INITIALIZATION_FAILED;
            }

            gOverlay.commandBuffers.resize(gOverlay.framebuffers.size());
            VkCommandBufferAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocInfo.commandPool = gOverlay.commandPool;
            allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocInfo.commandBufferCount = static_cast<uint32_t>(gOverlay.commandBuffers.size());

            if (pDispatch.AllocateCommandBuffers(device, &allocInfo, gOverlay.commandBuffers.data()) != VK_SUCCESS) {
                printf("[Overlay] Failed to allocate CommandBuffers\n");
                return VK_ERROR_INITIALIZATION_FAILED;
            }

            // --- 7. Record Static Command Buffers ---
            for (size_t i = 0; i < gOverlay.commandBuffers.size(); i++) {
                VkCommandBufferBeginInfo beginInfo{};
                beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

                if (pDispatch.BeginCommandBuffer(gOverlay.commandBuffers[i], &beginInfo) != VK_SUCCESS) {
                    printf("[Overlay] Failed to begin CommandBuffer %zu\n", i);
                    return VK_ERROR_INITIALIZATION_FAILED;
                }

                VkRenderPassBeginInfo renderPassBegin{};
                renderPassBegin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                renderPassBegin.renderPass = gOverlay.renderPass;
                renderPassBegin.framebuffer = gOverlay.framebuffers[i];
                renderPassBegin.renderArea.offset = {0, 0};
                renderPassBegin.renderArea.extent = gOverlay.extent;

                pDispatch.CmdBeginRenderPass(gOverlay.commandBuffers[i], &renderPassBegin, VK_SUBPASS_CONTENTS_INLINE);
                pDispatch.CmdBindPipeline(gOverlay.commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, gOverlay.pipeline);
                pDispatch.CmdDraw(gOverlay.commandBuffers[i], 4, 1, 0, 0); // 4 vertices for the triangle strip square
                pDispatch.CmdEndRenderPass(gOverlay.commandBuffers[i]);

                if (pDispatch.EndCommandBuffer(gOverlay.commandBuffers[i]) != VK_SUCCESS) {
                    printf("[Overlay] Failed to record CommandBuffer %zu\n", i);
                    return VK_ERROR_INITIALIZATION_FAILED;
                }
            }

            // --- 8. Create Semaphores for Synchronization ---
            gOverlay.overlaySemaphores.resize(gOverlay.images.size());
            VkSemaphoreCreateInfo semaphoreInfo{};
            semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
            for (size_t i = 0; i < gOverlay.images.size(); ++i) {
                if (pDispatch.CreateSemaphore(device, &semaphoreInfo, nullptr, &gOverlay.overlaySemaphores[i]) != VK_SUCCESS) {
                    printf("[Overlay] Failed to create Semaphore %zu\n", i);
                    return VK_ERROR_INITIALIZATION_FAILED;
                }
            }

            gOverlay.initialized = true;
            printf("[Overlay] Fully initialized successfully!\n");
        }

        return result;
    }

    static VkResult QueuePresentKHR(
        const vkroots::VkQueueDispatch& pDispatch,
        VkQueue queue,
        const VkPresentInfoKHR* pPresentInfo)
    {
        if (!gOverlay.initialized || pPresentInfo->swapchainCount == 0) {
            return pDispatch.QueuePresentKHR(queue, pPresentInfo);
        }

        gOverlay.graphicsQueue = queue;

        uint32_t imageIndex = pPresentInfo->pImageIndices[0];
        if (imageIndex >= gOverlay.commandBuffers.size()) {
            return pDispatch.QueuePresentKHR(queue, pPresentInfo);
        }

        // --- Synchronization ---
        // We submit our overlay command buffer to run on the present queue.
        // It must wait on the original wait semaphores the application specified (so it runs after app rendering is complete).
        // It will signal our overlaySemaphore when complete.
        // The subsequent presentation call will then wait on our overlaySemaphore instead of the original semaphores.

        std::vector<VkPipelineStageFlags> waitStages(pPresentInfo->waitSemaphoreCount, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.waitSemaphoreCount = pPresentInfo->waitSemaphoreCount;
        submitInfo.pWaitSemaphores = pPresentInfo->pWaitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages.data();
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &gOverlay.commandBuffers[imageIndex];
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &gOverlay.overlaySemaphores[imageIndex];

        // Submit to graphics queue
        if (pDispatch.QueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
            printf("[Overlay] QueueSubmit failed during QueuePresentKHR!\n");
            return pDispatch.QueuePresentKHR(queue, pPresentInfo);
        }

        // Construct modified VkPresentInfoKHR where the wait semaphore is our overlay semaphore
        VkPresentInfoKHR modifiedPresentInfo = *pPresentInfo;
        modifiedPresentInfo.waitSemaphoreCount = 1;
        modifiedPresentInfo.pWaitSemaphores = &gOverlay.overlaySemaphores[imageIndex];

        return pDispatch.QueuePresentKHR(queue, &modifiedPresentInfo);
    }
};

} // namespace WoTLKGuiLayer

VKROOTS_DEFINE_LAYER_INTERFACES(WoTLKGuiLayer::VkInstanceOverrides,
                                WoTLKGuiLayer::VkDeviceOverrides);
