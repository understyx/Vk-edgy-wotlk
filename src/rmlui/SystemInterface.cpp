#include "SystemInterface.h"
#include <chrono>
#include <cstdio>

namespace WoTLKGuiLayer {

static const auto g_startTime = std::chrono::steady_clock::now();

double RmlSystemInterface::GetElapsedTime()
{
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration<double>(now - g_startTime).count();
}

void RmlSystemInterface::SetMouseCursor(const Rml::String& /*cursor_name*/) {}

void RmlSystemInterface::SetClipboardText(const Rml::String& /*text*/) {}

void RmlSystemInterface::GetClipboardText(Rml::String& text)
{
    text.clear();
}

bool RmlSystemInterface::LogMessage(Rml::Log::Type type, const Rml::String& message)
{
    const char* prefix = "[RmlUi] ";
    switch (type) {
    case Rml::Log::LT_ERROR:
    case Rml::Log::LT_ASSERT:
        fprintf(stderr, "%sERROR: %s\n", prefix, message.c_str());
        break;
    case Rml::Log::LT_WARNING:
        fprintf(stderr, "%sWARN: %s\n", prefix, message.c_str());
        break;
    default:
        fprintf(stdout, "%s%s\n", prefix, message.c_str());
        break;
    }
    return true;
}

} // namespace WoTLKGuiLayer
