///|/ Copyright (c) Prusa Research 2020 - 2022 Vojtěch Bubník @bubnikv, Oleksandra Iushchenko @YuSanka, Enrico Turri @enricoturri1966, Lukáš Matěna @lukasmatena
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#ifndef slic3r_GUI_DoubleSlider_hpp_
#define slic3r_GUI_DoubleSlider_hpp_

#include "libslic3r/CustomGCode.hpp"
#include "wxExtensions.hpp"
#include "GLCanvas3D.hpp"

#include "ImGuiDoubleSlider.hpp"

#include <wx/window.h>
#include <wx/control.h>
#include <wx/dc.h>
#include <wx/slider.h>

#include <vector>
#include <set>

class wxMenu;

namespace Slic3r {

using namespace CustomGCode;
class PrintObject;
class Layer;

namespace DoubleSlider {

using namespace GUI;

/* For exporting GCode in GCodeWriter is used XYZF_NUM(val) = PRECISION(val, 3) for XYZ values. 
 * So, let use same value as a permissible error for layer height.
 */
constexpr double epsilon() { return 0.0011; }

// return true when areas are mostly equivalent
bool equivalent_areas(const double& bottom_area, const double& top_area);

// return true if color change was detected
bool check_color_change(const PrintObject* object, size_t frst_layer_id, size_t layers_cnt, bool check_overhangs,
                        // what to do with detected color change
                        // and return true when detection have to be desturbed
                        std::function<bool(const Layer*)> break_condition);

// custom message the slider sends to its parent to notify a tick-change:
wxDECLARE_EVENT(wxCUSTOMEVT_TICKSCHANGED, wxEvent);


enum FocusedItem {
    fiNone,
    fiRevertIcon,
    fiOneLayerIcon,
    fiCogIcon,
    fiColorBand,
    fiActionIcon,
    fiLowerThumb,
    fiHigherThumb,
    fiSmartWipeTower,
    fiTick
};

enum ConflictType
{
    ctNone,
    ctModeConflict,
    ctMeaninglessColorChange,
    ctMeaninglessToolChange,
    ctRedundant
};

enum MouseAction
{
    maNone,
    maAddMenu,                  // show "Add"  context menu for NOTexist active tick
    maEditMenu,                 // show "Edit" context menu for exist active tick
    maCogIconMenu,              // show context for "cog" icon
    maForceColorEdit,           // force color editing from colored band
    maAddTick,                  // force tick adding
    maDeleteTick,               // force tick deleting
    maCogIconClick,             // LeftMouseClick on "cog" icon
    maOneLayerIconClick,        // LeftMouseClick on "one_layer" icon
    maRevertIconClick,          // LeftMouseClick on "revert" icon
};

enum DrawMode
{
    dmRegular,
    dmSlaPrint,
    dmSequentialFffPrint,
    dmSequentialGCodeView,
};

enum LabelType
{
    ltHeightWithLayer,
    ltHeight,
    ltEstimatedTime,
};

struct TickCode
{
    bool operator<(const TickCode& other) const { return other.tick > this->tick; }
    bool operator>(const TickCode& other) const { return other.tick < this->tick; }

    int         tick = 0;
    Type        type = ColorChange;
    int         extruder = 0;
    std::string color;
    std::string extra;
};

class TickCodeInfo
{
    std::string custom_gcode;
    std::string pause_print_msg;
    bool        m_suppress_plus     = false;
    bool        m_suppress_minus    = false;
    bool        m_use_default_colors{ true };

    std::vector<std::string>* m_colors {nullptr};

    std::string get_color_for_tick(TickCode tick, Type type, const int extruder);

public:
    std::set<TickCode>  ticks {};
    Mode                mode = Undef;

    bool empty() const { return ticks.empty(); }
    void set_pause_print_msg(const std::string& message) { pause_print_msg = message; }

    bool add_tick(const int tick, Type type, int extruder, double print_z);
    bool edit_tick(std::set<TickCode>::iterator it, double print_z);
    void switch_code(Type type_from, Type type_to);
    bool switch_code_for_tick(std::set<TickCode>::iterator it, Type type_to, const int extruder);
    void erase_all_ticks_with_code(Type type);

    bool            has_tick_with_code(Type type);
    bool            has_tick(int tick);
    ConflictType    is_conflict_tick(const TickCode& tick, Mode out_mode, int only_extruder, double print_z);

    // Get used extruders for tick.
    // Means all extruders(tools) which will be used during printing from current tick to the end
    std::set<int>   get_used_extruders_for_tick(int tick, int only_extruder, double print_z, Mode force_mode = Undef) const;

    void suppress_plus (bool suppress) { m_suppress_plus = suppress; }
    void suppress_minus(bool suppress) { m_suppress_minus = suppress; }
    bool suppressed_plus () { return m_suppress_plus; }
    bool suppressed_minus() { return m_suppress_minus; }
    void set_default_colors(bool default_colors_on)  { m_use_default_colors = default_colors_on; }
    bool used_default_colors() const { return m_use_default_colors; }

    void set_extruder_colors(std::vector<std::string>* extruder_colors) { m_colors = extruder_colors; }
};


struct ExtrudersSequence
{
    bool            is_mm_intervals     = true;
    double          interval_by_mm      = 3.0;
    int             interval_by_layers  = 10;
    bool            random_sequence     { false };
    bool            color_repetition    { false };
    std::vector<size_t>  extruders      = { 0 };

    bool operator==(const ExtrudersSequence& other) const
    {
        return  (other.is_mm_intervals      == this->is_mm_intervals    ) &&
                (other.interval_by_mm       == this->interval_by_mm     ) &&
                (other.interval_by_layers   == this->interval_by_layers ) &&
                (other.random_sequence      == this->random_sequence    ) &&
                (other.color_repetition     == this->color_repetition   ) &&
                (other.extruders            == this->extruders          ) ;
    }
    bool operator!=(const ExtrudersSequence& other) const
    {
        return  (other.is_mm_intervals      != this->is_mm_intervals    ) ||
                (other.interval_by_mm       != this->interval_by_mm     ) ||
                (other.interval_by_layers   != this->interval_by_layers ) ||
                (other.random_sequence      != this->random_sequence    ) ||
                (other.color_repetition     != this->color_repetition   ) ||
                (other.extruders            != this->extruders          ) ;
    }

    void add_extruder(size_t pos, size_t extruder_id = size_t(0))
    {
        extruders.insert(extruders.begin() + pos+1, extruder_id);
    }

    void delete_extruder(size_t pos)
    {            
        if (extruders.size() == 1)
            return;// last item can't be deleted
        extruders.erase(extruders.begin() + pos);
    }

    void init(size_t extruders_count) 
    {
        extruders.clear();
        for (size_t extruder = 0; extruder < extruders_count; extruder++)
            extruders.push_back(extruder);
    }
};

class Control : public wxControl
{
public:
    Control(
        wxWindow *parent,
        int lowerValue,
        int higherValue,
        int minValue,
        int maxValue,
        long style = wxSL_VERTICAL,
        const wxString& name = wxEmptyString);
    ~Control() {}

    int     GetMinValue()   const { return imgui_ctrl.GetMinValue(); }
    int     GetMaxValue()   const { return imgui_ctrl.GetMaxValue(); }
    double  GetMinValueD() { return m_values.empty() ? 0. : m_values[GetMinValue()]; }
    double  GetMaxValueD() { return m_values.empty() ? 0. : m_values[GetMaxValue()]; }
    int     GetLowerValue() const { return imgui_ctrl.GetLowerValue(); }
    int     GetHigherValue()const { return imgui_ctrl.GetHigherValue(); }
    double  GetLowerValueD()  { return get_double_value(ssLower); }
    double  GetHigherValueD() { return get_double_value(ssHigher); }

    // Set low and high slider position. If the span is non-empty, disable the "one layer" mode.
    void    SetLowerValue (const int lower_val);
    void    SetHigherValue(const int higher_val);
    void    SetSelectionSpan(const int lower_val, const int higher_val);

    void    SetMaxValue(const int max_value);
    void    SetSliderValues(const std::vector<double>& values);
    void    ChangeOneLayerLock();
    void    SetSliderAlternateValues(const std::vector<double>& values) { m_alternate_values = values; }

    Info    GetTicksValues() const;
    void    SetTicksValues(const Info &custom_gcode_per_print_z);
    void    SetLayersTimes(const std::vector<float>& layers_times, float total_time);
    void    SetLayersTimes(const std::vector<double>& layers_times);

    void    SetDrawMode(bool is_sla_print, bool is_sequential_print);
    void    SetDrawMode(DrawMode mode) { m_draw_mode = mode; }

    void    SetManipulationMode(Mode mode)  { m_mode = mode; }
    Mode    GetManipulationMode() const     { return m_mode; }
    void    SetModeAndOnlyExtruder(const bool is_one_extruder_printed_model, const int only_extruder);
    void    SetExtruderColors(const std::vector<std::string>& extruder_colors);

    bool    IsNewPrint();

    void set_render_as_disabled(bool value) { m_render_as_disabled = value; }
    bool is_rendering_as_disabled() const { return m_render_as_disabled; }

    bool is_lower_at_min() const    { return imgui_ctrl.is_lower_at_min(); }
    bool is_higher_at_max() const   { return imgui_ctrl.is_higher_at_max(); }

    void UseDefaultColors(bool def_colors_on);
    void OnKeyDown(wxKeyEvent &event);
    void OnChar(wxKeyEvent &event);

    bool Show(bool show = true) override { imgui_ctrl.Show(show); return true; }
    bool Hide() { return Show(false); }

    void add_code_as_tick(Type type, int selected_extruder = -1);
    // add default action for tick, when press "+"
    void add_current_tick(bool call_from_keyboard = false);
    // delete current tick, when press "-"
    void delete_current_tick();
    void edit_tick(int tick = -1);
    void switch_one_layer_mode();
    void discard_all_thicks();
    void edit_extruder_sequence();
    void jump_to_value();
    void enable_action_icon(bool enable) { m_enable_action_icon = enable; }
    void show_cog_icon_context_menu();
    void auto_color_change();

    void imgui_render(GUI::GLCanvas3D& canvas, float extra_scale = 1.f);

private:

    bool    is_lower_thumb_editable();
    void    move_current_thumb(const bool condition);
    bool    is_wipe_tower_layer(int tick) const;

    wxString    get_label(int tick, LabelType label_type = ltHeightWithLayer) const;

    double      get_double_value(const SelectedSlider& selection);
    int         get_tick_from_value(double value, bool force_lower_bound = false);
    wxString    get_tooltip(int tick = -1);

    std::string get_color_for_tool_change_tick(std::set<TickCode>::const_iterator it) const;
    std::string get_color_for_color_change_tick(std::set<TickCode>::const_iterator it) const;

    // Get active extruders for tick. 
    // Means one current extruder for not existing tick OR 
    // 2 extruders - for existing tick (extruder before ToolChangeCode and extruder of current existing tick)
    // Use those values to disable selection of active extruders
    std::array<int, 2> get_active_extruders_for_tick(int tick) const;

    void    post_ticks_changed_event(Type type = Custom);
    bool    check_ticks_changed_event(Type type);

    void    append_change_extruder_menu_item (wxMenu*, bool switch_current_code = false);
    void    append_add_color_change_menu_item(wxMenu*, bool switch_current_code = false);

    bool        is_osx { false };

    bool        m_render_as_disabled{ false };
    bool        m_allow_editing{ true };

    bool        m_force_mode_apply = true;
    bool        m_enable_action_icon = true;
    bool        m_is_wipe_tower = false; //This flag indicates that there is multiple extruder print with wipe tower

    DrawMode    m_draw_mode = dmRegular;

    Mode        m_mode = SingleExtruder;
    int         m_only_extruder = -1;

    MouseAction m_mouse = maNone;
    FocusedItem m_focus = fiNone;

    bool        m_show_estimated_times{ false };

    std::vector<double> m_values;
    TickCodeInfo        m_ticks;
    std::vector<double> m_layers_times;
    std::vector<double> m_layers_values;
    std::vector<std::string>    m_extruder_colors;
    std::string         m_print_obj_idxs;

    std::vector<double> m_alternate_values;

    ExtrudersSequence   m_extruders_sequence;

    // ImGuiDS
    float       m_scale{ 1.f };
    bool        m_can_change_color{ true };

    void        draw_colored_band(const ImRect& groove, const ImRect& slideable_region);
    void        draw_ticks(const ImRect& slideable_region);
    void        render_menu();
    bool        render_button(const wchar_t btn_icon, const wchar_t btn_icon_hovered, const std::string& label_id, const ImVec2& pos, FocusedItem focus, int tick = -1);
    void        update_callbacks();

    GUI::ImGuiControl imgui_ctrl;
};

} // DoubleSlider;

} // Slic3r



#endif // slic3r_GUI_DoubleSlider_hpp_
