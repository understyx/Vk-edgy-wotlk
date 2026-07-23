#include "wow_game_object.h"
#include "wowmemory/offsets.h"
#include "wowmemory/memory_utils.h"

namespace WoWMemory {

WoWGameObject::WoWGameObject(uintptr_t baseAddress) : WoWObject(baseAddress) {}

uint64_t WoWGameObject::GetCreatedBy() const {
    if (!IsValid()) return 0;
    uintptr_t descPtrAddr = m_baseAddress + WoWOffsets::objectDescriptorOffset;
    if (!IsReadableRange(descPtrAddr, sizeof(uint32_t))) return 0;
    uintptr_t descAddr = *reinterpret_cast<const uint32_t*>(descPtrAddr);
    if (!descAddr) return 0;

    uintptr_t addr = descAddr + WoWDescriptors::OBJECT_FIELD_CREATED_BY * 4;
    if (!IsReadableRange(addr, sizeof(uint64_t))) return 0;
    return *reinterpret_cast<const uint64_t*>(addr);
}

uint32_t WoWGameObject::GetDisplayID() const {
    if (!IsValid()) return 0;
    uintptr_t descPtrAddr = m_baseAddress + WoWOffsets::objectDescriptorOffset;
    if (!IsReadableRange(descPtrAddr, sizeof(uint32_t))) return 0;
    uintptr_t descAddr = *reinterpret_cast<const uint32_t*>(descPtrAddr);
    if (!descAddr) return 0;

    uintptr_t addr = descAddr + WoWDescriptors::GAMEOBJECT_DISPLAYID * 4;
    if (!IsReadableRange(addr, sizeof(uint32_t))) return 0;
    return *reinterpret_cast<const uint32_t*>(addr);
}

uint32_t WoWGameObject::GetFlags() const {
    if (!IsValid()) return 0;
    uintptr_t descPtrAddr = m_baseAddress + WoWOffsets::objectDescriptorOffset;
    if (!IsReadableRange(descPtrAddr, sizeof(uint32_t))) return 0;
    uintptr_t descAddr = *reinterpret_cast<const uint32_t*>(descPtrAddr);
    if (!descAddr) return 0;

    uintptr_t addr = descAddr + WoWDescriptors::GAMEOBJECT_FLAGS * 4;
    if (!IsReadableRange(addr, sizeof(uint32_t))) return 0;
    return *reinterpret_cast<const uint32_t*>(addr);
}

uint32_t WoWGameObject::GetFaction() const {
    if (!IsValid()) return 0;
    uintptr_t descPtrAddr = m_baseAddress + WoWOffsets::objectDescriptorOffset;
    if (!IsReadableRange(descPtrAddr, sizeof(uint32_t))) return 0;
    uintptr_t descAddr = *reinterpret_cast<const uint32_t*>(descPtrAddr);
    if (!descAddr) return 0;

    uintptr_t addr = descAddr + WoWDescriptors::GAMEOBJECT_FACTION * 4;
    if (!IsReadableRange(addr, sizeof(uint32_t))) return 0;
    return *reinterpret_cast<const uint32_t*>(addr);
}

uint32_t WoWGameObject::GetLevel() const {
    if (!IsValid()) return 0;
    uintptr_t descPtrAddr = m_baseAddress + WoWOffsets::objectDescriptorOffset;
    if (!IsReadableRange(descPtrAddr, sizeof(uint32_t))) return 0;
    uintptr_t descAddr = *reinterpret_cast<const uint32_t*>(descPtrAddr);
    if (!descAddr) return 0;

    uintptr_t addr = descAddr + WoWDescriptors::GAMEOBJECT_LEVEL * 4;
    if (!IsReadableRange(addr, sizeof(uint32_t))) return 0;
    return *reinterpret_cast<const uint32_t*>(addr);
}

} // namespace WoWMemory
