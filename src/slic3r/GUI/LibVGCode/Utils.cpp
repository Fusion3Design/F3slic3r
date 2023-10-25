//################################################################################################################################
// PrusaSlicer development only -> !!!TO BE REMOVED!!!
#include "libslic3r/Technologies.hpp"
//################################################################################################################################

///|/ Copyright (c) Prusa Research 2023 Enrico Turri @enricoturri1966
///|/
///|/ libvgcode is released under the terms of the AGPLv3 or higher
///|/
#include "Utils.hpp"

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

} // namespace libvgcode

//################################################################################################################################
// PrusaSlicer development only -> !!!TO BE REMOVED!!!
#endif // ENABLE_NEW_GCODE_VIEWER
//################################################################################################################################
