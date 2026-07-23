#include "wow_object.h"
#include "wowmemory/offsets.h"
#include "wowmemory/memory_utils.h"

namespace WoWMemory {

WoWObject::WoWObject(uintptr_t baseAddress) : m_baseAddress(baseAddress) {}

uintptr_t WoWObject::GetBaseAddress() const {
    return m_baseAddress;
}

bool WoWObject::IsValid() const {
    return m_baseAddress != 0 && IsReadableRange(m_baseAddress, 4);
}

uint64_t WoWObject::GetGUID() const {
    if (!IsValid()) return 0;
    uintptr_t addr = m_baseAddress + WoWOffsets::objectGUID;
    if (!IsReadableRange(addr, sizeof(uint64_t))) return 0;
    return *reinterpret_cast<const uint64_t*>(addr);
}

uint32_t WoWObject::GetType() const {
    if (!IsValid()) return 0;
    uintptr_t addr = m_baseAddress + WoWOffsets::objectType;
    if (!IsReadableRange(addr, sizeof(uint32_t))) return 0;
    return *reinterpret_cast<const uint32_t*>(addr);
}

uint32_t WoWObject::GetEntry() const {
    if (!IsValid()) return 0;
    // Descriptor field for entry: OBJECT_FIELD_ENTRY
    uintptr_t descPtrAddr = m_baseAddress + WoWOffsets::objectDescriptorOffset;
    if (!IsReadableRange(descPtrAddr, sizeof(uint32_t))) return 0;
    uintptr_t descAddr = *reinterpret_cast<const uint32_t*>(descPtrAddr);
    if (!descAddr) return 0;
    uintptr_t entryAddr = descAddr + WoWDescriptors::OBJECT_FIELD_ENTRY * 4;
    if (!IsReadableRange(entryAddr, sizeof(uint32_t))) return 0;
    return *reinterpret_cast<const uint32_t*>(entryAddr);
}

float WoWObject::GetScaleX() const {
    if (!IsValid()) return 1.0f;
    uintptr_t descPtrAddr = m_baseAddress + WoWOffsets::objectDescriptorOffset;
    if (!IsReadableRange(descPtrAddr, sizeof(uint32_t))) return 1.0f;
    uintptr_t descAddr = *reinterpret_cast<const uint32_t*>(descPtrAddr);
    if (!descAddr) return 1.0f;
    uintptr_t scaleAddr = descAddr + WoWDescriptors::OBJECT_FIELD_SCALE_X * 4;
    if (!IsReadableRange(scaleAddr, sizeof(float))) return 1.0f;
    return *reinterpret_cast<const float*>(scaleAddr);
}

float WoWObject::GetX() const {
    if (!IsValid()) return 0.0f;
    uintptr_t addr = m_baseAddress + WoWOffsets::objectPosX;
    if (!IsReadableRange(addr, sizeof(float))) return 0.0f;
    return *reinterpret_cast<const float*>(addr);
}

float WoWObject::GetY() const {
    if (!IsValid()) return 0.0f;
    uintptr_t addr = m_baseAddress + WoWOffsets::objectPosY;
    if (!IsReadableRange(addr, sizeof(float))) return 0.0f;
    return *reinterpret_cast<const float*>(addr);
}

float WoWObject::GetZ() const {
    if (!IsValid()) return 0.0f;
    uintptr_t addr = m_baseAddress + WoWOffsets::objectPosZ;
    if (!IsReadableRange(addr, sizeof(float))) return 0.0f;
    return *reinterpret_cast<const float*>(addr);
}

float WoWObject::GetRotation() const {
    if (!IsValid()) return 0.0f;
    uintptr_t addr = m_baseAddress + WoWOffsets::objectRotation;
    if (!IsReadableRange(addr, sizeof(float))) return 0.0f;
    return *reinterpret_cast<const float*>(addr);
}

} // namespace WoWMemory
