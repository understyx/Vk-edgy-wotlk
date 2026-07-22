#include "wowmemory/object_manager.h"
#include "wowmemory/memory_utils.h"
#include "wowmemory/offsets.h"

namespace WoWMemory {

uintptr_t GetObjectBaseByGUID(uint64_t targetGUID)
{
    if (targetGUID == 0) return 0;
    uintptr_t connection = ReadAbs<uint32_t>(WoWOffsets::currentClientConnection);
    if (!connection) return 0;

    uintptr_t managerAddr = connection + WoWOffsets::currentManagerOffset;
    uintptr_t manager = ReadAbs<uint32_t>(managerAddr);
    if (!manager) return 0;

    uintptr_t objAddr = manager + WoWOffsets::firstObjectOffset;
    uintptr_t obj = ReadAbs<uint32_t>(objAddr);

    size_t count = 0;
    while (obj && (obj & 1u) == 0 && count < 5000) {
        uintptr_t guidAddr = obj + WoWOffsets::objectGUID;
        uint64_t currentGUID = ReadAbs<uint64_t>(guidAddr);
        if (currentGUID == targetGUID) {
            return obj;
        }
        uintptr_t nextAddr = obj + WoWOffsets::nextObjectOffset;
        obj = ReadAbs<uint32_t>(nextAddr);
        count++;
    }
    return 0;
}

} // namespace WoWMemory
