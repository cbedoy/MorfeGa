#ifndef MORFEGA_UTILITY_H
#define MORFEGA_UTILITY_H

#include <cassert>
#include <cmath>
#include <GLES3/gl3.h>

struct Vector3 {
    float x, y, z;

    Vector3() : x(0), y(0), z(0) {}
    Vector3(float x, float y, float z) : x(x), y(y), z(z) {}
    const float *data() const { return &x; }

    Vector3 operator+(const Vector3 &o) const { return {x + o.x, y + o.y, z + o.z}; }
    Vector3 operator-(const Vector3 &o) const { return {x - o.x, y - o.y, z - o.z}; }
    Vector3 operator*(float s) const { return {x * s, y * s, z * s}; }
    Vector3 operator*(const Vector3 &o) const { return {x * o.x, y * o.y, z * o.z}; }
    Vector3 operator/(float s) const { return {x / s, y / s, z / s}; }
    Vector3 &operator+=(const Vector3 &o) { x += o.x; y += o.y; z += o.z; return *this; }
    Vector3 &operator-=(const Vector3 &o) { x -= o.x; y -= o.y; z -= o.z; return *this; }
    Vector3 &operator*=(float s) { x *= s; y *= s; z *= s; return *this; }

    float dot(const Vector3 &o) const { return x * o.x + y * o.y + z * o.z; }
    Vector3 cross(const Vector3 &o) const {
        return {y * o.z - z * o.y, z * o.x - x * o.z, x * o.y - y * o.x};
    }
    float length() const { return std::sqrt(x * x + y * y + z * z); }
    Vector3 normalized() const { float l = length(); return l > 0 ? *this / l : Vector3{0,0,0}; }
    void normalize() { float l = length(); if (l > 0) { x /= l; y /= l; z /= l; } }
    Vector3 floor() const { return {std::floor(x), std::floor(y), std::floor(z)}; }
};

struct Vector2 {
    float x, y;
    Vector2() : x(0), y(0) {}
    Vector2(float x, float y) : x(x), y(y) {}
};

inline float radians(float deg) { return deg * (M_PI / 180.0f); }
inline float clamp(float v, float min, float max) { return v < min ? min : (v > max ? max : v); }

class Matrix4x4 {
public:
    float m[16] = {0};

    Matrix4x4() { m[0] = m[5] = m[10] = m[15] = 1.0f; }

    float *data() { return m; }
    const float *data() const { return m; }

    static Matrix4x4 identity() { return Matrix4x4(); }

    static Matrix4x4 perspective(float fovY, float aspect, float near, float far) {
        Matrix4x4 r;
        float f = 1.0f / std::tan(fovY / 2.0f);
        r.m[0] = f / aspect;
        r.m[5] = f;
        r.m[10] = (far + near) / (near - far);
        r.m[11] = -1.0f;
        r.m[14] = (2.0f * far * near) / (near - far);
        r.m[15] = 0.0f;
        return r;
    }

    static Matrix4x4 lookAt(const Vector3 &eye, const Vector3 &target, const Vector3 &up) {
        Vector3 f = (target - eye).normalized();
        Vector3 s = f.cross(up).normalized();
        Vector3 u = s.cross(f);

        Matrix4x4 r;
        r.m[0] = s.x; r.m[4] = s.y; r.m[8] = s.z; r.m[12] = -s.dot(eye);
        r.m[1] = u.x; r.m[5] = u.y; r.m[9] = u.z; r.m[13] = -u.dot(eye);
        r.m[2] = -f.x; r.m[6] = -f.y; r.m[10] = -f.z; r.m[14] = f.dot(eye);
        r.m[3] = 0; r.m[7] = 0; r.m[11] = 0; r.m[15] = 1;
        return r;
    }

    static Matrix4x4 translation(const Vector3 &t) {
        Matrix4x4 r;
        r.m[12] = t.x; r.m[13] = t.y; r.m[14] = t.z;
        return r;
    }

    Matrix4x4 operator*(const Matrix4x4 &o) const {
        Matrix4x4 r;
        for (int col = 0; col < 4; col++) {
            for (int row = 0; row < 4; row++) {
                r.m[col * 4 + row] =
                    m[0 * 4 + row] * o.m[col * 4 + 0] +
                    m[1 * 4 + row] * o.m[col * 4 + 1] +
                    m[2 * 4 + row] * o.m[col * 4 + 2] +
                    m[3 * 4 + row] * o.m[col * 4 + 3];
            }
        }
        return r;
    }

    Matrix4x4 inverse() const {
        float a00 = m[0], a01 = m[1], a02 = m[2], a03 = m[3];
        float a10 = m[4], a11 = m[5], a12 = m[6], a13 = m[7];
        float a20 = m[8], a21 = m[9], a22 = m[10], a23 = m[11];
        float a30 = m[12], a31 = m[13], a32 = m[14], a33 = m[15];

        float b00 = a00 * a11 - a01 * a10;
        float b01 = a00 * a12 - a02 * a10;
        float b02 = a00 * a13 - a03 * a10;
        float b03 = a01 * a12 - a02 * a11;
        float b04 = a01 * a13 - a03 * a11;
        float b05 = a02 * a13 - a03 * a12;
        float b06 = a20 * a31 - a21 * a30;
        float b07 = a20 * a32 - a22 * a30;
        float b08 = a20 * a33 - a23 * a30;
        float b09 = a21 * a32 - a22 * a31;
        float b10 = a21 * a33 - a23 * a31;
        float b11 = a22 * a33 - a23 * a32;

        float det = b00 * b11 - b01 * b10 + b02 * b09 + b03 * b08 - b04 * b07 + b05 * b06;

        Matrix4x4 r;
        float invDet = 1.0f / det;
        r.m[0] = (a11 * b11 - a12 * b10 + a13 * b09) * invDet;
        r.m[1] = (-a01 * b11 + a02 * b10 - a03 * b09) * invDet;
        r.m[2] = (a31 * b05 - a32 * b04 + a33 * b03) * invDet;
        r.m[3] = (-a21 * b05 + a22 * b04 - a23 * b03) * invDet;
        r.m[4] = (-a10 * b11 + a12 * b08 - a13 * b07) * invDet;
        r.m[5] = (a00 * b11 - a02 * b08 + a03 * b07) * invDet;
        r.m[6] = (-a30 * b05 + a32 * b02 - a33 * b01) * invDet;
        r.m[7] = (a20 * b05 - a22 * b02 + a23 * b01) * invDet;
        r.m[8] = (a10 * b10 - a11 * b08 + a13 * b06) * invDet;
        r.m[9] = (-a00 * b10 + a01 * b08 - a03 * b06) * invDet;
        r.m[10] = (a30 * b04 - a31 * b02 + a33 * b00) * invDet;
        r.m[11] = (-a20 * b04 + a21 * b02 - a23 * b00) * invDet;
        r.m[12] = (-a10 * b09 + a11 * b07 - a12 * b06) * invDet;
        r.m[13] = (a00 * b09 - a01 * b07 + a02 * b06) * invDet;
        r.m[14] = (-a30 * b03 + a31 * b01 - a32 * b00) * invDet;
        r.m[15] = (a20 * b03 - a21 * b01 + a22 * b00) * invDet;
        return r;
    }
};

class Utility {
public:
    static bool checkAndLogGlError(bool alwaysLog = false);
    static inline void assertGlError() { assert(checkAndLogGlError()); }
    static float *buildOrthographicMatrix(float *outMatrix, float halfHeight, float aspect, float near, float far);
    static float *buildIdentityMatrix(float *outMatrix);
};

#endif
