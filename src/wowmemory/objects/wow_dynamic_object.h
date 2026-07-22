#ifndef WOWMEMORY_OBJECTS_WOW_DYNAMIC_OBJECT_H
#define WOWMEMORY_OBJECTS_WOW_DYNAMIC_OBJECT_H

#include "wowmemory/objects/wow_object.h"

namespace WoWMemory {

class WoWDynamicObject : public WoWObject {
public:
    explicit WoWDynamicObject(uintptr_t baseAddress) : WoWObject(baseAddress) {}
};

} // namespace WoWMemory

#endif // WOWMEMORY_OBJECTS_WOW_DYNAMIC_OBJECT_H
