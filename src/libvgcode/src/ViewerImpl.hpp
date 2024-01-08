///|/ Copyright (c) Prusa Research 2023 Enrico Turri @enricoturri1966, Pavel Mikuš @Godrak
///|/
///|/ libvgcode is released under the terms of the AGPLv3 or higher
///|/
#ifndef VGCODE_VIEWERIMPL_HPP
#define VGCODE_VIEWERIMPL_HPP

#include "Settings.hpp"
#include "SegmentTemplate.hpp"
#include "OptionTemplate.hpp"
#if ENABLE_COG_AND_TOOL_MARKERS
#include "CogMarker.hpp"
#include "ToolMarker.hpp"
#endif // ENABLE_COG_AND_TOOL_MARKERS
#include "../include/PathVertex.hpp"
#include "../include/ColorRange.hpp"
#include "Bitset.hpp"
#include "ViewRange.hpp"
#include "Layers.hpp"
#include "ExtrusionRoles.hpp"

namespace libvgcode {

struct GCodeInputData;

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
    // Initialize shaders, uniform indices and segment geometry
    //
    void init();

    //
    // Reset all caches and free gpu memory
    //
    void reset();

    //
    // Setup all the variables used for visualization of the toolpaths
    // from the given gcode data.
    //
    void load(GCodeInputData&& gcode_data);

    //
    // Update the visibility property of toolpaths in dependence
    // of the current settings
    //
    void update_enabled_entities();

    //
    // Update the color of toolpaths in dependence of the current
    // view type and settings
    //
    void update_colors();

    //
    // Render the toolpaths
    //
    void render(const Mat4x4& view_matrix, const Mat4x4& projection_matrix);

    EViewType get_view_type() const { return m_settings.view_type; }
    void set_view_type(EViewType type);

    ETimeMode get_time_mode() const { return m_settings.time_mode; }
    void set_time_mode(ETimeMode mode);

    const Interval& get_layers_view_range() const { return m_layers.get_view_range(); }
    void set_layers_view_range(const Interval& range) { set_layers_view_range(range[0], range[1]); }
    void set_layers_view_range(Interval::value_type min, Interval::value_type max);

    bool is_top_layer_only_view_range() const { return m_settings.top_layer_only_view_range; }
    void set_top_layer_only_view_range(bool top_layer_only_view_range);

    bool is_spiral_vase_mode() const { return m_settings.spiral_vase_mode; }

    size_t get_layers_count() const { return m_layers.count(); }
    float get_layer_z(size_t layer_id) const { return m_layers.get_layer_z(layer_id); }
    std::vector<float> get_layers_zs() const { return m_layers.get_zs(); }

    size_t get_layer_id_at(float z) const { return m_layers.get_layer_id_at(z); }

    size_t get_used_extruders_count() const { return m_used_extruders_ids.size(); }
    const std::vector<uint8_t>& get_used_extruders_ids() const { return m_used_extruders_ids; }

    AABox get_bounding_box(EBBoxType type) const;

    bool is_option_visible(EOptionType type) const;
    void toggle_option_visibility(EOptionType type);

    bool is_extrusion_role_visible(EGCodeExtrusionRole role) const;
    void toggle_extrusion_role_visibility(EGCodeExtrusionRole role);

    const Interval& get_view_full_range() const { return m_view_range.get_full(); }
    const Interval& get_view_enabled_range() const { return m_view_range.get_enabled(); }
    const Interval& get_view_visible_range() const { return m_view_range.get_visible(); }
    void set_view_visible_range(uint32_t min, uint32_t max);

    size_t get_vertices_count() const { return m_vertices.size(); }
    const PathVertex& get_current_vertex() const { return get_vertex_at(m_view_range.get_visible()[1]); }
    const PathVertex& get_vertex_at(size_t id) const {
        return (id < m_vertices.size()) ? m_vertices[id] : DUMMY_PATH_VERTEX;
    }
    Color get_vertex_color(const PathVertex& vertex) const;

    size_t get_enabled_segments_count() const { return m_enabled_segments_count; }
    const Interval& get_enabled_segments_range() const { return m_enabled_segments_range.get(); }

    size_t get_enabled_options_count() const { return m_enabled_options_count; }
    const Interval& get_enabled_options_range() const { return m_enabled_options_range.get(); }

    std::vector<EGCodeExtrusionRole> get_extrusion_roles() const { return m_extrusion_roles.get_roles(); }
    float get_extrusion_role_time(EGCodeExtrusionRole role) const { return m_extrusion_roles.get_time(role, m_settings.time_mode); }
    size_t get_extrusion_roles_count() const { return m_extrusion_roles.get_roles_count(); }
    float get_extrusion_role_time(EGCodeExtrusionRole role, ETimeMode mode) const { return m_extrusion_roles.get_time(role, mode); }

    size_t get_options_count() const { return m_options.size(); }
    const std::vector<EOptionType>& get_options() const { return m_options; }

    float get_travels_time() const { return get_travels_time(m_settings.time_mode); }
    float get_travels_time(ETimeMode mode) const {
        return (mode < ETimeMode::COUNT) ? m_travels_time[static_cast<size_t>(mode)] : 0.0f;
    }
    std::vector<float> get_layers_times() const { return get_layers_times(m_settings.time_mode); }
    std::vector<float> get_layers_times(ETimeMode mode) const { return m_layers.get_times(mode); }

    size_t get_tool_colors_count() const { return m_tool_colors.size(); }
    const std::vector<Color>& get_tool_colors() const { return m_tool_colors; }
    void set_tool_colors(const std::vector<Color>& colors);

    const Color& get_extrusion_role_color(EGCodeExtrusionRole role) const;
    void set_extrusion_role_color(EGCodeExtrusionRole role, const Color& color);
    void reset_default_extrusion_roles_colors() { m_extrusion_roles_colors = DEFAULT_EXTRUSION_ROLES_COLORS; }

    const Color& get_option_color(EOptionType type) const;
    void set_option_color(EOptionType type, const Color& color);
    void reset_default_options_colors() { m_options_colors = DEFAULT_OPTIONS_COLORS; }

    const ColorRange& get_height_range() const { return m_height_range; }
    const ColorRange& get_width_range() const { return m_width_range; }
    const ColorRange& get_speed_range() const { return m_speed_range; }
    const ColorRange& get_fan_speed_range() const { return m_fan_speed_range; }
    const ColorRange& get_temperature_range() const { return m_temperature_range; }
    const ColorRange& get_volumetric_rate_range() const { return m_volumetric_rate_range; }
    const ColorRange& get_layer_time_range(EColorRangeType type) const {
        return (static_cast<size_t>(type) < m_layer_time_range.size()) ?
            m_layer_time_range[static_cast<size_t>(type)] : ColorRange::Dummy_Color_Range;
    }

    float get_travels_radius() const { return m_travels_radius; }
    void set_travels_radius(float radius);
    float get_wipes_radius() const { return m_wipes_radius; }
    void set_wipes_radius(float radius);

#if ENABLE_COG_AND_TOOL_MARKERS
    Vec3 get_cog_marker_position() const { return m_cog_marker.get_position(); }

    float get_cog_marker_scale_factor() const { return m_cog_marker_scale_factor; }
    void set_cog_marker_scale_factor(float factor) { m_cog_marker_scale_factor = std::max(factor, 0.001f); }

    bool is_tool_marker_enabled() const { return m_tool_marker.is_enabled(); }
    void enable_tool_marker(bool value) { m_tool_marker.enable(value); }

    const Vec3& get_tool_marker_position() const { return m_tool_marker.get_position(); }
    void set_tool_marker_position(const Vec3& position) { m_tool_marker.set_position(position); }

    float get_tool_marker_offset_z() const { return m_tool_marker.get_offset_z(); }
    void set_tool_marker_offset_z(float offset_z) { m_tool_marker.set_offset_z(offset_z); }

    float get_tool_marker_scale_factor() const { return m_tool_marker_scale_factor; }
    void set_tool_marker_scale_factor(float factor) { m_tool_marker_scale_factor = std::max(factor, 0.001f); }

    const Color& get_tool_marker_color() const { return m_tool_marker.get_color(); }
    void set_tool_marker_color(const Color& color) { m_tool_marker.set_color(color); }

    float get_tool_marker_alpha() const { return m_tool_marker.get_alpha(); }
    void set_tool_marker_alpha(float alpha) { m_tool_marker.set_alpha(alpha); }
#endif // ENABLE_COG_AND_TOOL_MARKERS

private:
    Settings m_settings;
    Layers m_layers;
    ViewRange m_view_range;
    ExtrusionRoles m_extrusion_roles;
    std::vector<EOptionType> m_options;
    std::array<float, TIME_MODES_COUNT> m_travels_time{ 0.0f, 0.0f };
    std::vector<uint8_t> m_used_extruders_ids;
    float m_travels_radius{ DEFAULT_TRAVELS_RADIUS };
    float m_wipes_radius{ DEFAULT_WIPES_RADIUS };

    bool m_initialized{ false };
    bool m_loading{ false };

    std::map<EGCodeExtrusionRole, Color> m_extrusion_roles_colors{ DEFAULT_EXTRUSION_ROLES_COLORS };
    std::map<EOptionType, Color> m_options_colors{ DEFAULT_OPTIONS_COLORS };

    //
    // The OpenGL element used to represent all toolpath segments
    //
    SegmentTemplate m_segment_template;

    //
    // The OpenGL element used to represent all option markers
    //
    OptionTemplate m_option_template;

#if ENABLE_COG_AND_TOOL_MARKERS
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
#endif // ENABLE_COG_AND_TOOL_MARKERS

    //
    // cpu buffer to store vertices
    //
    std::vector<PathVertex> m_vertices;
    Range m_enabled_segments_range;
    Range m_enabled_options_range;

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
    std::array<ColorRange, COLOR_RANGE_TYPES_COUNT> m_layer_time_range{
        ColorRange(EColorRangeType::Linear), ColorRange(EColorRangeType::Logarithmic)
    };
    std::vector<Color> m_tool_colors;

    //
    // OpenGL shader ids
    //
    unsigned int m_segments_shader_id{ 0 };
    unsigned int m_options_shader_id{ 0 };
#if ENABLE_COG_AND_TOOL_MARKERS
    unsigned int m_cog_marker_shader_id{ 0 };
    unsigned int m_tool_marker_shader_id{ 0 };
#endif // ENABLE_COG_AND_TOOL_MARKERS

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

#if ENABLE_COG_AND_TOOL_MARKERS
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
#endif // ENABLE_COG_AND_TOOL_MARKERS

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

    void update_view_full_range();
    void update_color_ranges();
    void update_heights_widths();
    void render_segments(const Mat4x4& view_matrix, const Mat4x4& projection_matrix, const Vec3& camera_position);
    void render_options(const Mat4x4& view_matrix, const Mat4x4& projection_matrix);
#if ENABLE_COG_AND_TOOL_MARKERS
    void render_cog_marker(const Mat4x4& view_matrix, const Mat4x4& projection_matrix);
    void render_tool_marker(const Mat4x4& view_matrix, const Mat4x4& projection_matrix);
#endif // ENABLE_COG_AND_TOOL_MARKERS

    //
    // Palette used to render extrusion moves by extrusion roles
    // EViewType: FeatureType
    //
    static const std::map<EGCodeExtrusionRole, Color> DEFAULT_EXTRUSION_ROLES_COLORS;
    //
    // Palette used to render options
    // EViewType: FeatureType
    //
    static const std::map<EOptionType, Color> DEFAULT_OPTIONS_COLORS;
};

} // namespace libvgcode

#endif // VGCODE_VIEWERIMPL_HPP