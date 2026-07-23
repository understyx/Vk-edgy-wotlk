#ifndef WOW_DYNAMIC_OBJECT_H
#define WOW_DYNAMIC_OBJECT_H

#include "wow_object.h"

namespace WoWMemory {

class WoWDynamicObject : public WoWObject {
public:
    explicit WoWDynamicObject(uintptr_t baseAddress);
    ~WoWDynamicObject() override = default;

    uint64_t GetCasterGUID() const;
    uint32_t GetSpellID() const;
    float GetRadius() const;
    uint32_t GetCastTime() const;
};

} // namespace WoWMemory

#endif // WOW_DYNAMIC_OBJECT_H
