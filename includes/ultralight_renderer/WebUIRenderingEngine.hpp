#pragma once
#include "game_data/GameDataQuery.hpp"
#include <vector>
#include <string>
#include <atomic>

#include "common/PointerSwapBuffer.hpp"

namespace UltralightRenderer {

// Double-buffered RGBA pixel structure to hold output textures rendered by the HTML/CSS engine.
struct WebTexture {
    uint32_t width = 1024;
    uint32_t height = 768;
    std::vector<uint8_t> rgbaPixels;

    WebTexture() {
        rgbaPixels.resize(width * height * 4, 0);
    }
};

// Simulated Virtual JS VM Engine state / callbacks
class JSShimRuntime {
public:
    JSShimRuntime();
    std::string evaluateTelemetryToJSON(const GameData::TelemetryData& telemetry);
};

// Simulated HTML canvas-style rasterization renderer matching Ultralight design.
class WebUIRenderingEngine {
public:
    WebUIRenderingEngine(uint32_t width = 1024, uint32_t height = 768);

    void loadHTML(const std::string& htmlContent);
    void updateTelemetryState(const std::string& jsonData);
    void render(WebTexture* outputTexture);

private:
    uint32_t m_width;
    uint32_t m_height;
    std::string m_html;
    std::string m_currentJsonState;
};

// Thread loop that reads telemetry from gameDataBuffer, feeds it to JS context, renders HTML overlays onto WebTexture.
void jsHtmlEngineThreadLoop(
    std::atomic<bool>& running,
    PointerSwapBuffer<GameData::TelemetryData>& gameDataBuffer,
    PointerSwapBuffer<WebTexture>& uiTextureBuffer);

} // namespace UltralightRenderer
