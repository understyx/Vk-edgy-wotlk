#include "wowmemory/unit_info.h"
#include "wowmemory/memory_utils.h"
#include "wowmemory/offsets.h"

namespace WoWMemory {

uintptr_t GetObjectBaseByGUID(uint64_t targetGUID)
{
    if (targetGUID == 0) return 0;
    uintptr_t connection = ReadAbs<uint32_t>(WoWOffsets::currentClientConnection);
    if (!connection) return 0;

    uintptr_t managerAddr = connection + WoWOffsets::currentManagerOffset;
    uintptr_t manager = ReadAbs<uint32_t>(managerAddr);
    if (!manager) return 0;

    uintptr_t objAddr = manager + WoWOffsets::firstObjectOffset;
    uintptr_t obj = ReadAbs<uint32_t>(objAddr);

    size_t count = 0;
    while (obj && (obj & 1u) == 0 && count < 5000) {
        uintptr_t guidAddr = obj + WoWOffsets::objectGUID;
        uint64_t currentGUID = ReadAbs<uint64_t>(guidAddr);
        if (currentGUID == targetGUID) {
            return obj;
        }
        uintptr_t nextAddr = obj + WoWOffsets::nextObjectOffset;
        obj = ReadAbs<uint32_t>(nextAddr);
        count++;
    }
    return 0;
}

void ReadPlayerUnitFields(uintptr_t playerBasePtr, GameData& out)
{
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
        auto readUF64 = [&](uint32_t relOffset) -> uint64_t {
            uintptr_t addr = unitFieldsAddr + relOffset;
            if (!IsReadableRange(addr, sizeof(uint64_t))) return 0ull;
            return *reinterpret_cast<const uint64_t*>(addr);
        };

        out.playerHealth    = readUF32(WoWOffsets::unitFieldHealth);
        out.playerMaxHealth = readUF32(WoWOffsets::unitFieldMaxHealth);
        out.playerLevel     = readUF32(WoWOffsets::unitFieldLevel);
        out.targetGUID      = readUF64(WoWOffsets::unitFieldTargetGUID);

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
        constexpr uint32_t kPowerTypeCount = 7;
        uint8_t pt = out.playerPowerType;
        if (pt < kPowerTypeCount) {
            uintptr_t curAddr = unitFieldsAddr + WoWOffsets::unitFieldPowers + pt * sizeof(uint32_t);
            uintptr_t maxAddr = unitFieldsAddr + WoWOffsets::unitFieldMaxPowers + pt * sizeof(uint32_t);
            if (IsReadableRange(curAddr, sizeof(uint32_t)))
                out.playerPower = *reinterpret_cast<const uint32_t*>(curAddr);
            if (IsReadableRange(maxAddr, sizeof(uint32_t)))
                out.playerMaxPower = *reinterpret_cast<const uint32_t*>(maxAddr);
        }
    }
}

void ReadTargetUnitFields(uintptr_t targetBasePtr, GameData& out)
{
    uintptr_t unitFieldsAddr = 0;
    uintptr_t ufPtrAddr = targetBasePtr + WoWOffsets::objectUnitFields;
    if (IsReadableRange(ufPtrAddr, sizeof(uint32_t))) {
        unitFieldsAddr = *reinterpret_cast<const uint32_t*>(ufPtrAddr);
    }

    if (unitFieldsAddr) {
        auto readUF32 = [&](uint32_t relOffset) -> uint32_t {
            uintptr_t addr = unitFieldsAddr + relOffset;
            if (!IsReadableRange(addr, sizeof(uint32_t))) return 0u;
            return *reinterpret_cast<const uint32_t*>(addr);
        };

        out.targetHealth    = readUF32(WoWOffsets::unitFieldHealth);
        out.targetMaxHealth = readUF32(WoWOffsets::unitFieldMaxHealth);
        out.targetLevel     = readUF32(WoWOffsets::unitFieldLevel);

        // Power type is stored as a byte in the object descriptor struct.
        uintptr_t descriptorAddr = 0;
        uintptr_t descPtrAddr = targetBasePtr + WoWOffsets::objectDescriptorOffset;
        if (IsReadableRange(descPtrAddr, sizeof(uint32_t))) {
            descriptorAddr = *reinterpret_cast<const uint32_t*>(descPtrAddr);
        }
        if (descriptorAddr) {
            uintptr_t ptAddr = descriptorAddr + WoWOffsets::unitFieldPowerTypeByteFromDescriptor;
            if (IsReadableRange(ptAddr, sizeof(uint8_t))) {
                out.targetPowerType = *reinterpret_cast<const uint8_t*>(ptAddr);
            }
        }

        // Read current and max power for the target's power type.
        constexpr uint32_t kPowerTypeCount = 7;
        uint8_t pt = out.targetPowerType;
        if (pt < kPowerTypeCount) {
            uintptr_t curAddr = unitFieldsAddr + WoWOffsets::unitFieldPowers + pt * sizeof(uint32_t);
            uintptr_t maxAddr = unitFieldsAddr + WoWOffsets::unitFieldMaxPowers + pt * sizeof(uint32_t);
            if (IsReadableRange(curAddr, sizeof(uint32_t)))
                out.targetPower = *reinterpret_cast<const uint32_t*>(curAddr);
            if (IsReadableRange(maxAddr, sizeof(uint32_t)))
                out.targetMaxPower = *reinterpret_cast<const uint32_t*>(maxAddr);
        }
    }
}

void ReadPlayerPosition(uintptr_t playerBasePtr, GameData& out)
{
    auto readRelFloat = [&](uint32_t relOffset) -> float {
        uintptr_t addr = playerBasePtr + relOffset;
        if (!IsReadableRange(addr, sizeof(float))) return 0.0f;
        return *reinterpret_cast<const float*>(addr);
    };
    out.playerPosX     = readRelFloat(WoWOffsets::objectPosX);
    out.playerPosY     = readRelFloat(WoWOffsets::objectPosY);
    out.playerPosZ     = readRelFloat(WoWOffsets::objectPosZ);
    out.playerRotation = readRelFloat(WoWOffsets::objectRotation);
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
