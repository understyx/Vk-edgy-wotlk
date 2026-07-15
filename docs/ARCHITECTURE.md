# WoTLK GUI Layer Architecture

## Overview

This project implements a Vulkan layer that provides an interactive GUI overlay for World of Warcraft (WotLK client). The architecture separates concerns into distinct modules for maintainability and scalability.

## Project Structure

```
├── src/                           # Source files
│   ├── main.cpp                   # Entry point (thin layer for compilation)
│   ├── vklayer/                   # Vulkan layer implementation
│   │   └── vk_layer.cpp           # Vulkan hook implementations
│   ├── wowmemory/                 # WoW memory reading (future)
│   │   └── wowmemory.cpp          # Game data reader (placeholder)
│   └── jshtml/                    # HTML/JS rendering (future)
│       └── jshtml.cpp             # UI rendering engine (placeholder)
│
├── include/                       # Header files
│   ├── vkroots.h                  # Vulkan layer infrastructure
│   ├── vklayer/
│   │   └── vk_layer.h             # Vulkan layer interfaces
│   ├── wowmemory/
│   │   └── wowmemory.h            # Memory reader interface
│   └── jshtml/
│       └── jshtml.h               # UI renderer interface
│
├── docs/                          # Documentation
│   ├── ARCHITECTURE.md            # This file
│   ├── VULKAN_LAYER_PHASES.md     # Vulkan layer lifecycle phases
│   └── README.md                  # User guide
│
├── VkLayer_understyx_WOTLK.json   # Vulkan layer manifest
├── CMakeLists.txt                 # Build configuration
└── wotlk_offsets.txt              # WoW memory offsets (to be created)
```

## Module Descriptions

### Vulkan Layer (src/vklayer/)

The Vulkan layer intercepts Vulkan API calls made by the game to draw custom overlay content.

**Key Components:**
- `VkInstanceOverrides` - Handles instance-level hooks (device creation)
- `VkDeviceOverrides` - Handles device-level hooks (swapchain, rendering, presentation)
- `OverlayContext` - Stores Vulkan resources (swapchain, images, pipeline, etc.)

**Current Implementation:**
Currently logs Vulkan function calls and captures device/swapchain information. Future phases will:
1. Create rendering pipeline for overlay
2. Record command buffers with overlay draw calls
3. Submit overlay rendering before frame presentation

### WoW Memory Reader (src/wowmemory/) - *Future*

Reads game state from WoW memory at regular intervals.

**Key Concepts:**
- Uses memory offsets defined in `wotlk_offsets.txt`
- Runs on separate thread for non-blocking reads
- Uses double-buffering to safely exchange data with rendering thread
- Reads: player position, health, target info, NPC data, buff/debuffs, quest status, etc.

**Planned Components:**
- `GameDataReader` - Performs actual memory reads
- `DoubleBufferedGameData` - Thread-safe data exchange
- `GameData` - Struct containing game state

### HTML/JS UI Renderer (src/jshtml/) - *Future*

Renders modern, interactive UI using HTML/CSS/JavaScript.

**Planned Features:**
- Integration with JavaScript engine (possibly Ultralight)
- Real-time UI updates based on game state
- Custom event handling and input processing
- Texture-based rendering compatible with Vulkan compositing

**Planned Components:**
- `HTMLRenderer` - Manages rendering pipeline
- `UILayout` - Manages layout configuration
- JavaScript bindings for accessing game data

## Data Flow

```
┌─────────────────────────────────────────────────────────────┐
│                    Game Process (WoW)                       │
└─────────────────────────────────────────────────────────────┘
                             ▲
                             │ Vulkan API calls
                             │
            ┌────────────────▼────────────────┐
            │  Vulkan Layer (vk_layer.cpp)   │
            │  - Hook Vulkan functions       │
            │  - Capture device/swapchain    │
            └──────────┬─────────────────────┘
                       │
        ┌──────────────┼──────────────┐
        │              │              │
        ▼              ▼              ▼
   [VkContext]   [GameData]      [UI State]
        │              │              │
        ▼              ▼              ▼
    [Vulkan    [WoWMemory]      [HTMLRenderer]
     Rendering]    Reader           (future)
        │              │              │
        └──────────────┼──────────────┘
                       │
                       ▼
            ┌─────────────────────┐
            │  Frame Composition  │
            │  - Combine overlays │
            │  - Submit to GPU    │
            └─────────────────────┘
                       │
                       ▼
            ┌─────────────────────┐
            │  Display on Screen  │
            └─────────────────────┘
```

## Vulkan Layer Phases

The Vulkan layer initialization happens in specific phases. See `VULKAN_LAYER_PHASES.md` for detailed information about each phase.

1. **Device Creation** - Capture VkDevice handle
2. **Swapchain Creation** - Capture swapchain and image format
3. **Image Acquisition** - Get the current image to render into
4. **Command Recording** - Record overlay drawing commands
5. **Queue Submission** - Submit overlay rendering
6. **Frame Presentation** - Present the final composited frame

## Build System

The project uses CMake for building. The resulting shared library is loaded by the Vulkan loader based on the manifest in `VkLayer_understyx_WOTLK.json`.

### Layer Discovery

For the Vulkan loader to find this layer:
```bash
export VK_LAYER_PATH=/path/to/Vk-edgy-wotlk
export VK_INSTANCE_LAYERS=VK_LAYER_understyx_WOTLK
```

## Threading Model

- **Main Thread** - Runs the Vulkan layer hooks (low-latency, non-blocking)
- **Memory Reader Thread** - Periodically reads game memory (can be blocking)
- **UI Rendering Thread** - May be needed for complex UI updates (future)

Data exchange between threads uses double-buffering to avoid synchronization overhead.

## Future Considerations

1. **Validation Layer Errors** - Current implementation has validation errors that need fixing
2. **Performance** - Minimize per-frame overhead from layer hooks
3. **UI Integration** - Integrate JS/HTML rendering with Vulkan compositing
4. **Memory Efficiency** - Optimize memory reads and storage
5. **Error Handling** - Add robust error handling and logging
6. **Configuration** - Support configuration files for customization
