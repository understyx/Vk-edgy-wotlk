/**
 * @file ultralight_manager.h
 * @brief Manager for Ultralight renderer with thread-safe game state updates
 * 
 * This class handles the initialization and management of Ultralight's renderer,
 * along with thread-safe communication between the game thread and the rendering thread.
 */

#ifndef ULTRALIGHT_MANAGER_H
#define ULTRALIGHT_MANAGER_H

#include <memory>
#include <string>
#include <queue>
#include <mutex>
#include <cstdint>
#include <Ultralight/Ultralight.h>

namespace WoWHTML {

/**
 * @class UltralightManager
 * @brief Manages Ultralight renderer and thread-safe game state communication
 */
class UltralightManager {
public:
    UltralightManager();
    ~UltralightManager();
    
    /**
     * @brief Initialize the Ultralight renderer
     * 
     * @param width Render target width
     * @param height Render target height
     * @param resourcePath Path to Ultralight resources (fonts, etc)
     * @return true if initialization succeeded
     */
    bool Initialize(uint32_t width, uint32_t height, const std::string& resourcePath = "");
    
    /**
     * @brief Shutdown and cleanup Ultralight resources
     */
    void Shutdown();
    
    /**
     * @brief Load HTML content into the view
     * 
     * @param htmlContent HTML content to load
     * @return true if loading succeeded
     */
    bool LoadHTML(const std::string& htmlContent);
    
    /**
     * @brief Load an HTML file into the view
     * 
     * @param htmlFilePath Path to HTML file
     * @return true if loading succeeded
     */
    bool LoadHTMLFile(const std::string& htmlFilePath);
    
    /**
     * @brief Queue game state data for the UI to consume
     * 
     * This method is thread-safe and can be called from the game thread.
     * The data is stored in a queue to be processed by the Ultralight thread.
     * 
     * @param jsonData JSON-formatted game state
     */
    void QueueGameStateUpdate(const std::string& jsonData);
    
    /**
     * @brief Render the Ultralight view to a bitmap
     * 
     * Should be called periodically to render the UI to an updatable bitmap.
     * 
     * @return true if rendering succeeded
     */
    bool Render();
    
    /**
     * @brief Get the rendered bitmap data
     * 
     * @param outWidth Output width
     * @param outHeight Output height
     * @return Pointer to RGBA8 bitmap data
     */
    const uint8_t* GetBitmapData(uint32_t& outWidth, uint32_t& outHeight) const;
    
    /**
     * @brief Get the size of the bitmap in bytes
     * 
     * @return Size of the bitmap data
     */
    uint32_t GetBitmapSize() const;
    
    /**
     * @brief Get the Ultralight view
     * 
     * @return RefPtr to the Ultralight view
     */
    ultralight::RefPtr<ultralight::View> GetView() const;
    
private:
    ultralight::RefPtr<ultralight::Renderer> mRenderer;
    ultralight::RefPtr<ultralight::View> mView;
    ultralight::RefPtr<ultralight::Bitmap> mBitmap;
    
    std::queue<std::string> mGameStateQueue;
    std::mutex mGameStateMutex;
    
    uint32_t mWidth = 0;
    uint32_t mHeight = 0;
    
    /**
     * @brief Process queued game state updates
     * 
     * Internal method to apply pending game state updates to the UI.
     */
    void ProcessGameStateUpdates();
};

} // namespace WoWHTML

#endif // ULTRALIGHT_MANAGER_H
