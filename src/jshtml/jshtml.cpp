/**
 * @file jshtml.cpp
 * @brief Implementation of HTMLRenderer using Ultralight
 */

#include "jshtml/jshtml.h"
#include "jshtml/ultralight_manager.h"
#include <memory>
#include <cstdio>

namespace WoWHTML {

// Global Ultralight manager instance
// NOTE: Using raw pointer instead of unique_ptr to prevent automatic cleanup
// The manager needs to persist for the lifetime of the layer, not get destroyed
// when static destructors run (which can happen prematurely during library unload)
static UltralightManager* gUltralightManager = nullptr;

// ============================================================================
// HTMLRenderer implementation
// ============================================================================

bool HTMLRenderer::Initialize(uint32_t width, uint32_t height)
{
    if (!gUltralightManager) {
        gUltralightManager = new UltralightManager();
    }
    
    mWidth = width;
    mHeight = height;
    
    if (!gUltralightManager->Initialize(width, height)) {
        fprintf(stderr, "HTMLRenderer: Failed to initialize Ultralight manager\n");
        return false;
    }
    
    fprintf(stderr, "HTMLRenderer: Initialized (%ux%u)\n", mWidth, mHeight);
    return true;
}

bool HTMLRenderer::LoadHTML(const std::string& htmlContent)
{
    if (!gUltralightManager) {
        fprintf(stderr, "HTMLRenderer: Ultralight manager not initialized\n");
        return false;
    }
    
    return gUltralightManager->LoadHTML(htmlContent);
}

bool HTMLRenderer::UpdateGameState(const std::string& jsonData)
{
    if (!gUltralightManager) {
        fprintf(stderr, "HTMLRenderer: Ultralight manager not initialized\n");
        return false;
    }
    
    // Queue the game state update for the rendering thread to process
    gUltralightManager->QueueGameStateUpdate(jsonData);
    return true;
}

uint64_t HTMLRenderer::RenderToTexture()
{
    if (!gUltralightManager) {
        fprintf(stderr, "HTMLRenderer: Ultralight manager not initialized\n");
        return 0;
    }
    
    if (!gUltralightManager->Render()) {
        fprintf(stderr, "HTMLRenderer: Failed to render\n");
        return 0;
    }
    
    uint32_t width, height;
    const uint8_t* bitmapData = gUltralightManager->GetBitmapData(width, height);
    
    if (!bitmapData) {
        fprintf(stderr, "HTMLRenderer: No bitmap data available\n");
        return 0;
    }
    
    // Return a handle that represents the bitmap data
    // In a full implementation, this would convert to a Vulkan texture
    // For now, we return a pointer cast to uint64_t
    return reinterpret_cast<uint64_t>(bitmapData);
}

void HTMLRenderer::GetTextureSize(uint32_t& outWidth, uint32_t& outHeight) const
{
    outWidth = mWidth;
    outHeight = mHeight;
}

// ============================================================================
// UILayout implementation
// ============================================================================

bool UILayout::SetLayoutFromJSON(const std::string& layoutJSON)
{
    // TODO: Parse JSON and configure layout
    fprintf(stderr, "UILayout: Setting layout from JSON (not yet implemented)\n");
    return true;
}

std::string UILayout::GetLayoutAsJSON() const
{
    // TODO: Serialize layout to JSON
    fprintf(stderr, "UILayout: Getting layout as JSON (not yet implemented)\n");
    return "{}";
}

} // namespace WoWHTML
