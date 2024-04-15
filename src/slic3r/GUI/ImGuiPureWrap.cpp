///|/ Copyright (c) Prusa Research 2018 - 2023 Oleksandra Iushchenko @YuSanka, Lukáš Matěna @lukasmatena, Enrico Turri @enricoturri1966, David Kocík @kocikdav, Vojtěch Bubník @bubnikv, Tomáš Mészáros @tamasmeszaros, Filip Sykala @Jony01, Lukáš Hejl @hejllukas, Vojtěch Král @vojtechkral
///|/ Copyright (c) 2019 Jason Tibbitts @jasontibbitts
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "ImGuiPureWrap.hpp"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/nowide/convert.hpp>

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include <imgui/imgui_internal.h>

namespace ImGuiPureWrap {

void set_display_size(float w, float h)
{
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(w, h);
    io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
}

ImVec2 calc_text_size(std::string_view text,
                                    bool  hide_text_after_double_hash,
                                    float wrap_width)
{
    return ImGui::CalcTextSize(text.data(), text.data() + text.length(),
                               hide_text_after_double_hash, wrap_width);
}

ImVec2 calc_text_size(const std::string& text,
                                    bool  hide_text_after_double_hash,
                                    float wrap_width)
{
    return ImGui::CalcTextSize(text.c_str(), NULL, hide_text_after_double_hash, wrap_width);
}

ImVec2 calc_button_size(const std::string &text, const ImVec2 &button_size)
{
    const ImVec2        text_size = calc_text_size(text);
    const ImGuiContext &g         = *GImGui;
    const ImGuiStyle   &style     = g.Style;

    return ImGui::CalcItemSize(button_size, text_size.x + style.FramePadding.x * 2.0f, text_size.y + style.FramePadding.y * 2.0f);
}

ImVec2 calc_button_size(const std::wstring& wtext, const ImVec2& button_size)
{
    const std::string text = boost::nowide::narrow(wtext);
    return calc_button_size(text, button_size);
}

ImVec2 get_item_spacing()
{
    const ImGuiContext &g     = *GImGui;
    const ImGuiStyle   &style = g.Style;
    return style.ItemSpacing;
}

float get_slider_float_height()
{
    const ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    return g.FontSize + style.FramePadding.y * 2.0f + style.ItemSpacing.y;
}

void set_next_window_pos(float x, float y, int flag, float pivot_x, float pivot_y)
{
    ImGui::SetNextWindowPos(ImVec2(x, y), (ImGuiCond)flag, ImVec2(pivot_x, pivot_y));
    ImGui::SetNextWindowSize(ImVec2(0.0, 0.0));
}

void set_next_window_bg_alpha(float alpha)
{
    ImGui::SetNextWindowBgAlpha(alpha);
}

void set_next_window_size(float x, float y, ImGuiCond cond)
{
	ImGui::SetNextWindowSize(ImVec2(x, y), cond);
}

bool begin(const std::string &name, int flags)
{
    return ImGui::Begin(name.c_str(), nullptr, (ImGuiWindowFlags)flags);
}

bool begin(const std::string& name, bool* close, int flags)
{
    return ImGui::Begin(name.c_str(), close, (ImGuiWindowFlags)flags);
}

void end()
{
    ImGui::End();
}

bool button(const std::string & label_utf8, const std::string& tooltip)
{
    const bool ret = ImGui::Button(label_utf8.c_str());

    if (!tooltip.empty() && ImGui::IsItemHovered()) {
        auto tooltip_utf8 = tooltip;
        ImGui::SetTooltip(tooltip_utf8.c_str(), nullptr);
    }

    return ret;
}

bool button(const std::string& label_utf8, float width, float height)
{
	return ImGui::Button(label_utf8.c_str(), ImVec2(width, height));
}

bool button(const std::wstring& wlabel, float width, float height)
{
    const std::string label = boost::nowide::narrow(wlabel);
    return button(label, width, height);
}

bool radio_button(const std::string& label_utf8, bool active)
{
    return ImGui::RadioButton(label_utf8.c_str(), active);
}

bool draw_radio_button(const std::string& name, float size, bool active,
    std::function<void(ImGuiWindow& window, const ImVec2& pos, float size)> draw_callback)
{
    ImGuiWindow& window = *ImGui::GetCurrentWindow();
    if (window.SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window.GetID(name.c_str());

    const ImVec2 pos = window.DC.CursorPos;
    const ImRect total_bb(pos, pos + ImVec2(size, size + style.FramePadding.y * 2.0f));
    ImGui::ItemSize(total_bb, style.FramePadding.y);
    if (!ImGui::ItemAdd(total_bb, id))
        return false;

    bool hovered, held;
    bool pressed = ImGui::ButtonBehavior(total_bb, id, &hovered, &held);
    if (pressed)
        ImGui::MarkItemEdited(id);

    if (hovered)
        window.DrawList->AddRect({ pos.x - 1.0f, pos.y - 1.0f }, { pos.x + size + 1.0f, pos.y + size + 1.0f }, ImGui::GetColorU32(ImGuiCol_CheckMark));

    if (active)
        window.DrawList->AddRect(pos, { pos.x + size, pos.y + size }, ImGui::GetColorU32(ImGuiCol_CheckMark));

    draw_callback(window, pos, size);

    IMGUI_TEST_ENGINE_ITEM_INFO(id, label, window.DC.LastItemStatusFlags);
    return pressed;
}

bool checkbox(const std::string& label_utf8, bool &value)
{
    return ImGui::Checkbox(label_utf8.c_str(), &value);
}

void text(const char *label)
{
    ImGui::Text("%s", label);
}

void text(const std::string &label)
{
    text(label.c_str());
}

void text(const std::wstring& wlabel)
{
    const std::string label = boost::nowide::narrow(wlabel);
    text(label.c_str());
}

void text_colored(const ImVec4& color, const char* label)
{
    ImGui::TextColored(color, "%s", label);
}

void text_colored(const ImVec4& color, const std::string& label)
{
    text_colored(color, label.c_str());
}

void text_wrapped(const char *label, float wrap_width)
{
    ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + wrap_width);
    text(label);
    ImGui::PopTextWrapPos();
}

void text_wrapped(const std::string &label, float wrap_width)
{
    text_wrapped(label.c_str(), wrap_width);
}

void tooltip(const char *label, float wrap_width)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 4.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 4.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 8.0f, 8.0f });
    ImGui::BeginTooltip();
    ImGui::PushTextWrapPos(wrap_width);
    ImGui::TextUnformatted(label);
    ImGui::PopTextWrapPos();
    ImGui::EndTooltip();
    ImGui::PopStyleVar(3);
}

void tooltip(const std::string& label, float wrap_width)
{
    tooltip(label.c_str(), wrap_width);
}

ImVec2 get_slider_icon_size()
{
    return calc_button_size(std::wstring(&ImGui::SliderFloatEditBtnIcon, 1));
}

static bool image_button_ex(ImGuiID id, ImTextureID texture_id, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1, const ImVec2& padding, const ImVec4& bg_col, const ImVec4& tint_col, ImGuiButtonFlags flags)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    const ImRect bb(window->DC.CursorPos, window->DC.CursorPos + size + padding * 2);
    ImGui::ItemSize(bb);
    if (!ImGui::ItemAdd(bb, id))
        return false;

    bool hovered, held;
    bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held, flags);

    // Render
    const ImU32 col = ImGui::GetColorU32((held && hovered) ? ImGuiCol_ButtonActive : hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
    ImGui::RenderNavHighlight(bb, id);
    ImGui::RenderFrame(bb.Min, bb.Max, col, true, ImClamp((float)ImMin(padding.x, padding.y), 0.0f, g.Style.FrameRounding));
    if (bg_col.w > 0.0f)
        window->DrawList->AddRectFilled(bb.Min + padding, bb.Max - padding, ImGui::GetColorU32(bg_col));
    window->DrawList->AddImage(texture_id, bb.Min + padding, bb.Max - padding, uv0, uv1, ImGui::GetColorU32(tint_col));

    return pressed;
}

bool image_button(ImTextureID user_texture_id, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1, int frame_padding, const ImVec4& bg_col, const ImVec4& tint_col, ImGuiButtonFlags flags)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    if (window->SkipItems)
        return false;

    // Default to using texture ID as ID. User can still push string/integer prefixes.
    ImGui::PushID((void*)(intptr_t)user_texture_id);
    const ImGuiID id = window->GetID("#image");
    ImGui::PopID();

    const ImVec2 padding = (frame_padding >= 0) ? ImVec2((float)frame_padding, (float)frame_padding) : g.Style.FramePadding;
    return image_button_ex(id, user_texture_id, size, uv0, uv1, padding, bg_col, tint_col, flags);
}

bool combo(const std::string& label, const std::vector<std::string>& options, int& selection, ImGuiComboFlags flags/* = 0*/, float label_width/* = 0.0f*/, float item_width/* = 0.0f*/)
{
    // this is to force the label to the left of the widget:
    const bool hidden_label = boost::starts_with(label, "##");
    if (!label.empty() && !hidden_label) {
        text(label);
        ImGui::SameLine(label_width);
    }
    ImGui::PushItemWidth(item_width);

    int selection_out = selection;
    bool res = false;

    const char *selection_str = selection < int(options.size()) && selection >= 0 ? options[selection].c_str() : "";
    if (ImGui::BeginCombo(hidden_label ? label.c_str() : ("##" + label).c_str(), selection_str, flags)) {
        for (int i = 0; i < (int)options.size(); i++) {
            if (ImGui::Selectable(options[i].c_str(), i == selection)) {
                selection_out = i;
                res = true;
            }
        }

        ImGui::EndCombo();
    }

    selection = selection_out;
    return res;
}

// Scroll up for one item 
void scroll_up()
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;

    float item_size_y = window->DC.PrevLineSize.y + g.Style.ItemSpacing.y;
    float win_top = window->Scroll.y;

    ImGui::SetScrollY(win_top - item_size_y);
}

// Scroll down for one item 
void scroll_down()
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;

    float item_size_y = window->DC.PrevLineSize.y + g.Style.ItemSpacing.y;
    float win_top = window->Scroll.y;

    ImGui::SetScrollY(win_top + item_size_y);
}

void process_mouse_wheel(int& mouse_wheel)
{
    if (mouse_wheel > 0)
        scroll_up();
    else if (mouse_wheel < 0)
        scroll_down();
    mouse_wheel = 0;
}

bool undo_redo_list(const ImVec2& size, const bool is_undo, bool (*items_getter)(const bool , int , const char**), int& hovered, int& selected, int& mouse_wheel)
{
    bool is_hovered = false;
    ImGui::ListBoxHeader("", size);

    int i=0;
    const char* item_text;
    while (items_getter(is_undo, i, &item_text)) {
        ImGui::Selectable(item_text, i < hovered);

        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", item_text);
            hovered = i;
            is_hovered = true;
        }

        if (ImGui::IsItemClicked())
            selected = i;
        i++;
    }

    if (is_hovered)
        process_mouse_wheel(mouse_wheel);

    ImGui::ListBoxFooter();
    return is_hovered;
}

void title(const std::string& str)
{
    text(str);
    ImGui::Separator();
}

bool want_mouse()
{
    return ImGui::GetIO().WantCaptureMouse;
}

bool want_keyboard()
{
    return ImGui::GetIO().WantCaptureKeyboard;
}

bool want_text_input()
{
    return ImGui::GetIO().WantTextInput;
}

bool want_any_input()
{
    const auto io = ImGui::GetIO();
    return io.WantCaptureMouse || io.WantCaptureKeyboard || io.WantTextInput;
}

void disable_background_fadeout_animation()
{
    GImGui->DimBgRatio = 1.0f;
}

template <typename T, typename Func> 
static bool input_optional(std::optional<T> &v, Func& f, std::function<bool(const T&)> is_default, const T& def_val)
{
    if (v.has_value()) {
        if (f(*v)) {
            if (is_default(*v)) v.reset();
            return true;
        }
    } else {
        T val = def_val;
        if (f(val)) {
            if (!is_default(val)) v = val;
            return true;
        }
    }
    return false;
}

bool input_optional_int(const char *        label,
                                      std::optional<int>& v,
                                      int                 step,
                                      int                 step_fast,
                                      ImGuiInputTextFlags flags,
                                      int                 def_val)
{
    auto func = [&](int &value) {
        return ImGui::InputInt(label, &value, step, step_fast, flags);
    };
    std::function<bool(const int &)> is_default = 
        [def_val](const int &value) -> bool { return value == def_val; };
    return input_optional(v, func, is_default, def_val);
}

bool input_optional_float(const char *          label,
                                        std::optional<float> &v,
                                        float                 step,
                                        float                 step_fast,
                                        const char *          format,
                                        ImGuiInputTextFlags   flags,
                                        float                 def_val)
{
    auto func = [&](float &value) {
        return ImGui::InputFloat(label, &value, step, step_fast, format, flags);
    };
    std::function<bool(const float &)> is_default =
        [def_val](const float &value) -> bool {
        return std::fabs(value-def_val) <= std::numeric_limits<float>::epsilon();
    };
    return input_optional(v, func, is_default, def_val);
}

bool drag_optional_float(const char *          label,
                                       std::optional<float> &v,
                                       float                 v_speed,
                                       float                 v_min,
                                       float                 v_max,
                                       const char *          format,
                                       float                 power,
                                       float                 def_val)
{
    auto func = [&](float &value) {
        return ImGui::DragFloat(label, &value, v_speed, v_min, v_max, format, power);
    };
    std::function<bool(const float &)> is_default =
        [def_val](const float &value) -> bool {
        return std::fabs(value-def_val) <= std::numeric_limits<float>::epsilon();
    };
    return input_optional(v, func, is_default, def_val);
}

std::optional<ImVec2> change_window_position(const char *window_name, bool try_to_fix) {
    ImGuiWindow *window = ImGui::FindWindowByName(window_name);
    // is window just created
    if (window == NULL)
        return {};

    // position of window on screen
    ImVec2 position = window->Pos;
    ImVec2 size     = window->SizeFull;

    // screen size
    ImVec2 screen = ImGui::GetMainViewport()->Size;

    std::optional<ImVec2> output_window_offset;
    if (position.x < 0) {
        if (position.y < 0)
            // top left 
            output_window_offset = ImVec2(0, 0); 
        else
            // only left
            output_window_offset = ImVec2(0, position.y); 
    } else if (position.y < 0) {
        // only top
        output_window_offset = ImVec2(position.x, 0); 
    } else if (screen.x < (position.x + size.x)) {
        if (screen.y < (position.y + size.y))
            // right bottom
            output_window_offset = ImVec2(screen.x - size.x, screen.y - size.y);
        else
            // only right
            output_window_offset = ImVec2(screen.x - size.x, position.y);
    } else if (screen.y < (position.y + size.y)) {
        // only bottom
        output_window_offset = ImVec2(position.x, screen.y - size.y);
    }

    if (!try_to_fix && output_window_offset.has_value())
        output_window_offset = ImVec2(-1, -1); // Put on default position

    return output_window_offset;
}

void left_inputs() {
    ImGui::ClearActiveID(); 
}

std::string trunc(const std::string &text,
                                float              width,
                                const char *       tail)
{
    float text_width = ImGui::CalcTextSize(text.c_str()).x;
    if (text_width < width) return text;
    float tail_width = ImGui::CalcTextSize(tail).x;
    assert(width > tail_width);
    if (width <= tail_width) return "Error: Can't add tail and not be under wanted width.";
    float allowed_width = width - tail_width;
    
    // guess approx count of letter
    float average_letter_width = calc_text_size(std::string_view("n")).x; // average letter width
    unsigned count_letter  = static_cast<unsigned>(allowed_width / average_letter_width);

    std::string_view text_ = text;
    std::string_view result_text = text_.substr(0, count_letter);
    text_width = calc_text_size(result_text).x;
    if (text_width < allowed_width) {
        // increase letter count
        while (count_letter < text.length()) {
            ++count_letter;
            std::string_view act_text = text_.substr(0, count_letter);
            text_width = calc_text_size(act_text).x;
            if (text_width > allowed_width) break;
            result_text = act_text;
        }
    } else {
        // decrease letter count
        while (count_letter > 1) {
            --count_letter;
            result_text = text_.substr(0, count_letter);
            text_width  = calc_text_size(result_text).x;
            if (text_width < allowed_width) break;            
        } 
    }
    return std::string(result_text) + tail;
}

void escape_double_hash(std::string &text)
{
    // add space between hashes
    const std::string search  = "##";
    const std::string replace = "# #";
    size_t pos = 0;
    while ((pos = text.find(search, pos)) != std::string::npos) 
        text.replace(pos, search.length(), replace);
}

void draw_cross_hair(const ImVec2 &position, float radius, ImU32 color, int num_segments, float thickness)
{
    auto draw_list = ImGui::GetOverlayDrawList();
    draw_list->AddCircle(position, radius, color, num_segments, thickness);
    auto dirs = {ImVec2{0, 1}, ImVec2{1, 0}, ImVec2{0, -1}, ImVec2{-1, 0}};
    for (const ImVec2 &dir : dirs) {
        ImVec2 start(position.x + dir.x * 0.5 * radius, position.y + dir.y * 0.5 * radius);
        ImVec2 end(position.x + dir.x * 1.5 * radius, position.y + dir.y * 1.5 * radius);
        draw_list->AddLine(start, end, color, thickness);
    }
}

bool contain_all_glyphs(const ImFont      *font,
                                     const std::string &text)
{
    if (font == nullptr) return false;
    if (!font->IsLoaded()) return false;
    const ImFontConfig *fc = font->ConfigData;
    if (fc == nullptr) return false;
    if (text.empty()) return true;
    return is_chars_in_ranges(fc->GlyphRanges, text.c_str());
}

bool is_char_in_ranges(const ImWchar *ranges,
                                      unsigned int   letter)
{
    for (const ImWchar *range = ranges; range[0] && range[1]; range += 2) {
        ImWchar from = range[0];
        ImWchar to   = range[1];
        if (from <= letter && letter <= to) return true;
        if (letter < to) return false; // ranges should be sorted
    }
    return false;
};

bool is_chars_in_ranges(const ImWchar *ranges,
                                       const char    *chars_ptr)
{
    while (*chars_ptr) {
        unsigned int c = 0;
        // UTF-8 to 32-bit character need imgui_internal
        int c_len = ImTextCharFromUtf8(&c, chars_ptr, NULL);
        chars_ptr += c_len;
        if (c_len == 0) break;
        if (!is_char_in_ranges(ranges, c)) return false;
    }
    return true;
}

} //  ImGuiPureWrap
