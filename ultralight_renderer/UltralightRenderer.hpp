#ifndef ULTRALIGHT_RENDERER_HPP
#define ULTRALIGHT_RENDERER_HPP

#include "UltralightStubs.hpp"
#include "../common/GameDataTypes.hpp"
#include "../common/PointerSwapBuffer.hpp"
#include <iostream>
#include <chrono>
#include <thread>
#include <atomic>
#include <sstream>

// Double-buffered structure representing the texture shared with Vulkan
struct WebTexture {
    uint32_t width = 0;
    uint32_t height = 0;
    std::vector<uint8_t> rgbaPixels;
};

inline void jsHtmlEngineThreadLoop(
    std::atomic<bool>& running,
    PointerSwapBuffer<PlayerData>& gameDataBuffer,
    DoubleBuffer<WebTexture>& outputTextureBuffer)
{
    std::cout << "[JS/HTML Thread] Started." << std::endl;

    // Initialize Ultralight Engine
    ultralight::Renderer* renderer = ultralight::Renderer::Create();
    ultralight::ViewConfig config;
    config.is_accelerated = false;

    // UI HUD dimension (e.g., 800 x 600 overlay)
    const uint32_t VIEW_WIDTH = 800;
    const uint32_t VIEW_HEIGHT = 600;
    auto view = renderer->CreateView(VIEW_WIDTH, VIEW_HEIGHT, config);

    // Initial HTML UI structure with placeholders and styling
    std::string html_ui =
        "<html>"
        "<head>"
        "  <style>"
        "    body { font-family: sans-serif; color: white; background: transparent; margin: 0; padding: 20px; }"
        "    .hud-card { background: rgba(0,0,0,0.7); border: 2px solid #ffcc00; padding: 15px; border-radius: 8px; width: 400px; }"
        "    .bar { height: 15px; background-color: #333; margin: 5px 0; border-radius: 4px; overflow: hidden; }"
        "    .hp { background-color: #ff3333; height: 100%; transition: width 0.1s; }"
        "    .mp { background-color: #3333ff; height: 100%; transition: width 0.1s; }"
        "  </style>"
        "  <script>"
        "    function updatePlayerData(name, level, hp, maxHp, mp, maxMp, x, y, z, target) {"
        "      document.getElementById('p-name').innerText = name + ' (Lvl ' + level + ')';"
        "      document.getElementById('p-hp-text').innerText = hp + ' / ' + maxHp;"
        "      document.getElementById('p-mp-text').innerText = mp + ' / ' + maxMp;"
        "      document.getElementById('p-pos').innerText = 'Pos: ' + x.toFixed(2) + ', ' + y.toFixed(2) + ', ' + z.toFixed(2);"
        "      document.getElementById('p-target').innerText = 'Target: ' + target;"
        "    }"
        "  </script>"
        "</head>"
        "<body>"
        "  <div class='hud-card'>"
        "    <h2 id='p-name' style='margin-top:0;'>Player Stats</h2>"
        "    <div>HP: <span id='p-hp-text'>-</span></div>"
        "    <div class='bar'><div class='hp' id='p-hp-bar' style='width: 100%;'></div></div>"
        "    <div>Mana: <span id='p-mp-text'>-</span></div>"
        "    <div class='bar'><div class='mp' id='p-mp-bar' style='width: 100%;'></div></div>"
        "    <div id='p-pos'>Pos: -</div>"
        "    <div id='p-target' style='color:#ffcc00; font-weight:bold;'>Target: None</div>"
        "  </div>"
        "</body>"
        "</html>";

    view->LoadHTML(html_ui);

    PlayerData current_telemetry;

    while (running.load(std::memory_order_relaxed)) {
        // 1. Consume latest telemetry from Game Data Thread
        bool has_new_telemetry = gameDataBuffer.update();
        if (has_new_telemetry) {
            PlayerData* latest = gameDataBuffer.get_read_buffer();
            std::memcpy(&current_telemetry, latest, sizeof(PlayerData));
        }

        // 2. Format JS evaluation string to update the UI
        std::stringstream js_call;
        js_call << "updatePlayerData('"
                << current_telemetry.name << "', "
                << current_telemetry.level << ", "
                << current_telemetry.health << ", "
                << current_telemetry.maxHealth << ", "
                << current_telemetry.mana << ", "
                << current_telemetry.maxMana << ", "
                << current_telemetry.posX << ", "
                << current_telemetry.posY << ", "
                << current_telemetry.posZ << ", '"
                << current_telemetry.targetName << "')";

        // Evaluate script inside Ultralight Web Engine
        view->EvaluateScript(js_call.str());

        // 3. Render HTML/CSS to virtual UI bitmap
        view->Render();

        // 4. Copy the rendered bitmap into the back-buffer of double-buffered structures
        WebTexture* back_tex = outputTextureBuffer.get_back();
        back_tex->width = VIEW_WIDTH;
        back_tex->height = VIEW_HEIGHT;

        size_t pixel_size = VIEW_WIDTH * VIEW_HEIGHT * 4;
        if (back_tex->rgbaPixels.size() != pixel_size) {
            back_tex->rgbaPixels.resize(pixel_size);
        }

        uint8_t* raw_src = view->bitmap()->LockPixels();
        std::memcpy(back_tex->rgbaPixels.data(), raw_src, pixel_size);
        view->bitmap()->UnlockPixels();

        // 5. Swap front and back pointers to seamlessly share WebUI texture with Vulkan
        outputTextureBuffer.swap();

        // Throttled UI refresh loop (~60Hz / 16.67 ms)
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    delete renderer;
    std::cout << "[JS/HTML Thread] Stopped." << std::endl;
}

#endif // ULTRALIGHT_RENDERER_HPP
