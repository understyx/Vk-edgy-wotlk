#pragma once

#include "double_buffer.h"
#include "communication_types.h"

#include <thread>
#include <chrono>
#include <atomic>
#include <iostream>
#include <vector>
#include <string>
#include <cmath>

class LayerThreadManager {
public:
    DoubleBuffer<GameData> gameDataBuffer;
    DoubleBuffer<UIData> uiDataBuffer;

    // Local copy of rendered UI data ready for Vulkan blit/drawing on the present hook
    UIData latestRenderedUIData;
    mutable std::mutex renderedUIMutex;

private:
    std::atomic<bool> m_running{false};
    std::thread m_gameDataThread;
    std::thread m_jsHtmlThread;
    std::thread m_renderingThread;

public:
    LayerThreadManager() = default;

    ~LayerThreadManager() {
        stop();
    }

    void start() {
        if (m_running.exchange(true)) {
            return; // Already running
        }
        std::cout << "[LayerThreadManager] Starting modern UI threads..." << std::endl;
        m_gameDataThread = std::thread(&LayerThreadManager::gameDataThreadLoop, this);
        m_jsHtmlThread = std::thread(&LayerThreadManager::jsHtmlThreadLoop, this);
        m_renderingThread = std::thread(&LayerThreadManager::renderingThreadLoop, this);
    }

    void stop() {
        if (!m_running.exchange(false)) {
            return; // Already stopped
        }
        std::cout << "[LayerThreadManager] Stopping modern UI threads..." << std::endl;
        if (m_gameDataThread.joinable()) m_gameDataThread.join();
        if (m_jsHtmlThread.joinable()) m_jsHtmlThread.join();
        if (m_renderingThread.joinable()) m_renderingThread.join();
        std::cout << "[LayerThreadManager] All threads stopped successfully." << std::endl;
    }

    bool isRunning() const {
        return m_running.load();
    }

    // Safely copy the latest UI frame prepared by the rendering thread
    void getLatestUIFrame(UIData& outFrame) const {
        std::lock_guard<std::mutex> lock(renderedUIMutex);
        outFrame = latestRenderedUIData;
    }

private:
    // Thread 1: Game Data Read Thread (Simulated)
    void gameDataThreadLoop() {
        std::cout << "[Thread 1: GameData] Started." << std::endl;
        uint32_t simulatedFrameId = 0;
        float angle = 0.0f;

        while (m_running.load()) {
            // Get write access to the back buffer
            GameData* back = gameDataBuffer.getBack();

            // Simulate reading game memory and updating structure
            simulatedFrameId++;
            back->frameId = simulatedFrameId;
            back->isPlayerAlive = true;
            back->playerHp = 85; // Simulated 85% health
            back->playerMaxHp = 100;

            // Move player position in a circle to simulate active movement
            angle += 0.05f;
            back->playerX = 150.0f + std::cos(angle) * 10.0f;
            back->playerY = 75.0f + std::sin(angle) * 10.0f;
            back->playerZ = 12.5f;

            if (simulatedFrameId % 100 < 50) {
                back->targetName = "Sylvanas Windrunner";
                back->targetHp = 42;
                back->targetMaxHp = 100;
            } else {
                back->targetName = "The Lich King";
                back->targetHp = 89;
                back->targetMaxHp = 100;
            }

            // Swap buffers atomically to make it available for JS/HTML thread
            gameDataBuffer.swap();

            // Sleep to simulate game update cycle (~60 FPS)
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
        std::cout << "[Thread 1: GameData] Stopped." << std::endl;
    }

    // Thread 2: JS/HTML VM & Layout Engine Thread (Simulated)
    void jsHtmlThreadLoop() {
        std::cout << "[Thread 2: JS/HTML] Started." << std::endl;
        uint32_t uiFrameCount = 0;
        GameData localGameData;

        while (m_running.load()) {
            // Retrieve latest game data from the first double buffer
            gameDataBuffer.getFrontCopy(localGameData);

            // Access back buffer of UI Render data
            UIData* backUI = uiDataBuffer.getBack();
            uiFrameCount++;
            backUI->uiFrameId = uiFrameCount;
            backUI->sourceGameFrameId = localGameData.frameId;
            backUI->elements.clear();

            // Run simulated HTML/JS layout rendering to produce elements:

            // 1. Player Health Bar Element
            UIElement playerHpBar;
            playerHpBar.id = "player_hp_bar";
            playerHpBar.type = "bar";
            playerHpBar.screenX = 20.0f;
            playerHpBar.screenY = 20.0f;
            playerHpBar.width = 200.0f;
            playerHpBar.height = 25.0f;
            playerHpBar.r = 0;
            playerHpBar.g = 220;
            playerHpBar.b = 0;
            playerHpBar.a = 255;
            playerHpBar.text = "Player HP: " + std::to_string(localGameData.playerHp) + "/" + std::to_string(localGameData.playerMaxHp);
            playerHpBar.value = (float)localGameData.playerHp / localGameData.playerMaxHp;
            backUI->elements.push_back(playerHpBar);

            // 2. Player Position Coordinates text display
            UIElement posLabel;
            posLabel.id = "player_pos";
            posLabel.type = "label";
            posLabel.screenX = 20.0f;
            posLabel.screenY = 50.0f;
            posLabel.width = 300.0f;
            posLabel.height = 20.0f;
            posLabel.r = 255;
            posLabel.g = 255;
            posLabel.b = 255;
            posLabel.a = 230;

            char posText[128];
            snprintf(posText, sizeof(posText), "X: %.2f | Y: %.2f | Z: %.2f", localGameData.playerX, localGameData.playerY, localGameData.playerZ);
            posLabel.text = posText;
            posLabel.value = 0.0f;
            backUI->elements.push_back(posLabel);

            // 3. Target Frame UI Element
            UIElement targetFrame;
            targetFrame.id = "target_frame";
            targetFrame.type = "box";
            targetFrame.screenX = 400.0f;
            targetFrame.screenY = 20.0f;
            targetFrame.width = 220.0f;
            targetFrame.height = 50.0f;
            targetFrame.r = 220;
            targetFrame.g = 0;
            targetFrame.b = 0;
            targetFrame.a = 255;
            targetFrame.text = localGameData.targetName + " (HP: " + std::to_string(localGameData.targetHp) + "%)";
            targetFrame.value = (float)localGameData.targetHp / localGameData.targetMaxHp;
            backUI->elements.push_back(targetFrame);

            // 4. StyxHTML engine version watermark
            UIElement watermark;
            watermark.id = "watermark";
            watermark.type = "label";
            watermark.screenX = 20.0f;
            watermark.screenY = 700.0f;
            watermark.width = 400.0f;
            watermark.height = 15.0f;
            watermark.r = 120;
            watermark.g = 150;
            watermark.b = 255;
            watermark.a = 255;
            watermark.text = "Modernized UI via StyxHTML VM (Thread 2) -> double buffered output";
            backUI->elements.push_back(watermark);

            // Swap buffers atomically to make it available for the rendering thread
            uiDataBuffer.swap();

            // Sleep to simulate layout & JS virtual machine tick rate (e.g. ~60 ticks per second)
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
        std::cout << "[Thread 2: JS/HTML] Stopped." << std::endl;
    }

    // Thread 3: Vulkan Rendering & Command Preparation Thread
    void renderingThreadLoop() {
        std::cout << "[Thread 3: Rendering] Started." << std::endl;
        UIData localUIData;

        while (m_running.load()) {
            if (uiDataBuffer.hasNewData()) {
                // Read latest UI data compiled from Thread 2
                uiDataBuffer.getFrontCopy(localUIData);

                // Simulate compilation and optimization of Vulkan pipeline resources
                // (e.g. organizing vertices, indices, updating descriptors, creating transfer barriers)
                // Once Vulkan-ready structs/commands are prepared, copy them to latestRenderedUIData
                {
                    std::lock_guard<std::mutex> lock(renderedUIMutex);
                    latestRenderedUIData = localUIData;
                }
            }

            // Sleep briefly to avoid high CPU usage while maintaining very low latency overlay updates
            std::this_thread::sleep_for(std::chrono::milliseconds(8));
        }
        std::cout << "[Thread 3: Rendering] Stopped." << std::endl;
    }
};
