#include "wow_container.h"
#include "wowmemory/offsets.h"
#include "wowmemory/memory_utils.h"

namespace WoWMemory {

WoWContainer::WoWContainer(uintptr_t baseAddress) : WoWItem(baseAddress) {}

uint32_t WoWContainer::GetNumSlots() const {
    if (!IsValid()) return 0;
    uintptr_t descPtrAddr = m_baseAddress + WoWOffsets::objectDescriptorOffset;
    if (!IsReadableRange(descPtrAddr, sizeof(uint32_t))) return 0;
    uintptr_t descAddr = *reinterpret_cast<const uint32_t*>(descPtrAddr);
    if (!descAddr) return 0;

    uintptr_t addr = descAddr + WoWDescriptors::CONTAINER_FIELD_NUM_SLOTS * 4;
    if (!IsReadableRange(addr, sizeof(uint32_t))) return 0;
    return *reinterpret_cast<const uint32_t*>(addr);
}

uint64_t WoWContainer::GetSlotGUID(uint32_t slotIndex) const {
    if (!IsValid()) return 0;
    uint32_t maxSlots = GetNumSlots();
    if (slotIndex >= maxSlots || slotIndex >= 36) return 0;

    uintptr_t descPtrAddr = m_baseAddress + WoWOffsets::objectDescriptorOffset;
    if (!IsReadableRange(descPtrAddr, sizeof(uint32_t))) return 0;
    uintptr_t descAddr = *reinterpret_cast<const uint32_t*>(descPtrAddr);
    if (!descAddr) return 0;

    uintptr_t addr = descAddr + WoWDescriptors::CONTAINER_FIELD_SLOT_1 * 4 + slotIndex * sizeof(uint64_t);
    if (!IsReadableRange(addr, sizeof(uint64_t))) return 0;
    return *reinterpret_cast<const uint64_t*>(addr);
}

} // namespace WoWMemory
