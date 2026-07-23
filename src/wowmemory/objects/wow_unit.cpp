#include "wow_unit.h"
#include "wowmemory/offsets.h"
#include "wowmemory/memory_utils.h"

namespace WoWMemory {

WoWUnit::WoWUnit(uintptr_t baseAddress) : WoWObject(baseAddress) {}

uint32_t WoWUnit::GetHealth() const {
    if (!IsValid()) return 0;
    uintptr_t ufPtrAddr = m_baseAddress + WoWOffsets::objectUnitFields;
    if (!IsReadableRange(ufPtrAddr, sizeof(uint32_t))) return 0;
    uintptr_t unitFieldsAddr = *reinterpret_cast<const uint32_t*>(ufPtrAddr);
    if (!unitFieldsAddr) return 0;

    uintptr_t addr = unitFieldsAddr + WoWDescriptors::UNIT_FIELD_HEALTH * 4;
    if (!IsReadableRange(addr, sizeof(uint32_t))) return 0;
    return *reinterpret_cast<const uint32_t*>(addr);
}

uint32_t WoWUnit::GetMaxHealth() const {
    if (!IsValid()) return 0;
    uintptr_t ufPtrAddr = m_baseAddress + WoWOffsets::objectUnitFields;
    if (!IsReadableRange(ufPtrAddr, sizeof(uint32_t))) return 0;
    uintptr_t unitFieldsAddr = *reinterpret_cast<const uint32_t*>(ufPtrAddr);
    if (!unitFieldsAddr) return 0;

    uintptr_t addr = unitFieldsAddr + WoWDescriptors::UNIT_FIELD_MAXHEALTH * 4;
    if (!IsReadableRange(addr, sizeof(uint32_t))) return 0;
    return *reinterpret_cast<const uint32_t*>(addr);
}

uint32_t WoWUnit::GetLevel() const {
    if (!IsValid()) return 0;
    uintptr_t ufPtrAddr = m_baseAddress + WoWOffsets::objectUnitFields;
    if (!IsReadableRange(ufPtrAddr, sizeof(uint32_t))) return 0;
    uintptr_t unitFieldsAddr = *reinterpret_cast<const uint32_t*>(ufPtrAddr);
    if (!unitFieldsAddr) return 0;

    uintptr_t addr = unitFieldsAddr + WoWDescriptors::UNIT_FIELD_LEVEL * 4;
    if (!IsReadableRange(addr, sizeof(uint32_t))) return 0;
    return *reinterpret_cast<const uint32_t*>(addr);
}

uint64_t WoWUnit::GetTargetGUID() const {
    if (!IsValid()) return 0;
    uintptr_t ufPtrAddr = m_baseAddress + WoWOffsets::objectUnitFields;
    if (!IsReadableRange(ufPtrAddr, sizeof(uint32_t))) return 0;
    uintptr_t unitFieldsAddr = *reinterpret_cast<const uint32_t*>(ufPtrAddr);
    if (!unitFieldsAddr) return 0;

    uintptr_t addr = unitFieldsAddr + WoWDescriptors::UNIT_FIELD_TARGET * 4;
    if (!IsReadableRange(addr, sizeof(uint64_t))) return 0;
    return *reinterpret_cast<const uint64_t*>(addr);
}

uint8_t WoWUnit::GetPowerType() const {
    if (!IsValid()) return 0;
    uintptr_t descPtrAddr = m_baseAddress + WoWOffsets::objectDescriptorOffset;
    if (!IsReadableRange(descPtrAddr, sizeof(uint32_t))) return 0;
    uintptr_t descriptorAddr = *reinterpret_cast<const uint32_t*>(descPtrAddr);
    if (!descriptorAddr) return 0;

    uintptr_t ptAddr = descriptorAddr + WoWOffsets::unitFieldPowerTypeByteFromDescriptor;
    if (!IsReadableRange(ptAddr, sizeof(uint8_t))) return 0;
    return *reinterpret_cast<const uint8_t*>(ptAddr);
}

uint32_t WoWUnit::GetPower(uint8_t powerType) const {
    if (!IsValid()) return 0;
    if (powerType >= 7) return 0;
    uintptr_t ufPtrAddr = m_baseAddress + WoWOffsets::objectUnitFields;
    if (!IsReadableRange(ufPtrAddr, sizeof(uint32_t))) return 0;
    uintptr_t unitFieldsAddr = *reinterpret_cast<const uint32_t*>(ufPtrAddr);
    if (!unitFieldsAddr) return 0;

    uintptr_t addr = unitFieldsAddr + WoWDescriptors::UNIT_FIELD_POWER1 * 4 + powerType * sizeof(uint32_t);
    if (!IsReadableRange(addr, sizeof(uint32_t))) return 0;
    return *reinterpret_cast<const uint32_t*>(addr);
}

uint32_t WoWUnit::GetMaxPower(uint8_t powerType) const {
    if (!IsValid()) return 0;
    if (powerType >= 7) return 0;
    uintptr_t ufPtrAddr = m_baseAddress + WoWOffsets::objectUnitFields;
    if (!IsReadableRange(ufPtrAddr, sizeof(uint32_t))) return 0;
    uintptr_t unitFieldsAddr = *reinterpret_cast<const uint32_t*>(ufPtrAddr);
    if (!unitFieldsAddr) return 0;

    uintptr_t addr = unitFieldsAddr + WoWDescriptors::UNIT_FIELD_MAXPOWER1 * 4 + powerType * sizeof(uint32_t);
    if (!IsReadableRange(addr, sizeof(uint32_t))) return 0;
    return *reinterpret_cast<const uint32_t*>(addr);
}

uint32_t WoWUnit::GetCastingSpellId() const {
    if (!IsValid()) return 0;
    uintptr_t addr = m_baseAddress + WoWOffsets::unitCastingIdOffset;
    if (!IsReadableRange(addr, sizeof(uint32_t))) return 0;
    return *reinterpret_cast<const uint32_t*>(addr);
}

uint32_t WoWUnit::GetChannelSpellId() const {
    if (!IsValid()) return 0;
    uintptr_t addr = m_baseAddress + WoWOffsets::unitChannelIdOffset;
    if (!IsReadableRange(addr, sizeof(uint32_t))) return 0;
    return *reinterpret_cast<const uint32_t*>(addr);
}

} // namespace WoWMemory
