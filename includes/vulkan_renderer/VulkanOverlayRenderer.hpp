#pragma once
#include "ultralight_renderer/WebUIRenderingEngine.hpp"
#include <atomic>

#include "common/PointerSwapBuffer.hpp"

namespace VulkanRenderer {

class VulkanOverlayRenderer {
public:
    VulkanOverlayRenderer();
    ~VulkanOverlayRenderer();

    void initializeVulkanResources();
    void updateAndRenderOverlay(const UltralightRenderer::WebTexture& texture);
};

// Rendering thread loop representing Vulkan present composition context.
void renderingThreadLoop(
    std::atomic<bool>& running,
    PointerSwapBuffer<UltralightRenderer::WebTexture>& uiTextureBuffer);

} // namespace VulkanRenderer
