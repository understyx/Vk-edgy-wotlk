#include "layer_threads.h"
#include <iostream>
#include <cassert>
#include <chrono>

int main() {
    std::cout << "=== Running Comprehensive JS Addon & HTML/Canvas WebTexture Rendering Tests ===" << std::endl;

    // 1. Create and verify the thread manager
    LayerThreadManager manager;
    assert(!manager.isRunning());

    // 2. Start the threads
    std::cout << "[Test] Starting thread manager..." << std::endl;
    manager.start();
    assert(manager.isRunning());

    // 3. Give the pipeline some time to warm up and render some frames
    std::cout << "[Test] Waiting for JS/HTML Engine to process addons and render to WebTexture..." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // 4. Retrieve the latest double-buffered UI and WebTexture frame
    UIData uiFrame;
    manager.getLatestUIFrame(uiFrame);

    std::cout << "[Test] Validating frame properties..." << std::endl;
    assert(uiFrame.uiFrameId > 0 && "UI frame ID should be greater than zero.");
    assert(uiFrame.sourceGameFrameId > 0 && "Source game frame ID should be greater than zero.");

    // 5. Verify the WebTexture size and pixels
    const auto& tex = uiFrame.webTexture;
    std::cout << "[Test] WebTexture dimensions: " << tex.width << "x" << tex.height << std::endl;
    assert(tex.width == 800);
    assert(tex.height == 600);
    assert(tex.rgbaPixels.size() == 800 * 600 * 4 && "Texture pixel buffer size must equal width * height * 4.");

    // 6. Verify pixel colors (from the green player health bar drawn by player_hp_addon.js)
    // Pixel (35, 42) is inside the green bar, below the white text overlay line (which is at Y=35)
    uint32_t sampleX = 35;
    uint32_t sampleY = 42;
    uint32_t offset = (sampleY * tex.width + sampleX) * 4;

    uint8_t r = tex.rgbaPixels[offset + 0];
    uint8_t g = tex.rgbaPixels[offset + 1];
    uint8_t b = tex.rgbaPixels[offset + 2];
    uint8_t a = tex.rgbaPixels[offset + 3];

    std::cout << "[Test] Sampled pixel at (" << sampleX << ", " << sampleY << "): RGBA("
              << (int)r << ", " << (int)g << ", " << (int)b << ", " << (int)a << ")" << std::endl;

    // Check that we drew the green player health bar: R=30, G=210, B=30, A=255
    assert(r == 30 && "Red channel of health bar should be 30.");
    assert(g == 210 && "Green channel of health bar should be 210.");
    assert(b == 30 && "Blue channel of health bar should be 30.");
    assert(a == 255 && "Alpha channel of health bar should be 255.");

    // Verify text overlay color (at Y=35)
    uint32_t textOffset = (35 * tex.width + sampleX) * 4;
    uint8_t textR = tex.rgbaPixels[textOffset + 0];
    uint8_t textG = tex.rgbaPixels[textOffset + 1];
    uint8_t textB = tex.rgbaPixels[textOffset + 2];
    uint8_t textA = tex.rgbaPixels[textOffset + 3];
    std::cout << "[Test] Sampled text overlay pixel at (" << sampleX << ", 35): RGBA("
              << (int)textR << ", " << (int)textG << ", " << (int)textB << ", " << (int)textA << ")" << std::endl;
    assert(textR == 255 && textG == 255 && textB == 255 && textA == 255 && "Text overlay pixel must be solid white.");

    // 7. Verify addon UI elements list
    assert(uiFrame.elements.size() >= 2 && "UI frame should have at least 2 addon elements.");
    bool foundHpBar = false;
    bool foundCoordTracker = false;
    for (const auto& elem : uiFrame.elements) {
        std::cout << "  - Element: '" << elem.id << "' Typ: '" << elem.type << "' Text: '" << elem.text << "'" << std::endl;
        if (elem.id == "addon_player_hp") {
            foundHpBar = true;
            assert(elem.value > 0.0f);
        } else if (elem.id == "addon_coord_tracker") {
            foundCoordTracker = true;
        }
    }
    assert(foundHpBar && "Player HP Bar addon element should be found.");
    assert(foundCoordTracker && "Coordinate tracker addon element should be found.");

    // 8. Stop the threads cleanly
    std::cout << "[Test] Stopping thread manager..." << std::endl;
    manager.stop();
    assert(!manager.isRunning());

    std::cout << "[Test Result] ALL JS ADDON AND HTML WEBTEXTURE RENDER TESTS PASSED SUCCESSFULLY!" << std::endl;
    return 0;
}
