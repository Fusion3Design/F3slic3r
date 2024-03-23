///|/ Copyright (c) Prusa Research 2020 - 2023 Oleksandra Iushchenko @YuSanka, Vojtěch Bubník @bubnikv, Tomáš Mészáros @tamasmeszaros, Lukáš Matěna @lukasmatena, Enrico Turri @enricoturri1966
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "libslic3r/libslic3r.h"
#include "DoubleSlider.hpp"
#include "libslic3r/GCode.hpp"
#include "GUI.hpp"
#include "GUI_App.hpp"
#include "Plater.hpp"
#include "I18N.hpp"
#include "ExtruderSequenceDialog.hpp"
#include "libslic3r/Print.hpp"
#include "libslic3r/AppConfig.hpp"
#include "GUI_Utils.hpp"
#include "MsgDialog.hpp"
#include "Tab.hpp"
#include "GUI_ObjectList.hpp"

#include <wx/dialog.h>
#include <wx/menu.h>
#include <wx/colordlg.h>

#include <cmath>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/split.hpp>
#include "Field.hpp"
#include "format.hpp"
#include "NotificationManager.hpp"

#include "ImGuiPureWrap.hpp"

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include <imgui/imgui_internal.h>

namespace Slic3r {

using namespace GUI;

namespace DoubleSlider {

constexpr double min_delta_area = scale_(scale_(25));  // equal to 25 mm2
constexpr double miscalculation = scale_(scale_(1));   // equal to 1 mm2

static const float LEFT_MARGIN             = 13.0f + 100.0f;  // avoid thumbnail toolbar
static const float HORIZONTAL_SLIDER_WIDTH = 90.0f;
static const float VERTICAL_SLIDER_HEIGHT  = 105.0f;

bool equivalent_areas(const double& bottom_area, const double& top_area)
{
    return fabs(bottom_area - top_area) <= miscalculation;
}

static std::string gcode(Type type)
{
    const PrintConfig& config = GUI::wxGetApp().plater()->fff_print().config();
    switch (type) {
    case ColorChange: return config.color_change_gcode;
    case PausePrint:  return config.pause_print_gcode;
    case Template:    return config.template_custom_gcode;
    default:          return "";
    }
}

int DSManagerForLayers::get_tick_from_value(double value, bool force_lower_bound/* = false*/)
{
    std::vector<double>::iterator it;
    if (m_is_wipe_tower && !force_lower_bound)
        it = std::find_if(m_values.begin(), m_values.end(),
                          [value](const double & val) { return fabs(value - val) <= epsilon(); });
    else
        it = std::lower_bound(m_values.begin(), m_values.end(), value - epsilon());

    if (it == m_values.end())
        return -1;
    return int(it - m_values.begin());
}

Info DSManagerForLayers::GetTicksValues() const
{
    Info custom_gcode_per_print_z;
    std::vector<CustomGCode::Item>& values = custom_gcode_per_print_z.gcodes;

    const int val_size = m_values.size();
    if (!m_values.empty())
        for (const TickCode& tick : m_ticks.ticks) {
            if (tick.tick > val_size)
                break;
            values.emplace_back(CustomGCode::Item{ m_values[tick.tick], tick.type, tick.extruder, tick.color, tick.extra });
        }

    if (m_force_mode_apply)
        custom_gcode_per_print_z.mode = m_mode;

    return custom_gcode_per_print_z;
}

void DSManagerForLayers::SetTicksValues(const Info& custom_gcode_per_print_z)
{
    if (m_values.empty()) {
        m_ticks.mode = m_mode;
        return;
    }

    const bool was_empty = m_ticks.empty();

    m_ticks.ticks.clear();
    const std::vector<CustomGCode::Item>& heights = custom_gcode_per_print_z.gcodes;
    for (auto h : heights) {
        int tick = get_tick_from_value(h.print_z);
        if (tick >=0)
            m_ticks.ticks.emplace(TickCode{ tick, h.type, h.extruder, h.color, h.extra });
    }
    
    if (!was_empty && m_ticks.empty())
        // Switch to the "Feature type"/"Tool" from the very beginning of a new object slicing after deleting of the old one
        post_ticks_changed_event();

    // init extruder sequence in respect to the extruders count 
    if (m_ticks.empty())
        m_extruders_sequence.init(m_extruder_colors.size());
    update_callbacks();

    if (custom_gcode_per_print_z.mode && !custom_gcode_per_print_z.gcodes.empty())
        m_ticks.mode = custom_gcode_per_print_z.mode;
}

void DSManagerForLayers::SetLayersTimes(const std::vector<float>& layers_times, float total_time)
{ 
    m_layers_times.clear();
    if (layers_times.empty())
        return;
    m_layers_times.resize(layers_times.size(), 0.0);
    m_layers_times[0] = layers_times[0];
    for (size_t i = 1; i < layers_times.size(); i++)
        m_layers_times[i] = m_layers_times[i - 1] + layers_times[i];

    // Erase duplicates values from m_values and save it to the m_layers_values
    // They will be used for show the correct estimated time for MM print, when "No sparce layer" is enabled
    // See https://github.com/prusa3d/PrusaSlicer/issues/6232
    if (m_is_wipe_tower && m_values.size() != m_layers_times.size()) {
        m_layers_values = m_values;
        sort(m_layers_values.begin(), m_layers_values.end());
        m_layers_values.erase(unique(m_layers_values.begin(), m_layers_values.end()), m_layers_values.end());

        // When whipe tower is used to the end of print, there is one layer which is not marked in layers_times
        // So, add this value from the total print time value
        if (m_layers_values.size() != m_layers_times.size())
            for (size_t i = m_layers_times.size(); i < m_layers_values.size(); i++)
                m_layers_times.push_back(total_time);
    }
}

void DSManagerForLayers::SetLayersTimes(const std::vector<double>& layers_times)
{ 
    m_is_wipe_tower = false;
    m_layers_times = layers_times;
    for (size_t i = 1; i < m_layers_times.size(); i++)
        m_layers_times[i] += m_layers_times[i - 1];
}

void DSManagerForLayers::SetDrawMode(bool is_sla_print, bool is_sequential_print)
{ 
    m_draw_mode = is_sla_print          ? dmSlaPrint            : 
                  is_sequential_print   ? dmSequentialFffPrint  : 
                                          dmRegular;

    update_callbacks();
}

void DSManagerForLayers::SetModeAndOnlyExtruder(const bool is_one_extruder_printed_model, const int only_extruder)
{
    m_mode = !is_one_extruder_printed_model ? MultiExtruder :
             only_extruder < 0              ? SingleExtruder :
                                              MultiAsSingle;
    if (!m_ticks.mode || (m_ticks.empty() && m_ticks.mode != m_mode))
        m_ticks.mode = m_mode;
    m_only_extruder = only_extruder;

    if (m_mode != SingleExtruder)
        UseDefaultColors(false);

    m_is_wipe_tower = m_mode != SingleExtruder;
}

void DSManagerForLayers::SetExtruderColors( const std::vector<std::string>& extruder_colors)
{
    m_extruder_colors = extruder_colors;
}

bool DSManagerForLayers::IsNewPrint(const std::string& idxs)
{
    if (idxs == "sla" || idxs == m_print_obj_idxs)
        return false;

    m_print_obj_idxs = idxs;
    return true;
}

void DSManagerForLayers::update_callbacks()
{
    if (m_ticks.empty() || m_draw_mode == dmSequentialFffPrint)
        imgui_ctrl.set_draw_scroll_line_cb(nullptr);
    else
        imgui_ctrl.set_draw_scroll_line_cb([this](const ImRect& scroll_line, const ImRect& slideable_region) { draw_colored_band(scroll_line, slideable_region); });
}

using namespace ImGui;

void DSManagerForLayers::draw_ticks(const ImRect& slideable_region)
{
    //if(m_draw_mode != dmRegular)
    //    return;
    //if (m_ticks.empty() || m_mode == MultiExtruder)
    //    return;
    if (m_ticks.empty())
        return;

    ImGuiContext& context = *GImGui;

    const ImVec2 tick_border = ImVec2(23.0f, 2.0f) * m_scale;
    // distance form center         begin  end 
    const ImVec2 tick_size   = ImVec2(19.0f, 11.0f) * m_scale;
    const float  tick_width  = 1.0f * m_scale;
    const float  icon_side   = wxGetApp().imgui()->GetTextureCustomRect(ImGui::PausePrint)->Height;
    const float  icon_offset = 0.5f * icon_side;;

    const ImU32 tick_clr         = ImGui::ColorConvertFloat4ToU32(ImGuiPureWrap::COL_ORANGE_DARK);
    const ImU32 tick_hovered_clr = ImGui::ColorConvertFloat4ToU32(ImGuiPureWrap::COL_WINDOW_BACKGROUND);

    auto get_tick_pos = [this, slideable_region](int tick) {
        return imgui_ctrl.GetPositionFromValue(tick, slideable_region);
    };

    std::set<TickCode>::const_iterator tick_it = m_ticks.ticks.begin();
    while (tick_it != m_ticks.ticks.end())
    {
        float tick_pos = get_tick_pos(tick_it->tick);

        //draw tick hover box when hovered
        ImRect tick_hover_box = ImRect(slideable_region.GetCenter().x - tick_border.x, tick_pos - tick_border.y, 
                                       slideable_region.GetCenter().x + tick_border.x, tick_pos + tick_border.y - tick_width);

        if (ImGui::IsMouseHoveringRect(tick_hover_box.Min, tick_hover_box.Max)) {
            ImGui::RenderFrame(tick_hover_box.Min, tick_hover_box.Max, tick_hovered_clr, false);
            break;
        }
        ++tick_it;
    }
     
    auto active_tick_it = m_ticks.ticks.find(TickCode{ imgui_ctrl.GetActiveValue() });

    tick_it = m_ticks.ticks.begin();
    while (tick_it != m_ticks.ticks.end())
    {
        float tick_pos = get_tick_pos(tick_it->tick);

        //draw ticks
        ImRect tick_left    = ImRect(slideable_region.GetCenter().x - tick_size.x, tick_pos - tick_width, slideable_region.GetCenter().x - tick_size.y, tick_pos);
        ImRect tick_right   = ImRect(slideable_region.GetCenter().x + tick_size.y, tick_pos - tick_width, slideable_region.GetCenter().x + tick_size.x, tick_pos);
        ImGui::RenderFrame(tick_left.Min, tick_left.Max, tick_clr, false);
        ImGui::RenderFrame(tick_right.Min, tick_right.Max, tick_clr, false);

        ImVec2      icon_pos    = ImVec2(tick_right.Max.x + icon_offset, tick_pos - icon_offset);
        std::string btn_label   = "tick " + std::to_string(tick_it->tick);

        //draw tick icon-buttons
        bool activate_this_tick = false;
        if (tick_it == active_tick_it && m_allow_editing) {
            // delete tick
            if (render_button(ImGui::RemoveTick, ImGui::RemoveTickHovered, btn_label, icon_pos, imgui_ctrl.IsActiveHigherThumb() ? fiHigherThumb : fiLowerThumb, tick_it->tick)) {
                Type type = tick_it->type;
                m_ticks.ticks.erase(tick_it);
                post_ticks_changed_event(type);
                break;
            }
        }        
        else if (m_draw_mode != dmRegular)// if we have non-regular draw mode, all ticks should be marked with error icon
            activate_this_tick = render_button(ImGui::ErrorTick, ImGui::ErrorTickHovered, btn_label, icon_pos, fiActionIcon, tick_it->tick);
        else if (tick_it->type == ColorChange || tick_it->type == ToolChange) {
            if (m_ticks.is_conflict_tick(*tick_it, m_mode, m_only_extruder, m_values[tick_it->tick]))
                activate_this_tick = render_button(ImGui::ErrorTick, ImGui::ErrorTickHovered, btn_label, icon_pos, fiActionIcon, tick_it->tick);
        }
        else if (tick_it->type == PausePrint)
            activate_this_tick = render_button(ImGui::PausePrint, ImGui::PausePrintHovered, btn_label, icon_pos, fiActionIcon, tick_it->tick);
        else
            activate_this_tick = render_button(ImGui::EditGCode, ImGui::EditGCodeHovered, btn_label, icon_pos, fiActionIcon, tick_it->tick);

        if (activate_this_tick) {
            imgui_ctrl.IsActiveHigherThumb() ? SetHigherValue(tick_it->tick) : SetLowerValue(tick_it->tick);
            break;
        }

        ++tick_it;
    }
}

inline int hex_to_int(const char c)
{
    return (c >= '0' && c <= '9') ? int(c - '0') : (c >= 'A' && c <= 'F') ? int(c - 'A') + 10 : (c >= 'a' && c <= 'f') ? int(c - 'a') + 10 : -1;
}

static std::array<float, 4> decode_color_to_float_array(const std::string color)
{
    // set alpha to 1.0f by default
    std::array<float, 4> ret = { 0, 0, 0, 1.0f };
    const char* c = color.data() + 1;
    if (color.size() == 7 && color.front() == '#') {
        for (size_t j = 0; j < 3; ++j) {
            int digit1 = hex_to_int(*c++);
            int digit2 = hex_to_int(*c++);
            if (digit1 == -1 || digit2 == -1) break;
            ret[j] = float(digit1 * 16 + digit2) / 255.0f;
        }
    }
    return ret;
}

void DSManagerForLayers::draw_colored_band(const ImRect& groove, const ImRect& slideable_region)
{
    if (m_ticks.empty() || m_draw_mode == dmSequentialFffPrint)
        return;

    ImVec2 blank_padding = ImVec2(5.0f, 2.0f) * m_scale;
    float  blank_width = 1.0f * m_scale;

    ImRect blank_rect = ImRect(groove.GetCenter().x - blank_width, groove.Min.y, groove.GetCenter().x + blank_width, groove.Max.y);

    ImRect main_band = ImRect(blank_rect);
    main_band.Expand(blank_padding);

    auto draw_band = [](const ImU32& clr, const ImRect& band_rc) {
        ImGui::RenderFrame(band_rc.Min, band_rc.Max, clr, false, band_rc.GetWidth() * 0.5);
        //cover round corner
        ImGui::RenderFrame(ImVec2(band_rc.Min.x, band_rc.Max.y - band_rc.GetWidth() * 0.5), band_rc.Max, clr, false);
    };

    auto draw_main_band = [&main_band](const ImU32& clr) {
        ImGui::RenderFrame(main_band.Min, main_band.Max, clr, false, main_band.GetWidth() * 0.5);
    };

    //draw main colored band
    const int default_color_idx = m_mode == MultiAsSingle ? std::max<int>(m_only_extruder - 1, 0) : 0;
    std::array<float, 4>rgba = decode_color_to_float_array(m_extruder_colors[default_color_idx]);
    ImU32 band_clr = IM_COL32(rgba[0] * 255.0f, rgba[1] * 255.0f, rgba[2] * 255.0f, rgba[3] * 255.0f);
    draw_main_band(band_clr);

    static float tick_pos;
    std::set<TickCode>::const_iterator tick_it = m_ticks.ticks.begin();
    while (tick_it != m_ticks.ticks.end())
    {
        //get position from tick
        tick_pos = imgui_ctrl.GetPositionFromValue(tick_it->tick, slideable_region);

        ImRect band_rect = ImRect(ImVec2(main_band.Min.x, std::min(tick_pos, main_band.Min.y)), 
                                  ImVec2(main_band.Max.x, std::min(tick_pos, main_band.Max.y)));

        if (main_band.Contains(band_rect)) {
            if ((m_mode == SingleExtruder && tick_it->type == ColorChange) ||
                (m_mode == MultiAsSingle && (tick_it->type == ToolChange || tick_it->type == ColorChange)))
            {
                const std::string clr_str = m_mode == SingleExtruder ? tick_it->color :
                    tick_it->type == ToolChange ?
                    get_color_for_tool_change_tick(tick_it) :
                    get_color_for_color_change_tick(tick_it);

                if (!clr_str.empty()) {
                    std::array<float, 4>rgba = decode_color_to_float_array(clr_str);
                    ImU32 band_clr = IM_COL32(rgba[0] * 255.0f, rgba[1] * 255.0f, rgba[2] * 255.0f, rgba[3] * 255.0f);
                    if (tick_it->tick == 0)
                        draw_main_band(band_clr);
                    else
                        draw_band(band_clr, band_rect);
                }
            }
        }
        tick_it++;
    }
}

void DSManagerForLayers::render_menu()
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 10.0f) * m_scale);
    ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 4.0f * m_scale);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 1, ImGui::GetStyle().ItemSpacing.y });

    ImGui::PushStyleColor(ImGuiCol_::ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));

    std::vector<std::string> colors = wxGetApp().plater()->get_extruder_colors_from_plater_config();
    int extruder_num = colors.size();

    if (imgui_ctrl.is_rclick_on_thumb()) {
        ImGui::OpenPopup("slider_menu_popup");
    }

    ImGui::PushStyleVar(ImGuiStyleVar_::ImGuiStyleVar_ChildRounding, 4.0f * m_scale);
    if (ImGui::BeginPopup("slider_menu_popup")) {
        if ((!imgui_ctrl.IsActiveHigherThumb() && GetLowerValueD() == 0.0) || 
            (imgui_ctrl.IsActiveHigherThumb() && GetHigherValueD() == 0.0))
        {
            menu_item_with_icon(_u8L("Add Pause").c_str(), "", ImVec2(0, 0), 0, false, false);
        }
        else
        {
            if (menu_item_with_icon(_u8L("Add Color Change").c_str(), "")) {
                add_code_as_tick(ColorChange);
            }
            if (menu_item_with_icon(_u8L("Add Pause").c_str(), "")) {
                add_code_as_tick(PausePrint);
            }
            if (menu_item_with_icon(_u8L("Add Custom G-code").c_str(), "")) {
                add_code_as_tick(Custom);
            }
            if (!gcode(Template).empty()) {
                if (menu_item_with_icon(_u8L("Add Custom Template").c_str(), "")) {
                    add_code_as_tick(Template);
                }
            }
        }

        //BBS render this menu item only when extruder_num > 1
        if (extruder_num > 1) {
            if (!m_can_change_color || m_draw_mode == dmSequentialFffPrint) {
                begin_menu(_u8L("Change Filament").c_str(), false);
            }
            else if (begin_menu(_u8L("Change Filament").c_str())) {
                for (int i = 0; i < extruder_num; i++) {
                    std::array<float, 4> rgba = decode_color_to_float_array(colors[i]);
                    ImU32                icon_clr = IM_COL32(rgba[0] * 255.0f, rgba[1] * 255.0f, rgba[2] * 255.0f, rgba[3] * 255.0f);
                    if (menu_item_with_icon((_u8L("Filament ") + std::to_string(i + 1)).c_str(), "", ImVec2(14, 14) * m_scale, icon_clr)) add_code_as_tick(ToolChange, i + 1);
                }
                end_menu();
            }
        }
        ImGui::EndPopup();
    }
    ImGui::PopStyleVar(1);

    ImGui::PopStyleColor(1);
    ImGui::PopStyleVar(3);
}

bool DSManagerForLayers::render_button(const wchar_t btn_icon, const wchar_t btn_icon_hovered, const std::string& label_id, const ImVec2& pos, FocusedItem focus, int tick /*= -1*/)
{
    float scale = (float)wxGetApp().em_unit() / 10.0f;

    ImGui::PushStyleVar(ImGuiStyleVar_::ImGuiStyleVar_WindowBorderSize, 0);
    ImGui::PushStyleVar(ImGuiStyleVar_::ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_::ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    ImGui::PushStyleColor(ImGuiCol_::ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_Text));

    int windows_flag =    ImGuiWindowFlags_NoTitleBar
                        | ImGuiWindowFlags_NoCollapse
                        | ImGuiWindowFlags_NoMove
                        | ImGuiWindowFlags_NoResize
                        | ImGuiWindowFlags_NoScrollbar
                        | ImGuiWindowFlags_NoScrollWithMouse;

    auto m_imgui = wxGetApp().imgui();
    ImGuiPureWrap::set_next_window_pos(pos.x, pos.y, ImGuiCond_Always);
    std::string win_name = label_id + "##btn_win";
    ImGuiPureWrap::begin(win_name, windows_flag);

    ImGuiContext& g = *GImGui;

    m_focus = focus;
    std::string tooltip = m_allow_editing ? into_u8(get_tooltip(tick)) : "";
    ImGui::SetCursorPos(ImVec2(0, 0));
    const bool ret = m_imgui->image_button(g.HoveredWindow == g.CurrentWindow ? btn_icon_hovered : btn_icon, tooltip, false);

    ImGuiPureWrap::end();

    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(3);

    return ret;
}

void DSManagerForGcode::imgui_render(const int canvas_width, const int canvas_height, float extra_scale/* = 0.1f*/)
{
    if (!imgui_ctrl.IsShown())
        return;
    m_scale = extra_scale * 0.1f * wxGetApp().em_unit();

    ImVec2 pos  = ImVec2{std::max(LEFT_MARGIN, 0.2f * canvas_width), canvas_height - HORIZONTAL_SLIDER_WIDTH * m_scale};
    ImVec2 size = ImVec2(canvas_width - 2 * pos.x, HORIZONTAL_SLIDER_WIDTH * m_scale);

    imgui_ctrl.Init(pos, size, m_scale);
    if (imgui_ctrl.render())
        process_thumb_move();
}

void DSManagerForLayers::imgui_render(const int canvas_width, const int canvas_height, float extra_scale/* = 0.1f*/)
{
    if (!imgui_ctrl.IsShown())
        return;
    m_scale = extra_scale * 0.1f * wxGetApp().em_unit();

    const float action_btn_sz   = wxGetApp().imgui()->GetTextureCustomRect(ImGui::DSRevert)->Height;
    const float tick_icon_side  = wxGetApp().imgui()->GetTextureCustomRect(ImGui::PausePrint)->Height;

    ImVec2 pos;
    ImVec2 size;

    pos.x = canvas_width - VERTICAL_SLIDER_HEIGHT * m_scale - tick_icon_side;
    pos.y = 1.f * action_btn_sz;
    if (m_allow_editing)
        pos.y += 2.f;
    size = ImVec2(VERTICAL_SLIDER_HEIGHT * m_scale, canvas_height - 4.f * action_btn_sz);
    imgui_ctrl.ShowLabelOnMouseMove(true);

    imgui_ctrl.Init(pos, size, m_scale);
    if (imgui_ctrl.render()) {
        // request one more frame if value was changes with mouse wheel
        if (GImGui->IO.MouseWheel != 0.0f)
            wxGetApp().imgui()->set_requires_extra_frame();
        process_thumb_move();
    }

    // draw action buttons

    const float groove_center_x = imgui_ctrl.GetGrooveRect().GetCenter().x;

    ImVec2 btn_pos = ImVec2(groove_center_x - 0.5f * action_btn_sz, pos.y - 0.25f * action_btn_sz);

    if (!m_ticks.empty() && m_allow_editing &&
        render_button(ImGui::DSRevert, ImGui::DSRevertHovered, "revert", btn_pos, fiRevertIcon))
        discard_all_thicks();

    btn_pos.y += 0.1f * action_btn_sz + size.y;
    const bool is_one_layer = imgui_ctrl.IsCombineThumbs();
    if (render_button(is_one_layer ? ImGui::Lock : ImGui::Unlock, is_one_layer ? ImGui::LockHovered : ImGui::UnlockHovered, "one_layer", btn_pos, fiOneLayerIcon))
        ChangeOneLayerLock();

    btn_pos.y += 1.2f * action_btn_sz;
    if (render_button(ImGui::DSSettings, ImGui::DSSettingsHovered, "settings", btn_pos, fiCogIcon))
        show_cog_icon_context_menu();

    if (m_draw_mode == dmSequentialFffPrint && imgui_ctrl.is_rclick_on_thumb()) {
        std::string tooltip = _u8L("The sequential print is on.\n"
                                   "It's impossible to apply any custom G-code for objects printing sequentually.");
        ImGuiPureWrap::tooltip(tooltip, ImGui::GetFontSize() * 20.0f);
    }
    else if (m_allow_editing)
        render_menu();
}

bool DSManagerForLayers::is_wipe_tower_layer(int tick) const
{
    if (!m_is_wipe_tower || tick >= (int)m_values.size())
        return false;
    if (tick == 0 || (tick == (int)m_values.size() - 1 && m_values[tick] > m_values[tick - 1]))
        return false;
    if ((m_values[tick - 1] == m_values[tick + 1] && m_values[tick] < m_values[tick + 1]) ||
        (tick > 0 && m_values[tick] < m_values[tick - 1]) ) // if there is just one wiping on the layer 
        return true;

    return false;
}

static std::string short_and_splitted_time(const std::string& time)
{
    // Parse the dhms time format.
    int days = 0;
    int hours = 0;
    int minutes = 0;
    int seconds = 0;
    if (time.find('d') != std::string::npos)
        ::sscanf(time.c_str(), "%dd %dh %dm %ds", &days, &hours, &minutes, &seconds);
    else if (time.find('h') != std::string::npos)
        ::sscanf(time.c_str(), "%dh %dm %ds", &hours, &minutes, &seconds);
    else if (time.find('m') != std::string::npos)
        ::sscanf(time.c_str(), "%dm %ds", &minutes, &seconds);
    else if (time.find('s') != std::string::npos)
        ::sscanf(time.c_str(), "%ds", &seconds);

    // Format the dhm time.
    auto get_d = [days]()   { return GUI::format(_u8L("%1%d"), days); };
    auto get_h = [hours]()  { return GUI::format(_u8L("%1%h"), hours); };
    auto get_m = [minutes](){ return GUI::format(_u8L("%1%m"), minutes); };
    auto get_s = [seconds](){ return GUI::format(_u8L("%1%s"), seconds); };

    if (days > 0)
        return GUI::format("%1%%2%\n%3%", get_d(), get_h(), get_m());
    if (hours > 0) {
        if (hours < 10 && minutes < 10 && seconds < 10)
            return GUI::format("%1%%2%%3%", get_h(), get_m(), get_s());
        if (hours > 10 && minutes > 10 && seconds > 10)
            return GUI::format("%1%\n%2%\n%3%", get_h(), get_m(), get_s());
        if ((minutes < 10 && seconds > 10) || (minutes > 10 && seconds < 10))
            return GUI::format("%1%\n%2%%3%", get_h(), get_m(), get_s());
        return GUI::format("%1%%2%\n%3%", get_h(), get_m(), get_s());
    }
    if (minutes > 0) {
        if (minutes > 10 && seconds > 10)
            return GUI::format("%1%\n%2%", get_m(), get_s());
        return GUI::format("%1%%2%", get_m(), get_s());
    }
    return get_s();
}

std::string DSManagerForLayers::get_label(int pos, LabelType label_type/* = ltHeightWithLayer*/) const
{
    const size_t value = pos;

    if (m_values.empty())
        return GUI::format("%1%", pos);
    if (value >= m_values.size())
        return "ErrVal";

    // When "Print Settings -> Multiple Extruders -> No sparse layer" is enabled, then "Smart" Wipe Tower is used for wiping.
    // As a result, each layer with tool changes is splited for min 3 parts: first tool, wiping, second tool ...
    // So, vertical slider have to respect to this case.
    // see https://github.com/prusa3d/PrusaSlicer/issues/6232.
    // m_values contains data for all layer's parts,
    // but m_layers_values contains just unique Z values.
    // Use this function for correct conversion slider position to number of printed layer
    auto get_layer_number = [this](int value, LabelType label_type) {
        if (label_type == ltEstimatedTime && m_layers_times.empty())
            return size_t(-1);
        double layer_print_z = m_values[is_wipe_tower_layer(value) ? std::max<int>(value - 1, 0) : value];
        auto it = std::lower_bound(m_layers_values.begin(), m_layers_values.end(), layer_print_z - epsilon());
        if (it == m_layers_values.end()) {
            it = std::lower_bound(m_values.begin(), m_values.end(), layer_print_z - epsilon());
            if (it == m_values.end())
                return size_t(-1);
            return size_t(value);
        }
        return size_t(it - m_layers_values.begin());
    };

    if (label_type == ltEstimatedTime) {
        if (m_is_wipe_tower) {
            size_t layer_number = get_layer_number(value, label_type);
            return (layer_number == size_t(-1) || layer_number == m_layers_times.size()) ? "" : short_and_splitted_time(get_time_dhms(m_layers_times[layer_number]));
        }
        return value < m_layers_times.size() ? short_and_splitted_time(get_time_dhms(m_layers_times[value])) : "";
    }
    std::string str = GUI::format("%1$.2f", m_values[value]);
    if (label_type == ltHeight)
        return str;
    if (label_type == ltHeightWithLayer) {
        size_t layer_number = m_is_wipe_tower ? get_layer_number(value, label_type) + 1 : (m_values.empty() ? value : value + 1);
        return GUI::format("%1%\n(%2%)", str, layer_number);
    }    

    return "";
}

std::string DSManagerForLayers::get_color_for_tool_change_tick(std::set<TickCode>::const_iterator it) const
{
    const int current_extruder = it->extruder == 0 ? std::max<int>(m_only_extruder, 1) : it->extruder;

    auto it_n = it;
    while (it_n != m_ticks.ticks.begin()) {
        --it_n;
        if (it_n->type == ColorChange && it_n->extruder == current_extruder)
            return it_n->color;
    }

    return m_extruder_colors[current_extruder-1]; // return a color for a specific extruder from the colors list 
}

std::string DSManagerForLayers::get_color_for_color_change_tick(std::set<TickCode>::const_iterator it) const
{
    const int def_extruder = std::max<int>(1, m_only_extruder);
    auto it_n = it;
    bool is_tool_change = false;
    while (it_n != m_ticks.ticks.begin()) {
        --it_n;
        if (it_n->type == ToolChange) {
            is_tool_change = true;
            if (it_n->extruder == it->extruder)
                return it->color;
            break;
        }
        if (it_n->type == ColorChange && it_n->extruder == it->extruder)
            return it->color;
    }
    if (!is_tool_change && it->extruder == def_extruder)
        return it->color;

    return "";
}

void DSManagerForLayers::ChangeOneLayerLock()
{
    imgui_ctrl.CombineThumbs(!imgui_ctrl.IsCombineThumbs()); 
    process_thumb_move();
}

wxString DSManagerForLayers::get_tooltip(int tick/*=-1*/)
{
    if (m_focus == fiNone)
        return "";
    if (m_focus == fiOneLayerIcon)
        return _L("One layer mode");
    if (m_focus == fiRevertIcon)
        return _L("Discard all custom changes");
    if (m_focus == fiCogIcon)
    {
        return m_mode == MultiAsSingle ?
        GUI::from_u8((boost::format(_u8L("Jump to height %s\n"
                                            "Set ruler mode\n"
                                            "or Set extruder sequence for the entire print")) % "(Shift + G)").str()) :
        GUI::from_u8((boost::format(_u8L("Jump to height %s\n"
                                            "or Set ruler mode")) % "(Shift + G)").str());
    }
    if (m_focus == fiColorBand)
        return m_mode != SingleExtruder ? "" :
               _L("Edit current color - Right click the colored slider segment");
    if (m_focus == fiSmartWipeTower)
        return _L("This is wipe tower layer");
    if (m_draw_mode == dmSlaPrint)
        return ""; // no drawn ticks and no tooltips for them in SlaPrinting mode

    wxString tooltip;
    const auto tick_code_it = m_ticks.ticks.find(TickCode{tick});

    if (tick_code_it == m_ticks.ticks.end() && m_focus == fiActionIcon)    // tick doesn't exist
    {
        if (m_draw_mode == dmSequentialFffPrint)
            return  (_L("The sequential print is on.\n"
                        "It's impossible to apply any custom G-code for objects printing sequentually.") + "\n");

        // Show mode as a first string of tooltop
        tooltip = "    " + _L("Print mode") + ": ";
        tooltip += (m_mode == SingleExtruder ? SingleExtruderMode :
                    m_mode == MultiAsSingle  ? MultiAsSingleMode  :
                                               MultiExtruderMode );
        tooltip += "\n\n";

        /* Note: just on OSX!!!
         * Right click event causes a little scrolling.
         * So, as a workaround we use Ctrl+LeftMouseClick instead of RightMouseClick
         * Show this information in tooltip
         * */

        // Show list of actions with new tick
        tooltip += ( m_mode == MultiAsSingle                                ?
                  _L("Add extruder change - Left click")                    :
                     m_mode == SingleExtruder                               ?
                  _L("Add color change - Left click for predefined color or "
                     "Shift + Left click for custom color selection")       :
                  _L("Add color change - Left click")  ) + " " +
                  _L("or press \"+\" key") + "\n" + (
                     is_osx ? 
                  _L("Add another code - Ctrl + Left click") :
                  _L("Add another code - Right click") );
    }

    if (tick_code_it != m_ticks.ticks.end())                                    // tick exists
    {
        if (m_draw_mode == dmSequentialFffPrint)
            return   _L("The sequential print is on.\n"
                        "It's impossible to apply any custom G-code for objects printing sequentually.\n" 
                        "This code won't be processed during G-code generation.");
        
        // Show custom Gcode as a first string of tooltop
        std::string space = "   ";
        tooltip = space;
        auto format_gcode = [space](std::string gcode) {
            // when the tooltip is too long, it starts to flicker, see: https://github.com/prusa3d/PrusaSlicer/issues/7368
            // so we limit the number of lines shown
            std::vector<std::string> lines;
            boost::split(lines, gcode, boost::is_any_of("\n"), boost::token_compress_off);
            static const size_t MAX_LINES = 10;
            if (lines.size() > MAX_LINES) {
                gcode = lines.front() + '\n';
                for (size_t i = 1; i < MAX_LINES; ++i) {
                    gcode += lines[i] + '\n';
                }
                gcode += "[" + into_u8(_L("continue")) + "]\n";
            }
            boost::replace_all(gcode, "\n", "\n" + space);
            return gcode;
        };
        tooltip +=  
        	tick_code_it->type == ColorChange ?
        		(m_mode == SingleExtruder ?
                	format_wxstr(_L("Color change (\"%1%\")"), gcode(ColorChange)) :
                    format_wxstr(_L("Color change (\"%1%\") for Extruder %2%"), gcode(ColorChange), tick_code_it->extruder)) :
	            tick_code_it->type == PausePrint ?
	                format_wxstr(_L("Pause print (\"%1%\")"), gcode(PausePrint)) :
	            tick_code_it->type == Template ?
	                format_wxstr(_L("Custom template (\"%1%\")"), gcode(Template)) :
		            tick_code_it->type == ToolChange ?
		                format_wxstr(_L("Extruder (tool) is changed to Extruder \"%1%\""), tick_code_it->extruder) :                
		                from_u8(format_gcode(tick_code_it->extra));// tick_code_it->type == Custom

        // If tick is marked as a conflict (exclamation icon),
        // we should to explain why
        ConflictType conflict = m_ticks.is_conflict_tick(*tick_code_it, m_mode, m_only_extruder, m_values[tick]);
        if (conflict != ctNone)
            tooltip += "\n\n" + _L("Note") + "! ";
        if (conflict == ctModeConflict)
            tooltip +=  _L("G-code associated to this tick mark is in a conflict with print mode.\n"
                           "Editing it will cause changes of Slider data.");
        else if (conflict == ctMeaninglessColorChange)
            tooltip +=  _L("There is a color change for extruder that won't be used till the end of print job.\n"
                           "This code won't be processed during G-code generation.");
        else if (conflict == ctMeaninglessToolChange)
            tooltip +=  _L("There is an extruder change set to the same extruder.\n"
                           "This code won't be processed during G-code generation.");
        else if (conflict == ctRedundant)
            tooltip +=  _L("There is a color change for extruder that has not been used before.\n"
                           "Check your settings to avoid redundant color changes.");

        // Show list of actions with existing tick
        if (m_focus == fiActionIcon)
        tooltip += "\n\n" + _L("Delete tick mark - Left click or press \"-\" key") + "\n" + (
                      is_osx ? 
                   _L("Edit tick mark - Ctrl + Left click") :
                   _L("Edit tick mark - Right click") );
    }
    return tooltip;

}

void DSManagerForLayers::append_change_extruder_menu_item(wxMenu* menu, bool switch_current_code/* = false*/)
{
    const int extruders_cnt = GUI::wxGetApp().extruders_edited_cnt();
    if (extruders_cnt > 1) {
        std::array<int, 2> active_extruders = get_active_extruders_for_tick(imgui_ctrl.GetActiveValue());

        std::vector<wxBitmapBundle*> icons = get_extruder_color_icons(true);

        wxMenu* change_extruder_menu = new wxMenu();

        for (int i = 1; i <= extruders_cnt; i++) {
            const bool is_active_extruder = i == active_extruders[0] || i == active_extruders[1];
            const wxString item_name = wxString::Format(_L("Extruder %d"), i) +
                                       (is_active_extruder ? " (" + _L("active") + ")" : "");

            if (m_mode == MultiAsSingle)
                append_menu_item(change_extruder_menu, wxID_ANY, item_name, "",
                    [this, i](wxCommandEvent&) { add_code_as_tick(ToolChange, i); }, icons[i-1], menu,
                    [is_active_extruder]() { return !is_active_extruder; }, GUI::wxGetApp().plater());
        }

        const wxString change_extruder_menu_name = m_mode == MultiAsSingle ? 
                                                   (switch_current_code ? _L("Switch code to Change extruder") : _L("Change extruder") ) : 
                                                   _L("Change extruder (N/A)");

        append_submenu(menu, change_extruder_menu, wxID_ANY, change_extruder_menu_name, _L("Use another extruder"),
            active_extruders[1] > 0 ? "edit_uni" : "change_extruder",
            [this]() {return m_mode == MultiAsSingle && !GUI::wxGetApp().obj_list()->has_paint_on_segmentation(); }, GUI::wxGetApp().plater());
    }
}

void DSManagerForLayers::append_add_color_change_menu_item(wxMenu* menu, bool switch_current_code/* = false*/)
{
    const int extruders_cnt = GUI::wxGetApp().extruders_edited_cnt();
    if (extruders_cnt > 1) {
        int tick = imgui_ctrl.GetActiveValue();
        std::set<int> used_extruders_for_tick = m_ticks.get_used_extruders_for_tick(tick, m_only_extruder, m_values[tick]);

        wxMenu* add_color_change_menu = new wxMenu();

        for (int i = 1; i <= extruders_cnt; i++) {
            const bool is_used_extruder = used_extruders_for_tick.empty() ? true : // #ys_FIXME till used_extruders_for_tick doesn't filled correct for mmMultiExtruder
                                          used_extruders_for_tick.find(i) != used_extruders_for_tick.end();
            const wxString item_name = wxString::Format(_L("Extruder %d"), i) +
                                       (is_used_extruder ? " (" + _L("used") + ")" : "");

            append_menu_item(add_color_change_menu, wxID_ANY, item_name, "",
                [this, i](wxCommandEvent&) { add_code_as_tick(ColorChange, i); }, "", menu,
                []() { return true; }, GUI::wxGetApp().plater());
        }

        const wxString menu_name = switch_current_code ? 
                                   format_wxstr(_L("Switch code to Color change (%1%) for:"), gcode(ColorChange)) : 
                                   format_wxstr(_L("Add color change (%1%) for:"), gcode(ColorChange));
        wxMenuItem* add_color_change_menu_item = menu->AppendSubMenu(add_color_change_menu, menu_name, "");
        add_color_change_menu_item->SetBitmap(*get_bmp_bundle("colorchange_add_m"));
    }
}

void DSManagerForLayers::UseDefaultColors(bool def_colors_on)
{
    m_ticks.set_default_colors(def_colors_on);
}

// Get active extruders for tick. 
// Means one current extruder for not existing tick OR 
// 2 extruders - for existing tick (extruder before ToolChange and extruder of current existing tick)
// Use those values to disable selection of active extruders
std::array<int, 2> DSManagerForLayers::get_active_extruders_for_tick(int tick) const
{
    int default_initial_extruder = m_mode == MultiAsSingle ? std::max<int>(1, m_only_extruder) : 1;
    std::array<int, 2> extruders = { default_initial_extruder, -1 };
    if (m_ticks.empty())
        return extruders;

    auto it = m_ticks.ticks.lower_bound(TickCode{tick});

    if (it != m_ticks.ticks.end() && it->tick == tick) // current tick exists
        extruders[1] = it->extruder;

    while (it != m_ticks.ticks.begin()) {
        --it;
        if(it->type == ToolChange) {
            extruders[0] = it->extruder;
            break;
        }
    }

    return extruders;
}

// Get used extruders for tick. 
// Means all extruders(tools) which will be used during printing from current tick to the end
std::set<int> TickCodeInfo::get_used_extruders_for_tick(int tick, int only_extruder, double print_z, Mode force_mode/* = Undef*/) const
{
    Mode e_mode = !force_mode ? mode : force_mode;

    if (e_mode == MultiExtruder) {
        // #ys_FIXME: get tool ordering from _correct_ place
        const ToolOrdering& tool_ordering = GUI::wxGetApp().plater()->fff_print().get_tool_ordering();

        if (tool_ordering.empty())
            return {};

        std::set<int> used_extruders;

        auto it_layer_tools = std::lower_bound(tool_ordering.begin(), tool_ordering.end(), print_z, [](const LayerTools &lhs, double rhs){ return lhs.print_z < rhs; });
        for (; it_layer_tools != tool_ordering.end(); ++it_layer_tools) {
            const std::vector<unsigned>& extruders = it_layer_tools->extruders;
            for (const auto& extruder : extruders)
                used_extruders.emplace(extruder+1);
        }

        return used_extruders;
    }

    const int default_initial_extruder = e_mode == MultiAsSingle ? std::max(only_extruder, 1) : 1;
    if (ticks.empty() || e_mode == SingleExtruder)
        return {default_initial_extruder};

    std::set<int> used_extruders;

    auto it_start = ticks.lower_bound(TickCode{tick});
    auto it = it_start;
    if (it == ticks.begin() && it->type == ToolChange &&
        tick != it->tick )  // In case of switch of ToolChange to ColorChange, when tick exists,
                            // we shouldn't change color for extruder, which will be deleted
    {
        used_extruders.emplace(it->extruder);
        if (tick < it->tick)
            used_extruders.emplace(default_initial_extruder);
    }

    while (it != ticks.begin()) {
        --it;
        if (it->type == ToolChange && tick != it->tick) {
            used_extruders.emplace(it->extruder);
            break;
        }
    }

    if (it == ticks.begin() && used_extruders.empty())
        used_extruders.emplace(default_initial_extruder);

    for (it = it_start; it != ticks.end(); ++it)
        if (it->type == ToolChange && tick != it->tick)
            used_extruders.emplace(it->extruder);

    return used_extruders;
}

void DSManagerForLayers::show_cog_icon_context_menu()
{
    wxMenu menu;

    append_menu_item(&menu, wxID_ANY, _L("Jump to height") + " (Shift+G)", "",
                    [this](wxCommandEvent&) { jump_to_value(); }, "", & menu);

    append_menu_check_item(&menu, wxID_ANY, _L("Use default colors"), _L("Use default colors on color change"),
        [this](wxCommandEvent&) {  
            UseDefaultColors(!m_ticks.used_default_colors()); }, &menu,
        []() { return true; }, [this]() { return m_ticks.used_default_colors(); }, GUI::wxGetApp().plater());

    append_menu_check_item(&menu, wxID_ANY, _L("Show estimated print time on mouse moving"), _L("Show estimated print time on the ruler"),
        [this](wxCommandEvent&) { m_show_estimated_times = !m_show_estimated_times; }, &menu,
        []() { return true; }, [this]() { return m_show_estimated_times; }, GUI::wxGetApp().plater());

    if (m_mode == MultiAsSingle && m_draw_mode == dmRegular)
        append_menu_item(&menu, wxID_ANY, _L("Set extruder sequence for the entire print"), "",
            [this](wxCommandEvent&) { edit_extruder_sequence(); }, "", &menu);

    if (GUI::wxGetApp().is_editor() && m_mode != MultiExtruder && m_draw_mode == dmRegular)
        append_menu_item(&menu, wxID_ANY, _L("Set auto color changes"), "",
            [this](wxCommandEvent&) { auto_color_change(); }, "", &menu);

    GUI::wxGetApp().plater()->PopupMenu(&menu);
}

bool check_color_change(const PrintObject* object, size_t frst_layer_id, size_t layers_cnt, bool check_overhangs, std::function<bool(const Layer*)> break_condition)
{
    double prev_area = area(object->get_layer(frst_layer_id)->lslices);

    bool detected = false;
    for (size_t i = frst_layer_id+1; i < layers_cnt; i++) {
        const Layer* layer = object->get_layer(i);
        double cur_area = area(layer->lslices);

        // check for overhangs
        if (check_overhangs && cur_area > prev_area && !equivalent_areas(prev_area, cur_area))
            break;

        // Check percent of the area decrease.
        // This value have to be more than min_delta_area and more then 10%
        if ((prev_area - cur_area > min_delta_area) && (cur_area / prev_area < 0.9)) {
            detected = true;
            if (break_condition(layer))
                break;
        }

        prev_area = cur_area;
    }
    return detected;
}

void DSManagerForLayers::auto_color_change()
{
    if (!m_ticks.empty()) {
        wxString msg_text = _L("This action will cause deletion of all ticks on vertical slider.") + "\n\n" +
                            _L("This action is not revertible.\nDo you want to proceed?");
        GUI::WarningDialog dialog(nullptr, msg_text, _L("Warning"), wxYES | wxNO);
        if (dialog.ShowModal() == wxID_NO)
            return;    
        m_ticks.ticks.clear();
    }

    int extruders_cnt = GUI::wxGetApp().extruders_edited_cnt();
//    int extruder = 2;

    const Print& print = GUI::wxGetApp().plater()->fff_print();  
    for (auto object : print.objects()) {
        // An object should to have at least 2 layers to apply an auto color change
        if (object->layer_count() < 2)
            continue;

        check_color_change(object, 1, object->layers().size(), false, [this, extruders_cnt](const Layer* layer)
        {
            int tick = get_tick_from_value(layer->print_z);
            if (tick >= 0 && !m_ticks.has_tick(tick)) {
                if (m_mode == SingleExtruder) {
                    m_ticks.set_default_colors(true);
                    m_ticks.add_tick(tick, ColorChange, 1, layer->print_z);
                }
                else {
                    int extruder = 2;
                    if (!m_ticks.empty()) {
                        auto it = m_ticks.ticks.end();
                        it--;
                        extruder = it->extruder + 1;
                        if (extruder > extruders_cnt)
                            extruder = 1;
                    }
                    m_ticks.add_tick(tick, ToolChange, extruder, layer->print_z);
                }
            }
            // allow max 3 auto color changes
            return m_ticks.ticks.size() > 2;
        });
    }

    if (m_ticks.empty())
        GUI::wxGetApp().plater()->get_notification_manager()->push_notification(GUI::NotificationType::EmptyAutoColorChange);
    update_callbacks();

    post_ticks_changed_event();
}

static std::string get_new_color(const std::string& color)
{
    wxColour clr(color);
    if (!clr.IsOk())
        clr = wxColour(0, 0, 0); // Don't set alfa to transparence

    auto data = new wxColourData();
    data->SetChooseFull(1);
    data->SetColour(clr);

    wxColourDialog dialog(GUI::wxGetApp().GetTopWindow(), data);
    dialog.CenterOnParent();
    if (dialog.ShowModal() == wxID_OK)
        return dialog.GetColourData().GetColour().GetAsString(wxC2S_HTML_SYNTAX).ToStdString();
    return "";
}

/* To avoid get an empty string from wxTextEntryDialog 
 * Let disable OK button, if TextCtrl is empty
 * OR input value is our of range (min..max), when min a nd max are positive
 * */
static void upgrade_text_entry_dialog(wxTextEntryDialog* dlg, double min = -1.0, double max = -1.0)
{
    GUI::wxGetApp().UpdateDlgDarkUI(dlg);

    // detect TextCtrl and OK button
    wxTextCtrl* textctrl {nullptr};
    wxWindowList& dlg_items = dlg->GetChildren();
    for (auto item : dlg_items) {
        textctrl = dynamic_cast<wxTextCtrl*>(item);
        if (textctrl)
            break;
    }

    if (!textctrl)
        return;

    textctrl->SetInsertionPointEnd();

    wxButton* btn_OK = static_cast<wxButton*>(dlg->FindWindowById(wxID_OK));
    btn_OK->Bind(wxEVT_UPDATE_UI, [textctrl, min, max](wxUpdateUIEvent& evt)
    {
        bool disable = textctrl->IsEmpty();
        if (!disable && min >= 0.0 && max >= 0.0) {
            double value = -1.0;
            if (!textctrl->GetValue().ToDouble(&value))    // input value couldn't be converted to double
                disable = true;
            else
                disable = value < min - epsilon() || value > max + epsilon();       // is input value is out of valid range ?
        }

        evt.Enable(!disable);
    }, btn_OK->GetId());
}

static std::string get_custom_code(const std::string& code_in, double height)
{
    wxString msg_text = _L("Enter custom G-code used on current layer") + ":";
    wxString msg_header = format_wxstr(_L("Custom G-code on current layer (%1% mm)."), height);

    // get custom gcode
    wxTextEntryDialog dlg(nullptr, msg_text, msg_header, code_in,
        wxTextEntryDialogStyle | wxTE_MULTILINE);
    upgrade_text_entry_dialog(&dlg);

    bool valid = true;
    std::string value;
    do {
        if (dlg.ShowModal() != wxID_OK)
            return "";

        value = into_u8(dlg.GetValue());
        valid = GUI::Tab::validate_custom_gcode("Custom G-code", value);
    } while (!valid);
    return value;
}

static std::string get_pause_print_msg(const std::string& msg_in, double height)
{
    wxString msg_text = _L("Enter short message shown on Printer display when a print is paused") + ":";
    wxString msg_header = format_wxstr(_L("Message for pause print on current layer (%1% mm)."), height);

    // get custom gcode
    wxTextEntryDialog dlg(nullptr, msg_text, msg_header, from_u8(msg_in),
        wxTextEntryDialogStyle);
    upgrade_text_entry_dialog(&dlg);

    if (dlg.ShowModal() != wxID_OK || dlg.GetValue().IsEmpty())
        return "";

    return into_u8(dlg.GetValue());
}

static double get_value_to_jump(double active_value, double min_z, double max_z, DrawMode mode)
{
    wxString msg_text   = _L("Enter the height you want to jump to") + ":";
    wxString msg_header = _L("Jump to height");
    wxString msg_in     = GUI::double_to_string(active_value);

    // get custom gcode
    wxTextEntryDialog dlg(nullptr, msg_text, msg_header, msg_in, wxTextEntryDialogStyle);
    upgrade_text_entry_dialog(&dlg, min_z, max_z);

    if (dlg.ShowModal() != wxID_OK || dlg.GetValue().IsEmpty())
        return -1.0;

    double value = -1.0;
    return dlg.GetValue().ToDouble(&value) ? value : -1.0;
}

void DSManagerForLayers::add_code_as_tick(Type type, int selected_extruder/* = -1*/)
{
    const int tick = imgui_ctrl.GetActiveValue();

    if ( !check_ticks_changed_event(type) )
        return;

    if (type == ColorChange && gcode(ColorChange).empty())
        GUI::wxGetApp().plater()->get_notification_manager()->push_notification(GUI::NotificationType::EmptyColorChangeCode);

    const int extruder = selected_extruder > 0 ? selected_extruder : std::max<int>(1, m_only_extruder);
    const auto it = m_ticks.ticks.find(TickCode{ tick });

    bool was_ticks = m_ticks.empty();
    
    if ( it == m_ticks.ticks.end() ) {
        // try to add tick
        if (!m_ticks.add_tick(tick, type, extruder, m_values[tick]))
            return;
    }
    else if (type == ToolChange || type == ColorChange) {
        // try to switch tick code to ToolChange or ColorChange accordingly
        if (!m_ticks.switch_code_for_tick(it, type, extruder))
            return;
    }
    else
        return;

    if (was_ticks != m_ticks.empty())
        update_callbacks();

    post_ticks_changed_event(type);
}

void DSManagerForLayers::add_current_tick(bool call_from_keyboard /*= false*/)
{
    const int tick = imgui_ctrl.GetActiveValue();
    auto it = m_ticks.ticks.find(TickCode{ tick });

    if (it != m_ticks.ticks.end() ||    // this tick is already exist
        !check_ticks_changed_event(m_mode == MultiAsSingle ? ToolChange : ColorChange))
        return;

    if (m_mode == SingleExtruder)
        add_code_as_tick(ColorChange);
    /*
    else
    {
        wxMenu menu;

        if (m_mode == MultiAsSingle)
            append_change_extruder_menu_item(&menu);
        else
            append_add_color_change_menu_item(&menu);

        wxPoint pos = wxDefaultPosition; 
        /* Menu position will be calculated from mouse click position, but...
         * if function is called from keyboard (pressing "+"), we should to calculate it
         * * /
        if (call_from_keyboard) {
            int width, height;
            get_size(&width, &height);

            const wxCoord coord = 0.75 * (is_horizontal() ? height : width);
            this->GetPosition(&width, &height);

            pos = is_horizontal() ? 
                  wxPoint(get_position_from_value(tick), height + coord) :
                  wxPoint(width + coord, get_position_from_value(tick));
        }

        GUI::wxGetApp().plater()->PopupMenu(&menu, pos);
    }
    */
}

void DSManagerForLayers::delete_current_tick()
{
    auto it = m_ticks.ticks.find(TickCode{ imgui_ctrl.GetActiveValue()});
    if (it == m_ticks.ticks.end() ||
        !check_ticks_changed_event(it->type))
        return;

    Type type = it->type;
    m_ticks.ticks.erase(it);
    post_ticks_changed_event(type);
}

void DSManagerForLayers::edit_tick(int tick/* = -1*/)
{
    if (tick < 0)
        tick = imgui_ctrl.GetActiveValue();
    const std::set<TickCode>::iterator it = m_ticks.ticks.find(TickCode{ tick });

    if (it == m_ticks.ticks.end() ||
        !check_ticks_changed_event(it->type))
        return;

    Type type = it->type;
    if (m_ticks.edit_tick(it, m_values[it->tick]))
        post_ticks_changed_event(type);
}

// discard all custom changes on DoubleSlider
void DSManagerForLayers::discard_all_thicks()
{
    m_ticks.ticks.clear();
    imgui_ctrl.ResetValues();
    update_callbacks();
    post_ticks_changed_event();
}

void DSManagerForLayers::edit_extruder_sequence()
{
    if (!check_ticks_changed_event(ToolChange))
        return;

    GUI::ExtruderSequenceDialog dlg(m_extruders_sequence);
    if (dlg.ShowModal() != wxID_OK)
        return;
    m_extruders_sequence = dlg.GetValue();

    m_ticks.erase_all_ticks_with_code(ToolChange);

    const int extr_cnt = m_extruders_sequence.extruders.size();
    if (extr_cnt == 1)
        return;

    int tick = 0;
    double value = 0.0;
    int extruder = -1;

    std::random_device rd;  //Will be used to obtain a seed for the random number engine
    std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
    std::uniform_int_distribution<> distrib(0, extr_cnt-1);

    while (tick <= imgui_ctrl.GetMaxValue())
    {
        bool color_repetition = false;
        if (m_extruders_sequence.random_sequence) {
            int rand_extr = distrib(gen);
            if (m_extruders_sequence.color_repetition)
                color_repetition = rand_extr == extruder;
            else
                while (rand_extr == extruder)
                    rand_extr = distrib(gen);
            extruder = rand_extr;
        }
        else {
            extruder++;
            if (extruder == extr_cnt)
                extruder = 0;
        }

        const int cur_extruder = m_extruders_sequence.extruders[extruder];

        bool meaningless_tick = tick == 0.0 && cur_extruder == extruder;
        if (!meaningless_tick && !color_repetition)
            m_ticks.ticks.emplace(TickCode{tick, ToolChange,cur_extruder + 1, m_extruder_colors[cur_extruder]});

        if (m_extruders_sequence.is_mm_intervals) {
            value += m_extruders_sequence.interval_by_mm;
            tick = get_tick_from_value(value, true);
            if (tick < 0)
                break;
        }
        else
            tick += m_extruders_sequence.interval_by_layers;
    }

    post_ticks_changed_event(ToolChange);
}

void DSManagerForLayers::jump_to_value()
{
    double value = get_value_to_jump(m_values[imgui_ctrl.GetActiveValue()],
                                     m_values[imgui_ctrl.GetMinValue()], m_values[imgui_ctrl.GetMaxValue()], m_draw_mode);
    if (value < 0.0)
        return;

    int tick_value = get_tick_from_value(value);

    if (imgui_ctrl.IsActiveHigherThumb())
        SetHigherValue(tick_value);
    else
        SetLowerValue(tick_value);
}

void DSManagerForLayers::post_ticks_changed_event(Type type /*= Custom*/)
{
//    m_force_mode_apply = type != ToolChange; // It looks like this condition is no needed now. Leave it for the testing
    process_ticks_changed();
}

bool DSManagerForLayers::check_ticks_changed_event(Type type)
{
    if ( m_ticks.mode == m_mode                                                     ||
        (type != ColorChange && type != ToolChange)                       ||
        (m_ticks.mode == SingleExtruder && m_mode == MultiAsSingle) || // All ColorChanges will be applied for 1st extruder
        (m_ticks.mode == MultiExtruder  && m_mode == MultiAsSingle) )  // Just mark ColorChanges for all unused extruders
        return true;

    if ((m_ticks.mode == SingleExtruder && m_mode == MultiExtruder ) ||
        (m_ticks.mode == MultiExtruder  && m_mode == SingleExtruder)    )
    {
        if (!m_ticks.has_tick_with_code(ColorChange))
            return true;

        wxString message = (m_ticks.mode == SingleExtruder ?
                            _L("The last color change data was saved for a single extruder printing.") :
                            _L("The last color change data was saved for a multi extruder printing.") 
                            ) + "\n" +
                            _L("Your current changes will delete all saved color changes.") + "\n\n\t" +
                            _L("Are you sure you want to continue?");

        //wxMessageDialog msg(this, message, _L("Notice"), wxYES_NO);
        GUI::MessageDialog msg(nullptr, message, _L("Notice"), wxYES_NO);
        if (msg.ShowModal() == wxID_YES) {
            m_ticks.erase_all_ticks_with_code(ColorChange);
            post_ticks_changed_event(ColorChange);
        }
        return false;
    }
    //          m_ticks_mode == MultiAsSingle
    if( m_ticks.has_tick_with_code(ToolChange) ) {
        wxString message =  m_mode == SingleExtruder ?                          (
                            _L("The last color change data was saved for a multi extruder printing.") + "\n\n" +
                            _L("Select YES if you want to delete all saved tool changes, \n"
                               "NO if you want all tool changes switch to color changes, \n"
                               "or CANCEL to leave it unchanged.") + "\n\n\t" +
                            _L("Do you want to delete all saved tool changes?")  
                            ): ( // MultiExtruder
                            _L("The last color change data was saved for a multi extruder printing with tool changes for whole print.") + "\n\n" +
                            _L("Your current changes will delete all saved extruder (tool) changes.") + "\n\n\t" +
                            _L("Are you sure you want to continue?")                  ) ;

        GUI::MessageDialog msg(nullptr, message, _L("Notice"), wxYES_NO | (m_mode == SingleExtruder ? wxCANCEL : 0));
        const int answer = msg.ShowModal();
        if (answer == wxID_YES) {
            m_ticks.erase_all_ticks_with_code(ToolChange);
            post_ticks_changed_event(ToolChange);
        }
        else if (m_mode == SingleExtruder && answer == wxID_NO) {
            m_ticks.switch_code(ToolChange, ColorChange);
            post_ticks_changed_event(ColorChange);
        }
        return false;
    }

    return true;
}

DSManagerForLayers::DSManagerForLayers( int lowerValue,
                                        int higherValue,
                                        int minValue,
                                        int maxValue,
                                        bool allow_editing/* = true*/) :
     m_allow_editing(allow_editing)
{
#ifdef __WXOSX__ 
    is_osx = true;
#endif //__WXOSX__
    Init(lowerValue, higherValue, minValue, maxValue, "layers_slider", false);

    imgui_ctrl.set_get_label_on_move_cb([this](int pos) { return m_show_estimated_times ? into_u8(get_label(pos, ltEstimatedTime)) : ""; });
    imgui_ctrl.set_extra_draw_cb([this](const ImRect& draw_rc) {return draw_ticks(draw_rc); });

    m_ticks.set_pause_print_msg(_u8L("Place bearings in slots and resume printing"));
    m_ticks.set_extruder_colors(&m_extruder_colors);
}

std::string TickCodeInfo::get_color_for_tick(TickCode tick, Type type, const int extruder)
{
    auto opposite_one_color = [](const std::string& color) {
        ColorRGB rgb;
        decode_color(color, rgb);
        return encode_color(opposite(rgb));
    };
    auto opposite_two_colors = [](const std::string& a, const std::string& b) {
        ColorRGB rgb1; decode_color(a, rgb1);
        ColorRGB rgb2; decode_color(b, rgb2);
        return encode_color(opposite(rgb1, rgb2));
    };

    if (mode == SingleExtruder && type == ColorChange && m_use_default_colors) {

        if (ticks.empty())
            return opposite_one_color((*m_colors)[0]);

        auto before_tick_it = std::lower_bound(ticks.begin(), ticks.end(), tick);
        if (before_tick_it == ticks.end()) {
            while (before_tick_it != ticks.begin())
                if (--before_tick_it; before_tick_it->type == ColorChange)
                    break;
            if (before_tick_it->type == ColorChange)
                return opposite_one_color(before_tick_it->color);

            return opposite_one_color((*m_colors)[0]);
        }

        if (before_tick_it == ticks.begin()) {
            const std::string& frst_color = (*m_colors)[0];
            if (before_tick_it->type == ColorChange)
                return opposite_two_colors(frst_color, before_tick_it->color);

            auto next_tick_it = before_tick_it;
            while (next_tick_it != ticks.end())
                if (++next_tick_it; next_tick_it->type == ColorChange)
                    break;
            if (next_tick_it->type == ColorChange)
                return opposite_two_colors(frst_color, next_tick_it->color);

            return opposite_one_color(frst_color);
        }

        std::string frst_color = "";
        if (before_tick_it->type == ColorChange)
            frst_color = before_tick_it->color;
        else {
            auto next_tick_it = before_tick_it;
            while (next_tick_it != ticks.end())
                if (++next_tick_it; next_tick_it->type == ColorChange) {
                    frst_color = next_tick_it->color;
                    break;
                }
        }

        while (before_tick_it != ticks.begin())
            if (--before_tick_it; before_tick_it->type == ColorChange)
                break;

        if (before_tick_it->type == ColorChange) {
            if (frst_color.empty())
                return opposite_one_color(before_tick_it->color);

            return opposite_two_colors(before_tick_it->color, frst_color);
        }

        if (frst_color.empty())
            return opposite_one_color((*m_colors)[0]);

        return opposite_two_colors((*m_colors)[0], frst_color);
    }

    std::string color = (*m_colors)[extruder - 1];

    if (type == ColorChange) {
        if (!ticks.empty()) {
            auto before_tick_it = std::lower_bound(ticks.begin(), ticks.end(), tick );
            while (before_tick_it != ticks.begin()) {
                --before_tick_it;
                if (before_tick_it->type == ColorChange && before_tick_it->extruder == extruder) {
                    color = before_tick_it->color;
                    break;
                }
            }
        }

        color = get_new_color(color);
    }
    return color;
}

bool TickCodeInfo::add_tick(const int tick, Type type, const int extruder, double print_z)
{
    std::string color;
    std::string extra;
    if (type == Custom)           // custom Gcode
    {
        extra = get_custom_code(custom_gcode, print_z);
        if (extra.empty())
            return false;
        custom_gcode = extra;
    }
    else if (type == PausePrint) {
        extra = get_pause_print_msg(pause_print_msg, print_z);
        if (extra.empty())
            return false;
        pause_print_msg = extra;
    }
    else {
        color = get_color_for_tick(TickCode{ tick }, type, extruder);
        if (color.empty())
            return false;
    }

    ticks.emplace(TickCode{ tick, type, extruder, color, extra });
    return true;
}

bool TickCodeInfo::edit_tick(std::set<TickCode>::iterator it, double print_z)
{
    // Save previously value of the tick before the call a Dialog from get_... functions,
    // otherwise a background process can change ticks values and current iterator wouldn't be valid for the moment of a Dialog close
    // and PS will crash (see https://github.com/prusa3d/PrusaSlicer/issues/10941)
    TickCode changed_tick = *it;

    std::string edited_value;
    if (it->type == ColorChange)
        edited_value = get_new_color(it->color);
    else if (it->type == PausePrint)
        edited_value = get_pause_print_msg(it->extra, print_z);
    else
        edited_value = get_custom_code(it->type == Template ? gcode(Template) : it->extra, print_z);

    if (edited_value.empty())
        return false;

    // Update iterator. For this moment its value can be invalid
    if (it = ticks.find(changed_tick); it == ticks.end())
        return false;

    if (it->type == ColorChange) {
        if (it->color == edited_value)
            return false;
        changed_tick.color = edited_value;
    }
    else if (it->type == Template) {
        if (gcode(Template) == edited_value)
            return false;
        changed_tick.extra = edited_value;
        changed_tick.type  = Custom;
    }
    else if (it->type == Custom || it->type == PausePrint) {
        if (it->extra == edited_value)
            return false;
        changed_tick.extra = edited_value;
    }

    ticks.erase(it);
    ticks.emplace(changed_tick);

    return true;
}

void TickCodeInfo::switch_code(Type type_from, Type type_to)
{
    for (auto it{ ticks.begin() }, end{ ticks.end() }; it != end; )
        if (it->type == type_from) {
            TickCode tick = *it;
            tick.type = type_to;
            tick.extruder = 1;
            ticks.erase(it);
            it = ticks.emplace(tick).first;
        }
        else
            ++it;
}

bool TickCodeInfo::switch_code_for_tick(std::set<TickCode>::iterator it, Type type_to, const int extruder)
{
    const std::string color = get_color_for_tick(*it, type_to, extruder);
    if (color.empty())
        return false;

    TickCode changed_tick   = *it;
    changed_tick.type       = type_to;
    changed_tick.extruder   = extruder;
    changed_tick.color      = color;

    ticks.erase(it);
    ticks.emplace(changed_tick);

    return true;
}

void TickCodeInfo::erase_all_ticks_with_code(Type type)
{
    for (auto it{ ticks.begin() }, end{ ticks.end() }; it != end; ) {
        if (it->type == type)
            it = ticks.erase(it);
        else
            ++it;
    }
}

bool TickCodeInfo::has_tick_with_code(Type type)
{
    for (const TickCode& tick : ticks)
        if (tick.type == type)
            return true;

    return false;
}

bool TickCodeInfo::has_tick(int tick)
{
    return ticks.find(TickCode{ tick }) != ticks.end();
}

ConflictType TickCodeInfo::is_conflict_tick(const TickCode& tick, Mode out_mode, int only_extruder, double print_z)
{
    if ((tick.type == ColorChange && (
            (mode == SingleExtruder && out_mode == MultiExtruder ) ||
            (mode == MultiExtruder  && out_mode == SingleExtruder)    )) ||
        (tick.type == ToolChange &&
            (mode == MultiAsSingle && out_mode != MultiAsSingle)) )
        return ctModeConflict;

    // check ColorChange tick
    if (tick.type == ColorChange) {
        // We should mark a tick as a "MeaninglessColorChange", 
        // if it has a ColorChange for unused extruder from current print to end of the print
        std::set<int> used_extruders_for_tick = get_used_extruders_for_tick(tick.tick, only_extruder, print_z, out_mode);

        if (used_extruders_for_tick.find(tick.extruder) == used_extruders_for_tick.end())
            return ctMeaninglessColorChange;

        // We should mark a tick as a "Redundant", 
        // if it has a ColorChange for extruder that has not been used before
        if (mode == MultiAsSingle && tick.extruder != std::max<int>(only_extruder, 1) )
        {
            auto it = ticks.lower_bound( tick );
            if (it == ticks.begin() && it->type == ToolChange && tick.extruder == it->extruder)
                return ctNone;

            while (it != ticks.begin()) {
                --it;
                if (it->type == ToolChange && tick.extruder == it->extruder)
                    return ctNone;
            }

            return ctRedundant;
        }
    }

    // check ToolChange tick
    if (mode == MultiAsSingle && tick.type == ToolChange) {
        // We should mark a tick as a "MeaninglessToolChange", 
        // if it has a ToolChange to the same extruder
        auto it = ticks.find(tick);
        if (it == ticks.begin())
            return tick.extruder == std::max<int>(only_extruder, 1) ? ctMeaninglessToolChange : ctNone;

        while (it != ticks.begin()) {
            --it;
            if (it->type == ToolChange)
                return tick.extruder == it->extruder ? ctMeaninglessToolChange : ctNone;
        }
    }

    return ctNone;
}

} // DoubleSlider

} // Slic3r


