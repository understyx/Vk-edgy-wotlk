/**
 * @file ultralight_manager.cpp
 * @brief Implementation of UltralightManager for Ultralight renderer management
 */

#include "jshtml/ultralight_manager.h"
#include <Ultralight/Ultralight.h>
#include <cstring>
#include <cstdio>

namespace WoWHTML {

UltralightManager::UltralightManager() 
    : mRenderer(nullptr), mView(nullptr), mBitmap(nullptr)
{
}

UltralightManager::~UltralightManager()
{
    Shutdown();
}

bool UltralightManager::Initialize(uint32_t width, uint32_t height, const std::string& resourcePath)
{
    if (mRenderer || mView) {
        fprintf(stderr, "UltralightManager: Already initialized\n");
        return false;
    }
    
    mWidth = width;
    mHeight = height;
    
    try {
        // Create Ultralight configuration
        auto config = ultralight::Config::Create();
        
        if (!resourcePath.empty()) {
            config->set_resource_path(ultralight::String(resourcePath.c_str()));
        }
        
        // Set other configuration options as needed
        config->set_cache_path(ultralight::String("./cache"));
        
        // Create the renderer
        mRenderer = ultralight::Renderer::Create(config);
        if (!mRenderer) {
            fprintf(stderr, "UltralightManager: Failed to create Ultralight renderer\n");
            return false;
        }
        
        // Create a view with the specified dimensions
        auto viewConfig = ultralight::ViewConfig();
        mView = mRenderer->CreateView(mWidth, mHeight, viewConfig);
        if (!mView) {
            fprintf(stderr, "UltralightManager: Failed to create Ultralight view\n");
            return false;
        }
        
        // Get the initial render target to create a bitmap
        auto renderTarget = mView->render_target();
        if (renderTarget && renderTarget->texture_id == 0) {
            // Use offscreen rendering
            mView->set_needs_paint(true);
        }
        
        fprintf(stderr, "UltralightManager: Successfully initialized (%ux%u)\n", mWidth, mHeight);
        return true;
    } 
    catch (const std::exception& e) {
        fprintf(stderr, "UltralightManager: Initialization failed with exception: %s\n", e.what());
        mRenderer = nullptr;
        mView = nullptr;
        return false;
    }
}

void UltralightManager::Shutdown()
{
    {
        std::lock_guard<std::mutex> lock(mGameStateMutex);
        while (!mGameStateQueue.empty()) {
            mGameStateQueue.pop();
        }
    }
    
    mView = nullptr;
    mRenderer = nullptr;
    mBitmap = nullptr;
    mWidth = 0;
    mHeight = 0;
    
    fprintf(stderr, "UltralightManager: Shutdown complete\n");
}

bool UltralightManager::LoadHTML(const std::string& htmlContent)
{
    if (!mView) {
        fprintf(stderr, "UltralightManager: Cannot load HTML, renderer not initialized\n");
        return false;
    }
    
    try {
        mView->LoadHTML(ultralight::String(htmlContent.c_str()));
        mView->Focus();
        return true;
    }
    catch (const std::exception& e) {
        fprintf(stderr, "UltralightManager: Failed to load HTML: %s\n", e.what());
        return false;
    }
}

bool UltralightManager::LoadHTMLFile(const std::string& htmlFilePath)
{
    if (!mView) {
        fprintf(stderr, "UltralightManager: Cannot load HTML file, renderer not initialized\n");
        return false;
    }
    
    try {
        std::string fileUrl = "file://" + htmlFilePath;
        mView->LoadURL(ultralight::String(fileUrl.c_str()));
        mView->Focus();
        return true;
    }
    catch (const std::exception& e) {
        fprintf(stderr, "UltralightManager: Failed to load HTML file: %s\n", e.what());
        return false;
    }
}

void UltralightManager::QueueGameStateUpdate(const std::string& jsonData)
{
    {
        std::lock_guard<std::mutex> lock(mGameStateMutex);
        mGameStateQueue.push(jsonData);
    }
}

void UltralightManager::ProcessGameStateUpdates()
{
    std::queue<std::string> updates;
    {
        std::lock_guard<std::mutex> lock(mGameStateMutex);
        updates = mGameStateQueue;
        while (!mGameStateQueue.empty()) {
            mGameStateQueue.pop();
        }
    }
    
    // Process each update by calling JavaScript
    if (!mView) {
        return;
    }
    
    while (!updates.empty()) {
        const auto& jsonData = updates.front();
        try {
            // Call JavaScript function to update game state
            // Example: window.updateGameState(jsonData)
            std::string jsCode = "if (window.updateGameState) { window.updateGameState(" + jsonData + "); }";
            mView->EvaluateScript(ultralight::String(jsCode.c_str()));
        }
        catch (const std::exception& e) {
            fprintf(stderr, "UltralightManager: Failed to process game state update: %s\n", e.what());
        }
        updates.pop();
    }
}

bool UltralightManager::Render()
{
    if (!mRenderer || !mView) {
        fprintf(stderr, "UltralightManager: Cannot render, not initialized\n");
        return false;
    }
    
    try {
        // Process any pending game state updates
        ProcessGameStateUpdates();
        
        // Update the renderer
        mRenderer->Update();
        
        // Render the view
        mRenderer->Render();
        
        // Get the render target
        auto renderTarget = mView->render_target();
        if (!renderTarget) {
            return false;
        }
        
        // For offscreen rendering, get the bitmap
        mBitmap = mView->bitmap();
        
        return mBitmap != nullptr;
    }
    catch (const std::exception& e) {
        fprintf(stderr, "UltralightManager: Render failed: %s\n", e.what());
        return false;
    }
}

const uint8_t* UltralightManager::GetBitmapData(uint32_t& outWidth, uint32_t& outHeight) const
{
    if (!mBitmap) {
        outWidth = 0;
        outHeight = 0;
        return nullptr;
    }
    
    outWidth = mBitmap->width();
    outHeight = mBitmap->height();
    
    return static_cast<const uint8_t*>(mBitmap->LockPixels());
}

uint32_t UltralightManager::GetBitmapSize() const
{
    if (!mBitmap) {
        return 0;
    }
    
    return mBitmap->width() * mBitmap->height() * 4; // RGBA format
}

ultralight::RefPtr<ultralight::View> UltralightManager::GetView() const
{
    return mView;
}

} // namespace WoWHTML
