#include "wow_quest.h"
#include "wowmemory/offsets.h"
#include "wowmemory/memory_utils.h"

namespace WoWMemory {

std::vector<QuestLogEntry> WoWQuest::GetActiveQuests() {
    std::vector<QuestLogEntry> list;
#ifdef _WIN32
    uintptr_t playerBasePtr = ReadAbs<uint32_t>(WoWOffsets::playerBase);
    if (playerBasePtr && IsReadableRange(playerBasePtr + 8, sizeof(uintptr_t))) {
        // Read descriptors pointer at playerBasePtr + 8 (the objectUnitFields / objectDescriptorOffset)
        uintptr_t descriptorsPtr = *reinterpret_cast<const uintptr_t*>(playerBasePtr + 8);
        if (descriptorsPtr && IsReadableRange(descriptorsPtr, sizeof(uint32_t) * WoWDescriptors::PLAYER_END)) {
            // Quest log starts at PLAYER_QUEST_LOG_1_1
            uintptr_t questLogOffset = WoWDescriptors::PLAYER_QUEST_LOG_1_1 * 4;
            const uint32_t* questLog = reinterpret_cast<const uint32_t*>(descriptorsPtr + questLogOffset);

            // There are 25 slots in the quest log. Each slot has 5 elements (20 bytes).
            for (int i = 0; i < 25; ++i) {
                uint32_t questId = questLog[i * 5];
                if (questId != 0) {
                    QuestLogEntry entry;
                    entry.ID = questId;
                    entry.State = questLog[i * 5 + 1];
                    entry.RequiredKills[0] = questLog[i * 5 + 2];
                    entry.RequiredKills[1] = questLog[i * 5 + 3];
                    entry.RequiredKills[2] = questLog[i * 5 + 4];
                    list.push_back(entry);
                }
            }
        }
    }
#endif
    return list;
}

bool WoWQuest::IsOnQuest(uint32_t questId) {
    auto quests = GetActiveQuests();
    for (const auto& q : quests) {
        if (q.ID == questId) return true;
    }
    return false;
}

bool WoWQuest::HasCompletedQuest(uint32_t questId) {
    // In IceFlake, completed quests are queried via memory pointer 0x00ACFDF4 or GetQuestRecordFromId.
    // In our passive memory model, we check if quest is completed from the quest log entry state or stub.
    auto quests = GetActiveQuests();
    for (const auto& q : quests) {
        if (q.ID == questId && (q.State & 1)) return true;
    }
    return false;
}

} // namespace WoWMemory
