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

    std::cout << "All memory safety assertions passed successfully!\n";
    return 0;
}
