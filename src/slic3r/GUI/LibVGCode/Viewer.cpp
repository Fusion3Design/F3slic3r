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
#include "slic3r/GUI/GUI_App.hpp"
#include "slic3r/GUI/ImGuiWrapper.hpp"
//################################################################################################################################

namespace libvgcode {

void Viewer::init()
{
    m_toolpaths.init();
}

void Viewer::load(const Slic3r::GCodeProcessorResult& gcode_result, const std::vector<std::string>& str_tool_colors)
{
    m_toolpaths.load(gcode_result, str_tool_colors);
}

void Viewer::render(const Mat4x4f& view_matrix, const Mat4x4f& projection_matrix)
{
    m_toolpaths.render(view_matrix, projection_matrix);

//################################################################################################################################
    // Debug
    render_debug_window();
//################################################################################################################################
}

EViewType Viewer::get_view_type() const
{
    return m_toolpaths.get_view_type();
}

void Viewer::set_view_type(EViewType type)
{
    m_toolpaths.set_view_type(type);
}

ETimeMode Viewer::get_time_mode() const
{
    return m_toolpaths.get_time_mode();
}

void Viewer::set_time_mode(ETimeMode mode)
{
    m_toolpaths.set_time_mode(mode);
}

const std::array<std::vector<float>, static_cast<size_t>(ETimeMode::COUNT)>& Viewer::get_layers_times() const
{
    return m_toolpaths.get_layers_times();
}

bool Viewer::is_option_visible(EOptionType type) const
{
    return m_toolpaths.is_option_visible(type);
}

void Viewer::toggle_option_visibility(EOptionType type)
{
    m_toolpaths.toggle_option_visibility(type);
}

bool Viewer::is_extrusion_role_visible(EGCodeExtrusionRole role) const
{
    return m_toolpaths.is_extrusion_role_visible(role);
}

void Viewer::toggle_extrusion_role_visibility(EGCodeExtrusionRole role)
{
    m_toolpaths.toggle_extrusion_role_visibility(role);
}

const std::array<size_t, 2>& Viewer::get_view_current_range() const
{
    return m_toolpaths.get_view_current_range();
}

const std::array<size_t, 2>& Viewer::get_view_global_range() const
{
    return m_toolpaths.get_view_global_range();
}

void Viewer::set_view_current_range(size_t min, size_t max)
{
    m_toolpaths.set_view_current_range(min, max);
}

Vec3f Viewer::get_cog_position() const
{
    return m_toolpaths.get_cog_marker_position();
}

float Viewer::get_cog_marker_scale_factor() const
{
    return m_toolpaths.get_cog_marker_scale_factor();
}

void Viewer::set_cog_marker_scale_factor(float factor)
{
    m_toolpaths.set_cog_marker_scale_factor(factor);
}

const Vec3f& Viewer::get_tool_marker_position() const
{
    return m_toolpaths.get_tool_marker_position();
}

void Viewer::set_tool_marker_position(const Vec3f& position)
{
    m_toolpaths.set_tool_marker_position(position);
}

float Viewer::get_tool_marker_scale_factor() const
{
    return m_toolpaths.get_tool_marker_scale_factor();
}

void Viewer::set_tool_marker_scale_factor(float factor)
{
    m_toolpaths.set_tool_marker_scale_factor(factor);
}

const Color& Viewer::get_tool_marker_color() const
{
    return m_toolpaths.get_tool_marker_color();
}

void Viewer::set_tool_marker_color(const Color& color)
{
    m_toolpaths.set_tool_marker_color(color);
}

float Viewer::get_tool_marker_alpha() const
{
    return m_toolpaths.get_tool_marker_alpha();
}

void Viewer::set_tool_marker_alpha(float alpha)
{
    m_toolpaths.set_tool_marker_alpha(alpha);
}

//################################################################################################################################
// Debug
void Viewer::render_debug_window()
{
    Slic3r::GUI::ImGuiWrapper& imgui = *Slic3r::GUI::wxGetApp().imgui();
    imgui.begin(std::string("LibVGCode Viewer Debug"), ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

    if (ImGui::BeginTable("Data", 2)) {

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        imgui.text_colored(Slic3r::GUI::ImGuiWrapper::COL_ORANGE_LIGHT, "# vertices");
        ImGui::TableSetColumnIndex(1);
        imgui.text(std::to_string(m_toolpaths.get_vertices_count()));

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        imgui.text_colored(Slic3r::GUI::ImGuiWrapper::COL_ORANGE_LIGHT, "# enabled lines");
        ImGui::TableSetColumnIndex(1);
        const std::pair<uint32_t, uint32_t>& enabled_segments_range = m_toolpaths.get_enabled_segments_range();
        imgui.text(std::to_string(m_toolpaths.get_enabled_segments_count()) + " [" + std::to_string(enabled_segments_range.first) + "-" + std::to_string(enabled_segments_range.second) + "]");

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        imgui.text_colored(Slic3r::GUI::ImGuiWrapper::COL_ORANGE_LIGHT, "# enabled options");
        ImGui::TableSetColumnIndex(1);
        const std::pair<uint32_t, uint32_t>& enabled_options_range = m_toolpaths.get_enabled_options_range();
        imgui.text(std::to_string(m_toolpaths.get_enabled_options_count()) + " [" + std::to_string(enabled_options_range.first) + "-" + std::to_string(enabled_options_range.second) + "]");

        ImGui::Separator();

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        imgui.text_colored(Slic3r::GUI::ImGuiWrapper::COL_ORANGE_LIGHT, "sequential range");
        ImGui::TableSetColumnIndex(1);
        const std::array<size_t, 2>& current_view_range = m_toolpaths.get_view_current_range();
        imgui.text(std::to_string(current_view_range[0]) + " - " + std::to_string(current_view_range[1]));

        auto add_range_property_row = [&imgui](const std::string& label, const std::array<float, 2>& range) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            imgui.text_colored(Slic3r::GUI::ImGuiWrapper::COL_ORANGE_LIGHT, label);
            ImGui::TableSetColumnIndex(1);
            char buf[64];
            ::sprintf(buf, "%.3f - %.3f", range[0], range[1]);
            imgui.text(buf);
        };

        add_range_property_row("height range", m_toolpaths.get_height_range());
        add_range_property_row("width range", m_toolpaths.get_width_range());
        add_range_property_row("speed range", m_toolpaths.get_speed_range());
        add_range_property_row("fan speed range", m_toolpaths.get_fan_speed_range());
        add_range_property_row("temperature range", m_toolpaths.get_temperature_range());
        add_range_property_row("volumetric rate range", m_toolpaths.get_volumetric_rate_range());
        add_range_property_row("layer time linear range", m_toolpaths.get_layer_time_linear_range());
        add_range_property_row("layer time logarithmic range", m_toolpaths.get_layer_time_logarithmic_range());

        ImGui::EndTable();

        ImGui::Separator();
        
        if (ImGui::BeginTable("Cog", 2)) {

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            imgui.text_colored(Slic3r::GUI::ImGuiWrapper::COL_ORANGE_LIGHT, "Cog marker scale factor");
            ImGui::TableSetColumnIndex(1);
            imgui.text(std::to_string(get_cog_marker_scale_factor()));

            ImGui::EndTable();
        }

        ImGui::Separator();

        if (ImGui::BeginTable("Tool", 2)) {

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            imgui.text_colored(Slic3r::GUI::ImGuiWrapper::COL_ORANGE_LIGHT, "Tool marker scale factor");
            ImGui::TableSetColumnIndex(1);
            imgui.text(std::to_string(get_tool_marker_scale_factor()));

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            imgui.text_colored(Slic3r::GUI::ImGuiWrapper::COL_ORANGE_LIGHT, "Tool marker color");
            ImGui::TableSetColumnIndex(1);
            Color color = get_tool_marker_color();
            if (ImGui::ColorPicker3("##ToolColor", color.data()))
                set_tool_marker_color(color);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            imgui.text_colored(Slic3r::GUI::ImGuiWrapper::COL_ORANGE_LIGHT, "Tool marker alpha");
            ImGui::TableSetColumnIndex(1);
            float tool_alpha = get_tool_marker_alpha();
            if (imgui.slider_float("##ToolAlpha", &tool_alpha, 0.25f, 0.75f))
                set_tool_marker_alpha(tool_alpha);

            ImGui::EndTable();
        }
    }

    imgui.end();
}
//################################################################################################################################

} // namespace libvgcode

//################################################################################################################################
// PrusaSlicer development only -> !!!TO BE REMOVED!!!
#endif // ENABLE_NEW_GCODE_VIEWER
//################################################################################################################################
