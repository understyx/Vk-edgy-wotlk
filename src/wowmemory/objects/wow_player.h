#ifndef WOWMEMORY_OBJECTS_WOW_PLAYER_H
#define WOWMEMORY_OBJECTS_WOW_PLAYER_H

#include "wowmemory/objects/wow_unit.h"

namespace WoWMemory {

class WoWPlayer : public WoWUnit {
public:
    explicit WoWPlayer(uintptr_t baseAddress) : WoWUnit(baseAddress) {}
};

} // namespace WoWMemory

#endif // WOWMEMORY_OBJECTS_WOW_PLAYER_H
