/**
 * @file wowmemory.h
 * @brief World of Warcraft memory reading interface
 * 
 * This module handles reading game state from World of Warcraft memory.
 * It uses offsets defined in wotlk_offsets.txt to locate and read game data
 * such as player position, NPC positions, quest data, etc.
 * 
 * The data is read on a separate thread and stored in a double buffer that
 * can be safely accessed by the rendering thread without locks.
 */

#ifndef WOWMEMORY_H
#define WOWMEMORY_H

#include <cstdint>
#include <memory>

namespace WoWMemory {

/**
 * @struct GameData
 * @brief Container for game state read from memory
 * 
 * This structure will be expanded to include player position, NPC data,
 * quest information, and other relevant game state.
 */
struct GameData {
    // TODO: Add game state fields based on wotlk_offsets.txt
    // Examples:
    // - Player position (x, y, z)
    // - Player health/mana
    // - Target information
    // - NPC positions
    // - Buff/debuff information
};

/**
 * @class GameDataReader
 * @brief Reads game data from WoW memory
 * 
 * This class handles the actual memory reading operations. It will be
 * instantiated in a separate thread to continuously read game state.
 */
class GameDataReader {
public:
    GameDataReader() = default;
    ~GameDataReader() = default;
    
    /**
     * @brief Initialize the reader with WoW process
     * @return true if initialization succeeded
     */
    bool Initialize();
    
    /**
     * @brief Read current game data from memory
     * @param outData Output structure to fill with game data
     * @return true if read succeeded
     */
    bool ReadGameData(GameData& outData);
    
private:
    // Process handle and base addresses will be stored here
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
