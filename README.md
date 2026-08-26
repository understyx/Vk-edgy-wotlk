# WoTLK GUI Vulkan Layer

A Vulkan layer that provides an interactive GUI overlay for World of Warcraft (WotLK client).

## Features

- **Vulkan Layer Architecture** - Intercepts game rendering via Vulkan API hooks
- **Modular Design** - Separated concerns for layer, memory reading, and UI rendering
- **Non-Blocking Overlay** - Layer hooks execute efficiently without blocking game rendering
- **Extensible** - Clear interfaces for adding new features


## Building

### Prerequisites

- CMake 3.16+
- Vulkan SDK
- C++20 compatible compiler
- 32-bit MinGW-w64 C++ compiler (`i686-w64-mingw32-g++`) for the injected reader DLL
- Linux (layer compilation is Linux-specific)

### Build Steps

```bash
mkdir build
cd build
cmake ..
make
```

This produces `libvklayer_understyx_wotlk.so` which is the Vulkan layer shared library.
When the 32-bit MinGW compiler is installed, it also produces
`build/dist/wowmemory.dll` and `build/dist/injector.exe`.

## Supported WoW client

The current absolute-address profile supports only the supplied stock enUS
WoW 3.3.5a (build 12340) executable:

- SHA-256: `bf644876709c591acc17c0da8cdf1814edcc9f1e6bc109a8c0d5c38c79dc953c`
- PE timestamp: `0x4C2452FE`
- preferred/actual image base: `0x00400000`
- image size: `0x009FD000`
- entry-point RVA: `0x00001000`

The injected reader validates the PE metadata before performing any absolute
memory reads and disables itself on a mismatch. The SHA-256 is the offline
identity reference; use the inspector to verify a client before building or
injecting:

```bash
python3 tools/inspect_wow_client.py /path/to/Wow.exe
```

See [docs/client-memory-profile.md](docs/client-memory-profile.md) for the
verified offsets and the remaining live-runtime validation work.

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
