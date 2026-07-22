#ifndef WOWMEMORY_UNIT_INFO_H
#define WOWMEMORY_UNIT_INFO_H

#include "wowmemory/wowmemory.h"
#include "wowmemory/object_manager.h"
#include "wowmemory/objects/wow_object.h"
#include "wowmemory/objects/wow_unit.h"
#include "wowmemory/objects/wow_player.h"
#include <cstdint>

namespace WoWMemory {

/**
 * @brief Helper to read player's unit fields (descriptor array) and fill corresponding fields.
 * @param playerBasePtr The base address of the player.
 * @param out The GameData object to populate.
 */
void ReadPlayerUnitFields(uintptr_t playerBasePtr, GameData& out);

/**
 * @brief Helper to read target's unit fields (descriptor array) and fill corresponding fields.
 * @param targetBasePtr The base address of the target.
 * @param out The GameData object to populate.
 */
void ReadTargetUnitFields(uintptr_t targetBasePtr, GameData& out);

/**
 * @brief Helper to read player's active position.
 * @param playerBasePtr The base address of the player.
 * @param out The GameData object to populate.
 */
void ReadPlayerPosition(uintptr_t playerBasePtr, GameData& out);

/**
 * @brief Retrieve name for any unit.
 * @param guid The GUID of the unit.
 * @param basePtr The base address of the unit.
 * @return The name of the unit as a string.
 */
std::string GetUnitName(uint64_t guid, uintptr_t basePtr);

/**
 * @brief Helper to read camera information.
 * @param out The GameData object to populate.
 */
void ReadCameraInfo(GameData& out);

} // namespace WoWMemory

#endif // WOWMEMORY_UNIT_INFO_H
