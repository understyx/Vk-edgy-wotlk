#include "wow_camera.h"
#include "wowmemory/offsets.h"
#include "wowmemory/memory_utils.h"

#ifdef _WIN32
#include <windows.h>
#endif
#include <cmath>

namespace WoWMemory {

WoWCamera::WoWCamera() {
#ifdef _WIN32
    if (IsReadableRange(WoWOffsets::Drawing::WorldFrame, sizeof(uint32_t))) {
        uint32_t worldFrame = *reinterpret_cast<const uint32_t*>(WoWOffsets::Drawing::WorldFrame);
        if (worldFrame && IsReadableRange(worldFrame + WoWOffsets::Drawing::ActiveCamera, sizeof(uint32_t))) {
            m_pointer = reinterpret_cast<void*>(*reinterpret_cast<const uint32_t*>(worldFrame + WoWOffsets::Drawing::ActiveCamera));
        }
    }
#endif
}

void* WoWCamera::GetPointer() const {
    return m_pointer;
}

bool WoWCamera::IsValid() const {
    return m_pointer != nullptr;
}

Vector3 WoWCamera::Forward() const {
    Vector3 vec;
#ifdef _WIN32
    if (m_pointer) {
        typedef Vector3* (__thiscall *Forward_t)(void*, Vector3*);
        uint32_t* vtable = *reinterpret_cast<uint32_t**>(m_pointer);
        if (vtable && IsReadableRange(reinterpret_cast<uintptr_t>(&vtable[1]), sizeof(uint32_t))) {
            auto fn = reinterpret_cast<Forward_t>(vtable[1]);
            fn(m_pointer, &vec);
        }
    }
#endif
    return vec;
}

Vector3 WoWCamera::Right() const {
    Vector3 vec;
#ifdef _WIN32
    if (m_pointer) {
        typedef Vector3* (__thiscall *Right_t)(void*, Vector3*);
        uint32_t* vtable = *reinterpret_cast<uint32_t**>(m_pointer);
        if (vtable && IsReadableRange(reinterpret_cast<uintptr_t>(&vtable[2]), sizeof(uint32_t))) {
            auto fn = reinterpret_cast<Right_t>(vtable[2]);
            fn(m_pointer, &vec);
        }
    }
#endif
    return vec;
}

Vector3 WoWCamera::Up() const {
    Vector3 vec;
#ifdef _WIN32
    if (m_pointer) {
        typedef Vector3* (__thiscall *Up_t)(void*, Vector3*);
        uint32_t* vtable = *reinterpret_cast<uint32_t**>(m_pointer);
        if (vtable && IsReadableRange(reinterpret_cast<uintptr_t>(&vtable[3]), sizeof(uint32_t))) {
            auto fn = reinterpret_cast<Up_t>(vtable[3]);
            fn(m_pointer, &vec);
        }
    }
#endif
    return vec;
}

Matrix WoWCamera::Projection() const {
    // Return custom projection matrix stub / perspective placeholder
    return Matrix::Identity();
}

Matrix WoWCamera::View() const {
    // Return custom view matrix stub / lookat placeholder
    return Matrix::Identity();
}

CameraInfo WoWCamera::GetCameraInfo() const {
    CameraInfo info;
#ifdef _WIN32
    if (m_pointer && IsReadableRange(reinterpret_cast<uintptr_t>(m_pointer), sizeof(CameraInfo))) {
        info = *reinterpret_cast<const CameraInfo*>(m_pointer);
    }
#endif
    return info;
}

} // namespace WoWMemory
