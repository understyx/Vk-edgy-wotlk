#ifndef WOWMEMORY_OBJECTS_WOW_GAMEOBJECT_H
#define WOWMEMORY_OBJECTS_WOW_GAMEOBJECT_H

#include "wowmemory/objects/wow_object.h"

namespace WoWMemory {

class WoWGameObject : public WoWObject {
public:
    explicit WoWGameObject(uintptr_t baseAddress) : WoWObject(baseAddress) {}
};

} // namespace WoWMemory

#endif // WOWMEMORY_OBJECTS_WOW_GAMEOBJECT_H
