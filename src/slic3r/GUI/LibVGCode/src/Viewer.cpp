//################################################################################################################################
// PrusaSlicer development only -> !!!TO BE REMOVED!!!
#include "libslic3r/Technologies.hpp"
//################################################################################################################################

///|/ Copyright (c) Prusa Research 2023 Enrico Turri @enricoturri1966, Pavel Mikuš @Godrak
///|/
///|/ libvgcode is released under the terms of the AGPLv3 or higher
///|/
#include "../include/Viewer.hpp"
#include "ViewerImpl.hpp"

//################################################################################################################################
// PrusaSlicer development only -> !!!TO BE REMOVED!!!
#if ENABLE_NEW_GCODE_VIEWER
//################################################################################################################################

namespace libvgcode {

Viewer::Viewer()
{
    m_impl = new ViewerImpl();
}

Viewer::~Viewer()
{
    delete m_impl;
}

void Viewer::init()
{
    m_impl->init();
}

void Viewer::reset()
{
    m_impl->reset();
}

void Viewer::load(GCodeInputData&& gcode_data)
{
    m_impl->load(std::move(gcode_data));
}

void Viewer::render(const Mat4x4& view_matrix, const Mat4x4& projection_matrix)
{
    m_impl->render(view_matrix, projection_matrix);
}

EViewType Viewer::get_view_type() const
{
    return m_impl->get_view_type();
}

void Viewer::set_view_type(EViewType type)
{
    m_impl->set_view_type(type);
}

ETimeMode Viewer::get_time_mode() const
{
    return m_impl->get_time_mode();
}

void Viewer::set_time_mode(ETimeMode mode)
{
    m_impl->set_time_mode(mode);
}

const std::array<uint32_t, 2>& Viewer::get_layers_view_range() const
{
    return m_impl->get_layers_view_range();
}

void Viewer::set_layers_view_range(const std::array<uint32_t, 2>& range)
{
    m_impl->set_layers_view_range(range);
}

void Viewer::set_layers_view_range(uint32_t min, uint32_t max)
{
    m_impl->set_layers_view_range(min, max);
}

bool Viewer::is_top_layer_only_view_range() const
{
    return m_impl->is_top_layer_only_view_range();
}

void Viewer::set_top_layer_only_view_range(bool top_layer_only_view_range)
{
    m_impl->set_top_layer_only_view_range(top_layer_only_view_range);
}

size_t Viewer::get_layers_count() const
{
    return m_impl->get_layers_count();
}

float Viewer::get_layer_z(size_t layer_id) const
{
    return m_impl->get_layer_z(layer_id);
}

std::vector<float> Viewer::get_layers_zs() const
{
    return m_impl->get_layers_zs();
}

size_t Viewer::get_layer_id_at(float z) const
{
    return m_impl->get_layer_id_at(z);
}

size_t Viewer::get_used_extruders_count() const
{
    return m_impl->get_used_extruders_count();
}

const std::vector<uint8_t>& Viewer::get_used_extruders_ids() const
{
    return m_impl->get_used_extruders_ids();
}

AABox Viewer::get_bounding_box(EBBoxType type) const
{
    return m_impl->get_bounding_box(type);
}

bool Viewer::is_option_visible(EOptionType type) const
{
    return m_impl->is_option_visible(type);
}

void Viewer::toggle_option_visibility(EOptionType type)
{
    m_impl->toggle_option_visibility(type);
}

bool Viewer::is_extrusion_role_visible(EGCodeExtrusionRole role) const
{
    return m_impl->is_extrusion_role_visible(role);
}

void Viewer::toggle_extrusion_role_visibility(EGCodeExtrusionRole role)
{
    m_impl->toggle_extrusion_role_visibility(role);
}

const std::array<uint32_t, 2>& Viewer::get_view_full_range() const
{
    return m_impl->get_view_full_range();
}

const std::array<uint32_t, 2>& Viewer::get_view_enabled_range() const
{
    return m_impl->get_view_enabled_range();
}

const std::array<uint32_t, 2>& Viewer::get_view_visible_range() const
{
    return m_impl->get_view_visible_range();
}

void Viewer::set_view_visible_range(uint32_t min, uint32_t max)
{
    m_impl->set_view_visible_range(min, max);
}

uint32_t Viewer::get_vertices_count() const
{
    return m_impl->get_vertices_count();
}

PathVertex Viewer::get_current_vertex() const
{
    return m_impl->get_current_vertex();
}

PathVertex Viewer::get_vertex_at(uint32_t id) const
{
    return m_impl->get_vertex_at(id);
}

size_t Viewer::get_enabled_segments_count() const
{
    return m_impl->get_enabled_segments_count();
}

const std::array<uint32_t, 2>& Viewer::get_enabled_segments_range() const
{
    return m_impl->get_enabled_segments_range();
}

size_t Viewer::get_enabled_options_count() const
{
    return m_impl->get_enabled_options_count();
}

const std::array<uint32_t, 2>& Viewer::get_enabled_options_range() const
{
    return m_impl->get_enabled_options_range();
}

uint32_t Viewer::get_extrusion_roles_count() const
{
    return m_impl->get_extrusion_roles_count();
}

std::vector<EGCodeExtrusionRole> Viewer::get_extrusion_roles() const
{
    return m_impl->get_extrusion_roles();
}

float Viewer::get_extrusion_role_time(EGCodeExtrusionRole role) const
{
    return m_impl->get_extrusion_role_time(role);
}

float Viewer::get_extrusion_role_time(EGCodeExtrusionRole role, ETimeMode mode) const
{
    return m_impl->get_extrusion_role_time(role, mode);
}

float Viewer::get_travels_time() const
{
    return m_impl->get_travels_time();
}

float Viewer::get_travels_time(ETimeMode mode) const
{
    return m_impl->get_travels_time(mode);
}

std::vector<float> Viewer::get_layers_times() const
{
    return m_impl->get_layers_times();
}

std::vector<float> Viewer::get_layers_times(ETimeMode mode) const
{
    return m_impl->get_layers_times(mode);
}

size_t Viewer::get_tool_colors_count() const
{
    return m_impl->get_tool_colors_count();
}

const std::vector<Color>& Viewer::get_tool_colors() const
{
    return m_impl->get_tool_colors();
}

void Viewer::set_tool_colors(const std::vector<Color>& colors)
{
    m_impl->set_tool_colors(colors);
}

const Color& Viewer::get_extrusion_role_color(EGCodeExtrusionRole role) const
{
    return m_impl->get_extrusion_role_color(role);
}

void Viewer::set_extrusion_role_color(EGCodeExtrusionRole role, const Color& color)
{
    m_impl->set_extrusion_role_color(role, color);
}

void Viewer::reset_default_extrusion_roles_colors()
{
    m_impl->reset_default_extrusion_roles_colors();
}

const Color& Viewer::get_option_color(EOptionType type) const
{
    return m_impl->get_option_color(type);
}

void Viewer::set_option_color(EOptionType type, const Color& color)
{
    m_impl->set_option_color(type, color);
}

void Viewer::reset_default_options_colors()
{
    m_impl->reset_default_options_colors();
}

const ColorRange& Viewer::get_height_range() const
{
    return m_impl->get_height_range();
}

const ColorRange& Viewer::get_width_range() const
{
    return m_impl->get_width_range();
}

const ColorRange& Viewer::get_speed_range() const
{
    return m_impl->get_speed_range();
}

const ColorRange& Viewer::get_fan_speed_range() const
{
    return m_impl->get_fan_speed_range();
}

const ColorRange& Viewer::get_temperature_range() const
{
    return m_impl->get_temperature_range();
}

const ColorRange& Viewer::get_volumetric_rate_range() const
{
    return m_impl->get_volumetric_rate_range();
}

const ColorRange& Viewer::get_layer_time_range(EColorRangeType type) const
{
    return m_impl->get_layer_time_range(type);
}

float Viewer::get_travels_radius() const
{
    return m_impl->get_travels_radius();
}

void Viewer::set_travels_radius(float radius)
{
    m_impl->set_travels_radius(radius);
}

float Viewer::get_wipes_radius() const
{
    return m_impl->get_wipes_radius();
}

void Viewer::set_wipes_radius(float radius)
{
    m_impl->set_wipes_radius(radius);
}

#if !ENABLE_NEW_GCODE_VIEWER_NO_COG_AND_TOOL_MARKERS
Vec3 Viewer::get_cog_position() const
{
    return m_impl->get_cog_marker_position();
}

float Viewer::get_cog_marker_scale_factor() const
{
    return m_impl->get_cog_marker_scale_factor();
}

void Viewer::set_cog_marker_scale_factor(float factor)
{
    m_impl->set_cog_marker_scale_factor(factor);
}

bool Viewer::is_tool_marker_enabled() const
{
    return m_impl->is_tool_marker_enabled();
}

void Viewer::enable_tool_marker(bool value)
{
    m_impl->enable_tool_marker(value);
}

const Vec3& Viewer::get_tool_marker_position() const
{
    return m_impl->get_tool_marker_position();
}

void Viewer::set_tool_marker_position(const Vec3& position)
{
    m_impl->set_tool_marker_position(position);
}

float Viewer::get_tool_marker_offset_z() const
{
    return m_impl->get_tool_marker_offset_z();
}

void Viewer::set_tool_marker_offset_z(float offset_z)
{
    m_impl->set_tool_marker_offset_z(offset_z);
}

float Viewer::get_tool_marker_scale_factor() const
{
    return m_impl->get_tool_marker_scale_factor();
}

void Viewer::set_tool_marker_scale_factor(float factor)
{
    m_impl->set_tool_marker_scale_factor(factor);
}

const Color& Viewer::get_tool_marker_color() const
{
    return m_impl->get_tool_marker_color();
}

void Viewer::set_tool_marker_color(const Color& color)
{
    m_impl->set_tool_marker_color(color);
}

float Viewer::get_tool_marker_alpha() const
{
    return m_impl->get_tool_marker_alpha();
}

void Viewer::set_tool_marker_alpha(float alpha)
{
    m_impl->set_tool_marker_alpha(alpha);
}
#endif // !ENABLE_NEW_GCODE_VIEWER_NO_COG_AND_TOOL_MARKERS

} // namespace libvgcode

//################################################################################################################################
// PrusaSlicer development only -> !!!TO BE REMOVED!!!
#endif // ENABLE_NEW_GCODE_VIEWER
//################################################################################################################################
