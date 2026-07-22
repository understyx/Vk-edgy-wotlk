#ifndef WOWMEMORY_OBJECTS_WOW_CONTAINER_H
#define WOWMEMORY_OBJECTS_WOW_CONTAINER_H

#include "wowmemory/objects/wow_item.h"

namespace WoWMemory {

class WoWContainer : public WoWItem {
public:
    explicit WoWContainer(uintptr_t baseAddress) : WoWItem(baseAddress) {}
};

} // namespace WoWMemory

#endif // WOWMEMORY_OBJECTS_WOW_CONTAINER_H
