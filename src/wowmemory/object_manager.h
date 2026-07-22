#ifndef WOWMEMORY_OBJECT_MANAGER_H
#define WOWMEMORY_OBJECT_MANAGER_H

#include <cstdint>

namespace WoWMemory {

/**
 * @brief Find any object's base address by walking the Object Manager list.
 * @param targetGUID The GUID of the object to search for.
 * @return The base address of the object, or 0 if not found.
 */
uintptr_t GetObjectBaseByGUID(uint64_t targetGUID);

} // namespace WoWMemory

#endif // WOWMEMORY_OBJECT_MANAGER_H
