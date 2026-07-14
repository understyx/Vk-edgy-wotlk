#pragma once
#include <string>
#include <cstdint>
#include <atomic>

// Include the full header so PointerSwapBuffer template is fully defined
#include "common/PointerSwapBuffer.hpp"

namespace GameData {

struct TelemetryData {
    uint32_t health = 100;
    uint32_t maxHealth = 100;
    uint32_t mana = 100;
    uint32_t maxMana = 100;
    float posX = 0.0f;
    float posY = 0.0f;
    float posZ = 0.0f;
    char zoneName[64] = "Azeroth";
    char targetName[64] = "None";
    uint32_t level = 1;
    uint32_t xp = 0;
};

// Simple loop simulating memory querying or hooking to retrieve client telemetry states.
void gameDataThreadLoop(std::atomic<bool>& running, PointerSwapBuffer<TelemetryData>& gameDataBuffer);

} // namespace GameData
