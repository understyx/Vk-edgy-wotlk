#ifndef GAME_DATA_HPP
#define GAME_DATA_HPP

#include "../common/GameDataTypes.hpp"
#include "../common/PointerSwapBuffer.hpp"
#include <iostream>
#include <chrono>
#include <thread>
#include <atomic>
#include <cstring>
#include <cmath>

inline void gameDataThreadLoop(std::atomic<bool>& running, PointerSwapBuffer<PlayerData>& gameDataBuffer) {
    std::cout << "[GameData Thread] Started." << std::endl;

    // Simulate updating telemetry data at a steady state rate (e.g., 60 Hz or custom interval)
    float angle = 0.0f;
    uint32_t tick = 0;
    const float PI = 3.1415926535f;

    while (running.load(std::memory_order_relaxed)) {
        // 1. Get write access from the triple-buffer structure
        PlayerData* writeBuf = gameDataBuffer.get_write_buffer();

        // 2. Fetch/Simulate game data telemetry (simulating memory reads or packet parsing)
        std::strncpy(writeBuf->name, "Arthas", sizeof(writeBuf->name));
        writeBuf->level = 80;
        writeBuf->maxHealth = 42500;

        // Simulating dynamically changing values
        writeBuf->health = writeBuf->maxHealth - (tick % 5000);
        writeBuf->maxMana = 15000;
        writeBuf->mana = writeBuf->maxMana - (tick % 2000);

        // Circular movement simulation
        writeBuf->posX = 150.0f + 50.0f * std::cos(angle);
        writeBuf->posY = -230.0f + 50.0f * std::sin(angle);
        writeBuf->posZ = 45.5f;

        std::strncpy(writeBuf->targetName, "The Lich King", sizeof(writeBuf->targetName));

        // 3. Publish the updated record to make it available to the JS/HTML thread
        gameDataBuffer.publish();

        // Increment step
        angle += 0.05f;
        if (angle > 2.0f * PI) {
            angle -= 2.0f * PI;
        }
        tick += 50;

        // Sleep to throttle queries to ~20Hz (50ms interval) for realistic telemetry querying
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    std::cout << "[GameData Thread] Stopped." << std::endl;
}

#endif // GAME_DATA_HPP
