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
#include "wowmemory/memory_utils.h"
#include "wowmemory/auras.h"
#include "wowmemory/unit_info.h"
#include "wowmemory/combat_log.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <string>

namespace WoWMemory {

// ---------------------------------------------------------------------------
// GameDataReader — private static helpers
// ---------------------------------------------------------------------------

std::string GameDataReader::ReadInlineString(uint32_t absAddr, size_t maxLen)
{
    return WoWMemory::ReadInlineString(absAddr, maxLen);
}

std::string GameDataReader::ReadIndirectString(uint32_t ptrAddr, size_t maxLen)
{
    return WoWMemory::ReadIndirectString(ptrAddr, maxLen);
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
    out.playerIsIngame    = ReadAbs<uint8_t>(WoWOffsets::playerIsIngame) != 0;
    out.playerComboPoints = ReadAbs<uint8_t>(WoWOffsets::comboPoints);

    // ---- GUIDs ----
    out.mouseOverGUID  = ReadAbs<uint64_t>(WoWOffsets::mouseOverGUID);
    out.lastTargetGUID = ReadAbs<uint64_t>(WoWOffsets::lastTargetGUID);

    // ---- Corpse ----
    out.corpseX = ReadAbs<float>(WoWOffsets::corpseX);
    out.corpseY = ReadAbs<float>(WoWOffsets::corpseY);
    out.corpseZ = ReadAbs<float>(WoWOffsets::corpseZ);

    // ---- Player object — position, unit fields, casting, auras ----
    // playerBase is the local player object pointer.
    // We try to retrieve it first via Object Manager from localPlayerGUID,
    // falling back to absolute playerBase pointer if not found.
    uintptr_t playerBasePtr = 0;
    if (out.localPlayerGUID != 0) {
        playerBasePtr = GetObjectBaseByGUID(out.localPlayerGUID);
    }
    if (!playerBasePtr) {
        playerBasePtr = ReadAbs<uint32_t>(WoWOffsets::playerBase);
    }

    if (playerBasePtr && IsReadableRange(playerBasePtr, 4)) {
        // --- Position ---
        ReadPlayerPosition(playerBasePtr, out);

        // --- Health (legacy path kept, now also via unit fields) ---
        if (IsReadableRange(playerBasePtr + WoWOffsets::playerHealth, sizeof(uint32_t))) {
            out.playerHealth = *reinterpret_cast<const uint32_t*>(
                playerBasePtr + WoWOffsets::playerHealth);
        } else {
            out.playerHealth = 0;
        }

        // --- Unit fields (descriptor array) ---
        ReadPlayerUnitFields(playerBasePtr, out);

        // --- Casting / channeling ---
        auto readRelU32 = [&](uint32_t relOffset) -> uint32_t {
            uintptr_t addr = playerBasePtr + relOffset;
            if (!IsReadableRange(addr, sizeof(uint32_t))) return 0u;
            return *reinterpret_cast<const uint32_t*>(addr);
        };
        out.castingSpellId = readRelU32(WoWOffsets::unitCastingIdOffset);
        out.channelSpellId = readRelU32(WoWOffsets::unitChannelIdOffset);

        // --- Auras ---
        out.auraCount = ReadAurasForUnit(playerBasePtr, out.auras, GameData::kMaxAuras);
    } else {
        out.playerHealth    = 0;
        out.playerMaxHealth = 0;
        out.playerLevel     = 0;
        out.playerPower     = 0;
        out.playerMaxPower  = 0;
        out.castingSpellId  = 0;
        out.channelSpellId  = 0;
        out.auraCount       = 0;
    }

    // ---- Target State & Auras ----
    out.targetAuraCount = 0;
    out.targetHealth    = 0;
    out.targetMaxHealth = 0;
    out.targetLevel     = 0;
    out.targetPower     = 0;
    out.targetMaxPower  = 0;
    out.targetPowerType = 0;
    out.targetName      = "";

    if (out.targetGUID != 0) {
        uintptr_t targetBasePtr = GetObjectBaseByGUID(out.targetGUID);
        if (targetBasePtr) {
            ReadTargetUnitFields(targetBasePtr, out);
            out.targetAuraCount = ReadAurasForUnit(targetBasePtr, out.targetAuras, GameData::kMaxTargetAuras);
            out.targetName = GetUnitName(out.targetGUID, targetBasePtr);
        }
    }

    // ---- Camera ----
    ReadCameraInfo(out);

    // ---- Combat log (incremental read of new events since last call) ----
    ReadCombatLogEvents(m_lastCombatLogNodeAddr, out);

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
