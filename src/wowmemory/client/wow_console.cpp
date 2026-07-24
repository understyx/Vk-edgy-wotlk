#include "wow_console.h"
#include "wowmemory/offsets.h"
#include "wowmemory/memory_utils.h"

#ifdef _WIN32
#include <windows.h>
#endif
#include <cstring>

namespace WoWMemory {

WoWConsole::WoWConsole() {
    Toggle(true);
}

WoWConsole::~WoWConsole() {
#ifdef _WIN32
    for (auto& pair : m_commandAllocations) {
        free(pair.second.first);
        free(pair.second.second);
    }
#endif
    m_commandAllocations.clear();
}

void WoWConsole::Toggle(bool enable) {
    (void)enable;
#ifdef _WIN32
    if (IsReadableRange(WoWOffsets::Console::Enable, sizeof(uint32_t))) {
        *reinterpret_cast<uint32_t*>(WoWOffsets::Console::Enable) = (enable ? 1 : 0);
    }
#endif
}

void WoWConsole::SetConsoleKey(const std::string& key) {
    (void)key;
    // Lua-free implementation of console key is ignored because we are not using WoW's Lua execution context.
}

void WoWConsole::Write(const std::string& text, WoWConsoleColor color) {
    (void)text;
    (void)color;
#ifdef _WIN32
    typedef void (__cdecl *ConsoleWriteA_t)(const char*, WoWConsoleColor, const char*);
    auto fn = reinterpret_cast<ConsoleWriteA_t>(WoWOffsets::Console::WriteA);
    fn(text.c_str(), color, nullptr);
#endif
}

bool WoWConsole::RegisterCommand(const std::string& command, CommandHandler_t handler, CommandCategory category, const std::string& help) {
    (void)command;
    (void)handler;
    (void)category;
    (void)help;
#ifdef _WIN32
    if (m_commandAllocations.find(command) != m_commandAllocations.end()) {
        return false;
    }

    typedef bool (__cdecl *ConsoleRegisterCommand_t)(const char*, void*, CommandCategory, const char*);
    auto fn = reinterpret_cast<ConsoleRegisterCommand_t>(WoWOffsets::Console::RegisterCommand);

    char* cmdPtr = _strdup(command.c_str());
    char* helpPtr = _strdup(help.c_str());

    m_commandAllocations[command] = std::make_pair(cmdPtr, helpPtr);

    return fn(cmdPtr, reinterpret_cast<void*>(handler), category, helpPtr);
#else
    return true;
#endif
}

void WoWConsole::UnregisterCommand(const std::string& command) {
    (void)command;
#ifdef _WIN32
    auto it = m_commandAllocations.find(command);
    if (it == m_commandAllocations.end()) {
        return;
    }

    typedef void (__cdecl *ConsoleUnregisterCommand_t)(const char*);
    auto fn = reinterpret_cast<ConsoleUnregisterCommand_t>(WoWOffsets::Console::UnregisterCommand);

    fn(static_cast<const char*>(it->second.first));

    free(it->second.first);
    free(it->second.second);
    m_commandAllocations.erase(it);
#endif
}

} // namespace WoWMemory
