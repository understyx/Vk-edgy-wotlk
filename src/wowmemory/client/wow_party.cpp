#include "wow_party.h"
#include "wowmemory/offsets.h"
#include "wowmemory/memory_utils.h"
#include <cstdio>

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
    uint32_t addr = WoWOffsets::Party::PartyArray + index * 32; // PartyEntry is 32 bytes
    uint64_t guid = ReadAbs<uint64_t>(addr);
    if (guid != 0) {
        uint8_t unknown[24];
        for (int i = 0; i < 24; ++i) {
            unknown[i] = ReadAbs<uint8_t>(addr + 8 + i);
        }
        fprintf(stderr, "[Party] Index %d, GUID %llu (0x%016llX) Unknown: ", index, (unsigned long long)guid, (unsigned long long)guid);
        for (int i = 0; i < 24; ++i) {
            fprintf(stderr, "%02X ", unknown[i]);
        }
        fprintf(stderr, "\n");
    }
    return guid;
#else
    return 0;
#endif
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
