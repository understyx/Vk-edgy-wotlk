#include "ultralight_renderer/WebUIRenderingEngine.hpp"
#include "common/PointerSwapBuffer.hpp"
#include <thread>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <iostream>

namespace UltralightRenderer {

JSShimRuntime::JSShimRuntime() {}

std::string JSShimRuntime::evaluateTelemetryToJSON(const GameData::TelemetryData& telemetry) {
    // Simulating JavaScript VM serialization of native C++ telemetry data
    std::ostringstream ss;
    ss << "{"
       << "\"health\":" << telemetry.health << ","
       << "\"maxHealth\":" << telemetry.maxHealth << ","
       << "\"mana\":" << telemetry.mana << ","
       << "\"maxMana\":" << telemetry.maxMana << ","
       << "\"posX\":" << std::fixed << std::setprecision(2) << telemetry.posX << ","
       << "\"posY\":" << telemetry.posY << ","
       << "\"posZ\":" << telemetry.posZ << ","
       << "\"zoneName\":\"" << telemetry.zoneName << "\","
       << "\"targetName\":\"" << telemetry.targetName << "\","
       << "\"level\":" << telemetry.level << ","
       << "\"xp\":" << telemetry.xp
       << "}";
    return ss.str();
}

WebUIRenderingEngine::WebUIRenderingEngine(uint32_t width, uint32_t height)
    : m_width(width), m_height(height) {}

void WebUIRenderingEngine::loadHTML(const std::string& htmlContent) {
    m_html = htmlContent;
}

void WebUIRenderingEngine::updateTelemetryState(const std::string& jsonData) {
    m_currentJsonState = jsonData;
}

void WebUIRenderingEngine::render(WebTexture* outputTexture) {
    outputTexture->width = m_width;
    outputTexture->height = m_height;
    if (outputTexture->rgbaPixels.size() != m_width * m_height * 4) {
        outputTexture->rgbaPixels.resize(m_width * m_height * 4, 0);
    }

    // Simulate HTML/CSS layout computation and canvas rasterization:
    // We will clear the frame and write colored telemetry display patterns.
    for (uint32_t y = 0; y < m_height; ++y) {
        for (uint32_t x = 0; x < m_width; ++x) {
            uint32_t offset = (y * m_width + x) * 4;

            // Translucent dark background for standard layout
            outputTexture->rgbaPixels[offset + 0] = 10;  // R
            outputTexture->rgbaPixels[offset + 1] = 10;  // G
            outputTexture->rgbaPixels[offset + 2] = 20;  // B
            outputTexture->rgbaPixels[offset + 3] = 120; // A (Transparent Web Canvas Overlay)
        }
    }

    // Rasterize horizontal progress bar (simulating "div health-bar") at the bottom of the screen
    // Let's parse out health from JSON state if present (for simulation)
    uint32_t healthPct = 80;
    if (!m_currentJsonState.empty()) {
        size_t pos = m_currentJsonState.find("\"health\":");
        if (pos != std::string::npos) {
            healthPct = std::stoi(m_currentJsonState.substr(pos + 9, 3));
        }
    }

    uint32_t barWidth = (m_width * healthPct) / 100;
    uint32_t barYStart = m_height - 60;
    uint32_t barYEnd = m_height - 40;

    for (uint32_t y = barYStart; y < barYEnd; ++y) {
        for (uint32_t x = 50; x < m_width - 50; ++x) {
            uint32_t offset = (y * m_width + x) * 4;
            if (x < 50 + barWidth && x < m_width - 50) {
                // Bright green healthy bar
                outputTexture->rgbaPixels[offset + 0] = 0;
                outputTexture->rgbaPixels[offset + 1] = 220;
                outputTexture->rgbaPixels[offset + 2] = 80;
                outputTexture->rgbaPixels[offset + 3] = 255;
            } else {
                // Dark background slot
                outputTexture->rgbaPixels[offset + 0] = 50;
                outputTexture->rgbaPixels[offset + 1] = 10;
                outputTexture->rgbaPixels[offset + 2] = 10;
                outputTexture->rgbaPixels[offset + 3] = 200;
            }
        }
    }
}

void jsHtmlEngineThreadLoop(
    std::atomic<bool>& running,
    PointerSwapBuffer<GameData::TelemetryData>& gameDataBuffer,
    PointerSwapBuffer<WebTexture>& uiTextureBuffer) {

    JSShimRuntime jsVm;
    WebUIRenderingEngine engine(1024, 768);
    engine.loadHTML("<html><body><div id='hud'>...</div></body></html>");

    while (running) {
        // Step 1: Query the latest Game Data available
        bool updated = gameDataBuffer.swapConsumer();
        if (updated || true) { // Render periodically or upon update
            const GameData::TelemetryData* telemetry = gameDataBuffer.getReadBuffer();

            // Step 2: Feed telemetry to virtual JS Engine to parse JSON context
            std::string telemetryJson = jsVm.evaluateTelemetryToJSON(*telemetry);

            // Step 3: Run Ultralight layout update and canvas element modifications
            engine.updateTelemetryState(telemetryJson);

            // Step 4: Rasterize HTML state into our WebTexture output
            WebTexture* writeTex = uiTextureBuffer.getWriteBuffer();
            engine.render(writeTex);

            // Step 5: Swap updated WebTexture for Vulkan context presentation
            uiTextureBuffer.swapProducer();
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS HTML Layout Engine Tick
    }
}

} // namespace UltralightRenderer
