#include "unit_info.h"
#include "object_manager.h"
#include "wowmemory/memory_utils.h"
#include "wowmemory/offsets.h"

namespace WoWMemory {

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
    // CWorldFrame::GetActiveCamera: *WorldFrame, then the camera pointer at
    // WorldFrame + 0x7E20.
    const uintptr_t worldFrame = ReadAbs<uint32_t>(WoWOffsets::Drawing::WorldFrame);
    if (!worldFrame) return;

    const uintptr_t cameraPtrAddress = worldFrame + WoWOffsets::Drawing::ActiveCamera;
    if (!IsReadableRange(cameraPtrAddress, sizeof(uint32_t))) return;

    const uintptr_t camera = *reinterpret_cast<const uint32_t*>(cameraPtrAddress);
    if (!camera) return;

    const uintptr_t yawAddress = camera + WoWOffsets::cameraYawOffset;
    const uintptr_t pitchAddress = camera + WoWOffsets::cameraPitchOffset;
    if (IsReadableRange(yawAddress, sizeof(float)))
        out.cameraYaw = *reinterpret_cast<const float*>(yawAddress);
    if (IsReadableRange(pitchAddress, sizeof(float)))
        out.cameraPitch = *reinterpret_cast<const float*>(pitchAddress);
}

std::string GetUnitName(uint64_t guid, uintptr_t basePtr)
{
    if (guid == 0) return "";

    // Check if GUID type indicates a Player
    uint16_t high = static_cast<uint16_t>(guid >> 48);
    if (high == 0x0000 || high == 0x0001) {
        // DbNameCache_GetInfoBlockById uses the hash-table subobject in place;
        // nameStore is not a pointer variable. Each bucket is a 12-byte record
        // whose +4 field selects the collision-link member in every node.
        const uintptr_t nameTable = WoWOffsets::nameStore;
        const uint32_t mask = ReadAbs<uint32_t>(
            static_cast<uint32_t>(nameTable + WoWOffsets::nameMask), 0xFFFFFFFFu);
        const uintptr_t buckets = ReadAbs<uint32_t>(
            static_cast<uint32_t>(nameTable + WoWOffsets::nameBase));
        if (mask != 0xFFFFFFFFu && buckets) {
            const uint32_t guidLow = static_cast<uint32_t>(guid);
            const uintptr_t bucket = buckets
                + static_cast<uintptr_t>(guidLow & mask) * WoWOffsets::nameBucketStride;
            const int32_t linkOffset = ReadAbs<int32_t>(
                static_cast<uint32_t>(bucket + WoWOffsets::nameBucketLinkOffset));
            uintptr_t node = ReadAbs<uint32_t>(
                static_cast<uint32_t>(bucket + WoWOffsets::nameBucketHeadOffset));

            // The observed link is +0x0c. Keep a conservative bound because a
            // damaged cache must never turn this passive lookup into an
            // arbitrary pointer walk.
            if (linkOffset > 0 && linkOffset <= 0x400) {
                size_t count = 0;
                while (node && (node & 1u) == 0 && count < 150) {
                    const uint32_t hashKey = ReadAbs<uint32_t>(
                        static_cast<uint32_t>(node + WoWOffsets::NameCache::NodeHashKey));
                    const uint64_t nodeGuid = ReadAbs<uint64_t>(
                        static_cast<uint32_t>(node + WoWOffsets::nameNodeGuidOffset));
                    if (hashKey == guidLow && nodeGuid == guid) {
                        return ReadInlineString(
                            static_cast<uint32_t>(node + WoWOffsets::nameString), 40);
                    }
                    node = ReadAbs<uint32_t>(
                        static_cast<uint32_t>(node + static_cast<uint32_t>(linkOffset)));
                    ++count;
                }
            }
        }
    } else {
        // Creature / NPC / Pet: Read from template DB record
        if (basePtr) {
            uintptr_t pTemplate = ReadAbs<uint32_t>(
                static_cast<uint32_t>(basePtr + WoWOffsets::UnitName::CreatureTemplate));
            if (pTemplate) {
                uintptr_t pName = ReadAbs<uint32_t>(
                    static_cast<uint32_t>(pTemplate + WoWOffsets::UnitName::CreatureTemplateName));
                if (pName && IsReadableRange(pName, 4)) {
                    return ReadInlineString(static_cast<uint32_t>(pName), 64);
                }
            }
        }
    }
    return "Target";
}

} // namespace WoWMemory
