#include "wowmemory/wowmemory.h"
#include <iostream>
#include <cassert>

int main() {
    std::cout << "Starting memory safety test...\n";

    WoWMemory::GameDataReader reader;
    WoWMemory::GameData data;

    // Attempting to read game data.
    // In a normal non-WoW process context, all the WoW absolute addresses (e.g., WoWOffsets::playerName)
    // are unmapped. Therefore, if the memory safety checks are working correctly,
    // this call should complete successfully without causing a segmentation fault.
    std::cout << "Calling ReadGameData with unmapped offsets...\n";
    bool success = reader.ReadGameData(data);
    std::cout << "ReadGameData completed. Success status: " << std::boolalpha << success << "\n";

    // Verify that we got safe defaults and didn't crash
    assert(success == true);
    assert(data.playerName.empty());
    assert(data.realmName.empty());
    assert(data.localPlayerGUID == 0);
    assert(data.continentName.empty());
    assert(data.zoneText.empty());
    assert(data.subZoneText.empty());
    assert(data.mapID == 0);
    assert(data.zoneID == 0);
    assert(data.playerHealth == 0);
    assert(!data.playerIsIngame);
    assert(!data.worldLoaded);
    assert(!data.isLoading);
    assert(!data.isIndoor);
    assert(data.gameState == 0);
    assert(data.tickCount == 0);
    assert(data.corpseX == 0.0f);
    assert(data.corpseY == 0.0f);
    assert(data.corpseZ == 0.0f);
    assert(data.auraCount == 0);
    assert(data.combatLogEventCount == 0);
    assert(data.numPartyMembers == 0);
    assert(data.partyDifficulty == 0);
    assert(data.numRaidMembers == 0);
    assert(data.raidDifficulty == 0);
    assert(data.activeQuestsCount == 0);
    assert(data.partyMembersList.empty());
    assert(data.raidMembersList.empty());

    // Serialization & Deserialization Test
    std::cout << "Testing serialization and deserialization...\n";
    WoWMemory::GameData testData;
    testData.playerName = "TestPlayer";
    testData.realmName = "TestRealm";
    testData.localPlayerGUID = 1234567890ULL;
    testData.mapID = 571;
    testData.playerHealth = 15000;
    testData.playerMaxHealth = 20000;
    testData.playerIsIngame = true;
    testData.playerPosX = 123.45f;
    testData.playerPosY = 678.90f;
    testData.playerPosZ = 12.34f;
    testData.auraCount = 2;
    testData.auras[0].spellId = 123;
    testData.auras[1].spellId = 456;
    testData.combatLogEventCount = 1;
    testData.numPartyMembers = 3;
    testData.partyDifficulty = 2;
    testData.numRaidMembers = 25;
    testData.raidDifficulty = 3;
    testData.activeQuestsCount = 10;

    // Add group members test data
    WoWMemory::GroupMemberData m1;
    m1.guid = 1111ULL;
    m1.name = "PartyMember1";
    m1.health = 8000;
    m1.maxHealth = 10000;
    m1.auraCount = 1;
    m1.auras[0].spellId = 999;
    testData.partyMembersList.push_back(m1);

    WoWMemory::GroupMemberData r1;
    r1.guid = 2222ULL;
    r1.name = "RaidMember1";
    r1.health = 12000;
    r1.maxHealth = 15000;
    r1.auraCount = 2;
    r1.auras[0].spellId = 888;
    r1.auras[1].spellId = 777;
    testData.raidMembersList.push_back(r1);

    testData.combatLogEvents[0].timestamp = 99999;
    testData.combatLogEvents[0].amount = 500;

    std::vector<uint8_t> buffer;
    WoWMemory::SerializeGameData(testData, buffer);
    assert(!buffer.empty());

    WoWMemory::GameData deserializedData;
    bool deserial_success = WoWMemory::DeserializeGameData(buffer, deserializedData);
    assert(deserial_success == true);

    assert(deserializedData.playerName == "TestPlayer");
    assert(deserializedData.realmName == "TestRealm");
    assert(deserializedData.localPlayerGUID == 1234567890ULL);
    assert(deserializedData.mapID == 571);
    assert(deserializedData.playerHealth == 15000);
    assert(deserializedData.playerMaxHealth == 20000);
    assert(deserializedData.playerIsIngame == true);
    assert(deserializedData.playerPosX == 123.45f);
    assert(deserializedData.playerPosY == 678.90f);
    assert(deserializedData.playerPosZ == 12.34f);
    assert(deserializedData.auraCount == 2);
    assert(deserializedData.auras[0].spellId == 123);
    assert(deserializedData.auras[1].spellId == 456);
    assert(deserializedData.combatLogEventCount == 1);
    assert(deserializedData.numPartyMembers == 3);
    assert(deserializedData.partyDifficulty == 2);
    assert(deserializedData.numRaidMembers == 25);
    assert(deserializedData.raidDifficulty == 3);
    assert(deserializedData.activeQuestsCount == 10);

    assert(deserializedData.partyMembersList.size() == 1);
    assert(deserializedData.partyMembersList[0].guid == 1111ULL);
    assert(deserializedData.partyMembersList[0].name == "PartyMember1");
    assert(deserializedData.partyMembersList[0].health == 8000);
    assert(deserializedData.partyMembersList[0].maxHealth == 10000);
    assert(deserializedData.partyMembersList[0].auraCount == 1);
    assert(deserializedData.partyMembersList[0].auras[0].spellId == 999);

    assert(deserializedData.raidMembersList.size() == 1);
    assert(deserializedData.raidMembersList[0].guid == 2222ULL);
    assert(deserializedData.raidMembersList[0].name == "RaidMember1");
    assert(deserializedData.raidMembersList[0].health == 12000);
    assert(deserializedData.raidMembersList[0].maxHealth == 15000);
    assert(deserializedData.raidMembersList[0].auraCount == 2);
    assert(deserializedData.raidMembersList[0].auras[0].spellId == 888);
    assert(deserializedData.raidMembersList[0].auras[1].spellId == 777);

    assert(deserializedData.combatLogEvents[0].timestamp == 99999);
    assert(deserializedData.combatLogEvents[0].amount == 500);

    std::cout << "Serialization & Deserialization assertions passed successfully!\n";

    std::cout << "All memory safety assertions passed successfully!\n";
    return 0;
}
