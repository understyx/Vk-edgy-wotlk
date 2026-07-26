#ifndef DISCORD_PRESENCE_H
#define DISCORD_PRESENCE_H

#include <string>
#include <thread>
#include <atomic>
#include "wowmemory/wowmemory.h"

namespace WoTLKGuiLayer {

class DiscordPresenceManager {
public:
    DiscordPresenceManager();
    ~DiscordPresenceManager();

    // Starts the background update thread if `--discord-presence` is active
    void Start();

    // Stop the thread
    void Stop();

private:
    void ThreadLoop();
    void LoadConfig();
    void WriteDefaultConfig(const std::string& path);
    bool ConnectSocket();
    void SendHandshake();
    void SendPresenceUpdate();
    std::string FormatTemplate(const std::string& templ, const WoWMemory::GameData& snapshot);

    std::thread m_thread;
    std::atomic<bool> m_running{false};
    int m_fd = -1;

    // Config options loaded from JSON
    std::string m_clientId;
    std::string m_detailsTempl;
    std::string m_stateTempl;
    std::string m_largeImage;
    std::string m_largeTextTempl;
    std::string m_smallImage;
    std::string m_smallTextTempl;
    int m_updateIntervalSec = 5;

    WoWMemory::GameDataReader m_gameReader;
    int64_t m_startTime = 0;
};

// Global instance of the Discord presence manager
extern DiscordPresenceManager gDiscordPresence;

} // namespace WoTLKGuiLayer

#endif // DISCORD_PRESENCE_H
