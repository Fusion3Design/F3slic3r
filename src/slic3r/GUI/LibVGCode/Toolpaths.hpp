///|/ Copyright (c) Prusa Research 2023 Enrico Turri @enricoturri1966, Pavel Mikuš @Godrak
///|/
///|/ libvgcode is released under the terms of the AGPLv3 or higher
///|/
#ifndef VGCODE_TOOLPATHS_HPP
#define VGCODE_TOOLPATHS_HPP

//################################################################################################################################
// PrusaSlicer development only -> !!!TO BE REMOVED!!!
#if ENABLE_NEW_GCODE_VIEWER
//################################################################################################################################

#include "SegmentTemplate.hpp"
#include "OptionTemplate.hpp"
#include "CogMarker.hpp"
#include "ToolMarker.hpp"
#include "PathVertex.hpp"
#include "Bitset.hpp"
#include "ColorRange.hpp"

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

class ViewRange;
struct Settings;

class Toolpaths
{
public:
    Toolpaths() = default;
    ~Toolpaths();
    Toolpaths(const Toolpaths& other) = delete;
    Toolpaths(Toolpaths&& other) = delete;
    Toolpaths& operator = (const Toolpaths& other) = delete;
    Toolpaths& operator = (Toolpaths&& other) = delete;

    //
    // Initialize shader, uniform indices and segment geometry
    //
    void init();

    //
    // Setup all the variables used for visualization and coloring of the toolpaths
    // from the gcode moves contained in the given gcode_result, according to the setting
    // contained in the given config.
    //
    void load(const Slic3r::GCodeProcessorResult& gcode_result, const std::vector<std::string>& str_tool_colors,
        const Settings& settings);

    //
    // Update the visibility property of toolpaths
    //
    void update_enabled_entities(const ViewRange& range, const Settings& settings);
    //
    // Update the color of toolpaths
    //
    void update_colors(const Settings& settings);

    //
    // Render the toolpaths
    //
    void render(const Mat4x4f& view_matrix, const Mat4x4f& projection_matrix, const Settings& settings);

    //
    // Properties getters
    //
    const std::array<std::vector<float>, static_cast<size_t>(ETimeMode::COUNT)>& get_layers_times() const;
    Vec3f get_cog_marker_position() const;
    float get_cog_marker_scale_factor() const;
    const Vec3f& get_tool_marker_position() const;
    float get_tool_marker_scale_factor() const;
    const Color& get_tool_marker_color() const;
    float get_tool_marker_alpha() const;

    //
    // Properties setters
    //
    void set_cog_marker_scale_factor(float factor);
    void set_tool_marker_position(const Vec3f& position);
    void set_tool_marker_scale_factor(float factor);
    void set_tool_marker_color(const Color& color);
    void set_tool_marker_alpha(float size);

//################################################################################################################################
    // Debug
    size_t get_vertices_count() const;
    size_t get_enabled_segments_count() const;
    size_t get_enabled_options_count() const;
    const std::pair<uint32_t, uint32_t>& get_enabled_segments_range() const;
    const std::pair<uint32_t, uint32_t>& get_enabled_options_range() const;
    const std::array<float, 2>& get_height_range() const;
    const std::array<float, 2>& get_width_range() const;
    const std::array<float, 2>& get_speed_range() const;
    const std::array<float, 2>& get_fan_speed_range() const;
    const std::array<float, 2>& get_temperature_range() const;
    const std::array<float, 2>& get_volumetric_rate_range() const;
    const std::array<float, 2>& get_layer_time_linear_range() const;
    const std::array<float, 2>& get_layer_time_logarithmic_range() const;
//################################################################################################################################

private:
    //
    // The OpenGL element used to represent all toolpath segments
    //
    SegmentTemplate m_segment_template;

    //
    // The OpenGL element used to represent all option markers
    //
    OptionTemplate m_option_template;

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

    //
    // cpu buffer to store vertices
    //
    std::vector<PathVertex> m_vertices;
//################################################################################################################################
    // Debug
    std::pair<uint32_t, uint32_t> m_enabled_segments_range{ 0, 0 };
    std::pair<uint32_t, uint32_t> m_enabled_options_range{ 0, 0 };
//################################################################################################################################

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
    void update_color_ranges(const Settings& settings);
    Color select_color(const PathVertex& v, const Settings& settings) const;
    void render_segments(const Mat4x4f& view_matrix, const Mat4x4f& projection_matrix, const Vec3f& camera_position);
    void render_options(const Mat4x4f& view_matrix, const Mat4x4f& projection_matrix);
    void render_cog_marker(const Mat4x4f& view_matrix, const Mat4x4f& projection_matrix);
    void render_tool_marker(const Mat4x4f& view_matrix, const Mat4x4f& projection_matrix);
};

} // namespace libvgcode

//################################################################################################################################
// PrusaSlicer development only -> !!!TO BE REMOVED!!!
#endif // ENABLE_NEW_GCODE_VIEWER
//################################################################################################################################

#endif // VGCODE_TOOLPATHS_HPP