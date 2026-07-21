/**
 * @file wowmemory.cpp
 * @brief WoW process memory reading implementation.
 *
 * This Vulkan layer is loaded into the WoW process, so all absolute addresses
 * from offsets.h can be accessed directly as pointers.  Every dereference is
 * guarded against null/zero base pointers to avoid access violations when the
 * game is in a state where a particular value is not yet initialised.
 */

#include "wowmemory/wowmemory.h"
#include "wowmemory/offsets.h"

#include <cstdint>
#include <cstring>
#include <string>
#include <unistd.h>
#include <sys/mman.h>

namespace WoWMemory {

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

/// Safely check if a memory range is mapped and readable.
static bool IsReadableRange(uintptr_t addr, size_t size)
{
    if (size == 0) return true;
    static const long page_size = sysconf(_SC_PAGESIZE);

    uintptr_t start = addr;
    uintptr_t end = start + size;

    uintptr_t first_page = start & ~(page_size - 1);
    uintptr_t last_page = (end - 1) & ~(page_size - 1);

    // 1-entry thread_local cache to avoid redundant mincore system calls.
    thread_local uintptr_t s_last_page = 0;
    thread_local bool s_last_readable = false;

    for (uintptr_t p = first_page; p <= last_page; p += page_size) {
        if (p == s_last_page) {
            if (!s_last_readable) return false;
            continue;
        }

        unsigned char vec;
        bool readable = (mincore(reinterpret_cast<void*>(p), page_size, &vec) == 0);
        s_last_page = p;
        s_last_readable = readable;

        if (!readable) return false;
    }
    return true;
}

/// Safely cast an absolute 32-bit address to a typed pointer.
template<typename T>
static const T* AbsPtr(uint32_t addr)
{
    return reinterpret_cast<const T*>(static_cast<uintptr_t>(addr));
}

template<typename T>
static T ReadAbs(uint32_t addr, T fallback = T{})
{
    if (!IsReadableRange(static_cast<uintptr_t>(addr), sizeof(T))) {
        return fallback;
    }
    return *AbsPtr<T>(addr);
}

/// Safely read a string up to maxLen, checking each page's readability first.
static std::string ReadSafeString(uintptr_t startAddr, size_t maxLen)
{
    if (!startAddr) return {};
    static const long page_size = sysconf(_SC_PAGESIZE);

    uintptr_t current = startAddr;
    size_t len = 0;

    uintptr_t current_page = 0;
    bool current_page_readable = false;

    while (len < maxLen) {
        uintptr_t page = current & ~(page_size - 1);
        if (page != current_page) {
            current_page = page;
            current_page_readable = IsReadableRange(current_page, page_size);
        }

        if (!current_page_readable) {
            break;
        }

        const char* p = reinterpret_cast<const char*>(current);
        if (*p == '\0') {
            break;
        }

        current++;
        len++;
    }

    if (len == 0) return {};
    return std::string(reinterpret_cast<const char*>(startAddr), len);
}

// ---------------------------------------------------------------------------
// GameDataReader — private static helpers
// ---------------------------------------------------------------------------

std::string GameDataReader::ReadInlineString(uint32_t absAddr, size_t maxLen)
{
    return ReadSafeString(static_cast<uintptr_t>(absAddr), maxLen);
}

std::string GameDataReader::ReadIndirectString(uint32_t ptrAddr, size_t maxLen)
{
    if (!ptrAddr) return {};
    uintptr_t strPtr = ReadAbs<uintptr_t>(ptrAddr);
    if (!strPtr) return {};
    return ReadSafeString(strPtr, maxLen);
}

// ---------------------------------------------------------------------------
// GameDataReader::ReadGameData
// ---------------------------------------------------------------------------

bool GameDataReader::ReadGameData(GameData& out)
{
    // ---- Identity ----
    out.playerName      = ReadInlineString(WoWOffsets::playerName, 12);
    out.realmName       = ReadInlineString(WoWOffsets::realmName, 64);
    out.localPlayerGUID = ReadAbs<uint64_t>(WoWOffsets::localPlayerGUID);

    // ---- World location ----
    // continentName, zoneText and subZoneText are char* pointers stored at
    // those addresses (the pointer holds the address of the string buffer).
    out.continentName = ReadIndirectString(WoWOffsets::continentName);
    out.zoneText      = ReadIndirectString(WoWOffsets::zoneText);
    out.subZoneText   = ReadIndirectString(WoWOffsets::subZoneText);
    out.mapID         = ReadAbs<uint32_t>(WoWOffsets::mapID);
    out.zoneID        = ReadAbs<uint32_t>(WoWOffsets::zoneID);

    // ---- Game state ----
    out.gameState    = ReadAbs<uint32_t>(WoWOffsets::gameState);
    out.worldLoaded  = ReadAbs<uint8_t>(WoWOffsets::worldLoaded) != 0;
    out.isLoading    = ReadAbs<uint8_t>(WoWOffsets::isLoading)   != 0;
    out.isIndoor     = ReadAbs<uint8_t>(WoWOffsets::isIndoor)    != 0;
    out.tickCount    = ReadAbs<uint32_t>(WoWOffsets::tickCount);

    // ---- Player state ----
    out.playerIsIngame = ReadAbs<uint8_t>(WoWOffsets::playerIsIngame) != 0;

    // playerHealth lives at playerBase_ptr + 0x19B8.
    // The null check on playerBasePtr guards the most common failure mode
    // (not ingame / object manager not ready). Further VA-range validation
    // is performed safely using our IsReadableRange check.
    uintptr_t playerBasePtr = ReadAbs<uintptr_t>(WoWOffsets::playerBase);
    if (playerBasePtr && IsReadableRange(playerBasePtr + WoWOffsets::playerHealth, sizeof(uint32_t))) {
        out.playerHealth = *reinterpret_cast<const uint32_t*>(
            playerBasePtr + WoWOffsets::playerHealth);
    } else {
        out.playerHealth = 0;
    }

    // ---- Corpse ----
    out.corpseX = ReadAbs<float>(WoWOffsets::corpseX);
    out.corpseY = ReadAbs<float>(WoWOffsets::corpseY);
    out.corpseZ = ReadAbs<float>(WoWOffsets::corpseZ);

    return true;
}

// ---------------------------------------------------------------------------
// DoubleBufferedGameData
// ---------------------------------------------------------------------------

void DoubleBufferedGameData::SwapBuffers()
{
    mReadIndex = 1 - mReadIndex;
}

const GameData& DoubleBufferedGameData::GetReadBuffer() const
{
    return mBuffers[mReadIndex];
}

GameData& DoubleBufferedGameData::GetWriteBuffer()
{
    return mBuffers[1 - mReadIndex];
}

} // namespace WoWMemory
