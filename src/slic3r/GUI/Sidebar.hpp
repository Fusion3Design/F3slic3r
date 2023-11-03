///|/ Copyright (c) Prusa Research 2018 - 2023 Tomáš Mészáros @tamasmeszaros, Oleksandra Iushchenko @YuSanka, Enrico Turri @enricoturri1966, David Kocík @kocikdav, Lukáš Hejl @hejllukas, Vojtěch Bubník @bubnikv, Lukáš Matěna @lukasmatena, Pavel Mikuš @Godrak, Filip Sykala @Jony01, Vojtěch Král @vojtechkral
///|/
///|/ ported from lib/Slic3r/GUI/Plater.pm:
///|/ Copyright (c) Prusa Research 2016 - 2019 Vojtěch Bubník @bubnikv, Vojtěch Král @vojtechkral, Enrico Turri @enricoturri1966, Oleksandra Iushchenko @YuSanka, Lukáš Matěna @lukasmatena, Tomáš Mészáros @tamasmeszaros
///|/ Copyright (c) 2018 Martin Loidl @LoidlM
///|/ Copyright (c) 2017 Matthias Gazzari @qtux
///|/ Copyright (c) Slic3r 2012 - 2016 Alessandro Ranellucci @alranel
///|/ Copyright (c) 2017 Joseph Lenox @lordofhyphens
///|/ Copyright (c) 2015 Daren Schwenke
///|/ Copyright (c) 2014 Mark Hindess
///|/ Copyright (c) 2012 Mike Sheldrake @mesheldrake
///|/ Copyright (c) 2012 Henrik Brix Andersen @henrikbrixandersen
///|/ Copyright (c) 2012 Sam Wong
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#ifndef slic3r_Sidebar_hpp_
#define slic3r_Sidebar_hpp_

#include <vector>

#include <wx/panel.h>

#include "libslic3r/Preset.hpp"
#include "GUI.hpp"
#include "Event.hpp"

class wxButton;
class wxScrolledWindow;
class wxString;

namespace Slic3r {

namespace GUI {

wxDECLARE_EVENT(EVT_SCHEDULE_BACKGROUND_PROCESS, SimpleEvent);

class ConfigOptionsGroup;
class ObjectManipulation;
class ObjectSettings;
class ObjectLayers;
class ObjectList;
class PlaterPresetComboBox;
class Plater;

enum class ActionButtonType : int {
    abReslice,
    abExport,
    abSendGCode
};

class Sidebar : public wxPanel
{
    ConfigOptionMode    m_mode{ConfigOptionMode::comSimple};
public:
    Sidebar(Plater *parent);
    Sidebar(Sidebar &&) = delete;
    Sidebar(const Sidebar &) = delete;
    Sidebar &operator=(Sidebar &&) = delete;
    Sidebar &operator=(const Sidebar &) = delete;
    ~Sidebar();

    void init_filament_combo(PlaterPresetComboBox **combo, const int extr_idx);
    void remove_unused_filament_combos(const size_t current_extruder_count);
    void update_all_preset_comboboxes();
    void update_presets(Preset::Type preset_type);
    void update_mode_sizer() const;
    void change_top_border_for_mode_sizer(bool increase_border);
    void update_reslice_btn_tooltip() const;
    void msw_rescale();
    void sys_color_changed();
    void update_mode_markers();

    ObjectManipulation*     obj_manipul();
    ObjectList*             obj_list();
    ObjectSettings*         obj_settings();
    ObjectLayers*           obj_layers();
    wxScrolledWindow*       scrolled_panel();
    wxPanel*                presets_panel();

    ConfigOptionsGroup*     og_freq_chng_params(const bool is_fff);
    wxButton*               get_wiping_dialog_button();
    void                    update_objects_list_extruder_column(size_t extruders_count);
    void                    show_info_sizer();
    void                    show_sliced_info_sizer(const bool show);
    void                    update_sliced_info_sizer();
    void                    enable_buttons(bool enable);
    void                    set_btn_label(const ActionButtonType btn_type, const wxString& label) const;
    bool                    show_reslice(bool show) const;
	bool                    show_export(bool show) const;
	bool                    show_send(bool show) const;
    bool                    show_eject(bool show)const;
	bool                    show_export_removable(bool show) const;
	bool                    get_eject_shown() const;
    bool                    is_multifilament();
    void                    update_mode();
    bool                    is_collapsed();
    void                    collapse(bool collapse);
    void                    update_ui_from_settings();

#ifdef _MSW_DARK_MODE
    void                    show_mode_sizer(bool show);
#endif

    std::vector<PlaterPresetComboBox*>&   combos_filament();

private:
    struct priv;
    std::unique_ptr<priv> p;
};

} // namespace GUI
} // namespace Slic3r

#endif
