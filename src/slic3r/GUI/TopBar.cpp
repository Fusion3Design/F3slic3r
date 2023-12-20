#include "TopBar.hpp"

#include "GUI_App.hpp"
#include "Plater.hpp"
#include "Search.hpp"
#include "UserAccount.hpp"
//#include "wxExtensions.hpp"
#include "format.hpp"
#include "I18N.hpp"

#include <wx/button.h>
#include <wx/sizer.h>

wxDEFINE_EVENT(wxCUSTOMEVT_TOPBAR_SEL_CHANGED, wxCommandEvent);

using namespace Slic3r::GUI;

#ifdef __APPLE__
#define down_arrow L"\u25BC";
#else
#define down_arrow L"\u23f7";
#endif


TopBarItemsCtrl::Button::Button(wxWindow* parent, const wxString& label, const std::string& icon_name, const int px_cnt)
:ScalableButton(parent, wxID_ANY, icon_name, label, wxDefaultSize, wxDefaultPosition, wxNO_BORDER, px_cnt)
{
    int btn_margin = em_unit(this);
    wxSize size = GetTextExtent(label) + wxSize(6 * btn_margin, int(1.5 * btn_margin));
    if (icon_name.empty())
        this->SetMinSize(size);
    if (label.IsEmpty()) {
        const int btn_side = px_cnt + btn_margin;
        this->SetMinSize(wxSize(btn_side, btn_side));
    }
    else
        this->SetMinSize(wxSize(-1, size.y));

    //button events
    Bind(wxEVT_SET_FOCUS,    [this](wxFocusEvent& event) { set_hovered(true ); event.Skip(); });
    Bind(wxEVT_KILL_FOCUS,   [this](wxFocusEvent& event) { set_hovered(false); event.Skip(); });
    Bind(wxEVT_ENTER_WINDOW, [this](wxMouseEvent& event) { set_hovered(true ); event.Skip(); });
    Bind(wxEVT_LEAVE_WINDOW, [this](wxMouseEvent& event) { set_hovered(false); event.Skip(); });
}

void TopBarItemsCtrl::Button::set_selected(bool selected)
{
    m_is_selected = selected;

    if (m_is_selected) {
#ifdef __APPLE__
        this->SetBackgroundColour(wxGetApp().get_highlight_default_clr());
#else
        this->SetBackgroundColour(wxGetApp().get_label_clr_default());
        this->SetForegroundColour(wxGetApp().get_window_default_clr());
#endif
    }
    else {
#ifdef _WIN32
        this->SetBackgroundColour(wxGetApp().get_window_default_clr());
#else
        this->SetBackgroundColour(wxTransparentColor);
#endif

#ifndef __APPLE__
        this->SetForegroundColour(wxGetApp().get_label_clr_default());
#endif
    }
}

void TopBarItemsCtrl::Button::set_hovered(bool hovered)
{
    using namespace Slic3r::GUI;
    const wxFont& new_font = hovered ? wxGetApp().bold_font() : wxGetApp().normal_font();

    this->SetFont(new_font);
#ifdef _WIN32
    this->GetParent()->Refresh(); // force redraw a background of the selected mode button
#else
    SetForegroundColour(wxSystemSettings::GetColour(set_focus ? wxSYS_COLOUR_BTNTEXT :
#if defined (__linux__) && defined (__WXGTK3__)
        wxSYS_COLOUR_GRAYTEXT
#elif defined (__linux__) && defined (__WXGTK2__)
        wxSYS_COLOUR_BTNTEXT
#else 
        wxSYS_COLOUR_BTNSHADOW
#endif    
    ));
#endif /* no _WIN32 */

    const wxColour& color = hovered       ? wxGetApp().get_color_selected_btn_bg() :
                            m_is_selected ? wxGetApp().get_label_clr_default()       :
                                            wxGetApp().get_window_default_clr();
    this->SetBackgroundColour(color);

    this->Refresh();
    this->Update();
}

TopBarItemsCtrl::ButtonWithPopup::ButtonWithPopup(wxWindow* parent, const wxString& label, const std::string& icon_name)
    :TopBarItemsCtrl::Button(parent, label, icon_name, 24)
{
    this->SetLabel(label);
}

TopBarItemsCtrl::ButtonWithPopup::ButtonWithPopup(wxWindow* parent, const std::string& icon_name, int icon_width/* = 20*/, int icon_height/* = 20*/)
    :TopBarItemsCtrl::Button(parent, "", icon_name, icon_width)
{
}

void TopBarItemsCtrl::ButtonWithPopup::SetLabel(const wxString& label)
{
    wxString full_label = "  " + label + "  " + down_arrow;
    ScalableButton::SetLabel(full_label);
}

static wxString get_workspace_name(Slic3r::ConfigOptionMode mode) 
{
    return  mode == Slic3r::ConfigOptionMode::comSimple   ? _L("Beginners") :
            mode == Slic3r::ConfigOptionMode::comAdvanced ? _L("Regulars")  : _L("Josef Prusa");

    return  mode == Slic3r::ConfigOptionMode::comSimple   ? _L("Beginner workspace") :
            mode == Slic3r::ConfigOptionMode::comAdvanced ? _L("Regular workspace")  : _L("Josef Prusa's workspace");
}

void TopBarItemsCtrl::ApplyWorkspacesMenu()
{
    wxMenuItemList& items = m_workspaces_menu.GetMenuItems();
    if (!items.IsEmpty()) {
        for (int id = int(m_workspaces_menu.GetMenuItemCount()) - 1; id >= 0; id--)
            m_workspaces_menu.Destroy(items[id]);
    }

    for (const Slic3r::ConfigOptionMode& mode : { Slic3r::ConfigOptionMode::comSimple,
                                                  Slic3r::ConfigOptionMode::comAdvanced,
                                                  Slic3r::ConfigOptionMode::comExpert }) {
        const wxString label = get_workspace_name(mode);
        append_menu_item(&m_workspaces_menu, wxID_ANY, label, label,
            [this, mode](wxCommandEvent&) {
                if (wxGetApp().get_mode() != mode)
                    wxGetApp().save_mode(mode);
            }, get_bmp_bundle("mode", 16, -1, wxGetApp().get_mode_btn_color(mode)));

        if (mode < Slic3r::ConfigOptionMode::comExpert)
            m_workspaces_menu.AppendSeparator();
    }
}

void TopBarItemsCtrl::CreateAuthMenu()
{
    m_user_menu_item = append_menu_item(&m_auth_menu, wxID_ANY, "", "",
        [this](wxCommandEvent& e) { 
            m_auth_btn->set_selected(true);
            wxGetApp().plater()->PopupMenu(&m_auth_menu, m_auth_btn->GetPosition());
        }, get_bmp_bundle("user", 16));

    m_auth_menu.AppendSeparator();

    m_connect_dummy_menu_item = append_menu_item(&m_auth_menu, wxID_ANY, _L("PrusaConnect Printers"), "",
        [this](wxCommandEvent&) { wxGetApp().plater()->get_user_account()->enqueue_connect_printers_action(); }, 
        "", nullptr, []() { return wxGetApp().plater()->get_user_account()->is_logged(); }, this->GetParent());

    m_login_menu_item = append_menu_item(&m_auth_menu, wxID_ANY, "", "",
        [this](wxCommandEvent&) {
            auto user_account = wxGetApp().plater()->get_user_account();
            if (user_account->is_logged())
                user_account->do_logout(wxGetApp().app_config);
            else
                user_account->do_login();
        }, get_bmp_bundle("login", 16));
}

void TopBarItemsCtrl::UpdateAuthMenu()
{
    auto user_account = wxGetApp().plater()->get_user_account();
    if (m_login_menu_item) {
        m_login_menu_item->SetItemLabel(user_account->is_logged() ? _L("PrusaAuth Log out") : _L("PrusaAuth Log in"));
        m_login_menu_item->SetBitmap(user_account->is_logged() ? *get_bmp_bundle("logout", 16) : *get_bmp_bundle("login", 16));
    }

    if (m_user_menu_item)
        m_user_menu_item->SetItemLabel(user_account->is_logged() ? from_u8(user_account->get_username()) : _L("Anonymus"));
}

void TopBarItemsCtrl::CreateSearch()
{
    m_search = new ::TextInput(this, wxGetApp().searcher().default_string, "", "search", wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
    wxGetApp().UpdateDarkUI(m_search);
    wxGetApp().searcher().set_search_input(m_search);
}

TopBarItemsCtrl::TopBarItemsCtrl(wxWindow *parent) :
    wxControl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE | wxTAB_TRAVERSAL)
{
#ifdef __WINDOWS__
    SetDoubleBuffered(true);
#endif //__WINDOWS__

    int em = em_unit(this);
    m_btn_margin  = std::lround(0.9 * em);
    m_line_margin = std::lround(0.1 * em);

    m_sizer = new wxFlexGridSizer(2);
    m_sizer->AddGrowableCol(0);
    m_sizer->SetFlexibleDirection(wxHORIZONTAL);
    this->SetSizer(m_sizer);

    wxBoxSizer* left_sizer = new wxBoxSizer(wxHORIZONTAL);

#ifdef __APPLE__
    auto logo = new wxStaticBitmap(this, wxID_ANY, *get_bmp_bundle(wxGetApp().logo_name(), 24));
    left_sizer->Add(logo, 1, wxALIGN_CENTER_VERTICAL | wxALL, m_btn_margin);
#else
    m_menu_btn = new ButtonWithPopup(this, _L("Menu"), wxGetApp().logo_name());
    left_sizer->Add(m_menu_btn, 0, wxALIGN_CENTER_VERTICAL | wxALL, m_btn_margin);
    
    m_menu_btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent& event) {
        m_menu_btn->set_selected(true);
        wxPoint pos = m_menu_btn->GetPosition();
        wxGetApp().plater()->PopupMenu(&m_main_menu, pos);
    });
    m_main_menu.Bind(wxEVT_MENU_CLOSE, [this](wxMenuEvent&) { m_menu_btn->set_selected(false); });
#endif

    m_buttons_sizer = new wxFlexGridSizer(1, m_btn_margin, m_btn_margin);
    left_sizer->Add(m_buttons_sizer, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 2 * m_btn_margin);

    CreateSearch();

    left_sizer->Add(m_search, 1, wxALIGN_CENTER_VERTICAL | wxLEFT, 3 * m_btn_margin);

    m_sizer->Add(left_sizer, 1, wxEXPAND);

    wxBoxSizer* right_sizer = new wxBoxSizer(wxHORIZONTAL);

    // create modes menu
    ApplyWorkspacesMenu();

    m_workspace_btn = new ButtonWithPopup(this, _L("Workspace"), "mode_simple");
    right_sizer->AddStretchSpacer(20);
    right_sizer->Add(m_workspace_btn, 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
    
    m_workspace_btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent& event) {
        m_workspace_btn->set_selected(true);
        wxPoint pos = m_workspace_btn->GetPosition();
        wxGetApp().plater()->PopupMenu(&m_workspaces_menu, pos);
    });
    m_workspaces_menu.Bind(wxEVT_MENU_CLOSE, [this](wxMenuEvent&) { m_workspace_btn->set_selected(false); });

    // create Auth menu
    CreateAuthMenu();

    m_auth_btn = new ButtonWithPopup(this, "user", 35);
    right_sizer->Add(m_auth_btn, 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT | wxRIGHT | wxLEFT, m_btn_margin);
    
    m_auth_btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent& event) {
        UpdateAuthMenu();
        m_auth_btn->set_selected(true);
        wxPoint pos = m_auth_btn->GetPosition();
        wxGetApp().plater()->PopupMenu(&m_auth_menu, pos);
    });    
    m_auth_menu.Bind(wxEVT_MENU_CLOSE, [this](wxMenuEvent&) { m_auth_btn->set_selected(false); });

    m_sizer->Add(right_sizer);

    m_sizer->SetItemMinSize(1, wxSize(42 * wxGetApp().em_unit(), -1));

    this->Bind(wxEVT_PAINT, &TopBarItemsCtrl::OnPaint, this);
}

void TopBarItemsCtrl::OnPaint(wxPaintEvent&)
{
    wxGetApp().UpdateDarkUI(this);
    m_search->Refresh();
    return;
    const wxSize sz = GetSize();
    wxPaintDC dc(this);

    if (m_selection < 0 || m_selection >= (int)m_pageButtons.size())
        return;

    const wxColour& btn_marker_color = wxGetApp().get_highlight_default_clr();

    // Draw orange bottom line

    dc.SetPen(btn_marker_color);
    dc.SetBrush(btn_marker_color);
    dc.DrawRectangle(1, sz.y - m_line_margin, sz.x, m_line_margin);
}

void TopBarItemsCtrl::UpdateMode()
{
    auto mode = wxGetApp().get_mode();

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

    m_sizer->Layout();
}

void TopBarItemsCtrl::UpdateModeMarkers()
{
    UpdateMode();
    ApplyWorkspacesMenu();
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
    append_submenu(&m_main_menu, menu, wxID_ANY, title, "cog");
}

void TopBarItemsCtrl::AppendMenuSeparaorItem()
{
    m_main_menu.AppendSeparator();
}
