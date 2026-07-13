#include "layer_threads.h"
#include <iostream>
#include <cassert>
#include <chrono>

int main() {
    std::cout << "=== Running Vulkan Layer UI Thread and Double Buffer Unit Tests ===" << std::endl;

    // 1. Create the thread manager
    LayerThreadManager manager;
    assert(!manager.isRunning() && "Thread manager should not be running initially.");

    // 2. Start the threads
    std::cout << "[Test] Starting thread manager..." << std::endl;
    manager.start();
    assert(manager.isRunning() && "Thread manager should be running after start().");

    // 3. Let it run for a while, periodically sampling and verifying double-buffered UI data
    std::cout << "[Test] Waiting for thread processing..." << std::endl;

    UIData uiFrame;
    bool receivedFrame = false;
    uint32_t lastUiFrameId = 0;

    for (int i = 0; i < 15; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));

        manager.getLatestUIFrame(uiFrame);
        if (uiFrame.uiFrameId > 0) {
            receivedFrame = true;
            assert(uiFrame.elements.size() > 0 && "UI Frame elements list should not be empty once frame is generated.");

            // Verify structure content correctness
            bool foundHpBar = false;
            bool foundPosLabel = false;
            for (const auto& elem : uiFrame.elements) {
                if (elem.id == "addon_player_hp") {
                    foundHpBar = true;
                    assert(elem.type == "bar");
                    assert(elem.value > 0.0f && elem.value <= 1.0f);
                } else if (elem.id == "addon_coord_tracker") {
                    foundPosLabel = true;
                    assert(elem.type == "label");
                }
            }

            assert(foundHpBar && "Should find a Player HP Bar in UI frame elements.");
            assert(foundPosLabel && "Should find a Player Position display in UI frame elements.");

            std::cout << "[Test Sample] UI Frame #" << uiFrame.uiFrameId
                      << " (Source Game Frame #" << uiFrame.sourceGameFrameId
                      << ") contains " << uiFrame.elements.size() << " elements. Target/HP bar info: "
                      << uiFrame.elements[0].text << std::endl;

            // Ensure frame ID is monotonically increasing
            assert(uiFrame.uiFrameId >= lastUiFrameId && "UI Frame ID must be monotonically increasing.");
            lastUiFrameId = uiFrame.uiFrameId;
        }
    }

    assert(receivedFrame && "Should have successfully received at least one UI frame from the rendering thread.");

    // 4. Stop the threads cleanly
    std::cout << "[Test] Stopping thread manager..." << std::endl;
    manager.stop();
    assert(!manager.isRunning() && "Thread manager should not be running after stop().");

    std::cout << "[Test Result] ALL DOUBLE-BUFFER AND THREAD SYNCHRONIZATION TESTS PASSED SUCCESSFULLY!" << std::endl;
    return 0;
}
