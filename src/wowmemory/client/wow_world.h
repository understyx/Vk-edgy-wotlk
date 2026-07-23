#ifndef WOWMEMORY_CLIENT_WOW_WORLD_H
#define WOWMEMORY_CLIENT_WOW_WORLD_H

#include "location.h"
#include <string>
#include <cstdint>

namespace WoWMemory {

struct TerrainClickEvent {
    uint64_t GUID = 0;
    Location Position;
    uint32_t Button = 1;
};

class WoWWorld {
public:
    static int CurrentMapId();
    static std::string CurrentMap();
    static std::string CurrentZone();
    static std::string CurrentSubZone();
    static uint32_t CurrentZoneId();

    static bool HandleTerrainClick(const Location& loc);
    static int Traceline(const Location& start, const Location& end, Location& result, uint32_t flags);
    static int Traceline(const Location& start, const Location& end);
    static int LineOfSightTest(Location start, Location end);
};

} // namespace WoWMemory

#endif // WOWMEMORY_CLIENT_WOW_WORLD_H
