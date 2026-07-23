#include "wow_dynamic_object.h"
#include "wowmemory/offsets.h"
#include "wowmemory/memory_utils.h"

namespace WoWMemory {

WoWDynamicObject::WoWDynamicObject(uintptr_t baseAddress) : WoWObject(baseAddress) {}

uint64_t WoWDynamicObject::GetCasterGUID() const {
    if (!IsValid()) return 0;
    uintptr_t descPtrAddr = m_baseAddress + WoWOffsets::objectDescriptorOffset;
    if (!IsReadableRange(descPtrAddr, sizeof(uint32_t))) return 0;
    uintptr_t descAddr = *reinterpret_cast<const uint32_t*>(descPtrAddr);
    if (!descAddr) return 0;

    uintptr_t addr = descAddr + WoWDescriptors::DYNAMICOBJECT_CASTER * 4;
    if (!IsReadableRange(addr, sizeof(uint64_t))) return 0;
    return *reinterpret_cast<const uint64_t*>(addr);
}

uint32_t WoWDynamicObject::GetSpellID() const {
    if (!IsValid()) return 0;
    uintptr_t descPtrAddr = m_baseAddress + WoWOffsets::objectDescriptorOffset;
    if (!IsReadableRange(descPtrAddr, sizeof(uint32_t))) return 0;
    uintptr_t descAddr = *reinterpret_cast<const uint32_t*>(descPtrAddr);
    if (!descAddr) return 0;

    uintptr_t addr = descAddr + WoWDescriptors::DYNAMICOBJECT_SPELLID * 4;
    if (!IsReadableRange(addr, sizeof(uint32_t))) return 0;
    return *reinterpret_cast<const uint32_t*>(addr);
}

float WoWDynamicObject::GetRadius() const {
    if (!IsValid()) return 0.0f;
    uintptr_t descPtrAddr = m_baseAddress + WoWOffsets::objectDescriptorOffset;
    if (!IsReadableRange(descPtrAddr, sizeof(uint32_t))) return 0.0f;
    uintptr_t descAddr = *reinterpret_cast<const uint32_t*>(descPtrAddr);
    if (!descAddr) return 0.0f;

    uintptr_t addr = descAddr + WoWDescriptors::DYNAMICOBJECT_RADIUS * 4;
    if (!IsReadableRange(addr, sizeof(float))) return 0.0f;
    return *reinterpret_cast<const float*>(addr);
}

uint32_t WoWDynamicObject::GetCastTime() const {
    if (!IsValid()) return 0;
    uintptr_t descPtrAddr = m_baseAddress + WoWOffsets::objectDescriptorOffset;
    if (!IsReadableRange(descPtrAddr, sizeof(uint32_t))) return 0;
    uintptr_t descAddr = *reinterpret_cast<const uint32_t*>(descPtrAddr);
    if (!descAddr) return 0;

    uintptr_t addr = descAddr + WoWDescriptors::DYNAMICOBJECT_CASTTIME * 4;
    if (!IsReadableRange(addr, sizeof(uint32_t))) return 0;
    return *reinterpret_cast<const uint32_t*>(addr);
}

} // namespace WoWMemory
