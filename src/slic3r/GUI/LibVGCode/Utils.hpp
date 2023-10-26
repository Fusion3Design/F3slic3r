///|/ Copyright (c) Prusa Research 2023 Enrico Turri @enricoturri1966
///|/
///|/ libvgcode is released under the terms of the AGPLv3 or higher
///|/
#ifndef VGCODE_UTILS_HPP
#define VGCODE_UTILS_HPP

//################################################################################################################################
// PrusaSlicer development only -> !!!TO BE REMOVED!!!
#if ENABLE_NEW_GCODE_VIEWER
//################################################################################################################################

#include "Types.hpp"

#include <cstdint>
#include <vector>

namespace libvgcode {

extern void add_vertex(const Vec3f& position, const Vec3f& normal, std::vector<float>& vertices);
extern void add_triangle(uint16_t v1, uint16_t v2, uint16_t v3, std::vector<uint16_t>& indices);
extern Vec3f normalize(const Vec3f& v);
extern float dot(const Vec3f& v1, const Vec3f& v2);
extern float length(const Vec3f& v);
extern bool operator == (const Vec3f& v1, const Vec3f& v2);
extern bool operator != (const Vec3f& v1, const Vec3f& v2);
extern Vec3f operator + (const Vec3f& v1, const Vec3f& v2);
extern Vec3f operator - (const Vec3f& v1, const Vec3f& v2);
extern Vec3f operator * (float f, const Vec3f& v);

} // namespace libvgcode

//################################################################################################################################
// PrusaSlicer development only -> !!!TO BE REMOVED!!!
#endif // ENABLE_NEW_GCODE_VIEWER
//################################################################################################################################

#endif // VGCODE_UTILS_HPP