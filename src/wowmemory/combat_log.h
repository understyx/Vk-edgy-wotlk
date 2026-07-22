#ifndef WOWMEMORY_COMBAT_LOG_H
#define WOWMEMORY_COMBAT_LOG_H

#include "wowmemory/wowmemory.h"
#include <cstdint>

namespace WoWMemory {

/**
 * @brief Incrementally read new combat log events from the WoW list.
 * @param lastNodeAddr Reference to the last successfully read combat log node address.
 * @param out The GameData object to populate with new combat log events.
 */
void ReadCombatLogEvents(uintptr_t& lastNodeAddr, GameData& out);

} // namespace WoWMemory

#endif // WOWMEMORY_COMBAT_LOG_H
