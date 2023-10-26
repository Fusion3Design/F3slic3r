//################################################################################################################################
// PrusaSlicer development only -> !!!TO BE REMOVED!!!
#include "libslic3r/Technologies.hpp"
//################################################################################################################################

///|/ Copyright (c) Prusa Research 2023 Enrico Turri @enricoturri1966
///|/
///|/ libvgcode is released under the terms of the AGPLv3 or higher
///|/
#include "Utils.hpp"

#include <cmath>

//################################################################################################################################
// PrusaSlicer development only -> !!!TO BE REMOVED!!!
#if ENABLE_NEW_GCODE_VIEWER
//################################################################################################################################

namespace libvgcode {

void add_vertex(const Vec3f& position, const Vec3f& normal, std::vector<float>& vertices)
{
    vertices.emplace_back(position[0]);
    vertices.emplace_back(position[1]);
    vertices.emplace_back(position[2]);
    vertices.emplace_back(normal[0]);
    vertices.emplace_back(normal[1]);
    vertices.emplace_back(normal[2]);
}

void add_triangle(uint16_t v1, uint16_t v2, uint16_t v3, std::vector<uint16_t>& indices)
{
    indices.emplace_back(v1);
    indices.emplace_back(v2);
    indices.emplace_back(v3);
}

Vec3f normalize(const Vec3f& v)
{
    const float length = std::sqrt(dot(v, v));
    assert(length > 0.0f);
    const float inv_length = 1.0f / length;
    return { v[0] * inv_length, v[1] * inv_length, v[2] * inv_length };
}

float dot(const Vec3f& v1, const Vec3f& v2)
{
    return v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2];
}

float length(const Vec3f& v)
{
    return std::sqrt(dot(v, v));
}

bool operator == (const Vec3f& v1, const Vec3f& v2) {
    return v1[0] == v2[0] && v1[1] == v2[1] && v1[2] == v2[2];
}

bool operator != (const Vec3f& v1, const Vec3f& v2) {
    return v1[0] != v2[0] || v1[1] != v2[1] || v1[2] != v2[2];
}

Vec3f operator + (const Vec3f& v1, const Vec3f& v2) {
    return { v1[0] + v2[0], v1[1] + v2[1], v1[2] + v2[2] };
}

Vec3f operator - (const Vec3f& v1, const Vec3f& v2) {
    return { v1[0] - v2[0], v1[1] - v2[1], v1[2] - v2[2] };
}

Vec3f operator * (float f, const Vec3f& v) {
    return { f * v[0], f * v[1], f * v[2] };
}

} // namespace libvgcode

//################################################################################################################################
// PrusaSlicer development only -> !!!TO BE REMOVED!!!
#endif // ENABLE_NEW_GCODE_VIEWER
//################################################################################################################################
