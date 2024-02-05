#ifndef slic3r_WebViewDialog_hpp_
#define slic3r_WebViewDialog_hpp_


#include "wx/artprov.h"
#include "wx/cmdline.h"
#include "wx/notifmsg.h"
#include "wx/settings.h"
#include "wx/webview.h"

#if wxUSE_WEBVIEW_EDGE
#include "wx/msw/webview_edge.h"
#endif

#include "wx/webviewarchivehandler.h"
#include "wx/webviewfshandler.h"
#include "wx/numdlg.h"
#include "wx/infobar.h"
#include "wx/filesys.h"
#include "wx/fs_arc.h"
#include "wx/fs_mem.h"
#include "wx/stdpaths.h"
#include <wx/panel.h>
#include <wx/tbarbase.h>
#include "wx/textctrl.h"
#include <wx/timer.h>


namespace Slic3r {
namespace GUI {

//#define DEBUG_URL_PANEL 

class WebViewPanel : public wxPanel
{
public:
    WebViewPanel(wxWindow *parent, const wxString& default_url);
    virtual ~WebViewPanel();

    void load_url(const wxString& url);
    void load_default_url_delayed();
    void load_error_page();

    void on_show(wxShowEvent& evt);
    virtual void on_script_message(wxWebViewEvent& evt);
    void on_loaded(wxWebViewEvent& evt);

    void on_idle(wxIdleEvent& evt);
    void on_url(wxCommandEvent& evt);
    void on_back_button(wxCommandEvent& evt);
    void on_forward_button(wxCommandEvent& evt);
    void on_stop_button(wxCommandEvent& evt);
    void on_reload_button(wxCommandEvent& evt);
    
    void on_view_source_request(wxCommandEvent& evt);
    void on_view_text_request(wxCommandEvent& evt);
    void on_tools_clicked(wxCommandEvent& evt);
    void on_error(wxWebViewEvent& evt);
   
    void run_script(const wxString& javascript);
    void on_run_script_custom(wxCommandEvent& evt);
    void on_add_user_script(wxCommandEvent& evt);
    void on_set_custom_user_agent(wxCommandEvent& evt);
    void on_clear_selection(wxCommandEvent& evt);
    void on_delete_selection(wxCommandEvent& evt);
    void on_select_all(wxCommandEvent& evt);
    void On_enable_context_menu(wxCommandEvent& evt);
    void On_enable_dev_tools(wxCommandEvent& evt);
    void on_close(wxCloseEvent& evt);

    wxTimer * m_LoginUpdateTimer{nullptr};

    wxString get_default_url() const { return m_default_url; }
    void set_default_url(const wxString& url) { m_default_url = url; }

protected:

    wxWebView* m_browser;
    bool m_load_default_url { false };
#ifdef DEBUG_URL_PANEL
    
    wxBoxSizer *bSizer_toolbar;
    wxButton *  m_button_back;
    wxButton *  m_button_forward;
    wxButton *  m_button_stop;
    wxButton *  m_button_reload;
    wxTextCtrl *m_url;
    wxButton *  m_button_tools;

    wxMenu* m_tools_menu;
    wxMenuItem* m_script_custom;
    
    wxInfoBar *m_info;
    wxStaticText* m_info_text;

    wxMenuItem* m_context_menu;
    wxMenuItem* m_dev_tools;
#endif
    long m_zoomFactor;

    // Last executed JavaScript snippet, for convenience.
    wxString m_javascript;
    wxString m_response_js;
    wxString m_default_url;

    //DECLARE_EVENT_TABLE()
};

class ConnectWebViewPanel : public WebViewPanel
{
public:
    ConnectWebViewPanel(wxWindow* parent);
    void on_script_message(wxWebViewEvent& evt) override;

    void connect_set_access_token();
    void connect_set_language();
private:
    std::map<std::string, std::function<void(void)>> m_actions;
};

class PrinterWebViewPanel : public WebViewPanel
{
public:
    PrinterWebViewPanel(wxWindow* parent, const wxString& default_url);
    
    void on_loaded(wxWebViewEvent& evt);

    void send_api_key();
    void send_credentials();
    void set_api_key(const std::string& key) { m_api_key = key; }
    void set_credentials(const std::string& usr, const std::string& psk) { m_usr = usr; m_psk = psk; }
    void clear() { m_api_key.clear(); m_usr.clear(); m_psk.clear(); m_api_key_sent = false; }
private:
    std::string m_api_key;
    std::string m_usr;
    std::string m_psk;
    bool m_api_key_sent {false};
};


class WebViewDialog : public wxDialog
{
public:
    WebViewDialog(wxWindow* parent, const wxString& url);
    virtual ~WebViewDialog();

    virtual void on_show(wxShowEvent& evt) = 0;
    virtual void on_script_message(wxWebViewEvent& evt) = 0;

    void run_script(const wxString& javascript);

protected:
    wxWebView* m_browser;
};

class PrinterPickWebViewDialog : public WebViewDialog
{
public:
    PrinterPickWebViewDialog(wxWindow* parent, std::string& ret_val);
    void on_show(wxShowEvent& evt) override;
    void on_script_message(wxWebViewEvent& evt) override;
private:
    std::string& m_ret_val;
};

class SourceViewDialog : public wxDialog
{
public:
    SourceViewDialog(wxWindow* parent, wxString source);
};

} // GUI
} // Slic3r

#endif /* slic3r_Tab_hpp_ */
