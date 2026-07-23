#ifndef WOWMEMORY_CLIENT_WOW_QUEST_H
#define WOWMEMORY_CLIENT_WOW_QUEST_H

#include <cstdint>
#include <vector>

namespace WoWMemory {

struct QuestLogEntry {
    uint32_t ID = 0;
    uint32_t State = 0;
    uint32_t RequiredKills[4] = {0};
};

class WoWQuest {
public:
    static std::vector<QuestLogEntry> GetActiveQuests();
    static bool IsOnQuest(uint32_t questId);
    static bool HasCompletedQuest(uint32_t questId);
};

} // namespace WoWMemory

#endif // WOWMEMORY_CLIENT_WOW_QUEST_H
