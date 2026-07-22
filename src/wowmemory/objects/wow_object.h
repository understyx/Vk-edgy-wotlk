#ifndef WOWMEMORY_OBJECTS_WOW_OBJECT_H
#define WOWMEMORY_OBJECTS_WOW_OBJECT_H

#include "wowmemory/memory_utils.h"
#include "wowmemory/offsets.h"
#include <cstdint>
#include <string>

namespace WoWMemory {

class WoWObject {
public:
    explicit WoWObject(uintptr_t baseAddress) : m_base(baseAddress) {}
    virtual ~WoWObject() = default;

    uintptr_t GetBaseAddress() const { return m_base; }
    bool IsValid() const { return m_base != 0 && IsReadableRange(m_base, 4); }

    uint64_t GetGUID() const {
        return ReadAbs<uint64_t>(m_base + WoWOffsets::objectGUID);
    }

    uint32_t GetObjectType() const {
        return ReadAbs<uint32_t>(m_base + WoWOffsets::objectType);
    }

    float GetX() const {
        return ReadAbs<float>(m_base + WoWOffsets::objectPosX);
    }

    float GetY() const {
        return ReadAbs<float>(m_base + WoWOffsets::objectPosY);
    }

    float GetZ() const {
        return ReadAbs<float>(m_base + WoWOffsets::objectPosZ);
    }

    float GetRotation() const {
        return ReadAbs<float>(m_base + WoWOffsets::objectRotation);
    }

    uintptr_t GetDescriptorAddress() const {
        return ReadAbs<uint32_t>(m_base + WoWOffsets::objectDescriptorOffset);
    }

protected:
    uintptr_t m_base;
};

} // namespace WoWMemory

#endif // WOWMEMORY_OBJECTS_WOW_OBJECT_H
