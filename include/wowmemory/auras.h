#ifndef WOWMEMORY_AURAS_H
#define WOWMEMORY_AURAS_H

#include "wowmemory/wowmemory.h"
#include <cstdint>

namespace WoWMemory {

/**
 * @brief Helper to read active auras for any unit base pointer.
 * @param unitBase The base address of the unit.
 * @param outAuras Pointer to the array of AuraInfo to fill.
 * @param maxAuras Maximum size of the outAuras array.
 * @return The number of parsed auras.
 */
uint32_t ReadAurasForUnit(uintptr_t unitBase, AuraInfo* outAuras, size_t maxAuras);

} // namespace WoWMemory

#endif // WOWMEMORY_AURAS_H
