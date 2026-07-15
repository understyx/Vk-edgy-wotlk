# Ultralight Integration Guide

## Overview

This document describes the Ultralight integration into the WoTLK Vulkan layer, enabling HTML/CSS/JavaScript-based UI rendering in the game overlay.

## Architecture

The Ultralight integration follows a producer-consumer pattern:

1. **Game Reading Thread** (Producer)
   - Reads game state (player position, FPS, stats, etc.)
   - Queues JSON state updates via `HTMLRenderer::UpdateGameState()`
   - Non-blocking, thread-safe operation

2. **Ultralight Rendering Thread** (Consumer)
   - Processes queued game state updates  
   - Executes JavaScript callbacks to update UI
   - Renders to bitmap during frame presentation
   - Outputs rendered UI to Vulkan rendering layer

3. **Vulkan Rendering Layer**
   - Receives rendered bitmap from Ultralight
   - Composites UI onto game screen
   - Presents final frame

## Usage Example

### Initialization

```cpp
#include "jshtml/jshtml.h"

// Create and initialize the HTML renderer
WoWHTML::HTMLRenderer renderer;
if (renderer.Initialize(1920, 1080)) {
    // Load the UI HTML file or inline HTML
    renderer.LoadHTML(R"(
        <html><body>
            <div id="stats"></div>
        </body></html>
    )");
}
```

### Sending Game State Updates

From your game reading thread, send game state updates as JSON:

```cpp
// In game loop / reading thread
std::string gameState = R"({
    "fps": 60,
    "location": "Stormwind",
    "playersOnline": 1250,
    "playerHealth": 5000,
    "playerMana": 3000
})";

renderer.UpdateGameState(gameState);
```

### Receiving Updates in HTML/JavaScript

The HTML UI file receives state updates via the `updateGameState()` callback:

```javascript
window.updateGameState = function(gameData) {
    // Update UI elements based on game state
    document.getElementById('fps').textContent = gameData.fps;
    document.getElementById('location').textContent = gameData.location;
    // ... update other UI elements
};
```

### Rendering

During frame presentation, call:

```cpp
uint64_t uiTextureHandle = renderer.RenderToTexture();
// Use the texture handle to get bitmap data:
// uint32_t width, height;
// const uint8_t* bitmapData = renderer.GetBitmapData(width, height);
```

## Thread Safety

The implementation uses mutex-protected queues for thread-safe communication:

- `UpdateGameState()` uses a mutex to safely queue updates from the game thread
- The rendering thread processes queued updates when `Render()` is called
- No blocking operations occur between threads

## Files

- `include/jshtml/jshtml.h` - Main HTMLRenderer interface
- `include/jshtml/ultralight_manager.h` - UltralightManager for internal use
- `src/jshtml/jshtml.cpp` - HTMLRenderer implementation
- `src/jshtml/ultralight_manager.cpp` - UltralightManager implementation
- `ui/overlay.html` - Example UI file

## Building

The CMakeLists.txt automatically links against Ultralight libraries:
- libUltralight.so
- libUltralightCore.so
- libWebCore.so
- libAppCore.so

Ensure these libraries are present in the `lib/` directory.

## Integration with Vulkan

The HTMLRenderer is integrated into the Vulkan layer via:

1. **Initialization**: In `VkDeviceOverrides::CreateSwapchainKHR()`, the renderer is created with swapchain dimensions
2. **Rendering**: In `VkDeviceOverrides::QueuePresentKHR()`, the renderer outputs the current frame's UI
3. **Cleanup**: In `VkDeviceOverrides::DestroySwapchainKHR()`, resources are freed

## Performance Considerations

- Game state updates are queued asynchronously to avoid blocking the game thread
- Ultralight rendering happens during frame presentation to minimize latency
- Bitmap data is available immediately after rendering for Vulkan upload
- Consider throttling update frequency if performance is impacted

## Future Enhancements

1. Direct texture output from Ultralight to Vulkan (bypass bitmap)
2. Input handling (mouse, keyboard) passed to Ultralight
3. Multiple views for different UI panels
4. Local resource loading (images, fonts) for UI assets
5. Caching of compiled JavaScript/CSS for performance

## Troubleshooting

### Blank UI overlay
- Check that HTML content was loaded successfully
- Verify game state updates are being sent
- Check console output for JavaScript errors

### Memory issues
- Reduce UI resolution if needed
- Minimize HTML DOM complexity
- Profile bitmap allocation and cleanup

### Rendering artifacts
- Ensure proper texture format conversion between Ultralight and Vulkan
- Check for layout transitions and synchronization issues
