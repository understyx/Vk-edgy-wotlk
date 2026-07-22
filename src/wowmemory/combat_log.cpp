#include "wowmemory/combat_log.h"
#include "wowmemory/memory_utils.h"
#include "wowmemory/offsets.h"

namespace WoWMemory {

void ReadCombatLogEvents(uintptr_t& lastNodeAddr, GameData& out)
{
    out.combatLogEventCount = 0;
    uintptr_t managerAddr = static_cast<uintptr_t>(WoWOffsets::combatLogListManager);
    uintptr_t headPtrAddr = managerAddr + WoWOffsets::combatLogListHeadOffset;
    uintptr_t tailPtrAddr = managerAddr + WoWOffsets::combatLogListTailOffset;

    uintptr_t headNode = 0;
    uintptr_t tailNode = 0;

    if (IsReadableRange(headPtrAddr, sizeof(uint32_t)))
        headNode = *reinterpret_cast<const uint32_t*>(headPtrAddr);
    if (IsReadableRange(tailPtrAddr, sizeof(uint32_t)))
        tailNode = *reinterpret_cast<const uint32_t*>(tailPtrAddr);

    if (tailNode != 0) {
        uintptr_t nodeAddr = 0;

        if (lastNodeAddr == 0) {
            // First call: start from the head of the list
            nodeAddr = headNode;
        } else if (lastNodeAddr == tailNode) {
            // Already at tail — nothing new
            nodeAddr = 0;
        } else {
            // Advance past the last node we processed
            uintptr_t nextAddr = lastNodeAddr + WoWOffsets::combatLogEventNextOffset;
            if (IsReadableRange(nextAddr, sizeof(uint32_t)))
                nodeAddr = *reinterpret_cast<const uint32_t*>(nextAddr);
            else
                lastNodeAddr = 0; // stale pointer, resync next frame
        }

        constexpr uint32_t kMaxPerFrame = static_cast<uint32_t>(GameData::kMaxCombatLogEvents);
        uint32_t processed = 0;

        // Odd address bits (nodeAddr & 1) indicate an invalid/misaligned node pointer.
        while (nodeAddr != 0 && (nodeAddr & 1u) == 0 && processed < kMaxPerFrame) {
            // Helper lambdas scoped to each node
            auto readU32 = [&](uint32_t off) -> uint32_t {
                uintptr_t a = nodeAddr + off;
                if (!IsReadableRange(a, sizeof(uint32_t))) return 0u;
                return *reinterpret_cast<const uint32_t*>(a);
            };
            auto readI32 = [&](uint32_t off) -> int32_t {
                uintptr_t a = nodeAddr + off;
                if (!IsReadableRange(a, sizeof(int32_t))) return 0;
                return *reinterpret_cast<const int32_t*>(a);
            };

            CombatLogEvent& ev = out.combatLogEvents[out.combatLogEventCount++];
            ev.timestamp           = readU32(WoWOffsets::combatLogEventTimestampOffset);
            ev.eventTypeId         = readI32(WoWOffsets::combatLogNodeEventTypeOffset);
            uint32_t srcLo         = readU32(WoWOffsets::combatLogNodeSrcGuidLowOffset);
            uint32_t srcHi         = readU32(WoWOffsets::combatLogNodeSrcGuidHighOffset);
            ev.sourceGuid          = (static_cast<uint64_t>(srcHi) << 32) | srcLo;
            uint32_t dstLo         = readU32(WoWOffsets::combatLogNodeDstGuidLowOffset);
            uint32_t dstHi         = readU32(WoWOffsets::combatLogNodeDstGuidHighOffset);
            ev.destGuid            = (static_cast<uint64_t>(dstHi) << 32) | dstLo;
            ev.amount              = readI32(WoWOffsets::combatLogNodeAmountOffset);
            ev.overkillOrPowerType = readI32(WoWOffsets::combatLogNodeOverkillOffset);
            ev.schoolMask          = readI32(WoWOffsets::combatLogNodeSchoolMaskOffset);
            ev.absorbed            = readI32(WoWOffsets::combatLogNodeAbsorbedOffset);
            ev.resisted            = readI32(WoWOffsets::combatLogNodeResistedOffset);
            ev.blockedOrMissType   = readI32(WoWOffsets::combatLogNodeBlockedOffset);
            ev.flags               = readU32(WoWOffsets::combatLogNodeFlagsOffset);

            lastNodeAddr = nodeAddr;
            ++processed;

            if (nodeAddr == tailNode) break;

            // Advance to next node
            uintptr_t nextAddr = nodeAddr + WoWOffsets::combatLogEventNextOffset;
            if (!IsReadableRange(nextAddr, sizeof(uint32_t))) {
                lastNodeAddr = 0;
                break;
            }
            uintptr_t nextNode = *reinterpret_cast<const uint32_t*>(nextAddr);
            if (nextNode == nodeAddr) {
                // Corrupt list: self-pointer detected; bail out and resync next frame.
                lastNodeAddr = 0;
                break;
            }
            nodeAddr = nextNode;
        }
    }
}

} // namespace WoWMemory
