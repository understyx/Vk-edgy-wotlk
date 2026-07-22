#ifndef WOWMEMORY_OBJECTS_WOW_UNIT_H
#define WOWMEMORY_OBJECTS_WOW_UNIT_H

#include "wowmemory/objects/wow_object.h"

namespace WoWMemory {

class WoWUnit : public WoWObject {
public:
    explicit WoWUnit(uintptr_t baseAddress) : WoWObject(baseAddress) {}

    uint32_t GetHealth() const {
        uintptr_t fields = GetDescriptorAddress();
        if (!fields) return 0;
        return ReadAbs<uint32_t>(fields + WoWOffsets::unitFieldHealth);
    }

    uint32_t GetMaxHealth() const {
        uintptr_t fields = GetDescriptorAddress();
        if (!fields) return 0;
        return ReadAbs<uint32_t>(fields + WoWOffsets::unitFieldMaxHealth);
    }

    uint32_t GetLevel() const {
        uintptr_t fields = GetDescriptorAddress();
        if (!fields) return 0;
        return ReadAbs<uint32_t>(fields + WoWOffsets::unitFieldLevel);
    }

    uint64_t GetTargetGUID() const {
        uintptr_t fields = GetDescriptorAddress();
        if (!fields) return 0;
        return ReadAbs<uint64_t>(fields + WoWOffsets::unitFieldTargetGUID);
    }

    uint8_t GetPowerType() const {
        uintptr_t fields = GetDescriptorAddress();
        if (!fields) return 0;
        uintptr_t ptAddr = fields + WoWOffsets::unitFieldPowerTypeByteFromDescriptor;
        if (!IsReadableRange(ptAddr, sizeof(uint8_t))) return 0;
        return *reinterpret_cast<const uint8_t*>(ptAddr);
    }

    uint32_t GetPower(uint8_t powerType) const {
        uintptr_t fields = GetDescriptorAddress();
        if (!fields) return 0;
        if (powerType >= 7) return 0;
        return ReadAbs<uint32_t>(fields + WoWOffsets::unitFieldPowers + powerType * sizeof(uint32_t));
    }

    uint32_t GetMaxPower(uint8_t powerType) const {
        uintptr_t fields = GetDescriptorAddress();
        if (!fields) return 0;
        if (powerType >= 7) return 0;
        return ReadAbs<uint32_t>(fields + WoWOffsets::unitFieldMaxPowers + powerType * sizeof(uint32_t));
    }

    uint32_t GetCastingSpellId() const {
        return ReadAbs<uint32_t>(m_base + WoWOffsets::unitCastingIdOffset);
    }

    uint32_t GetChannelSpellId() const {
        return ReadAbs<uint32_t>(m_base + WoWOffsets::unitChannelIdOffset);
    }
};

} // namespace WoWMemory

#endif // WOWMEMORY_OBJECTS_WOW_UNIT_H
