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

#include "Settings.hpp"
#include "Toolpaths.hpp"
#include "ViewRange.hpp"

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
    void load(const Slic3r::GCodeProcessorResult& gcode_result, const std::vector<std::string>& str_tool_colors);
    void render(const Mat4x4f& view_matrix, const Mat4x4f& projection_matrix);

    EViewType get_view_type() const;
    void set_view_type(EViewType type);

    ETimeMode get_time_mode() const;
    void set_time_mode(ETimeMode mode);

    const std::array<std::vector<float>, static_cast<size_t>(ETimeMode::COUNT)>& get_layers_times() const;

    bool is_option_visible(EOptionType type) const;
    void toggle_option_visibility(EOptionType type);

    bool is_extrusion_role_visible(EGCodeExtrusionRole role) const;
    void toggle_extrusion_role_visibility(EGCodeExtrusionRole role);

    const std::array<size_t, 2>& get_view_current_range() const;
    const std::array<size_t, 2>& get_view_global_range() const;
    //
    // min must be smaller than max
    // values are clamped to the view global range
    // 
    void set_view_current_range(size_t min, size_t max);

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

    const Vec3f& get_tool_marker_position() const;
    void set_tool_marker_position(const Vec3f& position);

    float get_tool_marker_scale_factor() const;
    void set_tool_marker_scale_factor(float factor);

    const Color& get_tool_marker_color() const;
    void set_tool_marker_color(const Color& color);

    float get_tool_marker_alpha() const;
    void set_tool_marker_alpha(float alpha);

private:
    Settings m_settings;
    Toolpaths m_toolpaths;
    ViewRange m_view_range;

//################################################################################################################################
    // Debug
    void render_debug_window();
//################################################################################################################################
};

} // namespace libvgcode

//################################################################################################################################
// PrusaSlicer development only -> !!!TO BE REMOVED!!!
#endif // ENABLE_NEW_GCODE_VIEWER
//################################################################################################################################

#endif // VGCODE_VIEWER_HPP
