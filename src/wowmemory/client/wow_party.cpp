#include "wow_party.h"
#include "wowmemory/offsets.h"
#include "wowmemory/memory_utils.h"

namespace WoWMemory {

int WoWParty::NumPartyMembers() {
    int count = 0;
    for (int i = 0; i < 5; ++i) {
        if (GetPartyMemberGuid(i) != 0) {
            count++;
        }
    }
    return count;
}

uint32_t WoWParty::Difficulty() {
#ifdef _WIN32
    return ReadAbs<uint32_t>(WoWOffsets::Party::DungeonDifficulty);
#else
    return 0;
#endif
}

uint64_t WoWParty::GetPartyMemberGuid(int index) {
#ifdef _WIN32
    uint32_t baseAddr = ReadAbs<uint32_t>(WoWOffsets::Party::PartyArray);
    if (baseAddr) {
        uint32_t addr = baseAddr + index * 32;
        if (IsReadableRange(addr, sizeof(uint64_t))) {
            return ReadAbs<uint64_t>(addr);
        }
    }
#endif
    return 0;
}

std::vector<uint64_t> WoWParty::GetMembersGuids() {
    std::vector<uint64_t> guids;
    for (int i = 0; i < 5; ++i) {
        uint64_t guid = GetPartyMemberGuid(i);
        if (guid != 0) {
            guids.push_back(guid);
        }
    }
    return guids;
}

} // namespace WoWMemory
