#pragma once

#include <RmlUi/Core/RenderInterface.h>
#include <vulkan/vulkan.h>
#include "vkroots.h"

#include <string>
#include <vector>

namespace WoTLKGuiLayer {

/**
 * @class VulkanRenderer
 * @brief Implements Rml::RenderInterface for a Vulkan overlay layer.
 *
 * Renders RmlUi elements on top of existing swapchain images using
 * VK_ATTACHMENT_LOAD_OP_LOAD so the game's own rendering is preserved.
 * Alpha blending is enabled so the UI composites correctly.
 *
 * Usage sequence per frame:
 *   1. renderer.SetCurrentImageIndex(imageIndex)
 *   2. cmd = renderer.PrepareFrame()          // begins cmd buffer + render pass
 *   3. rmlContext->Render()                   // calls CompileGeometry/RenderGeometry
 *   4. renderer.SubmitFrame(queue, wait, signal, fence)
 */
class VulkanRenderer : public Rml::RenderInterface {
public:
    VulkanRenderer() = default;
    ~VulkanRenderer() override;

    // ---- Lifecycle ----

    struct InitInfo {
        VkDevice                         device;
        VkPhysicalDevice                 physicalDevice;
        const vkroots::VkDeviceDispatch* dispatch;
        uint32_t                         graphicsQueueFamilyIndex;
        std::string                      shaderDir;
    };

    bool Initialize(const InitInfo& info);
    void Shutdown();

    // Called once after Initialize() when the graphics queue is available.
    // Uploads the white fallback texture and creates the graphics pipeline.
    void SetupWithQueue(VkQueue queue);

    // ---- Swapchain lifecycle ----
    void ResizeSwapchain(const std::vector<VkImage>& images,
                         VkFormat                    format,
                         VkExtent2D                  extent);
    void DestroySwapchainResources();

    // ---- Per-frame control ----
    void            SetCurrentImageIndex(uint32_t imageIndex);
    VkCommandBuffer PrepareFrame();
    void            SubmitFrame(VkQueue                         queue,
                                const std::vector<VkSemaphore>& waitSemaphores,
                                VkSemaphore                     signalSemaphore,
                                VkFence                         fence);

    // ---- Rml::RenderInterface ----
    Rml::CompiledGeometryHandle CompileGeometry(Rml::Span<const Rml::Vertex> vertices,
                                                Rml::Span<const int>         indices) override;
    void RenderGeometry(Rml::CompiledGeometryHandle geometry,
                        Rml::Vector2f               translation,
                        Rml::TextureHandle          texture) override;
    void ReleaseGeometry(Rml::CompiledGeometryHandle geometry) override;

    Rml::TextureHandle LoadTexture(Rml::Vector2i&     texture_dimensions,
                                   const Rml::String& source) override;
    Rml::TextureHandle GenerateTexture(Rml::Span<const Rml::byte> source,
                                       Rml::Vector2i              source_dimensions) override;
    void ReleaseTexture(Rml::TextureHandle texture_handle) override;

    void EnableScissorRegion(bool enable) override;
    void SetScissorRegion(Rml::Rectanglei region) override;
    void SetTransform(const Rml::Matrix4f* transform) override;

private:
    // ---- GPU helpers ----
    uint32_t FindMemoryType(uint32_t type_filter, VkMemoryPropertyFlags props) const;

    VkResult CreateDeviceBuffer(VkDeviceSize           size,
                                VkBufferUsageFlags     usage,
                                VkMemoryPropertyFlags  props,
                                VkBuffer&              out_buffer,
                                VkDeviceMemory&        out_memory) const;

    VkResult UploadToBuffer(VkDeviceMemory memory,
                            const void*    data,
                            VkDeviceSize   size) const;

    VkResult CreateDeviceImage(uint32_t              width,
                               uint32_t              height,
                               VkFormat              format,
                               VkImageUsageFlags     usage,
                               VkImage&              out_image,
                               VkDeviceMemory&       out_memory) const;

    VkResult UploadImageData(VkImage     image,
                             uint32_t    width,
                             uint32_t    height,
                             const void* pixels,
                             VkDeviceSize pixel_bytes);

    VkResult CreateTextureDescriptorSet(VkImageView view, VkDescriptorSet& out_set) const;
    VkResult CreateShaderModule(const std::vector<char>& code, VkShaderModule& out) const;

    // ---- Init helpers ----
    VkResult CreateCommandPool(uint32_t queue_family);
    VkResult CreateDescriptorSetLayout();
    VkResult CreatePipelineLayout();
    VkResult CreateSampler();
    VkResult CreateDescriptorPool();
    VkResult CreateRenderPass();
    VkResult CreatePipeline();
    VkResult CreateWhiteTextureResources();
    void     UploadWhiteTexture();

    // ---- Core handles ----
    VkDevice                         m_device         = VK_NULL_HANDLE;
    VkPhysicalDevice                 m_physicalDevice = VK_NULL_HANDLE;
    const vkroots::VkDeviceDispatch* m_dispatch       = nullptr;
    VkQueue                          m_uploadQueue    = VK_NULL_HANDLE;
    std::string                      m_shaderDir;

    // ---- Persistent Vulkan objects ----
    VkCommandPool         m_commandPool    = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_descSetLayout  = VK_NULL_HANDLE;
    VkPipelineLayout      m_pipelineLayout = VK_NULL_HANDLE;
    VkSampler             m_sampler        = VK_NULL_HANDLE;
    VkDescriptorPool      m_descPool       = VK_NULL_HANDLE;

    // ---- Swapchain-dependent objects ----
    VkFormat   m_swapchainFormat = VK_FORMAT_UNDEFINED;
    VkExtent2D m_swapchainExtent = {};
    VkRenderPass m_renderPass    = VK_NULL_HANDLE;
    VkPipeline   m_pipeline      = VK_NULL_HANDLE;

    struct FrameData {
        VkCommandBuffer cmdBuf      = VK_NULL_HANDLE;
        VkFramebuffer   framebuffer = VK_NULL_HANDLE;
        VkImageView     imageView   = VK_NULL_HANDLE;
    };
    std::vector<FrameData> m_frames;

    // ---- White fallback texture ----
    VkImage         m_whiteImage   = VK_NULL_HANDLE;
    VkImageView     m_whiteView    = VK_NULL_HANDLE;
    VkDeviceMemory  m_whiteMemory  = VK_NULL_HANDLE;
    VkDescriptorSet m_whiteDescSet = VK_NULL_HANDLE;
    bool            m_whiteReady   = false;

    // ---- Active-frame render state ----
    uint32_t        m_currentImage  = 0;
    VkCommandBuffer m_currentCmdBuf = VK_NULL_HANDLE;

    // Push constant layout must match rmlui.vert / rmlui.frag exactly
    struct PushConstants {
        float    transform[16]; // mat4  (64 B, column-major)
        float    translate[2];  // vec2  ( 8 B)
        uint32_t hasTexture;    // uint  ( 4 B)
        float    padding;       //       ( 4 B)
    };                          // total  80 B
    PushConstants   m_push          = {};
    bool            m_scissorActive = false;
    Rml::Rectanglei m_scissorRegion = {};

    // ---- Geometry buffer (heap-allocated, referenced by CompiledGeometryHandle) ----
    struct GeometryBuffer {
        VkBuffer       vertexBuffer  = VK_NULL_HANDLE;
        VkDeviceMemory vertexMemory  = VK_NULL_HANDLE;
        VkBuffer       indexBuffer   = VK_NULL_HANDLE;
        VkDeviceMemory indexMemory   = VK_NULL_HANDLE;
        uint32_t       indexCount    = 0;
    };

    // ---- Texture data (heap-allocated, referenced by TextureHandle) ----
    struct TextureData {
        VkImage         image   = VK_NULL_HANDLE;
        VkImageView     view    = VK_NULL_HANDLE;
        VkDeviceMemory  memory  = VK_NULL_HANDLE;
        VkDescriptorSet descSet = VK_NULL_HANDLE;
    };

    bool m_initialized = false;
};

} // namespace WoTLKGuiLayer
