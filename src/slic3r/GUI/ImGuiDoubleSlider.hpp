///|/ Copyright (c) Prusa Research 2020 - 2022 Vojtěch Bubník @bubnikv, Oleksandra Iushchenko @YuSanka, Enrico Turri @enricoturri1966, Lukáš Matěna @lukasmatena
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#ifndef slic3r_ImGUI_DoubleSlider_hpp_
#define slic3r_ImGUI_DoubleSlider_hpp_

#include "ImGuiPureWrap.hpp"
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "imgui/imgui_internal.h"

#include <sstream>

// this code is borrowed from https://stackoverflow.com/questions/16605967/set-precision-of-stdto-string-when-converting-floating-point-values
template <typename T>
std::string to_string_with_precision(const T a_value, const int n = 2)
{
    std::ostringstream out;
    out.precision(n);
    out << std::fixed << a_value;
    return std::move(out).str();
}

namespace Slic3r {
namespace GUI {

enum SelectedSlider {
    ssUndef,
    ssLower,
    ssHigher
};

enum LabelType
{
    ltHeightWithLayer,
    ltHeight,
    ltEstimatedTime,
};

class ImGuiControl
{
public:
    ImGuiControl(int lowerValue,
                 int higherValue,
                 int minValue,
                 int maxValue,
                 ImVec2 pos,
                 ImVec2 size,
                 ImGuiSliderFlags flags = ImGuiSliderFlags_None,
                 std::string name = "d_slider",
                 bool use_lower_thumb = true);
    ImGuiControl() {}
    ~ImGuiControl() {}

    int     GetMinValue() const     { return m_min_value; }
    int     GetMaxValue() const     { return m_max_value; }
    int     GetLowerValue()  const  { return m_lower_value; }
    int     GetHigherValue() const  { return m_higher_value; }
    int     GetActiveValue() const;
    float   GetPositionFromValue(int value, const ImRect& rect) const;

    // Set low and high slider position. If the span is non-empty, disable the "one layer" mode.
    void    SetLowerValue (const int lower_val);
    void    SetHigherValue(const int higher_val);
    void    SetSelectionSpan(const int lower_val, const int higher_val);

    void    SetMaxValue(const int max_value);
    void    CombineThumbs(bool combine);
    void    ResetValues();

    void    SetPos(ImVec2 pos)          { m_pos = pos; }
    void    SetSize(ImVec2 size)        { m_size = size; }
    void    SetScale(float scale)       { m_draw_opts.scale = scale; }
    void    Init(const ImVec2& pos, const ImVec2& size, float scale) {
                                          m_pos = pos; 
                                          m_size = size;
                                          m_draw_opts.scale = scale;
    }

    void Show(bool show)                { m_is_shown = show; }
    void Hide()                         { m_is_shown = false; }
    bool IsShown() const                { return m_is_shown; }
    void MoveActiveThumb(int delta);
    bool IsCombineThumbs() const        { return m_combine_thumbs; }
    bool IsActiveHigherThumb() const    { return m_selection == ssHigher; }

    void    ShowLabelOnMouseMove(bool show = true) { m_show_move_label = show; }
    ImRect  GetGrooveRect() const       { return m_draw_opts.groove(m_pos, m_size, is_horizontal()); }

    bool    is_horizontal() const       { return !(m_flags & ImGuiSliderFlags_Vertical); }
    bool    is_lower_at_min() const     { return m_lower_value == m_min_value; }
    bool    is_higher_at_max() const    { return m_higher_value == m_max_value; }
    bool    is_full_span() const        { return this->is_lower_at_min() && this->is_higher_at_max(); }
    bool    is_rclick_on_thumb() const  { return m_rclick_on_selected_thumb; }

    void    correct_lower_value();
    void    correct_higher_value();

    bool    render();
    void    draw_scroll_line(const ImRect& scroll_line, const ImRect& slideable_region);

    std::string get_label(int pos) const;

    void    set_get_label_on_move_cb(std::function<std::string(int)> cb)                    { m_cb_get_label_on_move = cb; }
    void    set_get_label_cb(std::function<std::string(int)> cb)                            { m_cb_get_label = cb; }
    void    set_draw_scroll_line_cb(std::function<void(const ImRect&, const ImRect&)> cb)   { m_cb_draw_scroll_line = cb; }
    void    set_extra_draw_cb(std::function<void(const ImRect&)> cb)                        { m_cb_extra_draw = cb; }

    struct DrawOptions {
        float       scale                   { 1.f }; // used for Retina on osx

        ImVec2      dummy_sz()           const { return ImVec2(24.0f, 44.0f) * scale; }
        ImVec2      thumb_dummy_sz()     const { return ImVec2(17.0f, 17.0f) * scale; }
        ImVec2      groove_sz()          const { return ImVec2(10.0f,  8.0f) * scale; }
        ImVec2      draggable_region_sz()const { return ImVec2(40.0f, 19.0f) * scale; }
        ImVec2      text_dummy_sz()      const { return ImVec2(50.0f, 34.0f) * scale; }
        ImVec2      text_padding()       const { return ImVec2( 5.0f,  2.0f) * scale; }

        float       thumb_radius()       const { return 14.0f * scale; }
        float       thumb_border()       const { return 2.0f * scale; }
        float       rounding()           const { return 2.0f * scale; }

        ImRect      groove(const ImVec2& pos, const ImVec2& size, bool is_horizontal) const;
        ImRect      draggable_region(const ImRect& groove, bool is_horizontal) const;
        ImRect      slider_line(const ImRect& draggable_region, const ImVec2& h_thumb_center, const ImVec2& l_thumb_center, bool is_horizontal) const;
    };

    struct Regions {
        ImRect higher_slideable_region;
        ImRect lower_slideable_region;
        ImRect higher_thumb;
        ImRect lower_thumb;
    };

private:

    SelectedSlider  m_selection;
    ImVec2          m_pos;
    ImVec2          m_size;
    std::string     m_name;
    ImGuiSliderFlags m_flags{ ImGuiSliderFlags_None };
    bool            m_is_shown{ true };

    int         m_min_value;
    int         m_max_value;
    int         m_lower_value;
    int         m_higher_value;
    int         m_mouse_pos_value;

    bool        m_rclick_on_selected_thumb{ false };

    bool        m_draw_lower_thumb{ true };
    bool        m_combine_thumbs  { false };
    bool        m_show_move_label{ false };

    DrawOptions m_draw_opts;
    Regions     m_regions;

    std::function<std::string(int)>                     m_cb_get_label          { nullptr };
    std::function<std::string(int)>                     m_cb_get_label_on_move  { nullptr };
    std::function<void(const ImRect&, const ImRect&)>   m_cb_draw_scroll_line   { nullptr };
    std::function<void(const ImRect&)>                  m_cb_extra_draw         { nullptr };

    void        apply_regions(int higher_value, int lower_value, const ImRect& draggable_region);
    std::string get_label_on_move(int pos) const { return m_cb_get_label_on_move ? m_cb_get_label_on_move(pos) : get_label(pos); }

    void        check_and_correct_thumbs(int* higher_value, int* lower_value);

    void        draw_background(const ImRect& slideable_region);
    void        draw_label(std::string label, const ImRect& thumb);
    void        draw_thumb(const ImVec2& center, bool mark = false);
    bool        draw_slider(int* higher_value, int* lower_value,
                            std::string& higher_label, std::string& lower_label,
                            const ImVec2& pos, const ImVec2& size, float scale = 1.0f);
};

} // GUI

} // Slic3r


#endif // slic3r_ImGUI_DoubleSlider_hpp_
