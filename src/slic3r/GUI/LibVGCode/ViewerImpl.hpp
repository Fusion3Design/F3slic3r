///|/ Copyright (c) Prusa Research 2023 Enrico Turri @enricoturri1966, Pavel Mikuš @Godrak
///|/
///|/ libvgcode is released under the terms of the AGPLv3 or higher
///|/
#ifndef VGCODE_VIEWERIMPL_HPP
#define VGCODE_VIEWERIMPL_HPP

//################################################################################################################################
// PrusaSlicer development only -> !!!TO BE REMOVED!!!
#if ENABLE_NEW_GCODE_VIEWER
//################################################################################################################################

#include "Settings.hpp"
#include "SegmentTemplate.hpp"
#include "OptionTemplate.hpp"
#if !ENABLE_NEW_GCODE_NO_COG_AND_TOOL_MARKERS
#include "CogMarker.hpp"
#include "ToolMarker.hpp"
#endif // !ENABLE_NEW_GCODE_NO_COG_AND_TOOL_MARKERS
#include "PathVertex.hpp"
#include "Bitset.hpp"
#include "ColorRange.hpp"
#include "ViewRange.hpp"
#include "Layers.hpp"

#include <vector>
#include <array>

//################################################################################################################################
// PrusaSlicer development only -> !!!TO BE REMOVED!!!
namespace Slic3r {
struct GCodeProcessorResult;
class Print;
} // namespace Slic3r
//################################################################################################################################

namespace libvgcode {

class ViewerImpl
{
public:
    ViewerImpl() = default;
    ~ViewerImpl();
    ViewerImpl(const ViewerImpl& other) = delete;
    ViewerImpl(ViewerImpl&& other) = delete;
    ViewerImpl& operator = (const ViewerImpl& other) = delete;
    ViewerImpl& operator = (ViewerImpl&& other) = delete;

    //
    // Initialize shader, uniform indices and segment geometry
    //
    void init();

    //
    // Setup all the variables used for visualization and coloring of the toolpaths
    // from the gcode moves contained in the given gcode_result.
    //
    void load(const Slic3r::GCodeProcessorResult& gcode_result, const std::vector<std::string>& str_tool_colors);

    //
    // Update the visibility property of toolpaths
    //
    void update_enabled_entities();
    //
    // Update the color of toolpaths
    //
    void update_colors();

    //
    // Render the toolpaths
    //
    void render(const Mat4x4f& view_matrix, const Mat4x4f& projection_matrix);

    //
    // Settings getters
    //
    EViewType get_view_type() const;
    ETimeMode get_time_mode() const;
    const std::array<uint32_t, 2>& get_layers_range() const;
    bool is_top_layer_only_view() const;
    bool is_option_visible(EOptionType type) const;
    bool is_extrusion_role_visible(EGCodeExtrusionRole role) const;

    //
    // Settings setters
    //
    void set_view_type(EViewType type);
    void set_time_mode(ETimeMode mode);
    void set_layers_range(const std::array<uint32_t, 2>& range);
    void set_layers_range(uint32_t min, uint32_t max);
    void set_top_layer_only_view(bool top_layer_only_view);
    void toggle_option_visibility(EOptionType type);
    void toggle_extrusion_role_visibility(EGCodeExtrusionRole role);

    //
    // View range getters
    //
    const std::array<uint32_t, 2>& get_view_current_range() const;
    const std::array<uint32_t, 2>& get_view_global_range() const;

    //
    // View range setters
    //
    void set_view_current_range(uint32_t min, uint32_t max);

    //
    // Properties getters
    //
    const std::array<std::vector<float>, static_cast<size_t>(ETimeMode::COUNT)>& get_layers_times() const;
#if !ENABLE_NEW_GCODE_NO_COG_AND_TOOL_MARKERS
    Vec3f get_cog_marker_position() const;
    float get_cog_marker_scale_factor() const;
    bool is_tool_marker_enabled() const;
    const Vec3f& get_tool_marker_position() const;
    float get_tool_marker_offset_z() const;
    float get_tool_marker_scale_factor() const;
    const Color& get_tool_marker_color() const;
    float get_tool_marker_alpha() const;
#endif // !ENABLE_NEW_GCODE_NO_COG_AND_TOOL_MARKERS

    //
    // Properties setters
    //
#if !ENABLE_NEW_GCODE_NO_COG_AND_TOOL_MARKERS
    void set_cog_marker_scale_factor(float factor);
    void enable_tool_marker(bool value);
    void set_tool_marker_position(const Vec3f& position);
    void set_tool_marker_offset_z(float offset_z);
    void set_tool_marker_scale_factor(float factor);
    void set_tool_marker_color(const Color& color);
    void set_tool_marker_alpha(float size);
#endif // !ENABLE_NEW_GCODE_NO_COG_AND_TOOL_MARKERS

private:
    Settings m_settings;
    Layers m_layers;
    Range m_layers_range;
    ViewRange m_view_range;
    Range m_old_current_range;

    //
    // The OpenGL element used to represent all toolpath segments
    //
    SegmentTemplate m_segment_template;

    //
    // The OpenGL element used to represent all option markers
    //
    OptionTemplate m_option_template;

#if !ENABLE_NEW_GCODE_NO_COG_AND_TOOL_MARKERS
    //
    // The OpenGL element used to represent the center of gravity
    //
    CogMarker m_cog_marker;
    float m_cog_marker_scale_factor{ 1.0f };

    //
    // The OpenGL element used to represent the tool nozzle
    //
    ToolMarker m_tool_marker;
    float m_tool_marker_scale_factor{ 1.0f };
#endif // !ENABLE_NEW_GCODE_NO_COG_AND_TOOL_MARKERS

    //
    // cpu buffer to store vertices
    //
    std::vector<PathVertex> m_vertices;
    std::vector<uint32_t> m_vertices_map;
#if ENABLE_NEW_GCODE_VIEWER_DEBUG
    std::pair<uint32_t, uint32_t> m_enabled_segments_range{ 0, 0 };
    std::pair<uint32_t, uint32_t> m_enabled_options_range{ 0, 0 };
#endif // ENABLE_NEW_GCODE_VIEWER_DEBUG

    //
    // Member variables used for toolpaths visibiliity
    //
    BitSet<> m_valid_lines_bitset;
    size_t m_enabled_segments_count{ 0 };
    size_t m_enabled_options_count{ 0 };

    //
    // Member variables used for toolpaths coloring
    //
    ColorRange m_height_range;
    ColorRange m_width_range;
    ColorRange m_speed_range;
    ColorRange m_fan_speed_range;
    ColorRange m_temperature_range;
    ColorRange m_volumetric_rate_range;
    std::array<ColorRange, static_cast<size_t>(ColorRange::EType::COUNT)> m_layer_time_range{
        ColorRange(ColorRange::EType::Linear), ColorRange(ColorRange::EType::Logarithmic)
    };
    std::array<std::vector<float>, static_cast<size_t>(ETimeMode::COUNT)> m_layers_times;
    std::vector<Color> m_tool_colors;

    //
    // OpenGL shader ids
    //
    unsigned int m_segments_shader_id{ 0 };
    unsigned int m_options_shader_id{ 0 };
    unsigned int m_cog_marker_shader_id{ 0 };
    unsigned int m_tool_marker_shader_id{ 0 };

    //
    // Cache for OpenGL uniforms id for segments shader 
    //
    int m_uni_segments_view_matrix_id{ -1 };
    int m_uni_segments_projection_matrix_id{ -1 };
    int m_uni_segments_camera_position_id{ -1 };
    int m_uni_segments_positions_tex_id{ -1 };
    int m_uni_segments_height_width_angle_tex_id{ -1 };
    int m_uni_segments_colors_tex_id{ -1 };
    int m_uni_segments_segment_index_tex_id{ -1 };

    //
    // Cache for OpenGL uniforms id for options shader 
    //
    int m_uni_options_view_matrix_id{ -1 };
    int m_uni_options_projection_matrix_id{ -1 };
    int m_uni_options_positions_tex_id{ -1 };
    int m_uni_options_height_width_angle_tex_id{ -1 };
    int m_uni_options_colors_tex_id{ -1 };
    int m_uni_options_segment_index_tex_id{ -1 };

#if !ENABLE_NEW_GCODE_NO_COG_AND_TOOL_MARKERS
    //
    // Cache for OpenGL uniforms id for cog marker shader 
    //
    int m_uni_cog_marker_world_center_position{ -1 };
    int m_uni_cog_marker_scale_factor{ -1 };
    int m_uni_cog_marker_view_matrix{ -1 };
    int m_uni_cog_marker_projection_matrix{ -1 };

    //
    // Cache for OpenGL uniforms id for tool marker shader 
    //
    int m_uni_tool_marker_world_origin{ -1 };
    int m_uni_tool_marker_scale_factor{ -1 };
    int m_uni_tool_marker_view_matrix{ -1 };
    int m_uni_tool_marker_projection_matrix{ -1 };
    int m_uni_tool_marker_color_base{ -1 };
#endif // !ENABLE_NEW_GCODE_NO_COG_AND_TOOL_MARKERS

    //
    // gpu buffers to store positions
    //
    unsigned int m_positions_buf_id{ 0 };
    unsigned int m_positions_tex_id{ 0 };
    //
    // gpu buffers to store heights, widths and angles
    //
    unsigned int m_heights_widths_angles_buf_id{ 0 };
    unsigned int m_heights_widths_angles_tex_id{ 0 };
    //
    // gpu buffers to store colors
    //
    unsigned int m_colors_buf_id{ 0 };
    unsigned int m_colors_tex_id{ 0 };
    //
    // gpu buffers to store enabled segments
    //
    unsigned int m_enabled_segments_buf_id{ 0 };
    unsigned int m_enabled_segments_tex_id{ 0 };
    //
    // gpu buffers to store enabled options
    //
    unsigned int m_enabled_options_buf_id{ 0 };
    unsigned int m_enabled_options_tex_id{ 0 };

    void reset();
    void update_view_global_range();
    void update_color_ranges();
    Color select_color(const PathVertex& v) const;
    void render_segments(const Mat4x4f& view_matrix, const Mat4x4f& projection_matrix, const Vec3f& camera_position);
    void render_options(const Mat4x4f& view_matrix, const Mat4x4f& projection_matrix);
#if !ENABLE_NEW_GCODE_NO_COG_AND_TOOL_MARKERS
    void render_cog_marker(const Mat4x4f& view_matrix, const Mat4x4f& projection_matrix);
    void render_tool_marker(const Mat4x4f& view_matrix, const Mat4x4f& projection_matrix);
#endif // !ENABLE_NEW_GCODE_NO_COG_AND_TOOL_MARKERS

#if ENABLE_NEW_GCODE_VIEWER_DEBUG
    // Debug
    void render_debug_window();
#endif // ENABLE_NEW_GCODE_VIEWER_DEBUG
};

} // namespace libvgcode

//################################################################################################################################
// PrusaSlicer development only -> !!!TO BE REMOVED!!!
#endif // ENABLE_NEW_GCODE_VIEWER
//################################################################################################################################

#endif // VGCODE_VIEWERIMPL_HPP