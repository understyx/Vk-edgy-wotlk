/**
 * @file jshtml.h
 * @brief JavaScript/HTML rendering engine interface
 * 
 * This module handles the rendering of HTML/CSS/JavaScript content to
 * be displayed as an overlay on top of the game.
 * 
 * It integrates with a JavaScript engine (possibly Ultralight) to provide
 * a modern web-based UI that can display complex layouts with styles and
 * dynamic behavior. The rendered output is then composited into the Vulkan
 * rendering pipeline.
 * 
 * The module accepts game state (from WoWMemory) and uses it to update
 * the HTML UI in real-time.
 */

#ifndef JSHTML_H
#define JSHTML_H

#include <string>
#include <memory>
#include <cstdint>

namespace WoWHTML {

/**
 * @class HTMLRenderer
 * @brief Renders HTML/CSS/JavaScript content for display
 * 
 * This class manages the lifecycle of the HTML rendering engine and
 * provides methods to load HTML content and render it to a texture.
 */
class HTMLRenderer {
public:
    HTMLRenderer() = default;
    ~HTMLRenderer() = default;
    
    /**
     * @brief Initialize the HTML renderer
     * 
     * Sets up the rendering context and prepares for rendering.
     * 
     * @param width Render target width
     * @param height Render target height
     * @return true if initialization succeeded
     */
    bool Initialize(uint32_t width, uint32_t height);
    
    /**
     * @brief Load an HTML file or string
     * 
     * @param htmlContent The HTML content to load
     * @return true if loading succeeded
     */
    bool LoadHTML(const std::string& htmlContent);
    
    /**
     * @brief Update JavaScript state with game data
     * 
     * This method allows passing game state to the JavaScript engine
     * so the UI can display and respond to game events.
     * 
     * @param jsonData JSON-formatted game state
     * @return true if update succeeded
     */
    bool UpdateGameState(const std::string& jsonData);
    
    /**
     * @brief Render the HTML content to a texture
     * 
     * @return Vulkan image handle that contains the rendered content
     */
    uint64_t RenderToTexture();
    
    /**
     * @brief Get the rendered texture dimensions
     * @return width and height of the rendered texture
     */
    void GetTextureSize(uint32_t& outWidth, uint32_t& outHeight) const;
    
private:
    uint32_t mWidth = 0;
    uint32_t mHeight = 0;
    // JavaScript VM and rendering context will be stored here
};

/**
 * @class UILayout
 * @brief Manages UI layout configuration and state
 * 
 * This class handles the layout logic for the overlay UI, including
 * positioning, sizing, and visibility of UI elements.
 */
class UILayout {
public:
    UILayout() = default;
    ~UILayout() = default;
    
    /**
     * @brief Set the layout configuration from JSON
     * @param layoutJSON JSON configuration string
     * @return true if configuration was valid
     */
    bool SetLayoutFromJSON(const std::string& layoutJSON);
    
    /**
     * @brief Get current layout as JSON
     * @return JSON representation of current layout
     */
    std::string GetLayoutAsJSON() const;
    
private:
    // Layout configuration and state
};

} // namespace WoWHTML

#endif // JSHTML_H
