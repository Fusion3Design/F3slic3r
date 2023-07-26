#include "TopBar.hpp"

#include "GUI_App.hpp"
#include "Plater.hpp"
//#include "wxExtensions.hpp"
#include "format.hpp"
#include "I18N.hpp"

#include <wx/button.h>
#include <wx/sizer.h>

wxDEFINE_EVENT(wxCUSTOMEVT_TOPBAR_SEL_CHANGED, wxCommandEvent);

using namespace Slic3r::GUI;

#define down_arrow L"\u23f7";


TopBarItemsCtrl::Button::Button(wxWindow* parent, const wxString& label, const std::string& icon_name, const int px_cnt)
:ScalableButton(parent, wxID_ANY, icon_name, label, wxDefaultSize, wxDefaultPosition, wxNO_BORDER, px_cnt)
{
    int btn_margin = em_unit(this);
    wxSize size = GetTextExtent(label) + wxSize(6 * btn_margin, int(1.5 * btn_margin));
    if (icon_name.empty())
        this->SetMinSize(size);
    else
        this->SetMinSize(wxSize(-1, size.y));

    const wxColour& selected_btn_bg = wxGetApp().get_label_clr_default();
    const wxColour& default_btn_bg  = wxGetApp().get_window_default_clr();

    this->Bind(wxEVT_SET_FOCUS, [this, selected_btn_bg, default_btn_bg](wxFocusEvent& event) { 
        this->SetBackgroundColour(selected_btn_bg);
        this->SetForegroundColour(default_btn_bg);
        event.Skip();  
    });
    this->Bind(wxEVT_KILL_FOCUS, [this, selected_btn_bg, default_btn_bg](wxFocusEvent& event) {
        if (!m_is_selected) {
            this->SetBackgroundColour(default_btn_bg);
            this->SetForegroundColour(selected_btn_bg);
        }
        event.Skip();
    });
}

TopBarItemsCtrl::ButtonWithPopup::ButtonWithPopup(wxWindow* parent, const wxString& label, const std::string& icon_name)
    :TopBarItemsCtrl::Button(parent, label, icon_name, 24)
{
    this->SetLabel(label);
}

void TopBarItemsCtrl::ButtonWithPopup::SetLabel(const wxString& label)
{
    wxString full_label = "  " + label + "  " + down_arrow;
    ScalableButton::SetLabel(full_label);
}

static wxString get_workspace_name(Slic3r::ConfigOptionMode mode) 
{
    return  mode == Slic3r::ConfigOptionMode::comSimple   ? _L("Beginner workspace") :
            mode == Slic3r::ConfigOptionMode::comAdvanced ? _L("Regular workspace")  : _L("Josef Prusa's workspace");
}

TopBarItemsCtrl::TopBarItemsCtrl(wxWindow *parent, bool add_mode_buttons/* = false*/) :
    wxControl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE | wxTAB_TRAVERSAL)
{
#ifdef __WINDOWS__
    SetDoubleBuffered(true);
#endif //__WINDOWS__

    int em = em_unit(this);// Slic3r::GUI::wxGetApp().em_unit();
    m_btn_margin  = std::lround(0.9 * em);
    m_line_margin = std::lround(0.1 * em);

    m_sizer = new wxBoxSizer(wxHORIZONTAL);
    this->SetSizer(m_sizer);

    m_menu_btn = new ButtonWithPopup(this, _L("Menu"), wxGetApp().logo_name());
    m_sizer->Add(m_menu_btn, 1, wxALIGN_CENTER_VERTICAL | wxALL, m_btn_margin);

    m_buttons_sizer = new wxFlexGridSizer(1, m_btn_margin, m_btn_margin);
    m_sizer->Add(m_buttons_sizer, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 2 * m_btn_margin);

    if (add_mode_buttons) {
        m_mode_sizer = new ModeSizer(this, m_btn_margin);
        m_sizer->AddStretchSpacer(20);
        m_sizer->Add(m_mode_sizer, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxBOTTOM, m_btn_margin);

        m_mode_sizer->ShowItems(false);
    }

    m_workspace_btn = new ButtonWithPopup(this, _L("Workspace"), "mode_simple");
    m_sizer->AddStretchSpacer(20);
    m_sizer->Add(m_workspace_btn, 1, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT | wxRIGHT, 2 * m_btn_margin);

    // create modes menu
    {
        for (const Slic3r::ConfigOptionMode& mode : { Slic3r::ConfigOptionMode::comSimple, Slic3r::ConfigOptionMode::comAdvanced, Slic3r::ConfigOptionMode::comExpert } ) {
            const wxString label = get_workspace_name(mode);
            append_menu_item(&m_workspace_modes, wxID_ANY, label, label,
                [this, mode](wxCommandEvent&) {
                    if (wxGetApp().get_mode() != mode)
                        wxGetApp().save_mode(mode);
                }, get_bmp_bundle("mode", 16, -1, wxGetApp().get_mode_btn_color(mode)), nullptr, []() { return true; }, this);

            if (mode < Slic3r::ConfigOptionMode::comExpert)
                m_workspace_modes.AppendSeparator();
        }
    }


    this->Bind(wxEVT_PAINT, &TopBarItemsCtrl::OnPaint, this);
    
    m_menu_btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent& event) {
        wxPoint pos = m_menu_btn->GetPosition();
        wxGetApp().plater()->PopupMenu(&m_menu, pos);
    });

    m_menu.Bind(wxEVT_MENU_CLOSE, [this](wxMenuEvent&) {
        if (m_menu_btn->HasFocus()) {
            wxPostEvent(m_menu_btn->GetEventHandler(), wxFocusEvent(wxEVT_KILL_FOCUS));
        }
    });
    
    m_workspace_btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent& event) {
        wxPoint pos = m_workspace_btn->GetPosition();
        wxGetApp().plater()->PopupMenu(&m_workspace_modes, pos);
    });

    m_workspace_modes.Bind(wxEVT_MENU_CLOSE, [this](wxMenuEvent&) {
        if (m_workspace_btn->HasFocus()) {
            wxPostEvent(m_workspace_btn->GetEventHandler(), wxFocusEvent(wxEVT_KILL_FOCUS));
        }
    });
}

void TopBarItemsCtrl::OnPaint(wxPaintEvent&)
{
    wxGetApp().UpdateDarkUI(this);
    const wxSize sz = GetSize();
    wxPaintDC dc(this);

    if (m_selection < 0 || m_selection >= (int)m_pageButtons.size())
        return;

    const wxColour& selected_btn_bg  = wxGetApp().get_label_clr_default();
    const wxColour& default_btn_bg   = wxGetApp().get_window_default_clr();
    const wxColour& btn_marker_color = wxGetApp().get_highlight_default_clr();

    // highlight selected notebook button

    for (int idx = 0; idx < int(m_pageButtons.size()); idx++) {
        wxButton* btn = m_pageButtons[idx];

        btn->SetBackgroundColour(idx == m_selection ? selected_btn_bg : default_btn_bg);
        btn->SetForegroundColour(idx == m_selection ? default_btn_bg : selected_btn_bg);
    }

    // highlight selected mode button

    //bool mode_is_focused = m_workspace_btn->HasFocus();

    //m_workspace_btn->SetBackgroundColour(mode_is_focused ? selected_btn_bg : default_btn_bg);
    //m_workspace_btn->SetForegroundColour(mode_is_focused ? default_btn_bg : selected_btn_bg);

    //if (m_mode_sizer) {
    //    const std::vector<ModeButton*>& mode_btns = m_mode_sizer->get_btns();
    //    for (int idx = 0; idx < int(mode_btns.size()); idx++) {
    //        ModeButton* btn = mode_btns[idx];
    //        btn->SetBackgroundColour(btn->is_selected() ? selected_btn_bg : default_btn_bg);

    //        //wxPoint pos = btn->GetPosition();
    //        //wxSize size = btn->GetSize();
    //        //const wxColour& clr = btn->is_selected() ? btn_marker_color : default_btn_bg;
    //        //dc.SetPen(clr);
    //        //dc.SetBrush(clr);
    //        //dc.DrawRectangle(pos.x, pos.y + size.y, size.x, sz.y - size.y);
    //    }
    //}

    // Draw orange bottom line

    dc.SetPen(btn_marker_color);
    dc.SetBrush(btn_marker_color);
    dc.DrawRectangle(1, sz.y - m_line_margin, sz.x, m_line_margin);
}

void TopBarItemsCtrl::UpdateMode()
{
    auto mode = wxGetApp().get_mode();
    m_mode_sizer->SetMode(mode);


    auto m_bmp = *get_bmp_bundle("mode", 16, -1, wxGetApp().get_mode_btn_color(mode));

    m_workspace_btn->SetBitmap(m_bmp);
    m_workspace_btn->SetBitmapCurrent(m_bmp);
    m_workspace_btn->SetBitmapPressed(m_bmp);
    m_workspace_btn->SetLabel(get_workspace_name(mode));

    m_workspace_btn->SetBitmapMargins(int(0.5 * em_unit(this)), 0);
    this->Layout();
}

void TopBarItemsCtrl::Rescale()
{
    int em = em_unit(this);
    m_btn_margin = std::lround(0.3 * em);
    m_line_margin = std::lround(0.1 * em);
    m_buttons_sizer->SetVGap(m_btn_margin);
    m_buttons_sizer->SetHGap(m_btn_margin);

    m_sizer->Layout();
}

void TopBarItemsCtrl::OnColorsChanged()
{
    m_menu_btn->sys_color_changed();

    for (ScalableButton* btn : m_pageButtons)
        btn->sys_color_changed();

    m_mode_sizer->sys_color_changed();

    m_sizer->Layout();
}

void TopBarItemsCtrl::UpdateModeMarkers()
{
    m_mode_sizer->update_mode_markers();
}

void TopBarItemsCtrl::UpdateSelection()
{
    for (Button* btn : m_pageButtons)
        btn->set_selected(false);

    if (m_selection >= 0)
        m_pageButtons[m_selection]->set_selected(true);

    Refresh();
}

void TopBarItemsCtrl::SetSelection(int sel)
{
    if (m_selection == sel)
        return;
    m_selection = sel;
    UpdateSelection();
}

bool TopBarItemsCtrl::InsertPage(size_t n, const wxString& text, bool bSelect/* = false*/, const std::string& bmp_name/* = ""*/)
{
    Button* btn = new Button(this, text);
    btn->Bind(wxEVT_BUTTON, [this, btn](wxCommandEvent& event) {
        if (auto it = std::find(m_pageButtons.begin(), m_pageButtons.end(), btn); it != m_pageButtons.end()) {
            m_selection = it - m_pageButtons.begin();
            wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_TOPBAR_SEL_CHANGED);
            evt.SetId(m_selection);
            wxPostEvent(this->GetParent(), evt);
            UpdateSelection();
        }
    });

    m_pageButtons.insert(m_pageButtons.begin() + n, btn);
    m_buttons_sizer->Insert(n, new wxSizerItem(btn, 0, wxALIGN_CENTER_VERTICAL));
    m_buttons_sizer->SetCols(m_buttons_sizer->GetCols() + 1);
    m_sizer->Layout();
    return true;
}

void TopBarItemsCtrl::RemovePage(size_t n)
{
    ScalableButton* btn = m_pageButtons[n];
    m_pageButtons.erase(m_pageButtons.begin() + n);
    m_buttons_sizer->Remove(n);
    btn->Reparent(nullptr);
    btn->Destroy();
    m_sizer->Layout();
}

void TopBarItemsCtrl::SetPageText(size_t n, const wxString& strText)
{
    ScalableButton* btn = m_pageButtons[n];
    btn->SetLabel(strText);
}

wxString TopBarItemsCtrl::GetPageText(size_t n) const
{
    ScalableButton* btn = m_pageButtons[n];
    return btn->GetLabel();
}

void TopBarItemsCtrl::AppendMenuItem(wxMenu* menu, const wxString& title)
{
    append_submenu(&m_menu, menu, wxID_ANY, title, "cog");
}

void TopBarItemsCtrl::AppendMenuSeparaorItem()
{
    m_menu.AppendSeparator();
}

void TopBarItemsCtrl::ShowMenu()
{
    wxPoint pos = m_menu_btn->GetPosition();
    wxGetApp().plater()->PopupMenu(&m_menu, pos);
}
