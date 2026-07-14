#ifndef GAME_DATA_TYPES_HPP
#define GAME_DATA_TYPES_HPP

#include <string>
#include <cstdint>

struct PlayerData {
    char name[64] = {0};
    uint32_t health = 0;
    uint32_t maxHealth = 0;
    uint32_t mana = 0;
    uint32_t maxMana = 0;
    float posX = 0.0f;
    float posY = 0.0f;
    float posZ = 0.0f;
    char targetName[64] = {0};
    uint32_t level = 1;
};

#endif // GAME_DATA_TYPES_HPP
