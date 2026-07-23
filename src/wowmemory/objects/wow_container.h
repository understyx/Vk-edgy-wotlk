#ifndef WOW_CONTAINER_H
#define WOW_CONTAINER_H

#include "wow_item.h"

namespace WoWMemory {

class WoWContainer : public WoWItem {
public:
    explicit WoWContainer(uintptr_t baseAddress);
    ~WoWContainer() override = default;

    uint32_t GetNumSlots() const;
    uint64_t GetSlotGUID(uint32_t slotIndex) const;
};

} // namespace WoWMemory

#endif // WOW_CONTAINER_H
