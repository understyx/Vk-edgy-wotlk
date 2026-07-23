#ifndef WOW_PLAYER_H
#define WOW_PLAYER_H

#include "wow_unit.h"

namespace WoWMemory {

class WoWPlayer : public WoWUnit {
public:
    explicit WoWPlayer(uintptr_t baseAddress);
    ~WoWPlayer() override = default;

    uint32_t GetXP() const;
    uint32_t GetNextLevelXP() const;
    uint32_t GetCoinage() const;
};

} // namespace WoWMemory

#endif // WOW_PLAYER_H
