#include "TopBar.hpp"
#include "TopBarMenus.hpp"

#include "GUI_App.hpp"
#include "Plater.hpp"
#include "Search.hpp"
#include "UserAccount.hpp"
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


TopBarItemsCtrl::Button::Button(wxWindow* parent, const wxString& label, const std::string& icon_name, const int px_cnt, wxSize size_def)
:ScalableButton(parent, wxID_ANY, icon_name, label, size_def, wxDefaultPosition, wxNO_BORDER, px_cnt)
#ifdef _WIN32
,m_background_color(wxGetApp().get_window_default_clr())
#else
,m_background_color(wxTransparentColor)
#endif
,m_foreground_color(wxGetApp().get_label_clr_default())
,m_bmp_bundle(icon_name.empty() ? wxBitmapBundle() : *get_bmp_bundle(icon_name, px_cnt))
{
    int btn_margin = em_unit(this);
    int x, y;
    GetTextExtent(label, &x, &y);
    wxSize size(x + 4 * btn_margin, y + int(1.5 * btn_margin));
    if (icon_name.empty())
        this->SetMinSize(size);
#ifdef _WIN32
    else if (label.IsEmpty()) {
        const int btn_side = px_cnt + btn_margin;
        this->SetMinSize(wxSize(btn_side, btn_side));
    }
    else
        this->SetMinSize(wxSize(-1, size.y));
#else
    else if (label.IsEmpty())
        this->SetMinSize(wxSize(px_cnt, px_cnt));
    else 
        this->SetMinSize(wxSize(size.x + px_cnt, size.y));
#endif

    //button events
    Bind(wxEVT_SET_FOCUS,    [this](wxFocusEvent& event) { set_hovered(true ); event.Skip(); });
    Bind(wxEVT_KILL_FOCUS,   [this](wxFocusEvent& event) { set_hovered(false); event.Skip(); });
    Bind(wxEVT_ENTER_WINDOW, [this](wxMouseEvent& event) { set_hovered(true ); event.Skip(); });
    Bind(wxEVT_LEAVE_WINDOW, [this](wxMouseEvent& event) { set_hovered(false); event.Skip(); });

    Bind(wxEVT_PAINT,        [this](wxPaintEvent&) { render(); });
}

void TopBarItemsCtrl::Button::set_selected(bool selected)
{
    m_is_selected = selected;

    m_foreground_color = m_is_selected ? wxGetApp().get_window_default_clr(): wxGetApp().get_label_clr_default() ;
    m_background_color = m_is_selected ? wxGetApp().get_label_clr_default() : 
#ifdef _WIN32
                                         wxGetApp().get_window_default_clr();
#else
                                         wxTransparentColor;
#endif

#ifdef __linux__
    this->SetBackgroundColour(m_background_color);
    this->SetForegroundColour(m_foreground_color);

    this->Refresh();
    this->Update();
#endif // __linux__
}

void TopBarItemsCtrl::Button::set_hovered(bool hovered)
{
    using namespace Slic3r::GUI;

#ifdef _WIN32
    this->GetParent()->Refresh(); // force redraw a background of the selected mode button
#endif /* no _WIN32 */

    m_background_color =    m_is_selected ? wxGetApp().get_label_clr_default()     :
                            hovered       ? wxGetApp().get_color_selected_btn_bg() :
                            
#ifdef _WIN32
                                            wxGetApp().get_window_default_clr();
#else
                                            wxTransparentColor;
#endif

#ifdef __linux__
    this->SetBackgroundColour(m_background_color);
#endif // __linux__

    this->Refresh();
    this->Update();
}

void TopBarItemsCtrl::Button::render()
{
    const wxRect rc(GetSize());
    wxPaintDC dc(this);

#ifdef _WIN32
    // Draw default background

    dc.SetPen(wxGetApp().get_window_default_clr());
    dc.SetBrush(wxGetApp().get_window_default_clr());
    dc.DrawRectangle(rc);
#endif

    int em = em_unit(this);

    // Draw def rect with rounded corners

    dc.SetPen(m_background_color);
    dc.SetBrush(m_background_color);
    dc.DrawRoundedRectangle(rc, int(0.4* em));

    wxPoint pt = { 0, 0 };

    if (m_bmp_bundle.IsOk()) {
        wxSize szIcon = get_preferred_size(m_bmp_bundle, this);
        pt.x = em;
        pt.y = (rc.height - szIcon.y) / 2;
#ifdef __WXGTK3__
        dc.DrawBitmap(m_bmp_bundle.GetBitmap(szIcon), pt);
#else
        dc.DrawBitmap(m_bmp_bundle.GetBitmapFor(this), pt);
#endif
        pt.x += szIcon.x;
    }

    // Draw text

    wxString text = GetLabelText();
    if (!text.IsEmpty()) {
        wxSize labelSize = dc.GetTextExtent(text);
        if (labelSize.x > rc.width)
            text = wxControl::Ellipsize(text, dc, wxELLIPSIZE_END, rc.width);
        pt.x += (rc.width - pt.x - labelSize.x) / 2;
        pt.y = (rc.height - labelSize.y) / 2;

        dc.SetTextForeground(m_foreground_color);
        dc.SetFont(GetFont());
        dc.DrawText(text, pt);
    }
}

void TopBarItemsCtrl::Button::sys_color_changed()
{
    ScalableButton::sys_color_changed();
#ifdef _WIN32
    m_background_color = wxGetApp().get_window_default_clr();
#endif
    m_foreground_color = wxGetApp().get_label_clr_default();
}

TopBarItemsCtrl::ButtonWithPopup::ButtonWithPopup(wxWindow* parent, const wxString& label, const std::string& icon_name, wxSize size)
    :TopBarItemsCtrl::Button(parent, label, icon_name, 24, size)
{
    if (size != wxDefaultSize)
        m_fixed_width = size.x;
    this->SetLabel(label);
}

TopBarItemsCtrl::ButtonWithPopup::ButtonWithPopup(wxWindow* parent, const std::string& icon_name, int icon_width/* = 20*/, int icon_height/* = 20*/)
    :TopBarItemsCtrl::Button(parent, "", icon_name, icon_width)
{
}

void TopBarItemsCtrl::ButtonWithPopup::SetLabel(const wxString& label)
{
    wxString text = label;
    int btn_height = GetMinSize().GetHeight();

    bool resize_and_layout{ false };
    if (m_fixed_width != wxDefaultCoord) {
        const int text_width    = m_fixed_width - 2 * btn_height;
        const int label_width   = GetTextExtent(text).GetWidth();
        if (label_width > text_width) {
            wxWindowDC wdc(this);
            text = wxControl::Ellipsize(text, wdc, wxELLIPSIZE_END, text_width);
            resize_and_layout = true;
        }
    }

    wxString full_label = "  " + text + "  ";
#ifndef __linux__
    full_label += down_arrow;
#endif
    ScalableButton::SetLabel(full_label);
    if (resize_and_layout) {
        SetMinSize(wxSize(m_fixed_width, btn_height));
        GetParent()->Layout();
    }
}

void TopBarItemsCtrl::UpdateAccountButton(bool avatar/* = false*/)
{
    auto user_account = wxGetApp().plater()->get_user_account();
    const wxString user_name = user_account->is_logged() ? from_u8(user_account->get_username()) : _L("Anonymous");   
    m_account_btn->SetLabel(user_name);
#ifdef __linux__
    if (avatar) {
        if (user_account->is_logged()) {
            boost::filesystem::path path = user_account->get_avatar_path(true);
            ScalableBitmap new_logo(this, path, m_account_btn->GetBitmapSize());
            if (new_logo.IsOk())
                m_account_btn->SetBitmap_(new_logo);
            else
                m_account_btn->SetBitmap_("user");
        }
        else {
            m_account_btn->SetBitmap_("user");
        }
    }
#else
    if (avatar) {
        if (user_account->is_logged()) {
            boost::filesystem::path path = user_account->get_avatar_path(true);
            ScalableBitmap new_logo(this, path, m_account_btn->GetBitmapSize());
            if (new_logo.IsOk())
                m_account_btn->SetBitmapBundle(new_logo.bmp());
            else
                m_account_btn->SetBitmapBundle(*get_bmp_bundle("user", 24));
        }
        else {
            m_account_btn->SetBitmapBundle(*get_bmp_bundle("user", 24));
        }
    }
#endif
    m_account_btn->Refresh();
}

void TopBarItemsCtrl::UnselectPopupButtons()
{
    if (m_menu_btn)
        m_menu_btn  ->set_selected(false);
    m_workspace_btn ->set_selected(false);
    m_account_btn   ->set_selected(false);
}

void TopBarItemsCtrl::CreateSearch()
{
    // Linux specific: If wxDefaultSize is used in constructor and than set just maxSize, 
    // than this max size will be used as a default control size and can't be resized.
    // So, set initial size for some minimum value
    m_search = new ::TextInput(this, wxGetApp().searcher().default_string, "", "search", wxDefaultPosition, wxSize(2 * em_unit(this), -1), wxTE_PROCESS_ENTER);
    m_search->SetMaxSize(wxSize(42*em_unit(this), -1));
    wxGetApp().UpdateDarkUI(m_search);
    wxGetApp().searcher().set_search_input(m_search);
}

void TopBarItemsCtrl::update_margins()
{
    int em = em_unit(this);
    m_btn_margin  = std::lround(0.9 * em);
    m_line_margin = std::lround(0.1 * em);
}

wxPoint TopBarItemsCtrl::ButtonWithPopup::get_popup_pos()
{
    wxPoint pos = this->GetPosition();
    pos.y += this->GetSize().GetHeight() + int(0.2 * wxGetApp().em_unit());
    return pos;
}

TopBarItemsCtrl::TopBarItemsCtrl(wxWindow *parent, TopBarMenus* menus/* = nullptr*/) :
    wxControl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE | wxTAB_TRAVERSAL)
    ,m_menus(menus)
{
#ifdef __WINDOWS__
    SetDoubleBuffered(true);
#endif //__WINDOWS__
    update_margins();

    m_sizer = new wxFlexGridSizer(2);
    m_sizer->AddGrowableCol(0);
    m_sizer->SetFlexibleDirection(wxHORIZONTAL);
    this->SetSizer(m_sizer);

    wxBoxSizer* left_sizer = new wxBoxSizer(wxHORIZONTAL);

#ifdef __APPLE__
    auto logo = new wxStaticBitmap(this, wxID_ANY, *get_bmp_bundle(wxGetApp().logo_name(), 40));
    left_sizer->Add(logo, 0, wxALIGN_CENTER_VERTICAL | wxALL, m_btn_margin);
#else
    m_menu_btn = new ButtonWithPopup(this, _L("Menu"), wxGetApp().logo_name());
    left_sizer->Add(m_menu_btn, 0, wxALIGN_CENTER_VERTICAL | wxALL, m_btn_margin);
    
    m_menu_btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent& event) {
        m_menu_btn->set_selected(true);
        m_menus->Popup(this, &m_menus->main, m_menu_btn->get_popup_pos());
    });
#endif

    m_buttons_sizer = new wxFlexGridSizer(1, m_btn_margin, m_btn_margin);
    left_sizer->Add(m_buttons_sizer, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, m_btn_margin);

    CreateSearch();

    wxBoxSizer* search_sizer = new wxBoxSizer(wxVERTICAL);
    search_sizer->Add(m_search, 1, wxEXPAND | wxALIGN_RIGHT);
    left_sizer->Add(search_sizer, 1, wxALIGN_CENTER_VERTICAL | wxLEFT, m_btn_margin);

    m_sizer->Add(left_sizer, 1, wxEXPAND);

    wxBoxSizer* right_sizer = new wxBoxSizer(wxHORIZONTAL);

    m_workspace_btn = new ButtonWithPopup(this, _L("Workspace"), "mode_simple");
    right_sizer->AddStretchSpacer(20);
    right_sizer->Add(m_workspace_btn, 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT | wxALL, m_btn_margin);
    
    m_workspace_btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent& event) {
        m_workspace_btn->set_selected(true);
        m_menus->Popup(this, &m_menus->workspaces, m_workspace_btn->get_popup_pos());
    });

    m_account_btn = new ButtonWithPopup(this, _L("Anonymous"), "user", wxSize(18 * em_unit(this), -1));
    right_sizer->Add(m_account_btn, 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT | wxRIGHT | wxLEFT, m_btn_margin);
    
    m_account_btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent& event) {
        m_account_btn->set_selected(true);
        m_menus->Popup(this, &m_menus->account, m_account_btn->get_popup_pos());
    });

    m_sizer->Add(right_sizer, 0, wxALIGN_CENTER_VERTICAL);

    m_sizer->SetItemMinSize(1, wxSize(42 * wxGetApp().em_unit(), -1));

    this->Bind(wxEVT_PAINT, &TopBarItemsCtrl::OnPaint, this);

    this->Bind(wxEVT_UPDATE_UI, [](wxUpdateUIEvent& evt) {
        auto user_account = wxGetApp().plater()->get_user_account();
        evt.Enable(user_account ? user_account->is_logged()             : false);
        evt.Check (user_account ? user_account->get_remember_session()  : false);
    }, m_menus->remember_me_item_id);
}

void TopBarItemsCtrl::OnPaint(wxPaintEvent&)
{
    wxGetApp().UpdateDarkUI(this);
    m_search->Refresh();
}

void TopBarItemsCtrl::UpdateMode()
{
    auto mode = wxGetApp().get_mode();

    wxBitmapBundle bmp = *get_bmp_bundle("mode", 16, -1, wxGetApp().get_mode_btn_color(mode));
#ifdef __linux__
    m_workspace_btn->SetBitmap(bmp);
    m_workspace_btn->SetBitmapCurrent(bmp);
    m_workspace_btn->SetBitmapPressed(bmp);
#else
    m_workspace_btn->SetBitmapBundle(bmp);
#endif

    m_workspace_btn->SetLabel(m_menus->get_workspace_name(mode));

    this->Layout();
}

void TopBarItemsCtrl::Rescale()
{
    update_margins();

    int em = em_unit(this);
    m_search->SetMinSize(wxSize(4 * em, -1));
    m_search->SetMaxSize(wxSize(42 * em, -1));
    m_search->Rescale();
    m_sizer->SetItemMinSize(1, wxSize(42 * em, -1));

    m_buttons_sizer->SetVGap(m_btn_margin);
    m_buttons_sizer->SetHGap(m_btn_margin);

    m_sizer->Layout();
}

void TopBarItemsCtrl::OnColorsChanged()
{
    wxGetApp().UpdateDarkUI(this);
    if (m_menu_btn)
        m_menu_btn->sys_color_changed();

    m_workspace_btn->sys_color_changed();
    m_account_btn->sys_color_changed();
    m_search->SysColorsChanged();

    UpdateSelection();
    UpdateMode();

    m_sizer->Layout();
}

void TopBarItemsCtrl::UpdateModeMarkers()
{
    UpdateMode();
    m_menus->ApplyWorkspacesMenu();
}

void TopBarItemsCtrl::UpdateSelection()
{
    for (Button* btn : m_pageButtons)
        btn->set_selected(false);

    if (m_selection >= 0)
        m_pageButtons[m_selection]->set_selected(true);

    Refresh();
}

void TopBarItemsCtrl::SetSelection(int sel, bool force /*= false*/)
{
    if (m_selection == sel && !force)
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

    // Under OSX call of btn->Reparent(nullptr) causes a crash, so as a workaround use RemoveChild() instead
    this->RemoveChild(btn);
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

void TopBarItemsCtrl::ShowFull()
{
    if (m_menu_btn)
        m_menu_btn->Show();
    m_account_btn->Show();
    UpdateAccountButton();
    m_menus->set_cb_on_user_item([this]() {
        m_account_btn->set_selected(true);
        m_menus->Popup(this, &m_menus->account, m_account_btn->get_popup_pos());
    });
}

void TopBarItemsCtrl::ShowJustMode()
{
    if (m_menu_btn)
        m_menu_btn->Hide();
    m_account_btn->Hide();
    m_menus->set_cb_on_user_item(nullptr);
}
