#ifndef WOWMEMORY_OBJECTS_WOW_CORPSE_H
#define WOWMEMORY_OBJECTS_WOW_CORPSE_H

#include "wowmemory/objects/wow_object.h"

namespace WoWMemory {

class WoWCorpse : public WoWObject {
public:
    explicit WoWCorpse(uintptr_t baseAddress) : WoWObject(baseAddress) {}
};

} // namespace WoWMemory

#endif // WOWMEMORY_OBJECTS_WOW_CORPSE_H
