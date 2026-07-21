# WoTLK GUI Vulkan Layer

A Vulkan layer that provides an interactive GUI overlay for World of Warcraft (WotLK client).

## Features

- **Vulkan Layer Architecture** - Intercepts game rendering via Vulkan API hooks
- **Modular Design** - Separated concerns for layer, memory reading, and UI rendering
- **Non-Blocking Overlay** - Layer hooks execute efficiently without blocking game rendering
- **Extensible** - Clear interfaces for adding new features


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


### With Validation Layers

Enable Vulkan validation to catch errors:

```bash
VK_INSTANCE_LAYERS=VK_LAYER_KHRONOS_validation:VK_LAYER_understyx_WOTLK ./game
```


## Known Issues

1. **Validation Layer Errors** - Current implementation may trigger Vulkan validation errors

