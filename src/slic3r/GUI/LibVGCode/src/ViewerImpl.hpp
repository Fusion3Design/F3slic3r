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
#if !ENABLE_NEW_GCODE_VIEWER_NO_COG_AND_TOOL_MARKERS
#include "CogMarker.hpp"
#include "ToolMarker.hpp"
#endif // !ENABLE_NEW_GCODE_VIEWER_NO_COG_AND_TOOL_MARKERS
#include "../include/PathVertex.hpp"
#include "../include/ColorRange.hpp"
#include "Bitset.hpp"
#include "ViewRange.hpp"
#include "Layers.hpp"
#include "ExtrusionRoles.hpp"

namespace libvgcode {

struct GCodeInputData;

//
// Palette used to render extrusion moves by extrusion roles
// EViewType: FeatureType
//
static const std::map<EGCodeExtrusionRole, Color> Default_Extrusion_Roles_Colors{ {
    { EGCodeExtrusionRole::None,                       { 230, 179, 179 } },
    { EGCodeExtrusionRole::Perimeter,                  { 255, 230,  77 } },
    { EGCodeExtrusionRole::ExternalPerimeter,          { 255, 125,  56 } },
    { EGCodeExtrusionRole::OverhangPerimeter,          {  31,  31, 255 } },
    { EGCodeExtrusionRole::InternalInfill,             { 176,  48,  41 } },
    { EGCodeExtrusionRole::SolidInfill,                { 150,  84, 204 } },
    { EGCodeExtrusionRole::TopSolidInfill,             { 240,  64,  64 } },
    { EGCodeExtrusionRole::Ironing,                    { 255, 140, 105 } },
    { EGCodeExtrusionRole::BridgeInfill,               {  77, 128, 186 } },
    { EGCodeExtrusionRole::GapFill,                    { 255, 255, 255 } },
    { EGCodeExtrusionRole::Skirt,                      {   0, 135, 110 } },
    { EGCodeExtrusionRole::SupportMaterial,            {   0, 255,   0 } },
    { EGCodeExtrusionRole::SupportMaterialInterface,   {   0, 128,   0 } },
    { EGCodeExtrusionRole::WipeTower,                  { 179, 227, 171 } },
    { EGCodeExtrusionRole::Custom,                     {  94, 209, 148 } }
} };

//
// Palette used to render options
//
static const std::map<EOptionType, Color> Default_Options_Colors{ {
    { EOptionType::Retractions,   { 205,  34, 214 } },
    { EOptionType::Unretractions, {  73, 173, 207 } },
    { EOptionType::Seams,         { 230, 230, 230 } },
    { EOptionType::ToolChanges,   { 193, 190,  99 } },
    { EOptionType::ColorChanges,  { 218, 148, 139 } },
    { EOptionType::PausePrints,   {  82, 240, 131 } },
    { EOptionType::CustomGCodes,  { 226, 210,  67 } }
} };

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

    EViewType get_view_type() const;
    void set_view_type(EViewType type);

    ETimeMode get_time_mode() const;
    void set_time_mode(ETimeMode mode);

    const std::array<uint32_t, 2>& get_layers_view_range() const;
    void set_layers_view_range(const std::array<uint32_t, 2>& range);
    void set_layers_view_range(uint32_t min, uint32_t max);

    bool is_top_layer_only_view_range() const;
    void set_top_layer_only_view_range(bool top_layer_only_view_range);

    size_t get_layers_count() const;
    float get_layer_z(size_t layer_id) const;
    std::vector<float> get_layers_zs() const;

    size_t get_layer_id_at(float z) const;

    size_t get_used_extruders_count() const;
    const std::vector<uint8_t>& get_used_extruders_ids() const;

    AABox get_bounding_box(EBBoxType type) const;

    bool is_option_visible(EOptionType type) const;
    void toggle_option_visibility(EOptionType type);

    bool is_extrusion_role_visible(EGCodeExtrusionRole role) const;
    void toggle_extrusion_role_visibility(EGCodeExtrusionRole role);

    const std::array<uint32_t, 2>& get_view_full_range() const;
    const std::array<uint32_t, 2>& get_view_enabled_range() const;
    const std::array<uint32_t, 2>& get_view_visible_range() const;
    void set_view_visible_range(uint32_t min, uint32_t max);

    size_t get_vertices_count() const;
    PathVertex get_current_vertex() const;
    PathVertex get_vertex_at(size_t id) const;

    size_t get_enabled_segments_count() const;
    const std::array<uint32_t, 2>& get_enabled_segments_range() const;

    size_t get_enabled_options_count() const;
    const std::array<uint32_t, 2>& get_enabled_options_range() const;

    size_t get_extrusion_roles_count() const;
    std::vector<EGCodeExtrusionRole> get_extrusion_roles() const;
    float get_extrusion_role_time(EGCodeExtrusionRole role) const;
    float get_extrusion_role_time(EGCodeExtrusionRole role, ETimeMode mode) const;

    float get_travels_time() const;
    float get_travels_time(ETimeMode mode) const;
    std::vector<float> get_layers_times() const;
    std::vector<float> get_layers_times(ETimeMode mode) const;

    size_t get_tool_colors_count() const;
    const std::vector<Color>& get_tool_colors() const;
    void set_tool_colors(const std::vector<Color>& colors);

    const Color& get_extrusion_role_color(EGCodeExtrusionRole role) const;
    void set_extrusion_role_color(EGCodeExtrusionRole role, const Color& color);
    void reset_default_extrusion_roles_colors();

    const Color& get_option_color(EOptionType type) const;
    void set_option_color(EOptionType type, const Color& color);
    void reset_default_options_colors();

    const ColorRange& get_height_range() const;
    const ColorRange& get_width_range() const;
    const ColorRange& get_speed_range() const;
    const ColorRange& get_fan_speed_range() const;
    const ColorRange& get_temperature_range() const;
    const ColorRange& get_volumetric_rate_range() const;
    const ColorRange& get_layer_time_range(EColorRangeType type) const;

    float get_travels_radius() const;
    void set_travels_radius(float radius);
    float get_wipes_radius() const;
    void set_wipes_radius(float radius);

#if !ENABLE_NEW_GCODE_VIEWER_NO_COG_AND_TOOL_MARKERS
    Vec3 get_cog_marker_position() const;

    float get_cog_marker_scale_factor() const;
    void set_cog_marker_scale_factor(float factor);

    bool is_tool_marker_enabled() const;
    void enable_tool_marker(bool value);

    const Vec3& get_tool_marker_position() const;
    void set_tool_marker_position(const Vec3& position);

    float get_tool_marker_offset_z() const;
    void set_tool_marker_offset_z(float offset_z);

    float get_tool_marker_scale_factor() const;
    void set_tool_marker_scale_factor(float factor);

    const Color& get_tool_marker_color() const;
    void set_tool_marker_color(const Color & color);

    float get_tool_marker_alpha() const;
    void set_tool_marker_alpha(float size);
#endif // !ENABLE_NEW_GCODE_VIEWER_NO_COG_AND_TOOL_MARKERS

private:
    Settings m_settings;
    Layers m_layers;
    ViewRange m_view_range;
    ExtrusionRoles m_extrusion_roles;
    std::array<float, Time_Modes_Count> m_travels_time{ 0.0f, 0.0f };
    std::vector<uint8_t> m_used_extruders_ids;
    float m_travels_radius{ Default_Travels_Radius };
    float m_wipes_radius{ Default_Wipes_Radius };

    bool m_initialized{ false };
    bool m_loading{ false };

    std::map<EGCodeExtrusionRole, Color> m_extrusion_roles_colors{ Default_Extrusion_Roles_Colors };
    std::map<EOptionType, Color> m_options_colors{ Default_Options_Colors };

    //
    // The OpenGL element used to represent all toolpath segments
    //
    SegmentTemplate m_segment_template;

    //
    // The OpenGL element used to represent all option markers
    //
    OptionTemplate m_option_template;

#if !ENABLE_NEW_GCODE_VIEWER_NO_COG_AND_TOOL_MARKERS
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
#endif // !ENABLE_NEW_GCODE_VIEWER_NO_COG_AND_TOOL_MARKERS

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
    std::array<ColorRange, Color_Range_Types_Count> m_layer_time_range{
        ColorRange(EColorRangeType::Linear), ColorRange(EColorRangeType::Logarithmic)
    };
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

#if !ENABLE_NEW_GCODE_VIEWER_NO_COG_AND_TOOL_MARKERS
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
#endif // !ENABLE_NEW_GCODE_VIEWER_NO_COG_AND_TOOL_MARKERS

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
    Color select_color(const PathVertex& v) const;
    void render_segments(const Mat4x4& view_matrix, const Mat4x4& projection_matrix, const Vec3& camera_position);
    void render_options(const Mat4x4& view_matrix, const Mat4x4& projection_matrix);
#if !ENABLE_NEW_GCODE_VIEWER_NO_COG_AND_TOOL_MARKERS
    void render_cog_marker(const Mat4x4& view_matrix, const Mat4x4& projection_matrix);
    void render_tool_marker(const Mat4x4& view_matrix, const Mat4x4& projection_matrix);
#endif // !ENABLE_NEW_GCODE_VIEWER_NO_COG_AND_TOOL_MARKERS
};

} // namespace libvgcode

//################################################################################################################################
// PrusaSlicer development only -> !!!TO BE REMOVED!!!
#endif // ENABLE_NEW_GCODE_VIEWER
//################################################################################################################################

#endif // VGCODE_VIEWERIMPL_HPP