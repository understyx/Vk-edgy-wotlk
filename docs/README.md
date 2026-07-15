# WoTLK GUI Vulkan Layer

A Vulkan layer that provides an interactive GUI overlay for World of Warcraft (WotLK client).

## Features

- **Vulkan Layer Architecture** - Intercepts game rendering via Vulkan API hooks
- **Modular Design** - Separated concerns for layer, memory reading, and UI rendering
- **Non-Blocking Overlay** - Layer hooks execute efficiently without blocking game rendering
- **Extensible** - Clear interfaces for adding new features

## Project Structure

See [ARCHITECTURE.md](docs/ARCHITECTURE.md) for detailed architecture documentation.

```
src/
├── main.cpp                  # Entry point
├── vklayer/                  # Vulkan layer implementation
├── wowmemory/               # WoW memory reader (future)
└── jshtml/                  # HTML/JS UI renderer (future)

include/
├── vkroots.h               # Vulkan layer infrastructure
├── vklayer/                # Layer interfaces
├── wowmemory/              # Memory reader interface
└── jshtml/                 # UI renderer interface
```

## Building

### Prerequisites

- CMake 3.10+
- Vulkan SDK
- C++17 compatible compiler
- Linux (layer compilation is Linux-specific)

### Build Steps

```bash
mkdir build
cd build
cmake ..
make
```

This produces `libvklayer_understyx_wotlk.so` which is the Vulkan layer shared library.

## Installing

Copy the built library and manifest to a known location:

```bash
# Install to Vulkan layers directory
mkdir -p ~/.config/vulkan/icd.d
cp libvklayer_understyx_wotlk.so ~/.config/vulkan/icd.d/
cp VkLayer_understyx_WOTLK.json ~/.config/vulkan/icd.d/
```

Or set environment variables to use local directory:

```bash
export VK_LAYER_PATH=/path/to/build/directory
export VK_INSTANCE_LAYERS=VK_LAYER_understyx_WOTLK
```

## Running

Launch the game with the layer enabled:

```bash
VK_LAYER_PATH=/path/to/build VK_INSTANCE_LAYERS=VK_LAYER_understyx_WOTLK wine WoW.exe
```

## Debugging

The layer prints diagnostic messages to stdout. Watch for messages like:

```
The app has made an image, I bet it's going to be frogtastically beautiful!
Frame Finished. Presenting image: 0
nothing blew up vkQueueSubmit!
```

### With Validation Layers

Enable Vulkan validation to catch errors:

```bash
VK_INSTANCE_LAYERS=VK_LAYER_KHRONOS_validation:VK_LAYER_understyx_WOTLK ./game
```

## Current Status

- ✅ Device initialization hook
- ✅ Swapchain creation hook
- ✅ Image acquisition hook
- ✅ Queue submission hook
- ✅ Frame presentation hook
- ⚠️ Vulkan validation errors (to be fixed)
- 🔄 Command recording and overlay rendering (in progress)
- 📋 WoW memory reading (planned)
- 📋 HTML/JS UI rendering (planned)

## Documentation

- [ARCHITECTURE.md](docs/ARCHITECTURE.md) - System architecture and module descriptions
- [VULKAN_LAYER_PHASES.md](docs/VULKAN_LAYER_PHASES.md) - Detailed explanation of Vulkan layer lifecycle

## Known Issues

1. **Validation Layer Errors** - Current implementation may trigger Vulkan validation errors
   - See issues section for tracking

## Future Work

1. Fix validation layer errors
2. Implement command buffer recording for overlay rendering
3. Add WoW memory reader with double-buffering
4. Integrate HTML/JS rendering engine
5. Support for multiple overlay elements
6. Input handling (mouse, keyboard)
7. Configuration system

## References

- [Vulkan API Specification](https://www.khronos.org/vulkan/)
- [Vulkan Tutorial](https://vulkan-tutorial.com/)
- [Vulkan Layers](https://github.com/KhronosGroup/Vulkan-Loader/tree/main/loader)

## License

To be determined

## Contributing

See CONTRIBUTING.md (to be created)

## Contact

For issues and questions, please open an issue on GitHub.
