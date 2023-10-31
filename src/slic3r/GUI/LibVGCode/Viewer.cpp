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

const std::array<std::vector<float>, static_cast<size_t>(ETimeMode::COUNT)>& Viewer::get_layers_times() const
{
    return m_impl.get_layers_times();
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
#endif // !ENABLE_NEW_GCODE_NO_COG_AND_TOOL_MARKERS

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

} // namespace libvgcode

//################################################################################################################################
// PrusaSlicer development only -> !!!TO BE REMOVED!!!
#endif // ENABLE_NEW_GCODE_VIEWER
//################################################################################################################################
