/**
 * @file wowmemory.cpp
 * @brief WoW process memory reading implementation.
 *
 * This Vulkan layer is loaded into the WoW process, so all absolute addresses
 * from offsets.h can be accessed directly as pointers.  Every dereference is
 * guarded against null/zero base pointers to avoid access violations when the
 * game is in a state where a particular value is not yet initialised.
 */

#include "wowmemory/wowmemory.h"
#include "wowmemory/offsets.h"
#include "wowmemory/memory_utils.h"
#include "wowmemory/auras.h"
#include "wowmemory/unit_info.h"
#include "wowmemory/combat_log.h"
#include "wowmemory/client/wow_party.h"
#include "wowmemory/client/wow_raid.h"
#include "wowmemory/client/wow_quest.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#ifndef _WIN32
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#endif

namespace WoWMemory {

// ---------------------------------------------------------------------------
// GameDataReader — private static helpers
// ---------------------------------------------------------------------------

std::string GameDataReader::ReadInlineString(uint32_t absAddr, size_t maxLen)
{
    return WoWMemory::ReadInlineString(absAddr, maxLen);
}

std::string GameDataReader::ReadIndirectString(uint32_t ptrAddr, size_t maxLen)
{
    return WoWMemory::ReadIndirectString(ptrAddr, maxLen);
}

// ---------------------------------------------------------------------------
#ifndef _WIN32
GameDataReader::GameDataReader()
{
    m_stopServer = false;
    m_serverThread = std::thread(&GameDataReader::ServerLoop, this);
}

GameDataReader::~GameDataReader()
{
    m_stopServer = true;
    if (m_serverFd != -1) {
        close(m_serverFd);
    }
    if (m_clientFd != -1) {
        close(m_clientFd);
    }
    if (m_serverThread.joinable()) {
        m_serverThread.join();
    }
}

static bool ReadExactly(int fd, void* buf, size_t size)
{
    char* p = reinterpret_cast<char*>(buf);
    size_t remaining = size;
    while (remaining > 0) {
        ssize_t n = recv(fd, p, remaining, 0);
        if (n <= 0) {
            return false;
        }
        p += n;
        remaining -= n;
    }
    return true;
}

void GameDataReader::ServerLoop()
{
    fprintf(stderr, "[WoW IPC Server] Initializing TCP server socket...\n");
    m_serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (m_serverFd == -1) {
        fprintf(stderr, "[WoW IPC Server] Failed to create TCP socket!\n");
        return;
    }

    int opt = 1;
    setsockopt(m_serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = htons(50055);

    if (bind(m_serverFd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        fprintf(stderr, "[WoW IPC Server] Failed to bind TCP socket to port 50055!\n");
        close(m_serverFd);
        m_serverFd = -1;
        return;
    }

    if (listen(m_serverFd, 3) < 0) {
        fprintf(stderr, "[WoW IPC Server] Failed to listen on TCP socket!\n");
        close(m_serverFd);
        m_serverFd = -1;
        return;
    }

    fprintf(stderr, "[WoW IPC Server] Server listening on 127.0.0.1:50055 successfully.\n");

    std::vector<uint8_t> recvBuf;
    uint32_t totalPacketsReceived = 0;

    while (!m_stopServer) {
        int addrlen = sizeof(address);
        m_clientFd = accept(m_serverFd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
        if (m_clientFd < 0) {
            if (m_stopServer) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        fprintf(stderr, "[WoW IPC Server] Accepted connection from WoW 32-bit DLL client!\n");

        while (!m_stopServer) {
            uint32_t payloadLen = 0;
            if (!ReadExactly(m_clientFd, &payloadLen, sizeof(payloadLen))) {
                fprintf(stderr, "[WoW IPC Server] Failed to read framing length. Connection closed by client.\n");
                break;
            }

            if (payloadLen > 1024 * 1024) { // safety limit 1MB
                fprintf(stderr, "[WoW IPC Server] Received invalid payload length: %u bytes! Aborting connection.\n", payloadLen);
                break;
            }

            recvBuf.resize(payloadLen);
            if (payloadLen > 0) {
                if (!ReadExactly(m_clientFd, recvBuf.data(), payloadLen)) {
                    fprintf(stderr, "[WoW IPC Server] Failed to read complete payload of size %u. Connection aborted.\n", payloadLen);
                    break;
                }
            }

            GameData data;
            if (DeserializeGameData(recvBuf, data)) {
                {
                    std::lock_guard<std::mutex> lock(m_cacheMutex);
                    m_cachedData = data;
                }
                totalPacketsReceived++;
                if (totalPacketsReceived % 200 == 1) {
                    fprintf(stderr, "[WoW IPC Server] Active receiving: packet #%u successfully received and deserialized (player: '%s', zone: '%s').\n",
                            totalPacketsReceived, data.playerName.c_str(), data.zoneText.c_str());
                }
            } else {
                fprintf(stderr, "[WoW IPC Server] Failed to deserialize GameData packet!\n");
            }
        }

        fprintf(stderr, "[WoW IPC Server] Closing client socket...\n");
        close(m_clientFd);
        m_clientFd = -1;
    }

    fprintf(stderr, "[WoW IPC Server] Server thread terminating...\n");
    close(m_serverFd);
    m_serverFd = -1;
}

bool GameDataReader::ReadGameData(GameData& out)
{
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    out = m_cachedData;
    return true;
}
#else
// GameDataReader::ReadGameData (Win32 implementation for DLL)
// ---------------------------------------------------------------------------

bool GameDataReader::ReadGameData(GameData& out)
{
    // ---- Identity ----
    out.playerName      = ReadInlineString(WoWOffsets::playerName, 12);
    out.realmName       = ReadInlineString(WoWOffsets::realmName, 64);
    out.localPlayerGUID = ReadAbs<uint64_t>(WoWOffsets::localPlayerGUID);

    // ---- World location ----
    // continentName, zoneText and subZoneText are char* pointers stored at
    // those addresses (the pointer holds the address of the string buffer).
    out.continentName = ReadIndirectString(WoWOffsets::continentName);
    out.zoneText      = ReadIndirectString(WoWOffsets::zoneText);
    out.subZoneText   = ReadIndirectString(WoWOffsets::subZoneText);
    out.mapID         = ReadAbs<uint32_t>(WoWOffsets::mapID);
    out.zoneID        = ReadAbs<uint32_t>(WoWOffsets::zoneID);

    // ---- Game state ----
    out.gameState    = ReadAbs<uint32_t>(WoWOffsets::gameState);
    out.worldLoaded  = ReadAbs<uint8_t>(WoWOffsets::worldLoaded) != 0;
    out.isLoading    = ReadAbs<uint8_t>(WoWOffsets::isLoading)   != 0;
    out.isIndoor     = ReadAbs<uint8_t>(WoWOffsets::isIndoor)    != 0;
    out.tickCount    = ReadAbs<uint32_t>(WoWOffsets::tickCount);

    // ---- Player state ----
    out.playerIsIngame    = ReadAbs<uint8_t>(WoWOffsets::playerIsIngame) != 0;
    out.playerComboPoints = ReadAbs<uint8_t>(WoWOffsets::comboPoints);

    // ---- GUIDs ----
    out.mouseOverGUID  = ReadAbs<uint64_t>(WoWOffsets::mouseOverGUID);
    out.lastTargetGUID = ReadAbs<uint64_t>(WoWOffsets::lastTargetGUID);

    // ---- Corpse ----
    out.corpseX = ReadAbs<float>(WoWOffsets::corpseX);
    out.corpseY = ReadAbs<float>(WoWOffsets::corpseY);
    out.corpseZ = ReadAbs<float>(WoWOffsets::corpseZ);

    // ---- Player object — position, unit fields, casting, auras ----
    // playerBase is the local player object pointer.
    // We try to retrieve it first via Object Manager from localPlayerGUID,
    // falling back to absolute playerBase pointer if not found.
    uintptr_t playerBasePtr = 0;
    if (out.localPlayerGUID != 0) {
        playerBasePtr = GetObjectBaseByGUID(out.localPlayerGUID);
    }
    if (!playerBasePtr) {
        playerBasePtr = ReadAbs<uint32_t>(WoWOffsets::playerBase);
    }

    if (playerBasePtr && IsReadableRange(playerBasePtr, 4)) {
        // --- Position ---
        ReadPlayerPosition(playerBasePtr, out);

        // --- Health (legacy path kept, now also via unit fields) ---
        if (IsReadableRange(playerBasePtr + WoWOffsets::playerHealth, sizeof(uint32_t))) {
            out.playerHealth = *reinterpret_cast<const uint32_t*>(
                playerBasePtr + WoWOffsets::playerHealth);
        } else {
            out.playerHealth = 0;
        }

        // --- Unit fields (descriptor array) ---
        ReadPlayerUnitFields(playerBasePtr, out);

        // --- Casting / channeling ---
        auto readRelU32 = [&](uint32_t relOffset) -> uint32_t {
            uintptr_t addr = playerBasePtr + relOffset;
            if (!IsReadableRange(addr, sizeof(uint32_t))) return 0u;
            return *reinterpret_cast<const uint32_t*>(addr);
        };
        out.castingSpellId = readRelU32(WoWOffsets::unitCastingIdOffset);
        out.channelSpellId = readRelU32(WoWOffsets::unitChannelIdOffset);

        // --- Auras ---
        out.auraCount = ReadAurasForUnit(playerBasePtr, out.auras, GameData::kMaxAuras);
    } else {
        out.playerHealth    = 0;
        out.playerMaxHealth = 0;
        out.playerLevel     = 0;
        out.playerPower     = 0;
        out.playerMaxPower  = 0;
        out.castingSpellId  = 0;
        out.channelSpellId  = 0;
        out.auraCount       = 0;
    }

    // ---- Target State & Auras ----
    out.targetAuraCount = 0;
    out.targetHealth    = 0;
    out.targetMaxHealth = 0;
    out.targetLevel     = 0;
    out.targetPower     = 0;
    out.targetMaxPower  = 0;
    out.targetPowerType = 0;
    out.targetName      = "";

    if (out.targetGUID != 0) {
        uintptr_t targetBasePtr = GetObjectBaseByGUID(out.targetGUID);
        if (targetBasePtr) {
            ReadTargetUnitFields(targetBasePtr, out);
            out.targetAuraCount = ReadAurasForUnit(targetBasePtr, out.targetAuras, GameData::kMaxTargetAuras);
            out.targetName = GetUnitName(out.targetGUID, targetBasePtr);
        }
    }

    // ---- Camera ----
    ReadCameraInfo(out);

    // ---- Combat log (incremental read of new events since last call) ----
    ReadCombatLogEvents(m_lastCombatLogNodeAddr, out);

    // ---- Party, Raid & Quest metrics via Client API ----
    out.numPartyMembers   = WoWParty::NumPartyMembers();
    out.partyDifficulty   = WoWParty::Difficulty();
    out.numRaidMembers    = WoWRaid::NumRaidMembers();
    out.raidDifficulty    = WoWRaid::Difficulty();
    out.activeQuestsCount = static_cast<uint32_t>(WoWQuest::GetActiveQuests().size());

    return true;
}
#endif

// ---------------------------------------------------------------------------
// Serialization / Deserialization Implementation
// ---------------------------------------------------------------------------

static void WriteString(std::vector<uint8_t>& buf, const std::string& s)
{
    uint32_t len = static_cast<uint32_t>(s.size());
    const uint8_t* pLen = reinterpret_cast<const uint8_t*>(&len);
    buf.insert(buf.end(), pLen, pLen + sizeof(len));
    if (len > 0) {
        const uint8_t* pStr = reinterpret_cast<const uint8_t*>(s.data());
        buf.insert(buf.end(), pStr, pStr + len);
    }
}

template<typename T>
static void WriteVal(std::vector<uint8_t>& buf, const T& val)
{
    const uint8_t* p = reinterpret_cast<const uint8_t*>(&val);
    buf.insert(buf.end(), p, p + sizeof(T));
}

static std::string ReadString(const uint8_t*& p, const uint8_t* end)
{
    if (p + sizeof(uint32_t) > end) return "";
    uint32_t len = *reinterpret_cast<const uint32_t*>(p);
    p += sizeof(uint32_t);
    if (p + len > end) return "";
    std::string s(reinterpret_cast<const char*>(p), len);
    p += len;
    return s;
}

template<typename T>
static T ReadVal(const uint8_t*& p, const uint8_t* end)
{
    if (p + sizeof(T) > end) return T{};
    T val = *reinterpret_cast<const T*>(p);
    p += sizeof(T);
    return val;
}

void SerializeGameData(const GameData& data, std::vector<uint8_t>& outBuf)
{
    outBuf.clear();
    // Pre-reserve to avoid frequent reallocations (approximate size)
    outBuf.reserve(sizeof(GameData) + 256);

    // 1. Strings
    WriteString(outBuf, data.playerName);
    WriteString(outBuf, data.realmName);
    WriteString(outBuf, data.continentName);
    WriteString(outBuf, data.zoneText);
    WriteString(outBuf, data.subZoneText);
    WriteString(outBuf, data.targetName);

    // 2. Integers and standard scalars
    WriteVal(outBuf, data.localPlayerGUID);
    WriteVal(outBuf, data.mapID);
    WriteVal(outBuf, data.zoneID);
    WriteVal(outBuf, data.playerHealth);
    WriteVal(outBuf, data.playerMaxHealth);
    WriteVal(outBuf, data.playerLevel);
    WriteVal(outBuf, data.playerPower);
    WriteVal(outBuf, data.playerMaxPower);
    WriteVal(outBuf, data.playerPowerType);
    WriteVal(outBuf, data.playerComboPoints);
    WriteVal(outBuf, data.playerIsIngame);

    WriteVal(outBuf, data.targetHealth);
    WriteVal(outBuf, data.targetMaxHealth);
    WriteVal(outBuf, data.targetPower);
    WriteVal(outBuf, data.targetMaxPower);
    WriteVal(outBuf, data.targetPowerType);
    WriteVal(outBuf, data.targetLevel);
    WriteVal(outBuf, data.isLoading);
    WriteVal(outBuf, data.worldLoaded);
    WriteVal(outBuf, data.isIndoor);

    WriteVal(outBuf, data.playerPosX);
    WriteVal(outBuf, data.playerPosY);
    WriteVal(outBuf, data.playerPosZ);
    WriteVal(outBuf, data.playerRotation);

    WriteVal(outBuf, data.gameState);
    WriteVal(outBuf, data.tickCount);

    WriteVal(outBuf, data.corpseX);
    WriteVal(outBuf, data.corpseY);
    WriteVal(outBuf, data.corpseZ);

    WriteVal(outBuf, data.targetGUID);
    WriteVal(outBuf, data.mouseOverGUID);
    WriteVal(outBuf, data.lastTargetGUID);

    WriteVal(outBuf, data.castingSpellId);
    WriteVal(outBuf, data.channelSpellId);

    // 3. Auras
    WriteVal(outBuf, data.auraCount);
    for (uint32_t i = 0; i < data.auraCount && i < GameData::kMaxAuras; ++i) {
        WriteVal(outBuf, data.auras[i].spellId);
    }

    WriteVal(outBuf, data.targetAuraCount);
    for (uint32_t i = 0; i < data.targetAuraCount && i < GameData::kMaxTargetAuras; ++i) {
        WriteVal(outBuf, data.targetAuras[i].spellId);
    }

    // 4. Combat Log Events
    WriteVal(outBuf, data.combatLogEventCount);
    for (uint32_t i = 0; i < data.combatLogEventCount && i < GameData::kMaxCombatLogEvents; ++i) {
        WriteVal(outBuf, data.combatLogEvents[i]);
    }

    // 5. Camera
    WriteVal(outBuf, data.cameraYaw);
    WriteVal(outBuf, data.cameraPitch);

    // 6. Party / Raid / Quest metrics
    WriteVal(outBuf, data.numPartyMembers);
    WriteVal(outBuf, data.partyDifficulty);
    WriteVal(outBuf, data.numRaidMembers);
    WriteVal(outBuf, data.raidDifficulty);
    WriteVal(outBuf, data.activeQuestsCount);
}

bool DeserializeGameData(const std::vector<uint8_t>& buf, GameData& outData)
{
    if (buf.empty()) return false;
    const uint8_t* p = buf.data();
    const uint8_t* end = p + buf.size();

    // 1. Strings
    outData.playerName = ReadString(p, end);
    outData.realmName = ReadString(p, end);
    outData.continentName = ReadString(p, end);
    outData.zoneText = ReadString(p, end);
    outData.subZoneText = ReadString(p, end);
    outData.targetName = ReadString(p, end);

    // 2. Integers and standard scalars
    outData.localPlayerGUID = ReadVal<uint64_t>(p, end);
    outData.mapID = ReadVal<uint32_t>(p, end);
    outData.zoneID = ReadVal<uint32_t>(p, end);
    outData.playerHealth = ReadVal<uint32_t>(p, end);
    outData.playerMaxHealth = ReadVal<uint32_t>(p, end);
    outData.playerLevel = ReadVal<uint32_t>(p, end);
    outData.playerPower = ReadVal<uint32_t>(p, end);
    outData.playerMaxPower = ReadVal<uint32_t>(p, end);
    outData.playerPowerType = ReadVal<uint8_t>(p, end);
    outData.playerComboPoints = ReadVal<uint8_t>(p, end);
    outData.playerIsIngame = ReadVal<bool>(p, end);

    outData.targetHealth = ReadVal<uint32_t>(p, end);
    outData.targetMaxHealth = ReadVal<uint32_t>(p, end);
    outData.targetPower = ReadVal<uint32_t>(p, end);
    outData.targetMaxPower = ReadVal<uint32_t>(p, end);
    outData.targetPowerType = ReadVal<uint8_t>(p, end);
    outData.targetLevel = ReadVal<uint32_t>(p, end);
    outData.isLoading = ReadVal<bool>(p, end);
    outData.worldLoaded = ReadVal<bool>(p, end);
    outData.isIndoor = ReadVal<bool>(p, end);

    outData.playerPosX = ReadVal<float>(p, end);
    outData.playerPosY = ReadVal<float>(p, end);
    outData.playerPosZ = ReadVal<float>(p, end);
    outData.playerRotation = ReadVal<float>(p, end);

    outData.gameState = ReadVal<uint32_t>(p, end);
    outData.tickCount = ReadVal<uint32_t>(p, end);

    outData.corpseX = ReadVal<float>(p, end);
    outData.corpseY = ReadVal<float>(p, end);
    outData.corpseZ = ReadVal<float>(p, end);

    outData.targetGUID = ReadVal<uint64_t>(p, end);
    outData.mouseOverGUID = ReadVal<uint64_t>(p, end);
    outData.lastTargetGUID = ReadVal<uint64_t>(p, end);

    outData.castingSpellId = ReadVal<uint32_t>(p, end);
    outData.channelSpellId = ReadVal<uint32_t>(p, end);

    // 3. Auras
    outData.auraCount = ReadVal<uint32_t>(p, end);
    if (outData.auraCount > GameData::kMaxAuras) return false;
    for (uint32_t i = 0; i < outData.auraCount; ++i) {
        outData.auras[i].spellId = ReadVal<uint32_t>(p, end);
    }

    outData.targetAuraCount = ReadVal<uint32_t>(p, end);
    if (outData.targetAuraCount > GameData::kMaxTargetAuras) return false;
    for (uint32_t i = 0; i < outData.targetAuraCount; ++i) {
        outData.targetAuras[i].spellId = ReadVal<uint32_t>(p, end);
    }

    // 4. Combat Log Events
    outData.combatLogEventCount = ReadVal<uint32_t>(p, end);
    if (outData.combatLogEventCount > GameData::kMaxCombatLogEvents) return false;
    for (uint32_t i = 0; i < outData.combatLogEventCount; ++i) {
        outData.combatLogEvents[i] = ReadVal<CombatLogEvent>(p, end);
    }

    // 5. Camera
    outData.cameraYaw = ReadVal<float>(p, end);
    outData.cameraPitch = ReadVal<float>(p, end);

    // 6. Party / Raid / Quest metrics
    outData.numPartyMembers = ReadVal<uint32_t>(p, end);
    outData.partyDifficulty = ReadVal<uint32_t>(p, end);
    outData.numRaidMembers = ReadVal<uint32_t>(p, end);
    outData.raidDifficulty = ReadVal<uint32_t>(p, end);
    outData.activeQuestsCount = ReadVal<uint32_t>(p, end);

    return p <= end;
}

// ---------------------------------------------------------------------------
// DoubleBufferedGameData
// ---------------------------------------------------------------------------

void DoubleBufferedGameData::SwapBuffers()
{
    mReadIndex = 1 - mReadIndex;
}

const GameData& DoubleBufferedGameData::GetReadBuffer() const
{
    return mBuffers[mReadIndex];
}

GameData& DoubleBufferedGameData::GetWriteBuffer()
{
    return mBuffers[1 - mReadIndex];
}

} // namespace WoWMemory
