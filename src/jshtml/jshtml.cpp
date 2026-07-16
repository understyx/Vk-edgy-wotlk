/**
 * @file jshtml.cpp
 * @brief Implementation of HTMLRenderer using Ultralight
 */

#include "jshtml/jshtml.h"
#include "jshtml/ultralight_manager.h"
#include <memory>
#include <cstdio>

namespace WoWHTML {

// ============================================================================
// Helper function to get the global Ultralight manager instance
// Uses Meyer's singleton pattern to ensure proper initialization and lifetime
// ============================================================================
static UltralightManager* GetGlobalUltralightManager() {
    static UltralightManager instance;
    return &instance;
}

// ============================================================================
// HTMLRenderer implementation
// ============================================================================

bool HTMLRenderer::Initialize(uint32_t width, uint32_t height)
{
    UltralightManager* manager = GetGlobalUltralightManager();
    
    mWidth = width;
    mHeight = height;
    
    if (!manager->Initialize(width, height)) {
        fprintf(stderr, "HTMLRenderer: Failed to initialize Ultralight manager\n");
        return false;
    }
    
    fprintf(stderr, "HTMLRenderer: Initialized (%ux%u)\n", mWidth, mHeight);
    return true;
}

bool HTMLRenderer::LoadHTML(const std::string& htmlContent)
{
    UltralightManager* manager = GetGlobalUltralightManager();
    return manager->LoadHTML(htmlContent);
}

bool HTMLRenderer::UpdateGameState(const std::string& jsonData)
{
    UltralightManager* manager = GetGlobalUltralightManager();
    
    // Queue the game state update for the rendering thread to process
    manager->QueueGameStateUpdate(jsonData);
    return true;
}

uint64_t HTMLRenderer::RenderToTexture()
{
    UltralightManager* manager = GetGlobalUltralightManager();
    
    if (!manager->Render()) {
        fprintf(stderr, "HTMLRenderer: Failed to render\n");
        return 0;
    }
    
    uint32_t width, height;
    const uint8_t* bitmapData = manager->GetBitmapData(width, height);
    
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
