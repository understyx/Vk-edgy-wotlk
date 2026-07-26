#include "vklayer/discord_presence.h"
#include "vklayer/vk_layer.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <vector>
#include <chrono>
#include <cstring>
#include <unistd.h>
#include <dlfcn.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>

using json = nlohmann::json;

namespace WoTLKGuiLayer {

static std::string GetLayerDir()
{
    Dl_info info;
    if (dladdr(reinterpret_cast<void*>(&GetLayerDir), &info) && info.dli_fname) {
        std::string path = info.dli_fname;
        auto pos = path.rfind('/');
        if (pos != std::string::npos)
            return path.substr(0, pos);
    }
    return ".";
}

static bool WriteAll(int fd, const void* buf, size_t size) {
    const char* p = reinterpret_cast<const char*>(buf);
    size_t remaining = size;
    while (remaining > 0) {
        ssize_t n = write(fd, p, remaining);
        if (n <= 0) return false;
        p += n;
        remaining -= n;
    }
    return true;
}

DiscordPresenceManager::DiscordPresenceManager() {
    m_clientId = "1123581321345589144";
    m_detailsTempl = "{playerName} - Lvl {playerLevel}";
    m_stateTempl = "{zoneText}";
    m_largeImage = "wotlk_logo";
    m_largeTextTempl = "WoTLK GUI Vulkan Layer";
    m_smallImage = "horde";
    m_smallTextTempl = "HP: {playerHealth} / {playerMaxHealth}";
    m_updateIntervalSec = 5;
}

DiscordPresenceManager::~DiscordPresenceManager() {
    Stop();
}

void DiscordPresenceManager::LoadConfig() {
    std::string configPath = "discord_presence.json";
    if (access(configPath.c_str(), F_OK) != 0) {
        std::string layerPath = GetLayerDir() + "/discord_presence.json";
        if (access(layerPath.c_str(), F_OK) == 0) {
            configPath = layerPath;
        } else {
            WriteDefaultConfig(configPath);
        }
    }

    std::ifstream f(configPath);
    if (f.is_open()) {
        json data = json::parse(f, nullptr, false);
        if (!data.is_discarded()) {
            m_clientId = data.value("client_id", m_clientId);
            m_detailsTempl = data.value("details", m_detailsTempl);
            m_stateTempl = data.value("state", m_stateTempl);
            m_largeImage = data.value("large_image", m_largeImage);
            m_largeTextTempl = data.value("large_text", m_largeTextTempl);
            m_smallImage = data.value("small_image", m_smallImage);
            m_smallTextTempl = data.value("small_text", m_smallTextTempl);
            m_updateIntervalSec = data.value("update_interval_seconds", m_updateIntervalSec);
            fprintf(stdout, "[WoTLKLayer] Loaded discord_presence.json configuration successfully.\n");
        } else {
            fprintf(stderr, "[WoTLKLayer] Failed to parse discord_presence.json. Using defaults.\n");
        }
    }
}

void DiscordPresenceManager::WriteDefaultConfig(const std::string& path) {
    json data = {
        {"client_id", "1123581321345589144"},
        {"details", "{playerName} - Lvl {playerLevel}"},
        {"state", "{zoneText} ({subZoneText})"},
        {"large_image", "wotlk_logo"},
        {"large_text", "WoTLK GUI Vulkan Layer"},
        {"small_image", "horde"},
        {"small_text", "HP: {playerHealth}/{playerMaxHealth}"},
        {"update_interval_seconds", 5}
    };
    std::ofstream f(path);
    if (f.is_open()) {
        f << data.dump(4) << std::endl;
    }
}

bool DiscordPresenceManager::ConnectSocket() {
    if (m_fd != -1) {
        close(m_fd);
        m_fd = -1;
    }

    const char* xdg = getenv("XDG_RUNTIME_DIR");
    std::vector<std::string> paths;
    if (xdg) {
        for (int i = 0; i < 10; ++i) {
            paths.push_back(std::string(xdg) + "/discord-ipc-" + std::to_string(i));
        }
    }
    for (int i = 0; i < 10; ++i) {
        paths.push_back("/tmp/discord-ipc-" + std::to_string(i));
    }
    if (xdg) {
        for (int i = 0; i < 10; ++i) {
            paths.push_back(std::string(xdg) + "/app/com.discordapp.Discord/discord-ipc-" + std::to_string(i));
        }
    }

    for (const auto& path : paths) {
        int fd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (fd < 0) continue;

        struct timeval tv;
        tv.tv_sec = 2;
        tv.tv_usec = 0;
        setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
        setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof(tv));

        struct sockaddr_un addr{};
        addr.sun_family = AF_UNIX;
        std::strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path) - 1);

        if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
            m_fd = fd;
            return true;
        }
        close(fd);
    }
    return false;
}

void DiscordPresenceManager::SendHandshake() {
    if (m_fd == -1) return;

    json payload = {
        {"v", 1},
        {"client_id", m_clientId}
    };
    std::string s = payload.dump();
    uint32_t opcode = 0;
    uint32_t length = static_cast<uint32_t>(s.size());

    std::vector<uint8_t> header(8);
    std::memcpy(header.data(), &opcode, 4);
    std::memcpy(header.data() + 4, &length, 4);

    if (WriteAll(m_fd, header.data(), 8)) {
        WriteAll(m_fd, s.data(), length);
    }

    char discardBuf[1024];
    (void)read(m_fd, discardBuf, sizeof(discardBuf));
}

void DiscordPresenceManager::SendPresenceUpdate() {
    if (m_fd == -1) return;

    WoWMemory::GameData snapshot;
    if (!m_gameReader.ReadGameData(snapshot)) {
        return;
    }

    json activity = json::object();

    std::string details = FormatTemplate(m_detailsTempl, snapshot);
    if (!details.empty()) {
        activity["details"] = details;
    }

    std::string state = FormatTemplate(m_stateTempl, snapshot);
    if (!state.empty()) {
        activity["state"] = state;
    }

    json assets = json::object();
    if (!m_largeImage.empty()) {
        assets["large_image"] = m_largeImage;
        std::string largeText = FormatTemplate(m_largeTextTempl, snapshot);
        if (!largeText.empty()) {
            assets["large_text"] = largeText;
        }
    }
    if (!m_smallImage.empty()) {
        assets["small_image"] = m_smallImage;
        std::string smallText = FormatTemplate(m_smallTextTempl, snapshot);
        if (!smallText.empty()) {
            assets["small_text"] = smallText;
        }
    }
    if (!assets.empty()) {
        activity["assets"] = assets;
    }

    json timestamps = json::object();
    timestamps["start"] = m_startTime;
    activity["timestamps"] = timestamps;

    json payload = {
        {"cmd", "SET_ACTIVITY"},
        {"args", {
            {"pid", getpid()},
            {"activity", activity}
        }},
        {"nonce", "understyx-presence-nonce"}
    };

    std::string s = payload.dump();
    uint32_t opcode = 1;
    uint32_t length = static_cast<uint32_t>(s.size());

    std::vector<uint8_t> header(8);
    std::memcpy(header.data(), &opcode, 4);
    std::memcpy(header.data() + 4, &length, 4);

    if (WriteAll(m_fd, header.data(), 8)) {
        WriteAll(m_fd, s.data(), length);
    }

    char discardBuf[1024];
    (void)read(m_fd, discardBuf, sizeof(discardBuf));
}

std::string DiscordPresenceManager::FormatTemplate(const std::string& templ, const WoWMemory::GameData& snapshot) {
    std::string result = templ;

    auto replaceAll = [&](const std::string& placeholder, const std::string& value) {
        size_t pos = 0;
        while ((pos = result.find(placeholder, pos)) != std::string::npos) {
            result.replace(pos, placeholder.length(), value);
            pos += value.length();
        }
    };

    replaceAll("{playerName}", snapshot.playerName.empty() ? "Character" : snapshot.playerName);
    replaceAll("{realmName}", snapshot.realmName.empty() ? "Realm" : snapshot.realmName);
    replaceAll("{zoneText}", snapshot.zoneText.empty() ? "Unknown Zone" : snapshot.zoneText);
    replaceAll("{subZoneText}", snapshot.subZoneText.empty() ? "" : snapshot.subZoneText);
    replaceAll("{continentName}", snapshot.continentName.empty() ? "" : snapshot.continentName);
    replaceAll("{playerHealth}", std::to_string(snapshot.playerHealth));
    replaceAll("{playerMaxHealth}", std::to_string(snapshot.playerMaxHealth));
    replaceAll("{playerLevel}", std::to_string(snapshot.playerLevel));
    replaceAll("{playerPower}", std::to_string(snapshot.playerPower));
    replaceAll("{playerMaxPower}", std::to_string(snapshot.playerMaxPower));
    replaceAll("{activeQuestsCount}", std::to_string(snapshot.activeQuestsCount));
    replaceAll("{numPartyMembers}", std::to_string(snapshot.numPartyMembers));

    return result;
}

void DiscordPresenceManager::ThreadLoop() {
    m_startTime = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    while (m_running) {
        if (m_fd == -1) {
            if (ConnectSocket()) {
                fprintf(stdout, "[WoTLKLayer] Connected to Discord UNIX socket successfully.\n");
                SendHandshake();
            } else {
                for (int i = 0; i < 5 && m_running; ++i) {
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                }
                continue;
            }
        }

        SendPresenceUpdate();

        for (int i = 0; i < m_updateIntervalSec && m_running; ++i) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    if (m_fd != -1) {
        close(m_fd);
        m_fd = -1;
    }
}

void DiscordPresenceManager::Start() {
    if (!HasLaunchOption("--discord-presence")) {
        return;
    }

    if (m_running) return;

    LoadConfig();

    m_running = true;
    m_thread = std::thread(&DiscordPresenceManager::ThreadLoop, this);
    fprintf(stdout, "[WoTLKLayer] Discord Presence Manager thread started successfully.\n");
}

void DiscordPresenceManager::Stop() {
    if (!m_running) return;
    m_running = false;
    if (m_thread.joinable()) {
        m_thread.join();
    }
    fprintf(stdout, "[WoTLKLayer] Discord Presence Manager thread stopped.\n");
}

DiscordPresenceManager gDiscordPresence;

} // namespace WoTLKGuiLayer
