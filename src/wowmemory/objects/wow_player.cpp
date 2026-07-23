#include "wow_player.h"
#include "wowmemory/offsets.h"
#include "wowmemory/memory_utils.h"

namespace WoWMemory {

WoWPlayer::WoWPlayer(uintptr_t baseAddress) : WoWUnit(baseAddress) {}

uint32_t WoWPlayer::GetXP() const {
    if (!IsValid()) return 0;
    uintptr_t ufPtrAddr = m_baseAddress + WoWOffsets::objectUnitFields;
    if (!IsReadableRange(ufPtrAddr, sizeof(uint32_t))) return 0;
    uintptr_t unitFieldsAddr = *reinterpret_cast<const uint32_t*>(ufPtrAddr);
    if (!unitFieldsAddr) return 0;

    uintptr_t addr = unitFieldsAddr + WoWDescriptors::PLAYER_XP * 4;
    if (!IsReadableRange(addr, sizeof(uint32_t))) return 0;
    return *reinterpret_cast<const uint32_t*>(addr);
}

uint32_t WoWPlayer::GetNextLevelXP() const {
    if (!IsValid()) return 0;
    uintptr_t ufPtrAddr = m_baseAddress + WoWOffsets::objectUnitFields;
    if (!IsReadableRange(ufPtrAddr, sizeof(uint32_t))) return 0;
    uintptr_t unitFieldsAddr = *reinterpret_cast<const uint32_t*>(ufPtrAddr);
    if (!unitFieldsAddr) return 0;

    uintptr_t addr = unitFieldsAddr + WoWDescriptors::PLAYER_NEXT_LEVEL_XP * 4;
    if (!IsReadableRange(addr, sizeof(uint32_t))) return 0;
    return *reinterpret_cast<const uint32_t*>(addr);
}

uint32_t WoWPlayer::GetCoinage() const {
    if (!IsValid()) return 0;
    uintptr_t ufPtrAddr = m_baseAddress + WoWOffsets::objectUnitFields;
    if (!IsReadableRange(ufPtrAddr, sizeof(uint32_t))) return 0;
    uintptr_t unitFieldsAddr = *reinterpret_cast<const uint32_t*>(ufPtrAddr);
    if (!unitFieldsAddr) return 0;

    uintptr_t addr = unitFieldsAddr + WoWDescriptors::PLAYER_FIELD_COINAGE * 4;
    if (!IsReadableRange(addr, sizeof(uint32_t))) return 0;
    return *reinterpret_cast<const uint32_t*>(addr);
}

} // namespace WoWMemory
