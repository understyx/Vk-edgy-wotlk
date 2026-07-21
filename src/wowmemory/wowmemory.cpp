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

namespace WoWMemory {

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

/// Safely cast an absolute 32-bit address to a typed pointer.
template<typename T>
static const T* AbsPtr(uint32_t addr)
{
    return reinterpret_cast<const T*>(static_cast<uintptr_t>(addr));
}

template<typename T>
static T ReadAbs(uint32_t addr)
{
    return *AbsPtr<T>(addr);
}

// ---------------------------------------------------------------------------
// GameDataReader — private static helpers
// ---------------------------------------------------------------------------

std::string GameDataReader::ReadInlineString(uint32_t absAddr, size_t maxLen)
{
    if (!absAddr) return {};
    const char* p = AbsPtr<char>(absAddr);
    if (!p) return {};
    // strnlen guards against missing null terminator
    size_t len = ::strnlen(p, maxLen);
    return std::string(p, len);
}

std::string GameDataReader::ReadIndirectString(uint32_t ptrAddr, size_t maxLen)
{
    if (!ptrAddr) return {};
    // Read the pointer stored at ptrAddr
    uintptr_t strPtr = ReadAbs<uintptr_t>(ptrAddr);
    if (!strPtr) return {};
    const char* p = reinterpret_cast<const char*>(strPtr);
    size_t len = ::strnlen(p, maxLen);
    return std::string(p, len);
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
    // (not ingame / object manager not ready).  Further VA-range validation
    // is not feasible without platform SEH / signal handling; the caller
    // (running inside the WoW process) accepts the same risk as any bot/hook.
    uintptr_t playerBasePtr = ReadAbs<uintptr_t>(WoWOffsets::playerBase);
    if (playerBasePtr) {
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
