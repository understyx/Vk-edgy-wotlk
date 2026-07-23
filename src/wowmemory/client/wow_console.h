#ifndef WOWMEMORY_CLIENT_WOW_CONSOLE_H
#define WOWMEMORY_CLIENT_WOW_CONSOLE_H

#include <string>
#include <map>
#include <utility>

#ifndef _WIN32
#ifndef __cdecl
#define __cdecl
#endif
#endif

namespace WoWMemory {

enum class WoWConsoleColor : int {
    Default = 0x0,
    Input = 0x1,
    Echo = 0x2,
    Error = 0x3,
    Warning = 0x4,
    Global = 0x5,
    Admin = 0x6,
    Highlight = 0x7,
    Background = 0x8,
};

enum class CommandCategory : int {
    Debug = 0x0,
    Graphics = 0x1,
    Console = 0x2,
    Combat = 0x3,
    Game = 0x4,
    Default = 0x5,
    Net = 0x6,
    Sound = 0x7,
    GM = 0x8,
};

typedef int (__cdecl *CommandHandler_t)(const char* cmd, const char* args);

class WoWConsole {
public:
    WoWConsole();
    ~WoWConsole();

    void Toggle(bool enable);
    void SetConsoleKey(const std::string& key);
    void Write(const std::string& text, WoWConsoleColor color);

    bool RegisterCommand(const std::string& command, CommandHandler_t handler, CommandCategory category, const std::string& help);
    void UnregisterCommand(const std::string& command);

private:
    std::map<std::string, std::pair<void*, void*>> m_commandAllocations;
};

} // namespace WoWMemory

#endif // WOWMEMORY_CLIENT_WOW_CONSOLE_H
