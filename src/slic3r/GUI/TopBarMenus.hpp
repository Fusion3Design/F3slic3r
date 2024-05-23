#ifndef slic3r_TopBarMenus_hpp_
#define slic3r_TopBarMenus_hpp_

#include <wx/menu.h>

class TopBarItemsCtrl;
class wxString;

namespace Slic3r {
namespace GUI {
class UserAccount;
}
}

class TopBarMenus
{
    // Prusa Account menu items
    wxMenuItem*             m_login_item        { nullptr };

    TopBarItemsCtrl*        m_popup_ctrl        { nullptr };

    std::function<void()>   m_cb_on_user_item { nullptr };

public:
    wxMenu          main;
    wxMenu          workspaces;
    wxMenu          account;
    wxWindowID      remember_me_item_id        { wxID_ANY };

    TopBarMenus();
    ~TopBarMenus() = default;

    void AppendMenuItem(wxMenu* menu, const wxString& title);
    void AppendMenuSeparaorItem();
    void ApplyWorkspacesMenu();
    void CreateAccountMenu();
    void UpdateAccountMenu(Slic3r::GUI::UserAccount* user_account = nullptr);

    void Popup(TopBarItemsCtrl* popup_ctrl, wxMenu* menu, wxPoint pos);
    void BindEvtClose();

    wxString    get_workspace_name(const /*ConfigOptionMode*/int mode);
    void        set_cb_on_user_item (std::function<void()> cb) { m_cb_on_user_item = cb; }
    void        sys_color_changed();

};

#endif // slic3r_TopBarMenus_hpp_
