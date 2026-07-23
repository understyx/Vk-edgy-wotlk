#ifndef WOWMEMORY_CLIENT_WOW_RAID_H
#define WOWMEMORY_CLIENT_WOW_RAID_H

#include <cstdint>
#include <vector>

namespace WoWMemory {

class WoWRaid {
public:
    static int NumRaidMembers();
    static uint32_t Difficulty();
    static uint64_t GetRaidMemberGuid(int index);
    static std::vector<uint64_t> GetMembersGuids();
};

} // namespace WoWMemory

#endif // WOWMEMORY_CLIENT_WOW_RAID_H
