#include "wow_world.h"
#include "wowmemory/offsets.h"
#include "wowmemory/memory_utils.h"

#ifdef _WIN32
#include <windows.h>
#endif

namespace WoWMemory {

int WoWWorld::CurrentMapId() {
#ifdef _WIN32
    return ReadAbs<int>(WoWOffsets::World::CurrentMapId);
#else
    return 0;
#endif
}

std::string WoWWorld::CurrentMap() {
#ifdef _WIN32
    return ReadIndirectString(WoWOffsets::World::InternalMapName);
#else
    return "";
#endif
}

std::string WoWWorld::CurrentZone() {
#ifdef _WIN32
    return ReadIndirectString(WoWOffsets::World::ZoneText);
#else
    return "";
#endif
}

std::string WoWWorld::CurrentSubZone() {
#ifdef _WIN32
    return ReadIndirectString(WoWOffsets::World::SubZoneText);
#else
    return "";
#endif
}

uint32_t WoWWorld::CurrentZoneId() {
#ifdef _WIN32
    return ReadAbs<uint32_t>(WoWOffsets::World::ZoneID);
#else
    return 0;
#endif
}

bool WoWWorld::HandleTerrainClick(const Location& loc) {
#ifdef _WIN32
    typedef bool (__cdecl *HandleTerrainClick_t)(const TerrainClickEvent*);
    auto fn = reinterpret_cast<HandleTerrainClick_t>(WoWOffsets::World::HandleTerrainClick);
    TerrainClickEvent ev;
    ev.GUID = 0;
    ev.Position = loc;
    ev.Button = 1; // Left / None
    return fn(&ev);
#else
    return true;
#endif
}

int WoWWorld::Traceline(const Location& start, const Location& end, Location& result, uint32_t flags) {
#ifdef _WIN32
    typedef int (__cdecl *Traceline_t)(const Location*, const Location*, Location*, float*, uint32_t, uint32_t);
    auto fn = reinterpret_cast<Traceline_t>(WoWOffsets::World::Traceline);
    float dist = 1.0f;
    Location startCopy = start;
    Location endCopy = end;
    return fn(&startCopy, &endCopy, &result, &dist, flags, 0);
#else
    result = end;
    return 0;
#endif
}

int WoWWorld::Traceline(const Location& start, const Location& end) {
    Location result;
    return Traceline(start, end, result, 0x120171);
}

int WoWWorld::LineOfSightTest(Location start, Location end) {
    start.Z += 1.3f;
    end.Z += 1.3f;
    Location result;
    return Traceline(start, end, result, 0x120171);
}

} // namespace WoWMemory
