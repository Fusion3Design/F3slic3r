///|/ Copyright (c) Prusa Research 2023 Enrico Turri @enricoturri1966
///|/
///|/ libvgcode is released under the terms of the AGPLv3 or higher
///|/
#ifndef VGCODE_TOOLMARKER_HPP
#define VGCODE_TOOLMARKER_HPP

//################################################################################################################################
// PrusaSlicer development only -> !!!TO BE REMOVED!!!
#if ENABLE_NEW_GCODE_VIEWER
//################################################################################################################################

#include "Types.hpp"

#include <cstdint>

namespace libvgcode {

class ToolMarker
{
public:
    ToolMarker() = default;
    ~ToolMarker();
    ToolMarker(const ToolMarker& other) = delete;
    ToolMarker(ToolMarker&& other) = delete;
    ToolMarker& operator = (const ToolMarker& other) = delete;
    ToolMarker& operator = (ToolMarker&& other) = delete;

    void init(uint16_t resolution, float tip_radius, float tip_height, float stem_radius, float stem_height);
    void render();

    const Vec3f& get_position() const;
    void set_position(const Vec3f& position);

    const Color& get_color() const;
    void set_color(const Color& color);

    float get_alpha() const;
    void set_alpha(float alpha);

private:
    Vec3f m_position{ toVec3f(0.0f) };
    Color m_color{ 1.0f, 1.0f, 1.0f };
    float m_alpha{ 0.5f };

    uint16_t m_indices_count{ 0 };
    unsigned int m_vao_id{ 0 };
    unsigned int m_vbo_id{ 0 };
    unsigned int m_ibo_id{ 0 };
};

} // namespace libvgcode

//################################################################################################################################
// PrusaSlicer development only -> !!!TO BE REMOVED!!!
#endif // ENABLE_NEW_GCODE_VIEWER
//################################################################################################################################

#endif // VGCODE_TOOLMARKER_HPP