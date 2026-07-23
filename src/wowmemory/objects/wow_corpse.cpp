#include "wow_corpse.h"
#include "wowmemory/offsets.h"
#include "wowmemory/memory_utils.h"

namespace WoWMemory {

WoWCorpse::WoWCorpse(uintptr_t baseAddress) : WoWObject(baseAddress) {}

uint64_t WoWCorpse::GetOwner() const {
    if (!IsValid()) return 0;
    uintptr_t descPtrAddr = m_baseAddress + WoWOffsets::objectDescriptorOffset;
    if (!IsReadableRange(descPtrAddr, sizeof(uint32_t))) return 0;
    uintptr_t descAddr = *reinterpret_cast<const uint32_t*>(descPtrAddr);
    if (!descAddr) return 0;

    uintptr_t addr = descAddr + WoWDescriptors::CORPSE_FIELD_OWNER * 4;
    if (!IsReadableRange(addr, sizeof(uint64_t))) return 0;
    return *reinterpret_cast<const uint64_t*>(addr);
}

uint64_t WoWCorpse::GetParty() const {
    if (!IsValid()) return 0;
    uintptr_t descPtrAddr = m_baseAddress + WoWOffsets::objectDescriptorOffset;
    if (!IsReadableRange(descPtrAddr, sizeof(uint32_t))) return 0;
    uintptr_t descAddr = *reinterpret_cast<const uint32_t*>(descPtrAddr);
    if (!descAddr) return 0;

    uintptr_t addr = descAddr + WoWDescriptors::CORPSE_FIELD_PARTY * 4;
    if (!IsReadableRange(addr, sizeof(uint64_t))) return 0;
    return *reinterpret_cast<const uint64_t*>(addr);
}

uint32_t WoWCorpse::GetDisplayID() const {
    if (!IsValid()) return 0;
    uintptr_t descPtrAddr = m_baseAddress + WoWOffsets::objectDescriptorOffset;
    if (!IsReadableRange(descPtrAddr, sizeof(uint32_t))) return 0;
    uintptr_t descAddr = *reinterpret_cast<const uint32_t*>(descPtrAddr);
    if (!descAddr) return 0;

    uintptr_t addr = descAddr + WoWDescriptors::CORPSE_FIELD_DISPLAY_ID * 4;
    if (!IsReadableRange(addr, sizeof(uint32_t))) return 0;
    return *reinterpret_cast<const uint32_t*>(addr);
}

uint32_t WoWCorpse::GetFlags() const {
    if (!IsValid()) return 0;
    uintptr_t descPtrAddr = m_baseAddress + WoWOffsets::objectDescriptorOffset;
    if (!IsReadableRange(descPtrAddr, sizeof(uint32_t))) return 0;
    uintptr_t descAddr = *reinterpret_cast<const uint32_t*>(descPtrAddr);
    if (!descAddr) return 0;

    uintptr_t addr = descAddr + WoWDescriptors::CORPSE_FIELD_FLAGS * 4;
    if (!IsReadableRange(addr, sizeof(uint32_t))) return 0;
    return *reinterpret_cast<const uint32_t*>(addr);
}

} // namespace WoWMemory
