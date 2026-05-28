#pragma once
#include <cmath>
#include <cstdint>
#include <algorithm>

constexpr float M_PI_F = 3.14159265358979323846f;
constexpr uint32_t FL_ONGROUND = (1 << 0);

struct Vector2 {
    float x, y;

    Vector2 operator-(const Vector2& o) const { return { x - o.x, y - o.y }; }
    Vector2 operator+(const Vector2& o) const { return { x + o.x, y + o.y }; }
    float Length() const { return std::sqrt(x * x + y * y); }
};

struct Vector3 {
    float x, y, z;

    Vector3 operator-(const Vector3& o) const { return { x - o.x, y - o.y, z - o.z }; }
    Vector3 operator+(const Vector3& o) const { return { x + o.x, y + o.y, z + o.z }; }
    Vector3 operator*(float s) const { return { x * s, y * s, z * s }; }
    Vector3 operator/(float s) const { return { x / s, y / s, z / s }; }

    float Length() const { return std::sqrt(x * x + y * y + z * z); }
    float Length2D() const { return std::sqrt(x * x + y * y); }
    float Distance(const Vector3& o) const { return (*this - o).Length(); }
    bool IsZero() const { return x == 0.0f && y == 0.0f && z == 0.0f; }
};

using QAngle = Vector3;

struct Matrix4x4 {
    float m[4][4];
};

namespace BoneIndex {
    constexpr int ORIGIN = 0;

    constexpr int PELVIS = 1;

    constexpr int SPINE_0 = 2;
    constexpr int SPINE_1 = 3;
    constexpr int SPINE_2 = 4;

    constexpr int NECK = 6;
    constexpr int HEAD = 7;

    constexpr int LEFT_SHOULDER = 9;
    constexpr int LEFT_ELBOW = 10;
    constexpr int LEFT_HAND = 11;

    constexpr int RIGHT_SHOULDER = 13;
    constexpr int RIGHT_ELBOW = 14;
    constexpr int RIGHT_HAND = 15;

    constexpr int LEFT_HIP = 17;
    constexpr int LEFT_KNEE = 18;
    constexpr int LEFT_FOOT = 19;

    constexpr int RIGHT_HIP = 20;
    constexpr int RIGHT_KNEE = 21;
    constexpr int RIGHT_FOOT = 22;

    constexpr int CHEST = 23;
    constexpr int BONE_COUNT = 24;
}

inline bool WorldToScreen(const Vector3& pos, Vector2& screen, const Matrix4x4& matrix, int width, int height) {
    float w = matrix.m[3][0] * pos.x + matrix.m[3][1] * pos.y + matrix.m[3][2] * pos.z + matrix.m[3][3];
    if (w < 0.01f) return false;

    float invW = 1.0f / w;
    float x = (matrix.m[0][0] * pos.x + matrix.m[0][1] * pos.y + matrix.m[0][2] * pos.z + matrix.m[0][3]) * invW;
    float y = (matrix.m[1][0] * pos.x + matrix.m[1][1] * pos.y + matrix.m[1][2] * pos.z + matrix.m[1][3]) * invW;

    screen.x = (width * 0.5f) * (1.0f + x);
    screen.y = (height * 0.5f) * (1.0f - y);
    return true;
}

inline void NormalizeAngles(Vector3& angle) {
    while (angle.y > 180.0f) angle.y -= 360.0f;
    while (angle.y < -180.0f) angle.y += 360.0f;
    angle.x = std::clamp(angle.x, -89.0f, 89.0f);
}

inline Vector3 CalculateAngle(const Vector3& src, const Vector3& dst) {
    Vector3 delta = dst - src;
    float hyp = delta.Length2D();
    Vector3 angle;
    angle.x = -std::atan2f(delta.z, hyp) * (180.0f / M_PI_F);
    angle.y = std::atan2f(delta.y, delta.x) * (180.0f / M_PI_F);
    angle.z = 0.0f;
    NormalizeAngles(angle);
    return angle;
}

inline float GetFov(const Vector3& viewAngle, const Vector3& aimAngle) {
    Vector3 delta = aimAngle - viewAngle;
    NormalizeAngles(delta);
    return std::sqrt(delta.x * delta.x + delta.y * delta.y);
}

inline Vector3 SmoothAngle(const Vector3& src, const Vector3& dst, float factor) {
    Vector3 delta = dst - src;
    NormalizeAngles(delta);
    Vector3 result;
    result.x = src.x + delta.x / factor;
    result.y = src.y + delta.y / factor;
    result.z = 0.0f;
    NormalizeAngles(result);
    return result;
}

struct EntityData {
    uintptr_t controller = 0;
    uintptr_t pawn = 0;
    Vector3 origin = {};
    Vector3 headPos = {};
    Vector3 bones[BoneIndex::BONE_COUNT] = {};
    int health = 0;
    uint8_t team = 0;
    int armor = 0;
    bool dormant = false;
    bool alive = false;
    bool hasHelmet = false;
    bool isScoped = false;
    char name[128] = {};
    float distance = 0.0f;
    bool valid = false;
    bool bonesValid = false;
};
