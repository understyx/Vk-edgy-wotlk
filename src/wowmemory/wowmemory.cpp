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
    uintptr_t strPtr = ReadAbs<uint32_t>(ptrAddr);
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
    // (The object manager path via currentClientConnection + currentManagerOffset
    //  can be used for iterating other objects, but is not needed here.)
    uintptr_t playerBasePtr = ReadAbs<uint32_t>(WoWOffsets::playerBase);

    if (playerBasePtr && IsReadableRange(playerBasePtr, 4)) {
        // --- Position ---
        auto readRelFloat = [&](uint32_t relOffset) -> float {
            uintptr_t addr = playerBasePtr + relOffset;
            if (!IsReadableRange(addr, sizeof(float))) return 0.0f;
            return *reinterpret_cast<const float*>(addr);
        };
        out.playerPosX     = readRelFloat(WoWOffsets::objectPosX);
        out.playerPosY     = readRelFloat(WoWOffsets::objectPosY);
        out.playerPosZ     = readRelFloat(WoWOffsets::objectPosZ);
        out.playerRotation = readRelFloat(WoWOffsets::objectRotation);

        // --- Health (legacy path kept, now also via unit fields) ---
        if (IsReadableRange(playerBasePtr + WoWOffsets::playerHealth, sizeof(uint32_t))) {
            out.playerHealth = *reinterpret_cast<const uint32_t*>(
                playerBasePtr + WoWOffsets::playerHealth);
        } else {
            out.playerHealth = 0;
        }

        // --- Unit fields (descriptor array) ---
        uintptr_t unitFieldsAddr = 0;
        uintptr_t ufPtrAddr = playerBasePtr + WoWOffsets::objectUnitFields;
        if (IsReadableRange(ufPtrAddr, sizeof(uint32_t))) {
            unitFieldsAddr = *reinterpret_cast<const uint32_t*>(ufPtrAddr);
        }

        if (unitFieldsAddr) {
            auto readUF32 = [&](uint32_t relOffset) -> uint32_t {
                uintptr_t addr = unitFieldsAddr + relOffset;
                if (!IsReadableRange(addr, sizeof(uint32_t))) return 0u;
                return *reinterpret_cast<const uint32_t*>(addr);
            };

            out.playerHealth    = readUF32(WoWOffsets::unitFieldHealth);
            out.playerMaxHealth = readUF32(WoWOffsets::unitFieldMaxHealth);
            out.playerLevel     = readUF32(WoWOffsets::unitFieldLevel);
            out.targetGUID      = [&]() -> uint64_t {
                uintptr_t addr = unitFieldsAddr + WoWOffsets::unitFieldTargetGUID;
                if (!IsReadableRange(addr, sizeof(uint64_t))) return 0ull;
                return *reinterpret_cast<const uint64_t*>(addr);
            }();

            // Power type is stored as a byte in the object descriptor struct.
            uintptr_t descriptorAddr = 0;
            uintptr_t descPtrAddr = playerBasePtr + WoWOffsets::objectDescriptorOffset;
            if (IsReadableRange(descPtrAddr, sizeof(uint32_t))) {
                descriptorAddr = *reinterpret_cast<const uint32_t*>(descPtrAddr);
            }
            if (descriptorAddr) {
                uintptr_t ptAddr = descriptorAddr + WoWOffsets::unitFieldPowerTypeByteFromDescriptor;
                if (IsReadableRange(ptAddr, sizeof(uint8_t))) {
                    out.playerPowerType = *reinterpret_cast<const uint8_t*>(ptAddr);
                }
            }

            // Read current and max power for the player's power type.
            // UNIT_FIELD_POWERS and UNIT_FIELD_MAXPOWERS are arrays of 7 uint32s.
            uint8_t pt = out.playerPowerType;
            if (pt < 7) {
                uintptr_t curAddr = unitFieldsAddr + WoWOffsets::unitFieldPowers + pt * sizeof(uint32_t);
                uintptr_t maxAddr = unitFieldsAddr + WoWOffsets::unitFieldMaxPowers + pt * sizeof(uint32_t);
                if (IsReadableRange(curAddr, sizeof(uint32_t)))
                    out.playerPower = *reinterpret_cast<const uint32_t*>(curAddr);
                if (IsReadableRange(maxAddr, sizeof(uint32_t)))
                    out.playerMaxPower = *reinterpret_cast<const uint32_t*>(maxAddr);
            }
        }

        // --- Casting / channeling ---
        auto readRelU32 = [&](uint32_t relOffset) -> uint32_t {
            uintptr_t addr = playerBasePtr + relOffset;
            if (!IsReadableRange(addr, sizeof(uint32_t))) return 0u;
            return *reinterpret_cast<const uint32_t*>(addr);
        };
        out.castingSpellId = readRelU32(WoWOffsets::unitCastingIdOffset);
        out.channelSpellId = readRelU32(WoWOffsets::unitChannelIdOffset);

        // --- Auras ---
        // WoW maintains two aura tables; table 1 is used when the aura count
        // stored at auraCount1Offset is non-zero, otherwise fall back to table 2.
        out.auraCount = 0;
        uint32_t auraCount1 = readRelU32(WoWOffsets::auraCount1Offset);
        uint32_t auraCount2 = readRelU32(WoWOffsets::auraCount2Offset);

        uintptr_t auraTablePtr = 0;
        uint32_t  totalAuras   = 0;

        if (auraCount1 > 0) {
            // Table 1: the pointer itself is stored inline at auraTable1Offset
            uintptr_t tAddr = playerBasePtr + WoWOffsets::auraTable1Offset;
            if (IsReadableRange(tAddr, sizeof(uint32_t))) {
                auraTablePtr = *reinterpret_cast<const uint32_t*>(tAddr);
                totalAuras   = auraCount1;
            }
        } else if (auraCount2 > 0) {
            // Table 2: an external pointer stored at auraTable2Offset
            uintptr_t tAddr = playerBasePtr + WoWOffsets::auraTable2Offset;
            if (IsReadableRange(tAddr, sizeof(uint32_t))) {
                auraTablePtr = *reinterpret_cast<const uint32_t*>(tAddr);
                totalAuras   = auraCount2;
            }
        }

        if (auraTablePtr && totalAuras > 0) {
            uint32_t count = totalAuras < GameData::kMaxAuras
                           ? totalAuras : static_cast<uint32_t>(GameData::kMaxAuras);
            for (uint32_t i = 0; i < count; ++i) {
                uintptr_t entryAddr = auraTablePtr
                    + static_cast<uintptr_t>(i) * WoWOffsets::auraStructSize
                    + WoWOffsets::auraStructSpellIdOffset;
                if (!IsReadableRange(entryAddr, sizeof(uint32_t))) break;
                uint32_t sid = *reinterpret_cast<const uint32_t*>(entryAddr);
                if (sid == 0) continue;
                out.auras[out.auraCount++].spellId = sid;
            }
        }
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

    // ---- Camera ----
    // Chain: *(cameraBasePtrOffset) + cameraOffset1 -> ptr -> + cameraOffset2 -> camera struct
    uintptr_t camPtr1 = ReadAbs<uint32_t>(WoWOffsets::cameraBasePtrOffset);
    if (camPtr1) {
        uintptr_t camPtr2Addr = camPtr1 + WoWOffsets::cameraOffset1;
        if (IsReadableRange(camPtr2Addr, sizeof(uint32_t))) {
            uintptr_t camPtr2 = *reinterpret_cast<const uint32_t*>(camPtr2Addr);
            if (camPtr2) {
                uintptr_t camStructAddr = camPtr2 + WoWOffsets::cameraOffset2;
                if (IsReadableRange(camStructAddr, sizeof(uint32_t))) {
                    uintptr_t camStruct = *reinterpret_cast<const uint32_t*>(camStructAddr);
                    if (camStruct) {
                        uintptr_t yawAddr   = camStruct + WoWOffsets::cameraYawOffset;
                        uintptr_t pitchAddr = camStruct + WoWOffsets::cameraPitchOffset;
                        if (IsReadableRange(yawAddr, sizeof(float)))
                            out.cameraYaw = *reinterpret_cast<const float*>(yawAddr);
                        if (IsReadableRange(pitchAddr, sizeof(float)))
                            out.cameraPitch = *reinterpret_cast<const float*>(pitchAddr);
                    }
                }
            }
        }
    }

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
