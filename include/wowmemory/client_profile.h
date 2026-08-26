#pragma once

#include <cstdint>
#include <string>

namespace WoWMemory {

// This address profile is valid only for the stock enUS WoW 3.3.5a (12340)
// executable fingerprinted in tools/inspect_wow_client.py.
inline constexpr uint16_t kSupportedClientMachine = 0x014C; // IMAGE_FILE_MACHINE_I386
inline constexpr uint32_t kSupportedClientTimestamp = 0x4C2452FE;
inline constexpr uint32_t kSupportedClientImageBase = 0x00400000;
inline constexpr uint32_t kSupportedClientImageSize = 0x009FD000;
inline constexpr uint32_t kSupportedClientEntryPointRva = 0x00001000;
inline constexpr const char* kSupportedClientSha256 =
    "bf644876709c591acc17c0da8cdf1814edcc9f1e6bc109a8c0d5c38c79dc953c";

struct ClientProfileStatus {
    bool supported = false;
    uintptr_t moduleBase = 0;
    uint16_t machine = 0;
    uint32_t timestamp = 0;
    uint32_t imageSize = 0;
    uint32_t entryPointRva = 0;
    std::string message;
};

// Cached inspection of the executable hosting the injected reader DLL.
const ClientProfileStatus& GetClientProfileStatus();

} // namespace WoWMemory
