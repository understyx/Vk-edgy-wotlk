#ifndef WOWMEMORY_CLIENT_LOCATION_H
#define WOWMEMORY_CLIENT_LOCATION_H

#include <cmath>

namespace WoWMemory {

struct Location {
    float X = 0.0f;
    float Y = 0.0f;
    float Z = 0.0f;

    Location() = default;
    Location(float x, float y, float z) : X(x), Y(y), Z(z) {}

    double DistanceTo(const Location& other) const {
        return std::sqrt(std::pow(X - other.X, 2) + std::pow(Y - other.Y, 2) + std::pow(Z - other.Z, 2));
    }
    double DistanceToSqr(const Location& other) const {
        return std::pow(X - other.X, 2) + std::pow(Y - other.Y, 2) + std::pow(Z - other.Z, 2);
    }
    double Distance2D(const Location& other) const {
        return std::sqrt(std::pow(X - other.X, 2) + std::pow(Y - other.Y, 2));
    }
    double Length() const {
        return std::sqrt(std::pow(X, 2) + std::pow(Y, 2) + std::pow(Z, 2));
    }
    Location Normalize() const {
        double len = Length();
        if (len == 0.0) return *this;
        return Location(static_cast<float>(X / len), static_cast<float>(Y / len), static_cast<float>(Z / len));
    }
};

struct Blackspot {
    Location position;
    float radius = 0.0f;

    Blackspot() = default;
    Blackspot(const Location& loc, float r) : position(loc), radius(r) {}
};

} // namespace WoWMemory

#endif // WOWMEMORY_CLIENT_LOCATION_H
