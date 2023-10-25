///|/ Copyright (c) Prusa Research 2023 Enrico Turri @enricoturri1966, Pavel Mikuš @Godrak
///|/
///|/ libvgcode is released under the terms of the AGPLv3 or higher
///|/
#ifndef VGCODE_PATHVERTEX_HPP
#define VGCODE_PATHVERTEX_HPP

//################################################################################################################################
// PrusaSlicer development only -> !!!TO BE REMOVED!!!
#if ENABLE_NEW_GCODE_VIEWER
//################################################################################################################################

#include "Types.hpp"

namespace libvgcode {

class PathVertex
{
public:
    PathVertex(const Vec3f& position, float height, float width, float feedrate, float fan_speed,
        float temperature, float volumetric_rate, EGCodeExtrusionRole role, EMoveType type,
        uint8_t extruder_id, uint8_t color_id, uint32_t layer_id);

    const Vec3f& get_position() const;
    float get_height() const;
    float get_width() const;
    float get_feedrate() const;
    float get_fan_speed() const;
    float get_temperature() const;
    float get_volumetric_rate() const;
    EMoveType get_type() const;
    EGCodeExtrusionRole get_role() const;
    uint8_t get_extruder_id() const;
    uint8_t get_color_id() const;
    uint32_t get_layer_id() const;

    bool is_extrusion() const;
    bool is_travel() const;
    bool is_option() const;
    bool is_wipe() const;
    bool is_custom_gcode() const;

private:
    Vec3f m_position{ toVec3f(0.0f) };
    float m_height{ 0.0f };
    float m_width{ 0.0f };
    float m_feedrate{ 0.0f };
    float m_fan_speed{ 0.0f };
    float m_temperature{ 0.0f };
    float m_volumetric_rate{ 0.0f };
    uint8_t m_extruder_id{ 0 };
    uint8_t m_color_id{ 0 };
    uint32_t m_layer_id{ 0 };
    EMoveType m_type{ EMoveType::Noop };
    EGCodeExtrusionRole m_role{ EGCodeExtrusionRole::None };
};

} // namespace libvgcode

//################################################################################################################################
// PrusaSlicer development only -> !!!TO BE REMOVED!!!
#endif // ENABLE_NEW_GCODE_VIEWER
//################################################################################################################################

#endif // VGCODE_PATHVERTEX_HPP