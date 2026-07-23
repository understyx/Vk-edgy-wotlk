#ifndef WOW_UNIT_H
#define WOW_UNIT_H

#include "wow_object.h"

namespace WoWMemory {

class WoWUnit : public WoWObject {
public:
    explicit WoWUnit(uintptr_t baseAddress);
    ~WoWUnit() override = default;

    uint32_t GetHealth() const;
    uint32_t GetMaxHealth() const;
    uint32_t GetLevel() const;
    uint64_t GetTargetGUID() const;
    uint8_t GetPowerType() const;
    uint32_t GetPower(uint8_t powerType) const;
    uint32_t GetMaxPower(uint8_t powerType) const;

    uint32_t GetCastingSpellId() const;
    uint32_t GetChannelSpellId() const;
};

} // namespace WoWMemory

#endif // WOW_UNIT_H
