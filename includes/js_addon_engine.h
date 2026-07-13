#pragma once

#include "communication_types.h"
#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <algorithm>

// Structure representing a JS Addon loaded by the layer
struct JSAddon {
    std::string name;
    std::string filename;
    std::string jsSource;
    bool isEnabled = true;
};

// Simulated HTML/JS Rendering Engine (similar to Ultralight or coherent UI)
class WebUIRenderingEngine {
public:
    WebUIRenderingEngine() = default;

    // Initializes/resizes a WebTexture and clears it with a transparent background
    void initTexture(WebTexture& tex, uint32_t width = 800, uint32_t height = 600) {
        tex.width = width;
        tex.height = height;
        tex.rgbaPixels.assign(width * height * 4, 0); // Transparent black
        tex.isDirty = true;
    }

    // Rasterizes a solid rectangle onto the WebTexture's pixel buffer
    void renderRect(WebTexture& tex, uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
        if (tex.rgbaPixels.empty()) {
            initTexture(tex);
        }

        for (uint32_t dy = 0; dy < h; ++dy) {
            uint32_t py = y + dy;
            if (py >= tex.height) continue;
            for (uint32_t dx = 0; dx < w; ++dx) {
                uint32_t px = x + dx;
                if (px >= tex.width) continue;
                uint32_t offset = (py * tex.width + px) * 4;
                tex.rgbaPixels[offset + 0] = r;
                tex.rgbaPixels[offset + 1] = g;
                tex.rgbaPixels[offset + 2] = b;
                tex.rgbaPixels[offset + 3] = a;
            }
        }
        tex.isDirty = true;
    }

    // Simulated HTML/CSS font rendering: draws a horizontal colored text line
    void renderTextLineSim(WebTexture& tex, uint32_t x, uint32_t y, uint32_t length, uint8_t r, uint8_t g, uint8_t b) {
        // Draw a neat 4-pixel high dotted line of text representation
        renderRect(tex, x, y, length, 4, r, g, b, 255);
    }
};

// JS Shim Runtime - binds C++ GameData to a simulated Javascript state
class JSShimRuntime {
private:
    std::vector<JSAddon> m_addons;
    WebUIRenderingEngine m_renderEngine;

public:
    JSShimRuntime() {
        loadDefaultAddons();
    }

    // Simulated method to load addons from folder
    void loadDefaultAddons() {
        // Addon 1: Modern HUD Player Health Bar
        JSAddon playerHud;
        playerHpAddon(playerHud);
        m_addons.push_back(playerHud);

        // Addon 2: Coordinate tracker Addon
        JSAddon coordTracker;
        coordsAddon(coordTracker);
        m_addons.push_back(coordTracker);
    }

    // Updates the JS addon variables and executes simulated script callbacks
    void runAddons(const GameData& gameData, UIData& outUIData) {
        // Initialize our web texture
        m_renderEngine.initTexture(outUIData.webTexture, 800, 600);
        outUIData.elements.clear();

        // Background web panel wash (transparent dark background)
        m_renderEngine.renderRect(outUIData.webTexture, 0, 0, 800, 600, 15, 15, 20, 100);

        for (const auto& addon : m_addons) {
            if (!addon.isEnabled) continue;

            // Simulate executing Javascript JS code matching the addon's context:
            if (addon.filename == "player_hp_addon.js") {
                // Simulated JS execution:
                // var hp = Game.getPlayerHP();
                // var maxHp = Game.getPlayerMaxHP();
                // Canvas.drawRect(20, 20, (hp / maxHp) * 200, 25, "green");
                float hpPercent = (float)gameData.playerHp / gameData.playerMaxHp;
                uint32_t barWidth = static_cast<uint32_t>(hpPercent * 200.0f);

                // 1. Rasterize onto WebTexture (using third-party engine simulation)
                m_renderEngine.renderRect(outUIData.webTexture, 30, 30, 200, 25, 40, 40, 45, 255); // Backing
                m_renderEngine.renderRect(outUIData.webTexture, 30, 30, barWidth, 25, 30, 210, 30, 255); // Fill green
                m_renderEngine.renderTextLineSim(outUIData.webTexture, 35, 35, 100, 255, 255, 255); // Simulated text characters

                // 2. Map back to UI elements list for layout validation
                UIElement elem;
                elem.id = "addon_player_hp";
                elem.type = "bar";
                elem.screenX = 30.0f;
                elem.screenY = 30.0f;
                elem.width = 200.0f;
                elem.height = 25.0f;
                elem.r = 30; elem.g = 210; elem.b = 30; elem.a = 255;
                elem.text = "JS ADDON: HP BAR (" + std::to_string(gameData.playerHp) + "%)";
                elem.value = hpPercent;
                outUIData.elements.push_back(elem);

            } else if (addon.filename == "coordinate_tracker_addon.js") {
                // Simulated JS execution:
                // var coords = Game.getPlayerCoords();
                // Canvas.drawText("X: " + coords.x + ", Y: " + coords.y, 30, 70, "white");

                // 1. Rasterize text onto WebTexture plane
                m_renderEngine.renderRect(outUIData.webTexture, 30, 70, 150, 15, 60, 60, 60, 200); // Backing box
                m_renderEngine.renderTextLineSim(outUIData.webTexture, 35, 75, 120, 255, 255, 255); // Text lines

                // 2. Element list mapping
                UIElement elem;
                elem.id = "addon_coord_tracker";
                elem.type = "label";
                elem.screenX = 30.0f;
                elem.screenY = 70.0f;
                elem.width = 150.0f;
                elem.height = 15.0f;
                elem.r = 255; elem.g = 255; elem.b = 255; elem.a = 255;

                char textBuffer[64];
                snprintf(textBuffer, sizeof(textBuffer), "Coords: X:%.1f Y:%.1f", gameData.playerX, gameData.playerY);
                elem.text = textBuffer;
                outUIData.elements.push_back(elem);
            }
        }
    }

    const std::vector<JSAddon>& getAddons() const { return m_addons; }

private:
    void playerHpAddon(JSAddon& addon) {
        addon.name = "Player HP HUD";
        addon.filename = "player_hp_addon.js";
        addon.jsSource =
            "// HTML+JS Addon for player HP HUD\n"
            "const playerHp = Game.getPlayerHP();\n"
            "const playerMaxHp = Game.getPlayerMaxHP();\n"
            "const ratio = playerHp / playerMaxHp;\n"
            "Canvas.drawRect(30, 30, ratio * 200, 25, 'rgba(30, 210, 30, 255)');\n";
    }

    void coordsAddon(JSAddon& addon) {
        addon.name = "Coordinate Tracker";
        addon.filename = "coordinate_tracker_addon.js";
        addon.jsSource =
            "// HTML+JS Addon for Tracking Player Coordinates\n"
            "const x = Game.getPlayerX();\n"
            "const y = Game.getPlayerY();\n"
            "Canvas.drawText(`Coords: X:${x.toFixed(1)} Y:${y.toFixed(1)}`, 30, 70, 'white');\n";
    }
};
