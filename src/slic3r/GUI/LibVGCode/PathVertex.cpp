//################################################################################################################################
// PrusaSlicer development only -> !!!TO BE REMOVED!!!
#include "libslic3r/Technologies.hpp"
//################################################################################################################################

///|/ Copyright (c) Prusa Research 2023 Enrico Turri @enricoturri1966, Pavel Mikuš @Godrak
///|/
///|/ libvgcode is released under the terms of the AGPLv3 or higher
///|/
#include "PathVertex.hpp"

//################################################################################################################################
// PrusaSlicer development only -> !!!TO BE REMOVED!!!
#if ENABLE_NEW_GCODE_VIEWER
//################################################################################################################################

namespace libvgcode {

PathVertex::PathVertex(const Vec3f& position, float height, float width, float feedrate, float fan_speed,
    float temperature, float volumetric_rate, EGCodeExtrusionRole role, EMoveType type, uint8_t extruder_id,
    uint8_t color_id, uint32_t layer_id)
: m_position(position)
, m_height(height)
, m_width(width)
, m_feedrate(feedrate)
, m_fan_speed(fan_speed)
, m_temperature(temperature)
, m_volumetric_rate(volumetric_rate)
, m_extruder_id(extruder_id)
, m_color_id(color_id)
, m_layer_id(layer_id)
, m_type(type)
, m_role(role)
{
}

const Vec3f& PathVertex::get_position() const
{
    return m_position;
}

float PathVertex::get_height() const
{
    return m_height;
}

float PathVertex::get_width() const
{
    return m_width;
}

float PathVertex::get_feedrate() const
{
    return m_feedrate;
}

float PathVertex::get_fan_speed() const
{
    return m_fan_speed;
}

float PathVertex::get_temperature() const
{
    return m_temperature;
}

float PathVertex::get_volumetric_rate() const
{
    return m_volumetric_rate;
}

EMoveType PathVertex::get_type() const
{
    return m_type;
}

EGCodeExtrusionRole PathVertex::get_role() const
{
    return m_role;
}

uint8_t PathVertex::get_extruder_id() const
{
    return m_extruder_id;
}

uint8_t PathVertex::get_color_id() const
{
    return m_color_id;
}

uint32_t PathVertex::get_layer_id() const
{
    return m_layer_id;
}

bool PathVertex::is_extrusion() const
{
    return m_type == EMoveType::Extrude;
}

bool PathVertex::is_travel() const
{
    return m_type == EMoveType::Travel;
}

bool PathVertex::is_wipe() const
{
    return m_type == EMoveType::Wipe;
}

bool PathVertex::is_option() const
{
    switch (m_type)
    {
    case EMoveType::Retract:
    case EMoveType::Unretract:
    case EMoveType::Seam:
    case EMoveType::ToolChange:
    case EMoveType::ColorChange:
    case EMoveType::PausePrint:
    case EMoveType::CustomGCode:
    {
        return true;
    }
    default:
    {
        return false;
    }
    }
}

bool PathVertex::is_custom_gcode() const
{
    return m_type == EMoveType::Extrude && m_role == EGCodeExtrusionRole::Custom;
}

} // namespace libvgcode

//################################################################################################################################
// PrusaSlicer development only -> !!!TO BE REMOVED!!!
#endif // ENABLE_NEW_GCODE_VIEWER
//################################################################################################################################
