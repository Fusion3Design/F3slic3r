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
    m_impl.init();
}

void Viewer::load(const Slic3r::GCodeProcessorResult& gcode_result, const std::vector<std::string>& str_tool_colors)
{
    m_impl.load(gcode_result, str_tool_colors);
}

void Viewer::render(const Mat4x4f& view_matrix, const Mat4x4f& projection_matrix)
{
    m_impl.render(view_matrix, projection_matrix);
}

EViewType Viewer::get_view_type() const
{
    return m_impl.get_view_type();
}

void Viewer::set_view_type(EViewType type)
{
    m_impl.set_view_type(type);
}

ETimeMode Viewer::get_time_mode() const
{
    return m_impl.get_time_mode();
}

void Viewer::set_time_mode(ETimeMode mode)
{
    m_impl.set_time_mode(mode);
}

const std::array<uint32_t, 2>& Viewer::get_layers_range() const
{
    return m_impl.get_layers_range();
}

void Viewer::set_layers_range(const std::array<uint32_t, 2>& range)
{
    m_impl.set_layers_range(range);
}

void Viewer::set_layers_range(uint32_t min, uint32_t max)
{
    m_impl.set_layers_range(min, max);
}

bool Viewer::is_top_layer_only_view() const
{
    return m_impl.is_top_layer_only_view();
}

void Viewer::set_top_layer_only_view(bool top_layer_only_view)
{
    m_impl.set_top_layer_only_view(top_layer_only_view);
}

bool Viewer::is_option_visible(EOptionType type) const
{
    return m_impl.is_option_visible(type);
}

void Viewer::toggle_option_visibility(EOptionType type)
{
    m_impl.toggle_option_visibility(type);
}

bool Viewer::is_extrusion_role_visible(EGCodeExtrusionRole role) const
{
    return m_impl.is_extrusion_role_visible(role);
}

void Viewer::toggle_extrusion_role_visibility(EGCodeExtrusionRole role)
{
    m_impl.toggle_extrusion_role_visibility(role);
}

const std::array<uint32_t, 2>& Viewer::get_view_current_range() const
{
    return m_impl.get_view_current_range();
}

const std::array<uint32_t, 2>& Viewer::get_view_global_range() const
{
    return m_impl.get_view_global_range();
}

void Viewer::set_view_current_range(uint32_t min, uint32_t max)
{
    m_impl.set_view_current_range(min, max);
}

uint32_t Viewer::get_vertices_count() const
{
    return m_impl.get_vertices_count();
}

PathVertex Viewer::get_current_vertex() const
{
    return m_impl.get_current_vertex();
}

PathVertex Viewer::get_vertex_at(uint32_t id) const
{
    return m_impl.get_vertex_at(id);
}

uint32_t Viewer::get_extrusion_roles_count() const
{
    return m_impl.get_extrusion_roles_count();
}

std::vector<EGCodeExtrusionRole> Viewer::get_extrusion_roles() const
{
    return m_impl.get_extrusion_roles();
}

float Viewer::get_extrusion_role_time(EGCodeExtrusionRole role) const
{
    return m_impl.get_extrusion_role_time(role);
}

float Viewer::get_extrusion_role_time(EGCodeExtrusionRole role, ETimeMode mode) const
{
    return m_impl.get_extrusion_role_time(role, mode);
}

float Viewer::get_travels_time() const
{
    return m_impl.get_travels_time();
}

float Viewer::get_travels_time(ETimeMode mode) const
{
    return m_impl.get_travels_time(mode);
}

std::vector<float> Viewer::get_layers_times() const
{
    return m_impl.get_layers_times();
}

std::vector<float> Viewer::get_layers_times(ETimeMode mode) const
{
    return m_impl.get_layers_times(mode);
}

#if !ENABLE_NEW_GCODE_NO_COG_AND_TOOL_MARKERS
Vec3f Viewer::get_cog_position() const
{
    return m_impl.get_cog_marker_position();
}

float Viewer::get_cog_marker_scale_factor() const
{
    return m_impl.get_cog_marker_scale_factor();
}

void Viewer::set_cog_marker_scale_factor(float factor)
{
    m_impl.set_cog_marker_scale_factor(factor);
}

bool Viewer::is_tool_marker_enabled() const
{
    return m_impl.is_tool_marker_enabled();
}

void Viewer::enable_tool_marker(bool value)
{
    m_impl.enable_tool_marker(value);
}

const Vec3f& Viewer::get_tool_marker_position() const
{
    return m_impl.get_tool_marker_position();
}

void Viewer::set_tool_marker_position(const Vec3f& position)
{
    m_impl.set_tool_marker_position(position);
}

float Viewer::get_tool_marker_scale_factor() const
{
    return m_impl.get_tool_marker_scale_factor();
}

void Viewer::set_tool_marker_scale_factor(float factor)
{
    m_impl.set_tool_marker_scale_factor(factor);
}

const Color& Viewer::get_tool_marker_color() const
{
    return m_impl.get_tool_marker_color();
}

void Viewer::set_tool_marker_color(const Color& color)
{
    m_impl.set_tool_marker_color(color);
}

float Viewer::get_tool_marker_alpha() const
{
    return m_impl.get_tool_marker_alpha();
}

void Viewer::set_tool_marker_alpha(float alpha)
{
    m_impl.set_tool_marker_alpha(alpha);
}
#endif // !ENABLE_NEW_GCODE_NO_COG_AND_TOOL_MARKERS

} // namespace libvgcode

//################################################################################################################################
// PrusaSlicer development only -> !!!TO BE REMOVED!!!
#endif // ENABLE_NEW_GCODE_VIEWER
//################################################################################################################################
