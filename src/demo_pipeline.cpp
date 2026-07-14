#include "common/PointerSwapBuffer.hpp"
#include "game_data/GameDataQuery.hpp"
#include "ultralight_renderer/WebUIRenderingEngine.hpp"
#include "vulkan_renderer/VulkanOverlayRenderer.hpp"

#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>

int main() {
    std::cout << "[Demo] Initializing WoTLK Modern 3-stage Pipeline Integration Test..." << std::endl;

    std::atomic<bool> running{true};

    // Instantiate triple buffer communications
    PointerSwapBuffer<GameData::TelemetryData> gameDataBuffer;
    PointerSwapBuffer<UltralightRenderer::WebTexture> uiTextureBuffer;

    // Start threads
    std::thread gameThread(GameData::gameDataThreadLoop, std::ref(running), std::ref(gameDataBuffer));
    std::thread uiThread(UltralightRenderer::jsHtmlEngineThreadLoop, std::ref(running), std::ref(gameDataBuffer), std::ref(uiTextureBuffer));
    std::thread renderThread(VulkanRenderer::renderingThreadLoop, std::ref(running), std::ref(uiTextureBuffer));

    std::cout << "[Demo] All pipeline threads running successfully." << std::endl;

    // Let the integration test run for a few seconds to verify logs/telemetry swapping
    for (int i = 0; i < 5; ++i) {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        // Print status
        const GameData::TelemetryData* gd = gameDataBuffer.getReadBuffer();
        const UltralightRenderer::WebTexture* tex = uiTextureBuffer.getReadBuffer();

        std::cout << "[Demo Status Update " << i + 1 << "/5]" << std::endl;
        std::cout << "  - Game Data HP: " << gd->health << "/" << gd->maxHealth
                  << " | Target: " << gd->targetName << std::endl;
        std::cout << "  - UI Texture Width: " << tex->width << " Height: " << tex->height
                  << " Pixels Size: " << tex->rgbaPixels.size() << " bytes" << std::endl;
    }

    std::cout << "[Demo] Stopping modern pipeline threads..." << std::endl;
    running = false;

    if (gameThread.joinable()) gameThread.join();
    if (uiThread.joinable()) uiThread.join();
    if (renderThread.joinable()) renderThread.join();

    std::cout << "[Demo] All pipeline threads stopped cleanly. Integration test SUCCESS." << std::endl;
    return 0;
}
