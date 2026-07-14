#include "common/GameDataTypes.hpp"
#include "common/PointerSwapBuffer.hpp"
#include "game_data/GameData.hpp"
#include "ultralight_renderer/UltralightRenderer.hpp"
#include "vulkan_renderer/VulkanRenderer.hpp"
#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>
#include <cassert>

int main() {
    std::cout << "=== Starting 3-Stage Game UI Pipeline Groundwork Demo ===" << std::endl;

    std::atomic<bool> pipelineRunning(true);

    // 1. Instantiating the shared buffers
    // Triple-buffered PlayerData structure (Game Data Thread -> JS/HTML Thread)
    PointerSwapBuffer<PlayerData> gameDataBuffer;

    // Double-buffered WebTexture structure (JS/HTML Thread -> Vulkan Thread)
    DoubleBuffer<WebTexture> outputTextureBuffer;

    // 2. Spawn the 3 miniproject threads
    std::cout << "[Orchestrator] Spawning threads..." << std::endl;
    std::thread tGameData(gameDataThreadLoop, std::ref(pipelineRunning), std::ref(gameDataBuffer));
    std::thread tJsHtml(jsHtmlEngineThreadLoop, std::ref(pipelineRunning), std::ref(gameDataBuffer), std::ref(outputTextureBuffer));
    std::thread tVulkan(renderingThreadLoop, std::ref(pipelineRunning), std::ref(outputTextureBuffer));

    // Let the pipeline execute for a few seconds to verify complete end-to-end integration
    std::cout << "[Orchestrator] Running end-to-end pipeline..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(3));

    // 3. Graceful termination of the pipeline
    std::cout << "[Orchestrator] Stopping pipeline threads..." << std::endl;
    pipelineRunning.store(false, std::memory_order_relaxed);

    tGameData.join();
    tJsHtml.join();
    tVulkan.join();

    std::cout << "=== 3-Stage Pipeline Groundwork Complete & Verified! ===" << std::endl;
    return 0;
}
