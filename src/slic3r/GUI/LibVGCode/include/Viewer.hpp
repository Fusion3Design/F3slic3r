///|/ Copyright (c) Prusa Research 2023 Enrico Turri @enricoturri1966, Pavel Mikuš @Godrak
///|/
///|/ libvgcode is released under the terms of the AGPLv3 or higher
///|/
#ifndef VGCODE_VIEWER_HPP
#define VGCODE_VIEWER_HPP

//################################################################################################################################
// PrusaSlicer development only -> !!!TO BE REMOVED!!!
#if ENABLE_NEW_GCODE_VIEWER
//################################################################################################################################

#include "Types.hpp"

namespace libvgcode {

class ViewerImpl;
struct GCodeInputData;
struct PathVertex;
class ColorRange;

class Viewer
{
public:
    Viewer();
    ~Viewer();
    Viewer(const Viewer& other) = delete;
    Viewer(Viewer&& other) = delete;
    Viewer& operator = (const Viewer& other) = delete;
    Viewer& operator = (Viewer&& other) = delete;

    void init();
    void reset();
    void load(GCodeInputData&& gcode_data);
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

    //
    // min must be smaller than max
    // values are clamped to the current view global range
    // 
    void set_view_visible_range(uint32_t min, uint32_t max);

    //
    // Return the count of vertices used to render the toolpaths
    //
    uint32_t get_vertices_count() const;

    //
    // Return the vertex pointed by the max value of the view current range
    //
    PathVertex get_current_vertex() const;

    //
    // Return the vertex at the given index
    //
    PathVertex get_vertex_at(uint32_t id) const;

    //
    // Return the count of path segments enabled for rendering
    //
    size_t get_enabled_segments_count() const;

    const std::array<uint32_t, 2>& get_enabled_segments_range() const;

    //
    // Return the count of options enabled for rendering
    //
    size_t get_enabled_options_count() const;

    const std::array<uint32_t, 2>& get_enabled_options_range() const;

    //
    // Return the count of detected extrusion roles
    //
    uint32_t get_extrusion_roles_count() const;

    //
    // Return the list of detected extrusion roles
    //
    std::vector<EGCodeExtrusionRole> get_extrusion_roles() const;

    //
    // Return the estimated time for the given role and the current time mode
    //
    float get_extrusion_role_time(EGCodeExtrusionRole role) const;

    //
    // Return the estimated time for the given role and the given time mode
    //
    float get_extrusion_role_time(EGCodeExtrusionRole role, ETimeMode mode) const;

    //
    // Return the estimated time for the travel moves and the current time mode
    //
    float get_travels_time() const;

    //
    // Return the estimated time for the travel moves and the given time mode
    //
    float get_travels_time(ETimeMode mode) const;

    //
    // Return the list of layers time for the current time mode
    //
    std::vector<float> get_layers_times() const;

    //
    // Return the list of layers time for the given time mode
    //
    std::vector<float> get_layers_times(ETimeMode mode) const;

    size_t get_tool_colors_count() const;
    const std::vector<Color>& get_tool_colors() const;
    void set_tool_colors(const std::vector<Color>& colors);

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
    //
    // Returns the position of the center of gravity of the toolpaths.
    // It does not take in account extrusions of type:
    // Skirt
    // Support Material
    // Support Material Interface
    // WipeTower
    // Custom
    //
    Vec3 get_cog_position() const;

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
    void set_tool_marker_color(const Color& color);

    float get_tool_marker_alpha() const;
    void set_tool_marker_alpha(float alpha);
#endif // !ENABLE_NEW_GCODE_VIEWER_NO_COG_AND_TOOL_MARKERS

private:
    ViewerImpl* m_impl{ nullptr };
};

} // namespace libvgcode

//################################################################################################################################
// PrusaSlicer development only -> !!!TO BE REMOVED!!!
#endif // ENABLE_NEW_GCODE_VIEWER
//################################################################################################################################

#endif // VGCODE_VIEWER_HPP
