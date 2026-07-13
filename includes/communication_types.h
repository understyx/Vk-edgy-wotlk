#pragma once

#include <string>
#include <vector>

// Structure for Game Data (read from game memory by Game Data Read Thread)
struct GameData {
    uint32_t frameId = 0;
    bool isPlayerAlive = true;
    uint32_t playerHp = 100;
    uint32_t playerMaxHp = 100;
    float playerX = 0.0f;
    float playerY = 0.0f;
    float playerZ = 0.0f;
    std::string targetName = "None";
    uint32_t targetHp = 0;
    uint32_t targetMaxHp = 100;
};

// Simulated UI Element structure (produced by JS+HTML engine thread)
struct UIElement {
    std::string id;
    std::string type;       // "label", "bar", "box"
    float screenX = 0.0f;
    float screenY = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    uint8_t r = 255;
    uint8_t g = 255;
    uint8_t b = 255;
    uint8_t a = 255;
    std::string text;
    float value = 0.0f;     // for progress/health bars (0.0 to 1.0)
};

// Structure for UI Data (passed from JS/HTML thread to Vulkan rendering thread)
struct UIData {
    uint32_t uiFrameId = 0;
    uint32_t sourceGameFrameId = 0;
    std::vector<UIElement> elements;
};
