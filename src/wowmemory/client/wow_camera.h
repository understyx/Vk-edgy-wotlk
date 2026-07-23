#ifndef WOWMEMORY_CLIENT_WOW_CAMERA_H
#define WOWMEMORY_CLIENT_WOW_CAMERA_H

#include <cstdint>

namespace WoWMemory {

struct Vector3 {
    float X = 0.0f;
    float Y = 0.0f;
    float Z = 0.0f;

    Vector3() = default;
    Vector3(float x, float y, float z) : X(x), Y(y), Z(z) {}

    Vector3 operator+(const Vector3& other) const {
        return Vector3(X + other.X, Y + other.Y, Z + other.Z);
    }
};

struct Matrix {
    float M[4][4] = {};

    static Matrix Identity() {
        Matrix m;
        for (int i = 0; i < 4; ++i) m.M[i][i] = 1.0f;
        return m;
    }
};

struct CameraInfo {
    void* Vtable = nullptr;
    uint32_t Type = 0;
    void* View = nullptr;
    Vector3 Position;
    float Facing = 0.0f;
    float Pitch = 0.0f;
    float Roll = 0.0f;
    float FieldOfView = 0.0f;
    float NearPlane = 0.0f;
    float FarPlane = 0.0f;
    float Aspect = 0.0f;
};

class WoWCamera {
public:
    WoWCamera();

    void* GetPointer() const;
    bool IsValid() const;

    Vector3 Forward() const;
    Vector3 Right() const;
    Vector3 Up() const;

    Matrix Projection() const;
    Matrix View() const;

    CameraInfo GetCameraInfo() const;

private:
    void* m_pointer = nullptr;
};

} // namespace WoWMemory

#endif // WOWMEMORY_CLIENT_WOW_CAMERA_H
