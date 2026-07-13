#pragma once

#include "double_buffer.h"
#include "communication_types.h"
#include "js_addon_engine.h"

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

    // The JS Addon Shim Sandbox & HTML/CSS Canvas rendering engine (e.g. Ultralight)
    JSShimRuntime m_jsRuntime;

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

            // Execute registered JS addons and render directly onto backUI->webTexture plane
            m_jsRuntime.runAddons(localGameData, *backUI);

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
                // Read latest UI data compiled from Thread 2 (contains WebTexture + elements list)
                uiDataBuffer.getFrontCopy(localUIData);

                // Simulate upload of WebTexture to GPU as a Vulkan image
                if (localUIData.webTexture.isDirty) {
                    // Simulating Vulkan copy/upload commands (vkCmdCopyBufferToImage)
                    localUIData.webTexture.isDirty = false;
                }

                // Copy to final presentation buffer under lock
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
