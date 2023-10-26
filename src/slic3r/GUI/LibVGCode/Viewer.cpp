//################################################################################################################################
// PrusaSlicer development only -> !!!TO BE REMOVED!!!
#include "libslic3r/Technologies.hpp"
//################################################################################################################################

///|/ Copyright (c) Prusa Research 2023 Enrico Turri @enricoturri1966, Pavel Mikuš @Godrak
///|/
///|/ libvgcode is released under the terms of the AGPLv3 or higher
///|/
#include "Viewer.hpp"

//################################################################################################################################
// PrusaSlicer development only -> !!!TO BE REMOVED!!!
#if ENABLE_NEW_GCODE_VIEWER

#include "libslic3r/GCode/GCodeProcessor.hpp"
//################################################################################################################################

namespace libvgcode {

void Viewer::init()
{
    m_toolpaths.init();
}

void Viewer::load(const Slic3r::GCodeProcessorResult& gcode_result, const std::vector<std::string>& str_tool_colors)
{
    m_toolpaths.load(gcode_result, str_tool_colors);
}

void Viewer::render(const Mat4x4f& view_matrix, const Mat4x4f& projection_matrix)
{
    m_toolpaths.render(view_matrix, projection_matrix);
}

EViewType Viewer::get_view_type() const
{
    return m_toolpaths.get_view_type();
}

void Viewer::set_view_type(EViewType type)
{
    m_toolpaths.set_view_type(type);
}

ETimeMode Viewer::get_time_mode() const
{
    return m_toolpaths.get_time_mode();
}

void Viewer::set_time_mode(ETimeMode mode)
{
    m_toolpaths.set_time_mode(mode);
}

const std::array<std::vector<float>, static_cast<size_t>(ETimeMode::COUNT)>& Viewer::get_layers_times() const
{
    return m_toolpaths.get_layers_times();
}

bool Viewer::is_option_visible(EOptionType type) const
{
    return m_toolpaths.is_option_visible(type);
}

void Viewer::toggle_option_visibility(EOptionType type)
{
    m_toolpaths.toggle_option_visibility(type);
}

bool Viewer::is_extrusion_role_visible(EGCodeExtrusionRole role) const
{
    return m_toolpaths.is_extrusion_role_visible(role);
}

void Viewer::toggle_extrusion_role_visibility(EGCodeExtrusionRole role)
{
    m_toolpaths.toggle_extrusion_role_visibility(role);
}

const std::array<size_t, 2>& Viewer::get_view_current_range() const
{
    return m_toolpaths.get_view_current_range();
}

const std::array<size_t, 2>& Viewer::get_view_global_range() const
{
    return m_toolpaths.get_view_global_range();
}

void Viewer::set_view_current_range(size_t min, size_t max)
{
    m_toolpaths.set_view_current_range(min, max);
}

Vec3f Viewer::get_cog_position() const
{
    return m_toolpaths.get_cog_marker_position();
}

float Viewer::get_cog_marker_scale_factor() const
{
    return m_toolpaths.get_cog_marker_scale_factor();
}

void Viewer::set_cog_marker_scale_factor(float factor)
{
    m_toolpaths.set_cog_marker_scale_factor(factor);
}

const Vec3f& Viewer::get_tool_marker_position() const
{
    return m_toolpaths.get_tool_marker_position();
}

void Viewer::set_tool_marker_position(const Vec3f& position)
{
    m_toolpaths.set_tool_marker_position(position);
}

float Viewer::get_tool_marker_scale_factor() const
{
    return m_toolpaths.get_tool_marker_scale_factor();
}

void Viewer::set_tool_marker_scale_factor(float factor)
{
    m_toolpaths.set_tool_marker_scale_factor(factor);
}

const Color& Viewer::get_tool_marker_color() const
{
    return m_toolpaths.get_tool_marker_color();
}

void Viewer::set_tool_marker_color(const Color& color)
{
    m_toolpaths.set_tool_marker_color(color);
}

float Viewer::get_tool_marker_alpha() const
{
    return m_toolpaths.get_tool_marker_alpha();
}

void Viewer::set_tool_marker_alpha(float alpha)
{
    m_toolpaths.set_tool_marker_alpha(alpha);
}

} // namespace libvgcode

//################################################################################################################################
// PrusaSlicer development only -> !!!TO BE REMOVED!!!
#endif // ENABLE_NEW_GCODE_VIEWER
//################################################################################################################################
