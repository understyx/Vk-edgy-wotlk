#pragma once

#include <cstdint>
#include <RmlUi/Core/SystemInterface.h>

namespace WoTLKGuiLayer {

/**
 * @class RmlSystemInterface
 * @brief Provides time and logging services to RmlUi.
 */
class RmlSystemInterface : public Rml::SystemInterface {
public:
    double GetElapsedTime() override;
    void SetMouseCursor(const Rml::String& cursor_name) override;
    void SetClipboardText(const Rml::String& text) override;
    void GetClipboardText(Rml::String& text) override;
    bool LogMessage(Rml::Log::Type type, const Rml::String& message) override;
};

} // namespace WoTLKGuiLayer
