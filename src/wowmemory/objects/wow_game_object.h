#ifndef WOW_GAME_OBJECT_H
#define WOW_GAME_OBJECT_H

#include "wow_object.h"

namespace WoWMemory {

class WoWGameObject : public WoWObject {
public:
    explicit WoWGameObject(uintptr_t baseAddress);
    ~WoWGameObject() override = default;

    uint64_t GetCreatedBy() const;
    uint32_t GetDisplayID() const;
    uint32_t GetFlags() const;
    uint32_t GetFaction() const;
    uint32_t GetLevel() const;
};

} // namespace WoWMemory

#endif // WOW_GAME_OBJECT_H
