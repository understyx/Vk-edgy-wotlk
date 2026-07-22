#ifndef WOWMEMORY_OBJECTS_WOW_ITEM_H
#define WOWMEMORY_OBJECTS_WOW_ITEM_H

#include "wowmemory/objects/wow_object.h"

namespace WoWMemory {

class WoWItem : public WoWObject {
public:
    explicit WoWItem(uintptr_t baseAddress) : WoWObject(baseAddress) {}
};

} // namespace WoWMemory

#endif // WOWMEMORY_OBJECTS_WOW_ITEM_H
