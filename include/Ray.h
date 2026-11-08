#pragma once
#include "Math3D.h"

struct Ray {
    Vec3 origin, direction;
};

// A direção local não é normalizada: t continua sendo a distância no mundo.
inline Ray transformRay(const Mat4 &inverse, const Ray &ray) {
    return {transformPoint(inverse, ray.origin), transformDirection(inverse, ray.direction)};
}

inline bool intersectBox(const Ray &ray, Vec3 low, Vec3 high, float &distance) {
    float near = 0, far = 1e30f;
    const float origin[] = {ray.origin.x, ray.origin.y, ray.origin.z};
    const float direction[] = {ray.direction.x, ray.direction.y, ray.direction.z};
    const float minimum[] = {low.x, low.y, low.z}, maximum[] = {high.x, high.y, high.z};
    for (int i = 0; i < 3; ++i) {
        if (std::abs(direction[i]) < 1e-12f) {
            if (origin[i] < minimum[i] || origin[i] > maximum[i]) return false;
            continue;
        }
        float a = (minimum[i] - origin[i]) / direction[i];
        float b = (maximum[i] - origin[i]) / direction[i];
        if (a > b) std::swap(a, b);
        near = std::max(near, a);
        far = std::min(far, b);
        if (near > far) return false;
    }
    distance = near;
    return far > 0;
}

inline bool intersectTriangle(const Ray &ray, Vec3 a, Vec3 b, Vec3 c, float &distance) {
    const Vec3 edge1 = b - a, edge2 = c - a;
    const Vec3 p = cross(ray.direction, edge2);
    const float determinant = dot(edge1, p);
    if (std::abs(determinant) < 1e-9f) return false;
    const float inverse = 1 / determinant;
    const Vec3 offset = ray.origin - a;
    const float u = dot(offset, p) * inverse;
    if (u < 0 || u > 1) return false;
    const Vec3 q = cross(offset, edge1);
    const float v = dot(ray.direction, q) * inverse;
    if (v < 0 || u + v > 1) return false;
    distance = dot(edge2, q) * inverse;
    return distance > 0;
}

struct PageSurface {
    Vec3 topLeft, bottomRight;

    bool intersect(const Ray &localRay, Vec2 &uv, float &distance) const {
        if (localRay.direction.y >= -1e-8f || bottomRight.x <= topLeft.x ||
            bottomRight.z <= topLeft.z) return false;
        distance = (topLeft.y - localRay.origin.y) / localRay.direction.y;
        if (distance <= 0) return false;
        const Vec3 p = localRay.origin + localRay.direction * distance;
        uv = {(p.x - topLeft.x) / (bottomRight.x - topLeft.x),
              (p.z - topLeft.z) / (bottomRight.z - topLeft.z)};
        return uv.x >= 0 && uv.x <= 1 && uv.y >= 0 && uv.y <= 1;
    }
};
