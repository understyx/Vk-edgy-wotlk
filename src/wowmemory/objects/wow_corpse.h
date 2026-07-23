#ifndef WOW_CORPSE_H
#define WOW_CORPSE_H

#include "wow_object.h"

namespace WoWMemory {

class WoWCorpse : public WoWObject {
public:
    explicit WoWCorpse(uintptr_t baseAddress);
    ~WoWCorpse() override = default;

    uint64_t GetOwner() const;
    uint64_t GetParty() const;
    uint32_t GetDisplayID() const;
    uint32_t GetFlags() const;
};

} // namespace WoWMemory

#endif // WOW_CORPSE_H
