#ifndef WOW_OBJECT_H
#define WOW_OBJECT_H

#include <cstdint>
#include <string>

namespace WoWMemory {

class WoWObject {
protected:
    uintptr_t m_baseAddress;

public:
    explicit WoWObject(uintptr_t baseAddress);
    virtual ~WoWObject() = default;

    uintptr_t GetBaseAddress() const;
    bool IsValid() const;

    uint64_t GetGUID() const;
    uint32_t GetType() const;
    uint32_t GetEntry() const;
    float GetScaleX() const;

    float GetX() const;
    float GetY() const;
    float GetZ() const;
    float GetRotation() const;
};

} // namespace WoWMemory

#endif // WOW_OBJECT_H
