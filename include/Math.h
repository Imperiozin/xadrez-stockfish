#pragma once
#include <algorithm>
#include <cmath>

struct Vec3 {
    float x = 0, y = 0, z = 0;

    Vec3 operator+(Vec3 b) const {
        return {x + b.x, y + b.y, z + b.z};
    }

    Vec3 operator-(Vec3 b) const {
        return {x - b.x, y - b.y, z - b.z};
    }

    Vec3 operator*(float s) const {
        return {x * s, y * s, z * s};
    }
};

inline float dot(Vec3 a, Vec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline Vec3 cross(Vec3 a, Vec3 b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

inline Vec3 normalized(Vec3 a) {
    return a * (1 / std::sqrt(dot(a, a)));
}

struct Mat4 {
    float v[16]{};

    static Mat4 identity() {
        Mat4 m;
        m.v[0] = m.v[5] = m.v[10] = m.v[15] = 1;
        return m;
    }
};

inline Mat4 operator*(const Mat4 &a, const Mat4 &b) {
    Mat4 m;
    for (int c = 0; c < 4; ++c) {
        for (int r = 0; r < 4; ++r) {
            for (int k = 0; k < 4; ++k) {
                m.v[c * 4 + r] += a.v[k * 4 + r] * b.v[c * 4 + k];
            }
        }
    }
    return m;
}

inline Mat4 transform(Vec3 p, Vec3 s, float angle = 0) {
    auto m = Mat4::identity();
    float c = std::cos(angle), n = std::sin(angle);
    m.v[0] = c * s.x;
    m.v[2] = -n * s.x;
    m.v[5] = s.y;
    m.v[8] = n * s.z;
    m.v[10] = c * s.z;
    m.v[12] = p.x;
    m.v[13] = p.y;
    m.v[14] = p.z;
    return m;
}

inline Mat4 perspective(float aspect) {
    Mat4 m;
    float f = 1 / std::tan(.65f / 2);
    m.v[0] = f / aspect;
    m.v[5] = f;
    m.v[10] = -101.f / 99.f;
    m.v[11] = -1;
    m.v[14] = -200.f / 99.f;
    return m;
}

inline Mat4 lookAt(Vec3 eye, Vec3 target = {}) {
    Vec3 f = normalized(target - eye), s = normalized(cross(f, {0, 1, 0})), u = cross(s, f);
    auto m = Mat4::identity();
    m.v[0] = s.x;
    m.v[4] = s.y;
    m.v[8] = s.z;
    m.v[1] = u.x;
    m.v[5] = u.y;
    m.v[9] = u.z;
    m.v[2] = -f.x;
    m.v[6] = -f.y;
    m.v[10] = -f.z;
    m.v[12] = -dot(s, eye);
    m.v[13] = -dot(u, eye);
    m.v[14] = dot(f, eye);
    return m;
}
