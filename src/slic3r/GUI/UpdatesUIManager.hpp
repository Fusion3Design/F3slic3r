///|/ Copyright (c) Prusa Research 2018 - 2020 Oleksandra Iushchenko @YuSanka
///|/
#ifndef slic3r_GUI_UpdatesUIManager_hpp_
#define slic3r_GUI_UpdatesUIManager_hpp_

#include "GUI_Utils.hpp"

class wxWindow;
class wxEvent;
class wxSizer;
class wxFlexGridSizer;

namespace Slic3r { 
namespace GUI {

class PresetArchiveDatabase;

class UIManager
{
    struct OnlineEntry {
        OnlineEntry(bool use, const std::string &id, const std::string &name, const std::string &description, const std::string &visibility) :
            use(use), id(id), name(name), description(description), visibility(visibility) {}

        bool            use;
        std::string     id;
        std::string     name;
        std::string     description;
        std::string   	visibility;
    };

    struct OfflineEntry {
        OfflineEntry(bool use, const std::string &id, const std::string &name, const std::string &description, const std::string &source, bool is_ok) :
            use(use), id(id), name(name), description(description), source(source), is_ok(is_ok) {}

        bool            use;
        std::string     id;
        std::string     name;
        std::string     description;
        std::string   	source;
        bool            is_ok;
    };

    PresetArchiveDatabase*      m_pad           { nullptr };
    wxWindow*                   m_parent        { nullptr };
    wxSizer*                    m_main_sizer    { nullptr };

    wxFlexGridSizer*            m_online_sizer  { nullptr };
    wxFlexGridSizer*            m_offline_sizer { nullptr };

    std::vector<OnlineEntry>    m_online_entries;
    std::vector<OfflineEntry>   m_offline_entries;

    std::set<std::string>       m_online_selections;
    std::set<std::string>       m_offline_selections;

    void fill_entries();
    void fill_grids();

    void update();

    void remove_offline_repos(const std::string& id);
    void load_offline_repos();

public:
    UIManager() {}
    UIManager(wxWindow* parent, PresetArchiveDatabase* pad, int em);
    ~UIManager() {}

    wxSizer*    get_sizer() { return m_main_sizer; }
    void        set_used_archives();
};

class ManageUpdatesDialog : public DPIDialog
{
public:
    ManageUpdatesDialog(PresetArchiveDatabase* pad);
    ~ManageUpdatesDialog() {}

protected:
    void on_dpi_changed(const wxRect &suggested_rect) override;
    
private:

    std::unique_ptr<UIManager> m_manager    { nullptr };

    void onCloseDialog(wxEvent &);
    void onOkDialog(wxEvent &);
};

} // namespace GUI
} // namespace Slic3r

#endif
