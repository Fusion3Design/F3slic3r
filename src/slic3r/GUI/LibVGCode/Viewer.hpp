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

#include "ViewerImpl.hpp"
#include "Types.hpp"
#include "GCodeInputData.hpp"

//################################################################################################################################
// PrusaSlicer development only -> !!!TO BE REMOVED!!!
namespace Slic3r {
struct GCodeProcessorResult;
class Print;
} // namespace Slic3r
//################################################################################################################################

namespace libvgcode {

class Viewer
{
public:
    Viewer() = default;
    ~Viewer() = default;
    Viewer(const Viewer& other) = delete;
    Viewer(Viewer&& other) = delete;
    Viewer& operator = (const Viewer& other) = delete;
    Viewer& operator = (Viewer&& other) = delete;

    void init();
    void reset();
    void load(const Slic3r::GCodeProcessorResult& gcode_result, GCodeInputData&& gcode_data);
    void render(const Mat4x4f& view_matrix, const Mat4x4f& projection_matrix);

    EViewType get_view_type() const;
    void set_view_type(EViewType type);

    ETimeMode get_time_mode() const;
    void set_time_mode(ETimeMode mode);

    const std::array<uint32_t, 2>& get_layers_range() const;
    void set_layers_range(const std::array<uint32_t, 2>& range);
    void set_layers_range(uint32_t min, uint32_t max);

    bool is_top_layer_only_view_range() const;
    void set_top_layer_only_view_range(bool top_layer_only_view_range);

    bool is_option_visible(EOptionType type) const;
    void toggle_option_visibility(EOptionType type);

    bool is_extrusion_role_visible(EGCodeExtrusionRole role) const;
    void toggle_extrusion_role_visibility(EGCodeExtrusionRole role);

    const std::array<uint32_t, 2>& get_view_current_range() const;
    const std::array<uint32_t, 2>& get_view_global_range() const;
    //
    // min must be smaller than max
    // values are clamped to the view global range
    // 
    void set_view_current_range(uint32_t min, uint32_t max);

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

#if !ENABLE_NEW_GCODE_NO_COG_AND_TOOL_MARKERS
    //
    // Returns the position of the center of gravity of the toolpaths.
    // It does not take in account extrusions of type:
    // Skirt
    // Support Material
    // Support Material Interface
    // WipeTower
    // Custom
    //
    Vec3f get_cog_position() const;

    float get_cog_marker_scale_factor() const;
    void set_cog_marker_scale_factor(float factor);

    bool is_tool_marker_enabled() const;
    void enable_tool_marker(bool value);

    const Vec3f& get_tool_marker_position() const;
    void set_tool_marker_position(const Vec3f& position);

    float get_tool_marker_scale_factor() const;
    void set_tool_marker_scale_factor(float factor);

    const Color& get_tool_marker_color() const;
    void set_tool_marker_color(const Color& color);

    float get_tool_marker_alpha() const;
    void set_tool_marker_alpha(float alpha);
#endif // !ENABLE_NEW_GCODE_NO_COG_AND_TOOL_MARKERS

private:
    ViewerImpl m_impl;
};

} // namespace libvgcode

//################################################################################################################################
// PrusaSlicer development only -> !!!TO BE REMOVED!!!
#endif // ENABLE_NEW_GCODE_VIEWER
//################################################################################################################################

#endif // VGCODE_VIEWER_HPP
