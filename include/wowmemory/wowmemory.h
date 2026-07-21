/**
 * @file wowmemory.h
 * @brief World of Warcraft memory reading interface
 *
 * This module reads live game state directly from WoW's memory.
 * Because this code runs inside the WoW process as a Vulkan layer, all
 * absolute addresses from offsets.h can be dereferenced directly.
 *
 * The data is read on demand (e.g. once per rendered frame) and stored in a
 * double buffer so the render thread always sees a consistent snapshot.
 */

#ifndef WOWMEMORY_H
#define WOWMEMORY_H

#include <cstdint>
#include <memory>
#include <string>

namespace WoWMemory {

/**
 * @struct GameData
 * @brief Snapshot of WoW game state read from process memory.
 *
 * String fields are read as null-terminated char arrays at their absolute
 * addresses.  Numeric fields are plain uint32/uint64 reads.
 */
struct GameData {
    // --- Identity ---
    std::string playerName;      ///< Local player name (max 12 chars)
    std::string realmName;       ///< Realm / server name
    uint64_t    localPlayerGUID = 0; ///< GUID of the local player

    // --- World location ---
    std::string continentName;   ///< Continent / instance name
    std::string zoneText;        ///< Current zone display name
    std::string subZoneText;     ///< Current sub-zone display name
    uint32_t    mapID   = 0;     ///< Map identifier
    uint32_t    zoneID  = 0;     ///< Zone identifier

    // --- Player state ---
    uint32_t    playerHealth    = 0;  ///< Current health (read via playerBase + 0x19B8)
    bool        playerIsIngame  = false;
    bool        isLoading       = false;
    bool        worldLoaded     = false;
    bool        isIndoor        = false;

    // --- Game state ---
    uint32_t    gameState  = 0;  ///< Internal game state enum
    uint32_t    tickCount  = 0;  ///< Game tick counter

    // --- Corpse ---
    float       corpseX = 0.0f;
    float       corpseY = 0.0f;
    float       corpseZ = 0.0f;
};

/**
 * @class GameDataReader
 * @brief Reads live game data from WoW process memory.
 *
 * Because this code runs inside the WoW process, memory is read via direct
 * pointer casts using the absolute addresses from offsets.h.
 */
class GameDataReader {
public:
    GameDataReader() = default;
    ~GameDataReader() = default;

    /**
     * @brief Read current game data from memory.
     * @param outData Output structure to fill with game data.
     * @return true on success; false if a read would fault (null pointer guard).
     */
    bool ReadGameData(GameData& outData);

private:
    /// Read a bounded null-terminated string directly from an absolute address.
    static std::string ReadInlineString(uint32_t absAddr, size_t maxLen = 64);

    /// Read a null-terminated string through a char* pointer stored at absAddr.
    static std::string ReadIndirectString(uint32_t ptrAddr, size_t maxLen = 64);
};

/**
 * @class DoubleBufferedGameData
 * @brief Thread-safe double buffer for game data
 * 
 * This class manages two buffers - one being read by the rendering thread
 * and one being written by the game data reader thread. This allows safe
 * data exchange without locks.
 */
class DoubleBufferedGameData {
public:
    DoubleBufferedGameData() = default;
    ~DoubleBufferedGameData() = default;
    
    /**
     * @brief Swap the read and write buffers
     * 
     * Called by the writer thread after updating the write buffer.
     * The rendering thread can then safely read the swapped buffer.
     */
    void SwapBuffers();
    
    /**
     * @brief Get the current read buffer
     * @return Reference to the current read buffer
     */
    const GameData& GetReadBuffer() const;
    
    /**
     * @brief Get the current write buffer
     * @return Reference to the current write buffer
     */
    GameData& GetWriteBuffer();
    
private:
    GameData mBuffers[2];
    int mReadIndex = 0;
};

} // namespace WoWMemory

#endif // WOWMEMORY_H
