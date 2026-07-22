#include "wowmemory/auras.h"
#include "wowmemory/memory_utils.h"
#include "wowmemory/offsets.h"
#include <algorithm>
#include <cstdio>

namespace WoWMemory {

uint32_t ReadAurasForUnit(uintptr_t unitBase, AuraInfo* outAuras, size_t maxAuras)
{
    if (!unitBase) return 0;

    // Inline lambda to read int32 at relative offset from unitBase
    auto readRelI32 = [&](uint32_t relOffset) -> int32_t {
        uintptr_t addr = unitBase + relOffset;
        if (!IsReadableRange(addr, sizeof(int32_t))) return 0;
        return *reinterpret_cast<const int32_t*>(addr);
    };

    // Inline lambda to read uint32 at relative offset from unitBase
    auto readRelU32 = [&](uint32_t relOffset) -> uint32_t {
        uintptr_t addr = unitBase + relOffset;
        if (!IsReadableRange(addr, sizeof(uint32_t))) return 0u;
        return *reinterpret_cast<const uint32_t*>(addr);
    };

    int32_t auraCount1 = readRelI32(WoWOffsets::auraCount1Offset);
    uintptr_t auraTablePtr = 0;
    uint32_t  totalAuras   = 0;

    if (auraCount1 == -1) {
        // Dynamic table: pointer at auraTable2Offset, count at auraCount2Offset
        totalAuras = readRelU32(WoWOffsets::auraCount2Offset);
        uintptr_t tAddr = unitBase + WoWOffsets::auraTable2Offset;
        if (IsReadableRange(tAddr, sizeof(uint32_t)))
            auraTablePtr = *reinterpret_cast<const uint32_t*>(tAddr);
    } else if (auraCount1 > 0) {
        // Inline table: entries start directly at auraTable1Offset (no dereference)
        totalAuras   = static_cast<uint32_t>(auraCount1);
        auraTablePtr = unitBase + WoWOffsets::auraTable1Offset;
    }

    uint32_t count = 0;
    if (auraTablePtr && totalAuras > 0) {
        // Safe debug logging to help validate structure offsets
        printf("[WoWMemory] Aura count=%d table=%p\n",
               static_cast<int>(auraCount1),
               reinterpret_cast<void*>(auraTablePtr));

        uint32_t limit = std::min(totalAuras, static_cast<uint32_t>(maxAuras));
        for (uint32_t i = 0; i < limit; ++i) {
            uintptr_t entryAddr = auraTablePtr
                + static_cast<uintptr_t>(i) * WoWOffsets::auraStructSize
                + WoWOffsets::auraStructSpellIdOffset;
            if (!IsReadableRange(entryAddr, sizeof(uint32_t))) {
                printf("[WoWMemory]   [%02u] (unreadable address %p)\n", i, reinterpret_cast<void*>(entryAddr));
                break;
            }
            uint32_t sid = *reinterpret_cast<const uint32_t*>(entryAddr);
            printf("[WoWMemory]   [%02u] spell=%u\n", i, sid);
            if (sid == 0) continue;
            outAuras[count++].spellId = sid;
        }
    }
    return count;
}

} // namespace WoWMemory
