#include "game_data/GameDataQuery.hpp"
#include "common/PointerSwapBuffer.hpp"
#include <thread>
#include <chrono>
#include <cstring>
#include <atomic>
#include <cmath>

namespace GameData {

void gameDataThreadLoop(std::atomic<bool>& running, PointerSwapBuffer<TelemetryData>& gameDataBuffer) {
    float angle = 0.0f;
    uint32_t frameCounter = 0;

    while (running) {
        // Retrieve the current write buffer from the triple buffer
        TelemetryData* data = gameDataBuffer.getWriteBuffer();

        // Simulate game telemetry state updates
        data->health = 80 + (frameCounter % 21); // oscillates between 80 and 100
        data->maxHealth = 100;
        data->mana = 50 + (frameCounter % 51); // oscillates between 50 and 100
        data->maxMana = 100;

        // Circular motion simulation for position coords
        angle += 0.05f;
        data->posX = 1500.0f + 100.0f * std::cos(angle);
        data->posY = -2300.0f + 100.0f * std::sin(angle);
        data->posZ = 120.0f;

        data->level = 80;
        data->xp = (frameCounter * 25) % 1000;

        std::strncpy(data->zoneName, "Icecrown Citadel", sizeof(data->zoneName) - 1);
        data->zoneName[sizeof(data->zoneName) - 1] = '\0';

        if (frameCounter % 4 < 2) {
            std::strncpy(data->targetName, "The Lich King", sizeof(data->targetName) - 1);
        } else {
            std::strncpy(data->targetName, "Lich King's Ghoul", sizeof(data->targetName) - 1);
        }
        data->targetName[sizeof(data->targetName) - 1] = '\0';

        // Swap the update to the shared / consumer structure
        gameDataBuffer.swapProducer();

        frameCounter++;
        std::this_thread::sleep_for(std::chrono::milliseconds(33)); // ~30Hz query/simulation rate
    }
}

} // namespace GameData
