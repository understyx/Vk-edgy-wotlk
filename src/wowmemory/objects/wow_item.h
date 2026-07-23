#ifndef WOW_ITEM_H
#define WOW_ITEM_H

#include "wow_object.h"

namespace WoWMemory {

class WoWItem : public WoWObject {
public:
    explicit WoWItem(uintptr_t baseAddress);
    ~WoWItem() override = default;

    uint64_t GetOwner() const;
    uint64_t GetContained() const;
    uint64_t GetCreator() const;
    uint32_t GetStackCount() const;
    uint32_t GetDuration() const;
    uint32_t GetFlags() const;
    uint32_t GetDurability() const;
    uint32_t GetMaxDurability() const;
};

} // namespace WoWMemory

#endif // WOW_ITEM_H
