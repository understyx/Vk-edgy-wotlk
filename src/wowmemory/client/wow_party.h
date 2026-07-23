#ifndef WOWMEMORY_CLIENT_WOW_PARTY_H
#define WOWMEMORY_CLIENT_WOW_PARTY_H

#include <cstdint>
#include <vector>

namespace WoWMemory {

class WoWParty {
public:
    static int NumPartyMembers();
    static uint32_t Difficulty();
    static uint64_t GetPartyMemberGuid(int index);
    static std::vector<uint64_t> GetMembersGuids();
};

} // namespace WoWMemory

#endif // WOWMEMORY_CLIENT_WOW_PARTY_H
