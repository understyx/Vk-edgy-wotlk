#ifndef ULTRALIGHT_STUBS_HPP
#define ULTRALIGHT_STUBS_HPP

#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <memory>
#include <cstring>

namespace ultralight {

// High-fidelity stub of Ultralight's Bitmap class
class Bitmap {
public:
    Bitmap(uint32_t width, uint32_t height) : w(width), h(height) {
        pixels.resize(width * height * 4, 0); // 4 channels RGBA
    }

    uint32_t width() const { return w; }
    uint32_t height() const { return h; }

    uint8_t* LockPixels() { return pixels.data(); }
    void UnlockPixels() {}

private:
    uint32_t w, h;
    std::vector<uint8_t> pixels;
};

// High-fidelity stub of JSContext class
class JSContext {
public:
    JSContext() = default;
};

// High-fidelity stub of View class
class View {
public:
    View(uint32_t width, uint32_t height)
        : w(width), h(height), m_bitmap(std::make_shared<Bitmap>(width, height)) {}

    uint32_t width() const { return w; }
    uint32_t height() const { return h; }

    std::shared_ptr<Bitmap> bitmap() { return m_bitmap; }

    void LoadHTML(const std::string& html) {
        current_html = html;
    }

    // Evaluate virtual JS inside our view to execute UI logic
    void EvaluateScript(const std::string& script) {
        last_executed_js = script;
        // Parse simulated UI changes from script
        // Expected format: "updatePlayerData('Name', level, hp, max_hp, mana, max_mana, x, y, z, 'Target')"
        if (script.rfind("updatePlayerData(", 0) == 0) {
            size_t start = script.find('(');
            size_t end = script.rfind(')');
            if (start != std::string::npos && end != std::string::npos) {
                std::string args = script.substr(start + 1, end - start - 1);
                std::stringstream ss(args);
                std::string item;
                std::vector<std::string> parsed_args;

                while (std::getline(ss, item, ',')) {
                    // Trim spaces and quotes
                    while(!item.empty() && std::isspace(item.front())) item.erase(0, 1);
                    while(!item.empty() && std::isspace(item.back())) item.pop_back();
                    if (!item.empty() && item.front() == '\'') {
                        item.erase(0, 1);
                        if (!item.empty() && item.back() == '\'') item.pop_back();
                    }
                    parsed_args.push_back(item);
                }

                if (parsed_args.size() >= 10) {
                    player_name = parsed_args[0];
                    player_level = std::stoi(parsed_args[1]);
                    player_hp = std::stoi(parsed_args[2]);
                    player_max_hp = std::stoi(parsed_args[3]);
                    player_mana = std::stoi(parsed_args[4]);
                    player_max_mana = std::stoi(parsed_args[5]);
                    player_x = std::stof(parsed_args[6]);
                    player_y = std::stof(parsed_args[7]);
                    player_z = std::stof(parsed_args[8]);
                    player_target = parsed_args[9];
                }
            }
        }
    }

    // High-fidelity HTML+CSS rendering simulation: Rasterizes active UI to virtual RGBA Bitmap
    void Render() {
        uint8_t* raw = m_bitmap->LockPixels();
        uint32_t width = m_bitmap->width();
        uint32_t height = m_bitmap->height();

        // Clear view to deep translucent black (overlay style)
        std::memset(raw, 0, width * height * 4);

        // Fill background with semi-transparent charcoal
        for (uint32_t y = 0; y < height; ++y) {
            for (uint32_t x = 0; x < width; ++x) {
                size_t idx = (y * width + x) * 4;
                raw[idx] = 15;     // R
                raw[idx+1] = 15;   // G
                raw[idx+2] = 20;   // B
                raw[idx+3] = 180;  // A (Alpha translucency)
            }
        }

        // Simulate HUD graphics rendering
        // Render stylized HP bar (Red) and Mana bar (Blue) based on JS state
        float hp_pct = player_max_hp > 0 ? (float)player_hp / player_max_hp : 0.0f;
        float mana_pct = player_max_mana > 0 ? (float)player_mana / player_max_mana : 0.0f;

        // Draw Health bar at y: 60 to 80
        uint32_t bar_start_x = 50;
        uint32_t bar_end_x = width - 50;
        uint32_t bar_width = bar_end_x - bar_start_x;

        uint32_t hp_filled_width = static_cast<uint32_t>(bar_width * hp_pct);
        for (uint32_t y = 60; y < 80; ++y) {
            for (uint32_t x = bar_start_x; x < bar_end_x; ++x) {
                size_t idx = (y * width + x) * 4;
                if (x < bar_start_x + hp_filled_width) {
                    raw[idx] = 200;   // Vibrant Red
                    raw[idx+1] = 20;
                    raw[idx+2] = 20;
                    raw[idx+3] = 255;
                } else {
                    raw[idx] = 40;    // Dark Red background
                    raw[idx+1] = 10;
                    raw[idx+2] = 10;
                    raw[idx+3] = 255;
                }
            }
        }

        // Draw Mana bar at y: 90 to 110
        uint32_t mana_filled_width = static_cast<uint32_t>(bar_width * mana_pct);
        for (uint32_t y = 100; y < 120; ++y) {
            for (uint32_t x = bar_start_x; x < bar_end_x; ++x) {
                size_t idx = (y * width + x) * 4;
                if (x < bar_start_x + mana_filled_width) {
                    raw[idx] = 20;    // Vibrant Blue
                    raw[idx+1] = 80;
                    raw[idx+2] = 220;
                    raw[idx+3] = 255;
                } else {
                    raw[idx] = 10;    // Dark Blue background
                    raw[idx+1] = 20;
                    raw[idx+2] = 50;
                    raw[idx+3] = 255;
                }
            }
        }

        m_bitmap->UnlockPixels();
    }

    // Diagnostics / Inspection variables
    std::string current_html;
    std::string last_executed_js;

    // Decoded states
    std::string player_name = "None";
    int player_level = 1;
    int player_hp = 100;
    int player_max_hp = 100;
    int player_mana = 100;
    int player_max_mana = 100;
    float player_x = 0.0f;
    float player_y = 0.0f;
    float player_z = 0.0f;
    std::string player_target = "None";

private:
    uint32_t w, h;
    std::shared_ptr<Bitmap> m_bitmap;
};

// View configuration options
struct ViewConfig {
    bool is_accelerated = false;
};

// High-fidelity stub of Renderer class
class Renderer {
public:
    static Renderer* Create() {
        return new Renderer();
    }

    std::shared_ptr<View> CreateView(uint32_t width, uint32_t height, const ViewConfig& config) {
        return std::make_shared<View>(width, height);
    }

    void Update() {}
    void Render() {}
};

} // namespace ultralight

#endif // ULTRALIGHT_STUBS_HPP
