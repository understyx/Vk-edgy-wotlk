#include "wow_raid.h"
#include "wowmemory/offsets.h"
#include "wowmemory/memory_utils.h"

namespace WoWMemory {

int WoWRaid::NumRaidMembers() {
#ifdef _WIN32
    return ReadAbs<int>(WoWOffsets::Raid::RaidCount);
#else
    return 0;
#endif
}

uint32_t WoWRaid::Difficulty() {
#ifdef _WIN32
    return ReadAbs<uint32_t>(WoWOffsets::Raid::RaidDifficulty);
#else
    return 0;
#endif
}

uint64_t WoWRaid::GetRaidMemberGuid(int index) {
#ifdef _WIN32
    uint32_t ptr = ReadAbs<uint32_t>(WoWOffsets::Raid::RaidArray + index * sizeof(uint32_t));
    if (ptr && IsReadableRange(ptr + 48, sizeof(uint64_t))) {
        return ReadAbs<uint64_t>(ptr + 48);
    }
#endif
    return 0;
}

std::vector<uint64_t> WoWRaid::GetMembersGuids() {
    std::vector<uint64_t> guids;
    int num = NumRaidMembers();
    for (int i = 0; i < num; ++i) {
        uint64_t guid = GetRaidMemberGuid(i);
        if (guid != 0) {
            guids.push_back(guid);
        }
    }
    return guids;
}

} // namespace WoWMemory
