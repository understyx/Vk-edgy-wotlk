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
 * @struct AuraInfo
 * @brief Minimal descriptor for a single active aura on the local player.
 */
struct AuraInfo {
    uint32_t spellId = 0;  ///< Spell ID of the aura
};

/**
 * @struct CombatLogEvent
 * @brief A single combat log entry read from the WoW in-process linked list.
 *
 * Layout based on AppendLinkedListNode / handle_combat_log_entry disassembly
 * for WotLK 3.3.5a (build 12340).  Fields marked as "unknown" in the Python
 * reference are omitted; only fields with confirmed semantics are included.
 */
struct CombatLogEvent {
    uint32_t timestamp           = 0;  ///< Game timestamp (ms since boot)
    int32_t  eventTypeId         = 0;  ///< Internal event type identifier
    uint64_t sourceGuid          = 0;  ///< GUID of the event source
    uint64_t destGuid            = 0;  ///< GUID of the event destination
    int32_t  amount              = 0;  ///< Primary amount (damage / heal / energize)
    int32_t  overkillOrPowerType = 0;  ///< Overkill / overheal amount, or power type
    int32_t  schoolMask          = 0;  ///< Spell school bitmask
    int32_t  absorbed            = 0;  ///< Absorbed amount
    int32_t  resisted            = 0;  ///< Resisted amount
    int32_t  blockedOrMissType   = 0;  ///< Blocked amount or miss type
    uint32_t flags               = 0;  ///< Bit 0 = critical hit; bit 1/2 = TBD
};

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
    uint32_t    playerHealth    = 0;  ///< Current health
    uint32_t    playerMaxHealth = 0;  ///< Maximum health
    uint32_t    playerLevel     = 0;  ///< Character level
    uint32_t    playerPower     = 0;  ///< Current power (mana/rage/energy/…)
    uint32_t    playerMaxPower  = 0;  ///< Maximum power
    uint8_t     playerPowerType = 0;  ///< Power type byte (0=mana,1=rage,3=energy,…)
    uint8_t     playerComboPoints = 0; ///< Combo points
    bool        playerIsIngame  = false;
    bool        isLoading       = false;
    bool        worldLoaded     = false;
    bool        isIndoor        = false;

    // --- Player position ---
    float       playerPosX    = 0.0f;
    float       playerPosY    = 0.0f;
    float       playerPosZ    = 0.0f;
    float       playerRotation= 0.0f; ///< Facing angle in radians

    // --- Game state ---
    uint32_t    gameState  = 0;  ///< Internal game state enum
    uint32_t    tickCount  = 0;  ///< Game tick counter

    // --- Corpse ---
    float       corpseX = 0.0f;
    float       corpseY = 0.0f;
    float       corpseZ = 0.0f;

    // --- GUIDs ---
    uint64_t    targetGUID    = 0;  ///< Current target GUID (from unit fields)
    uint64_t    mouseOverGUID = 0;  ///< Mouse-over GUID
    uint64_t    lastTargetGUID= 0;  ///< Last target GUID

    // --- Casting / channeling ---
    uint32_t    castingSpellId = 0;   ///< Currently cast spell id (0 = none)
    uint32_t    channelSpellId = 0;   ///< Currently channeled spell id (0 = none)

    // --- Auras (local player only) ---
    static constexpr size_t kMaxAuras = 40;
    AuraInfo    auras[kMaxAuras];     ///< Active auras on local player
    uint32_t    auraCount = 0;        ///< Number of valid entries in auras[]

    // --- Target Auras ---
    static constexpr size_t kMaxTargetAuras = 40;
    AuraInfo    targetAuras[kMaxTargetAuras]; ///< Active auras on target
    uint32_t    targetAuraCount = 0;          ///< Number of valid entries in targetAuras[]

    // --- Combat log (new events since last ReadGameData call) ---
    static constexpr size_t kMaxCombatLogEvents = 64;
    CombatLogEvent combatLogEvents[kMaxCombatLogEvents]; ///< New events this frame
    uint32_t       combatLogEventCount = 0;              ///< Number of valid entries

    // --- Camera ---
    float       cameraYaw   = 0.0f;  ///< Camera yaw in radians
    float       cameraPitch = 0.0f;  ///< Camera pitch in radians
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

    /// Tracks the last combat log node address read, for incremental reading.
    uintptr_t m_lastCombatLogNodeAddr = 0;
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
