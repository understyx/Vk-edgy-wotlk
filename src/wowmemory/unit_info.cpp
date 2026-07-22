#include "wowmemory/unit_info.h"
#include "wowmemory/memory_utils.h"
#include "wowmemory/offsets.h"
#include "wowmemory/objects/wow_player.h"

namespace WoWMemory {

void ReadPlayerUnitFields(uintptr_t playerBasePtr, GameData& out)
{
    WoWPlayer player(playerBasePtr);
    if (!player.IsValid()) return;

    out.playerHealth    = player.GetHealth();
    out.playerMaxHealth = player.GetMaxHealth();
    out.playerLevel     = player.GetLevel();
    out.targetGUID      = player.GetTargetGUID();
    out.playerPowerType = player.GetPowerType();
    out.playerPower     = player.GetPower(out.playerPowerType);
    out.playerMaxPower  = player.GetMaxPower(out.playerPowerType);
}

void ReadTargetUnitFields(uintptr_t targetBasePtr, GameData& out)
{
    WoWUnit target(targetBasePtr);
    if (!target.IsValid()) return;

    out.targetHealth    = target.GetHealth();
    out.targetMaxHealth = target.GetMaxHealth();
    out.targetLevel     = target.GetLevel();
    out.targetPowerType = target.GetPowerType();
    out.targetPower     = target.GetPower(out.targetPowerType);
    out.targetMaxPower  = target.GetMaxPower(out.targetPowerType);
}

void ReadPlayerPosition(uintptr_t playerBasePtr, GameData& out)
{
    WoWPlayer player(playerBasePtr);
    if (!player.IsValid()) return;

    out.playerPosX     = player.GetX();
    out.playerPosY     = player.GetY();
    out.playerPosZ     = player.GetZ();
    out.playerRotation = player.GetRotation();
}

void ReadCameraInfo(GameData& out)
{
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
}

std::string GetUnitName(uint64_t guid, uintptr_t basePtr)
{
    if (guid == 0) return "";

    // Check if GUID type indicates a Player
    uint16_t high = static_cast<uint16_t>(guid >> 48);
    if (high == 0x0000 || high == 0x0001) {
        // Player: Lookup in Name Cache (NameStore)
        uintptr_t nameCache = ReadAbs<uint32_t>(WoWOffsets::nameStore);
        if (nameCache) {
            uint32_t mask = ReadAbs<uint32_t>(nameCache + WoWOffsets::nameMask);
            uintptr_t base = ReadAbs<uint32_t>(nameCache + WoWOffsets::nameBase);
            if (base) {
                uint32_t index = static_cast<uint32_t>(guid) & mask;
                uintptr_t node = ReadAbs<uint32_t>(base + index * 4);
                size_t count = 0;
                while (node && (node & 1) == 0 && count < 150) {
                    uint64_t nodeGuid = ReadAbs<uint64_t>(node);
                    if (nodeGuid == guid) {
                        return ReadInlineString(node + WoWOffsets::nameString, 40);
                    }
                    node = ReadAbs<uint32_t>(node + WoWOffsets::nameNodeNextOffset);
                    count++;
                }
            }
        }
    } else {
        // Creature / NPC / Pet: Read from template DB record
        if (basePtr) {
            uintptr_t pTemplate = ReadAbs<uint32_t>(basePtr + 0x960);
            if (pTemplate) {
                // Try offset 0x18 first
                uintptr_t pName = ReadAbs<uint32_t>(pTemplate + 0x18);
                if (pName && IsReadableRange(pName, 4)) {
                    return ReadInlineString(pName, 64);
                }
                // Fallback to offset 0x0
                pName = ReadAbs<uint32_t>(pTemplate + 0x0);
                if (pName && IsReadableRange(pName, 4)) {
                    return ReadInlineString(pName, 64);
                }
            }
        }
    }
    return "Target";
}

} // namespace WoWMemory
