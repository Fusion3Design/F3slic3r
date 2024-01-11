///|/ Copyright (c) Prusa Research 2023 Enrico Turri @enricoturri1966, Pavel Mikuš @Godrak
///|/
///|/ libvgcode is released under the terms of the AGPLv3 or higher
///|/
#ifndef VGCODE_VIEWER_HPP
#define VGCODE_VIEWER_HPP

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

    //
    // Initialize the viewer.
    // This method must be called after a valid OpenGL context has been already created
    // and before calling any other method of the viewer.
    //
    void init();
    //
    // Release the resources used by the viewer.
    // This method must be called before releasing the OpenGL context if the viewer
    // goes out of scope after releasing it.
    //
    void shutdown();
    //
    // Reset the contents of the viewer.
    // Automatically called by load() method.
    //
    void reset();
    //
    // Setup the viewer content from the given data.
    //
    void load(GCodeInputData&& gcode_data);
    //
    // Render the toolpaths according to the current settings and
    // using the given camera matrices.
    //
    void render(const Mat4x4& view_matrix, const Mat4x4& projection_matrix);

    //
    // ************************************************************************
    // Settings
    // The following methods can be used to query/customize the parameters
    // used to chenge the way toolpaths are rendered.
    // ************************************************************************
    //

    //
    // View type
    // See: EViewType
    //
    EViewType get_view_type() const;
    void set_view_type(EViewType type);
    //
    // Time mode
    // See: ETimeMode
    //
    ETimeMode get_time_mode() const;
    void set_time_mode(ETimeMode mode);
    //
    // Top layer only
    // Whether or not the visible range is limited to the current top layer only.
    //
    bool is_top_layer_only_view_range() const;
    void set_top_layer_only_view_range(bool top_layer_only);
    //
    // Spiral vase mode
    // Whether or not the gcode was generated with spiral vase mode enabled.
    // See: GCodeInputData
    //
    bool is_spiral_vase_mode() const;

    //
    // ************************************************************************
    // Layers range
    // The following methods can be used to query/customize the visualized
    // layers range.
    // Layers are detected during the call to load() method.
    // Their count can be queried using get_layers_count() method.
    // Their global range is [0..get_layers_count() - 1].
    // ************************************************************************
    //

    //
    // Return the current layers range.
    //
    const Interval& get_layers_view_range() const;
    //
    // Set the current layers range with the given interval.
    // Values are clamped to [0..get_layers_count() - 1].
    //
    void set_layers_view_range(const Interval& range);
    //
    // Set the current layers range with the given min and max values.
    // Values are clamped to [0..get_layers_count() - 1].
    //
    void set_layers_view_range(Interval::value_type min, Interval::value_type max);
    //
    // Return the count of detected layers.
    //
    size_t get_layers_count() const;

    //
    // ************************************************************************
    // Layers zs
    // The following methods can be used to query the layers zs.
    // Layers are detected during the call to load() method.
    // Their count can be queried using get_layers_count() method.
    // Their global range is [0..get_layers_count() - 1].
    // ************************************************************************
    //

    //
    // Return the z of the layer with the given id
    // or 0.0f if the id does not belong to [0..get_layers_count() - 1].
    //
    float get_layer_z(size_t layer_id) const;
    //
    // Return the list of zs of the detected layers.
    //
    std::vector<float> get_layers_zs() const;
    //
    // Return the id of the layer closest to the given z.
    //
    size_t get_layer_id_at(float z) const;

    //
    // ************************************************************************
    // Extruders
    // The following methods can be used to query informations about extruders.
    // Extruders are detected during the call to load() method.
    // ************************************************************************
    //

    //
    // Return the count of detected used extruders.
    //
    size_t get_used_extruders_count() const;
    //
    // Return the list of ids of the detected used extruders.
    //
    const std::vector<uint8_t>& get_used_extruders_ids() const;

    AABox get_bounding_box(EBBoxType type) const;

    bool is_option_visible(EOptionType type) const;
    void toggle_option_visibility(EOptionType type);

    bool is_extrusion_role_visible(EGCodeExtrusionRole role) const;
    void toggle_extrusion_role_visibility(EGCodeExtrusionRole role);

    const Interval& get_view_full_range() const;
    const Interval& get_view_enabled_range() const;
    const Interval& get_view_visible_range() const;

    //
    // Set the current visible range.
    // values are clamped to the current view global range
    // 
    void set_view_visible_range(uint32_t min, uint32_t max);

    //
    // Return the count of vertices used to render the toolpaths
    //
    size_t get_vertices_count() const;

    //
    // Return the vertex pointed by the max value of the view visible range
    //
    const PathVertex& get_current_vertex() const;

    //
    // Return the vertex at the given index
    //
    const PathVertex& get_vertex_at(size_t id) const;

    //
    // Return the color used to render the given vertex with the current settings.
    //
    Color get_vertex_color(const PathVertex& vertex) const;

    //
    // Return the count of path segments enabled for rendering
    //
    size_t get_enabled_segments_count() const;

    //
    // Return the Interval containing the enabled segments
    //
    const Interval& get_enabled_segments_range() const;

    //
    // Return the count of options enabled for rendering
    //
    size_t get_enabled_options_count() const;

    //
    // Return the Interval containing the enabled options
    //
    const Interval& get_enabled_options_range() const;

    //
    // Return the count of detected extrusion roles
    //
    size_t get_extrusion_roles_count() const;

    //
    // Return the list of detected extrusion roles
    //
    std::vector<EGCodeExtrusionRole> get_extrusion_roles() const;

    //
    // Return the count of detected options
    //
    size_t get_options_count() const;

    //
    // Return the list of detected options
    //
    const std::vector<EOptionType>& get_options() const;

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
    const Palette& get_tool_colors() const;
    void set_tool_colors(const Palette& colors);

    const Color& get_extrusion_role_color(EGCodeExtrusionRole role) const;
    void set_extrusion_role_color(EGCodeExtrusionRole role, const Color& color);
    void reset_default_extrusion_roles_colors();

    const Color& get_option_color(EOptionType type) const;
    void set_option_color(EOptionType type, const Color& color);
    void reset_default_options_colors();

    //
    // Get the color range for the given view type.
    // Valid view types are:
    // EViewType::Height
    // EViewType::Width
    // EViewType::Speed
    // EViewType::FanSpeed
    // EViewType::Temperature
    // EViewType::VolumetricFlowRate
    // EViewType::LayerTimeLinear
    // EViewType::LayerTimeLogarithmic
    //
    const ColorRange& get_color_range(EViewType type) const;
    //
    // Set the palette for the color range corresponding to the given view type
    // with the given value.
    // Valid view types are:
    // EViewType::Height
    // EViewType::Width
    // EViewType::Speed
    // EViewType::FanSpeed
    // EViewType::Temperature
    // EViewType::VolumetricFlowRate
    // EViewType::LayerTimeLinear
    // EViewType::LayerTimeLogarithmic
    //
    void set_color_range_palette(EViewType type, const Palette& palette);

    //
    // Get the radius, in mm, of the cylinders used to render the travel moves.
    //
    float get_travels_radius() const;
    //
    // Set the radius, in mm, of the cylinders used to render the travel moves.
    // Radius is clamped to [MIN_TRAVELS_RADIUS_MM..MAX_TRAVELS_RADIUS_MM]
    //
    void set_travels_radius(float radius);
    //
    // Get the radius, in mm, of the cylinders used to render the wipe moves.
    //
    float get_wipes_radius() const;
    //
    // Set the radius, in mm, of the cylinders used to render the wipe moves.
    // Radius is clamped to [MIN_WIPES_RADIUS_MM..MAX_WIPES_RADIUS_MM]
    //
    void set_wipes_radius(float radius);

#if ENABLE_COG_AND_TOOL_MARKERS
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
#endif // ENABLE_COG_AND_TOOL_MARKERS

private:
    ViewerImpl* m_impl{ nullptr };
};

} // namespace libvgcode

#endif // VGCODE_VIEWER_HPP
