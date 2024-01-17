#include "WebViewDialog.hpp"

#include "slic3r/GUI/I18N.hpp"
#include "slic3r/GUI/wxExtensions.hpp"
#include "slic3r/GUI/GUI_App.hpp"
#include "slic3r/GUI/GUI.hpp"
#include "slic3r/GUI/MainFrame.hpp"
#include "slic3r/GUI/Plater.hpp"
#include "libslic3r_version.h"
#include "libslic3r/Utils.hpp"
#include "slic3r/GUI/UserAccount.hpp"
#include "slic3r/GUI/format.hpp"

#include <wx/sizer.h>
#include <wx/toolbar.h>
#include <wx/textdlg.h>

#include <boost/log/trivial.hpp>

#include "slic3r/GUI/WebView.hpp"


namespace Slic3r {
namespace GUI {


WebViewPanel::WebViewPanel(wxWindow *parent, const wxString& default_url)
        : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize)
        , m_default_url (default_url)
 {
    //wxString url = "https://dev.connect.prusa3d.com/prusa-slicer/printers";
    //std::string strlang = wxGetApp().app_config->get("language");
    //if (strlang != "")
    //    url = wxString::Format("file://%s/web/homepage/index.html?lang=%s", from_u8(resources_dir()), strlang);
    //m_bbl_user_agent = wxString::Format("BBL-Slicer/v%s", SLIC3R_VERSION);

    wxBoxSizer* topsizer = new wxBoxSizer(wxVERTICAL);
#ifdef DEBUG_URL_PANEL
    // Create the button
    bSizer_toolbar = new wxBoxSizer(wxHORIZONTAL);

    m_button_back = new wxButton(this, wxID_ANY, wxT("Back"), wxDefaultPosition, wxDefaultSize, 0);
    m_button_back->Enable(false);
    bSizer_toolbar->Add(m_button_back, 0, wxALL, 5);

    m_button_forward = new wxButton(this, wxID_ANY, wxT("Forward"), wxDefaultPosition, wxDefaultSize, 0);
    m_button_forward->Enable(false);
    bSizer_toolbar->Add(m_button_forward, 0, wxALL, 5);

    m_button_stop = new wxButton(this, wxID_ANY, wxT("Stop"), wxDefaultPosition, wxDefaultSize, 0);

    bSizer_toolbar->Add(m_button_stop, 0, wxALL, 5);

    m_button_reload = new wxButton(this, wxID_ANY, wxT("Reload"), wxDefaultPosition, wxDefaultSize, 0);
    bSizer_toolbar->Add(m_button_reload, 0, wxALL, 5);

    m_url = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
    bSizer_toolbar->Add(m_url, 1, wxALL | wxEXPAND, 5);

    m_button_tools = new wxButton(this, wxID_ANY, wxT("Tools"), wxDefaultPosition, wxDefaultSize, 0);
    bSizer_toolbar->Add(m_button_tools, 0, wxALL, 5);

    // Create panel for find toolbar.
    wxPanel* panel = new wxPanel(this);
    topsizer->Add(bSizer_toolbar, 0, wxEXPAND, 0);
    topsizer->Add(panel, wxSizerFlags().Expand());

    // Create sizer for panel.
    wxBoxSizer* panel_sizer = new wxBoxSizer(wxVERTICAL);
    panel->SetSizer(panel_sizer);

    // Create the info panel
    m_info = new wxInfoBar(this);
    topsizer->Add(m_info, wxSizerFlags().Expand());
#endif

    // Create the webview
    m_browser = WebView::CreateWebView(this, m_default_url);
    if (m_browser == nullptr) {
        wxLogError("Could not init m_browser");
        return;
    }

    SetSizer(topsizer);

    topsizer->Add(m_browser, wxSizerFlags().Expand().Proportion(1));

#ifdef DEBUG_URL_PANEL
    // Create the Tools menu
    m_tools_menu = new wxMenu();
    wxMenuItem* viewSource = m_tools_menu->Append(wxID_ANY, _L("View Source"));
    wxMenuItem* viewText = m_tools_menu->Append(wxID_ANY, _L("View Text"));
    m_tools_menu->AppendSeparator();
  
    wxMenu* script_menu = new wxMenu;
   
    m_script_custom = script_menu->Append(wxID_ANY, "Custom script");
    m_tools_menu->AppendSubMenu(script_menu, _L("Run Script"));
    wxMenuItem* addUserScript = m_tools_menu->Append(wxID_ANY, _L("Add user script"));
    wxMenuItem* setCustomUserAgent = m_tools_menu->Append(wxID_ANY, _L("Set custom user agent"));

#endif
    //Zoom
    m_zoomFactor = 100;

    
    Bind(wxEVT_SHOW, &WebViewPanel::on_show, this);

    // Connect the webview events
    Bind(wxEVT_WEBVIEW_ERROR, &WebViewPanel::on_error, this, m_browser->GetId());
    Bind(wxEVT_WEBVIEW_SCRIPT_MESSAGE_RECEIVED, &WebViewPanel::on_script_message, this, m_browser->GetId());

#ifdef DEBUG_URL_PANEL
    // Connect the button events
    Bind(wxEVT_BUTTON, &WebViewPanel::on_back, this, m_button_back->GetId());
    Bind(wxEVT_BUTTON, &WebViewPanel::on_forward, this, m_button_forward->GetId());
    Bind(wxEVT_BUTTON, &WebViewPanel::on_stop, this, m_button_stop->GetId());
    Bind(wxEVT_BUTTON, &WebViewPanel::on_reload, this, m_button_reload->GetId());
    Bind(wxEVT_BUTTON, &WebViewPanel::on_tools_clicked, this, m_button_tools->GetId());
    Bind(wxEVT_TEXT_ENTER, &WebViewPanel::on_url, this, m_url->GetId());

    // Connect the menu events
    Bind(wxEVT_MENU, &WebViewPanel::on_view_source_request, this, viewSource->GetId());
    Bind(wxEVT_MENU, &WebViewPanel::on_view_text_request, this, viewText->GetId());

    Bind(wxEVT_MENU, &WebViewPanel::on_run_script_custom, this, m_script_custom->GetId());
    Bind(wxEVT_MENU, &WebViewPanel::on_add_user_script, this, addUserScript->GetId());
#endif
    //Connect the idle events
    Bind(wxEVT_IDLE, &WebViewPanel::on_idle, this);
    Bind(wxEVT_CLOSE_WINDOW, &WebViewPanel::on_close, this);

    m_LoginUpdateTimer = nullptr;
 }

WebViewPanel::~WebViewPanel()
{
    BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << " Start";
    SetEvtHandlerEnabled(false);
#ifdef DEBUG_URL_PANEL
    delete m_tools_menu;

    if (m_LoginUpdateTimer != nullptr) {
        m_LoginUpdateTimer->Stop();
        delete m_LoginUpdateTimer;
        m_LoginUpdateTimer = NULL;
    }
#endif
    BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << " End";
}


void WebViewPanel::load_url(wxString& url)
{
    this->Show();
    this->Raise();
#ifdef DEBUG_URL_PANEL
    m_url->SetLabelText(url);
#endif
    m_browser->LoadURL(url);
    m_browser->SetFocus();
}

void WebViewPanel::load_default_url()
{
    assert(!m_default_url.empty());
    load_url(m_default_url);
}

void WebViewPanel::on_show(wxShowEvent& evt)
{

}

void WebViewPanel::on_idle(wxIdleEvent& WXUNUSED(evt))
{
    if (m_browser->IsBusy())
        wxSetCursor(wxCURSOR_ARROWWAIT);
    else
        wxSetCursor(wxNullCursor);

#ifdef DEBUG_URL_PANEL
    m_button_stop->Enable(m_browser->IsBusy());
#endif
}

/**
    * Callback invoked when user entered an URL and pressed enter
    */
void WebViewPanel::on_url(wxCommandEvent& WXUNUSED(evt))
{
#ifdef DEBUG_URL_PANEL
    m_browser->LoadURL(m_url->GetValue());
    m_browser->SetFocus();
#endif
}

/**
    * Callback invoked when user pressed the "back" button
    */
void WebViewPanel::on_back(wxCommandEvent& WXUNUSED(evt))
{
    m_browser->GoBack();
}

/**
    * Callback invoked when user pressed the "forward" button
    */
void WebViewPanel::on_forward(wxCommandEvent& WXUNUSED(evt))
{
    m_browser->GoForward();
}

/**
    * Callback invoked when user pressed the "stop" button
    */
void WebViewPanel::on_stop(wxCommandEvent& WXUNUSED(evt))
{
    m_browser->Stop();
}

/**
    * Callback invoked when user pressed the "reload" button
    */
void WebViewPanel::on_reload(wxCommandEvent& WXUNUSED(evt))
{
    m_browser->Reload();
}



void WebViewPanel::on_close(wxCloseEvent& evt)
{
    this->Hide();
}


void WebViewPanel::on_script_message(wxWebViewEvent& evt)
{
}



/**
    * Invoked when user selects the "View Source" menu item
    */
void WebViewPanel::on_view_source_request(wxCommandEvent& WXUNUSED(evt))
{
    SourceViewDialog dlg(this, m_browser->GetPageSource());
    dlg.ShowModal();
}

/**
    * Invoked when user selects the "View Text" menu item
    */
void WebViewPanel::on_view_text_request(wxCommandEvent& WXUNUSED(evt))
{
    wxDialog textViewDialog(this, wxID_ANY, "Page Text",
        wxDefaultPosition, wxSize(700, 500),
        wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);

    wxTextCtrl* text = new wxTextCtrl(this, wxID_ANY, m_browser->GetPageText(),
        wxDefaultPosition, wxDefaultSize,
        wxTE_MULTILINE |
        wxTE_RICH |
        wxTE_READONLY);

    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(text, 1, wxEXPAND);
    SetSizer(sizer);
    textViewDialog.ShowModal();
}

/**
    * Invoked when user selects the "Menu" item
    */
void WebViewPanel::on_tools_clicked(wxCommandEvent& WXUNUSED(evt))
{
#ifdef DEBUG_URL_PANEL
    wxPoint position = ScreenToClient(wxGetMousePosition());
    PopupMenu(m_tools_menu, position.x, position.y);
#endif
}

void WebViewPanel::run_script(const wxString& javascript)
{
    // Remember the script we run in any case, so the next time the user opens
    // the "Run Script" dialog box, it is shown there for convenient updating.
    m_javascript = javascript;

    if (!m_browser) return;

    bool res = WebView::run_script(m_browser, javascript);
    BOOST_LOG_TRIVIAL(debug) << "RunScript " << javascript << " " << res;
}


void WebViewPanel::on_run_script_custom(wxCommandEvent& WXUNUSED(evt))
{
    wxTextEntryDialog dialog
    (
        this,
        "Please enter JavaScript code to execute",
        wxGetTextFromUserPromptStr,
        m_javascript,
        wxOK | wxCANCEL | wxCENTRE | wxTE_MULTILINE
    );
    if (dialog.ShowModal() != wxID_OK)
        return;

    run_script(dialog.GetValue());
}

void WebViewPanel::on_add_user_script(wxCommandEvent& WXUNUSED(evt))
{
    wxString userScript = "window.wx_test_var = 'wxWidgets webview sample';";
    wxTextEntryDialog dialog
    (
        this,
        "Enter the JavaScript code to run as the initialization script that runs before any script in the HTML document.",
        wxGetTextFromUserPromptStr,
        userScript,
        wxOK | wxCANCEL | wxCENTRE | wxTE_MULTILINE
    );
    if (dialog.ShowModal() != wxID_OK)
        return;

    if (!m_browser->AddUserScript(dialog.GetValue()))
        wxLogError("Could not add user script");
}

void WebViewPanel::on_set_custom_user_agent(wxCommandEvent& WXUNUSED(evt))
{
    wxString customUserAgent = "Mozilla/5.0 (iPhone; CPU iPhone OS 13_1_3 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/13.0.1 Mobile/15E148 Safari/604.1";
    wxTextEntryDialog dialog
    (
        this,
        "Enter the custom user agent string you would like to use.",
        wxGetTextFromUserPromptStr,
        customUserAgent,
        wxOK | wxCANCEL | wxCENTRE
    );
    if (dialog.ShowModal() != wxID_OK)
        return;

    if (!m_browser->SetUserAgent(customUserAgent))
        wxLogError("Could not set custom user agent");
}

void WebViewPanel::on_clear_selection(wxCommandEvent& WXUNUSED(evt))
{
    m_browser->ClearSelection();
}

void WebViewPanel::on_delete_selection(wxCommandEvent& WXUNUSED(evt))
{
    m_browser->DeleteSelection();
}

void WebViewPanel::on_select_all(wxCommandEvent& WXUNUSED(evt))
{
    m_browser->SelectAll();
}

/**
    * Callback invoked when a loading error occurs
    */
void WebViewPanel::on_error(wxWebViewEvent& evt)
{
#define WX_ERROR_CASE(type) \
case type: \
    category = #type; \
    break;

    wxString category;
    switch (evt.GetInt())
    {
        WX_ERROR_CASE(wxWEBVIEW_NAV_ERR_CONNECTION);
        WX_ERROR_CASE(wxWEBVIEW_NAV_ERR_CERTIFICATE);
        WX_ERROR_CASE(wxWEBVIEW_NAV_ERR_AUTH);
        WX_ERROR_CASE(wxWEBVIEW_NAV_ERR_SECURITY);
        WX_ERROR_CASE(wxWEBVIEW_NAV_ERR_NOT_FOUND);
        WX_ERROR_CASE(wxWEBVIEW_NAV_ERR_REQUEST);
        WX_ERROR_CASE(wxWEBVIEW_NAV_ERR_USER_CANCELLED);
        WX_ERROR_CASE(wxWEBVIEW_NAV_ERR_OTHER);
    }

    //Show the info bar with an error
#ifdef DEBUG_URL_PANEL

    m_info->ShowMessage(_L("An error occurred loading ") + evt.GetURL() + "\n" +
        "'" + category + "'", wxICON_ERROR);
#endif
}


SourceViewDialog::SourceViewDialog(wxWindow* parent, wxString source) :
                  wxDialog(parent, wxID_ANY, "Source Code",
                           wxDefaultPosition, wxSize(700,500),
                           wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
    wxTextCtrl* text = new wxTextCtrl(this, wxID_ANY, source,
                                      wxDefaultPosition, wxDefaultSize,
                                      wxTE_MULTILINE |
                                      wxTE_RICH |
                                      wxTE_READONLY);

    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(text, 1, wxEXPAND);
    SetSizer(sizer);
}

ConnectWebViewPanel::ConnectWebViewPanel(wxWindow* parent)
    : WebViewPanel(parent, L"https://dev.connect.prusa3d.com/prusa-slicer/printers")
{
}
void ConnectWebViewPanel::on_show(wxShowEvent& evt)
{
    // run script with access token to login
    if (evt.IsShown()) {
        std::string token = wxGetApp().plater()->get_user_account()->get_access_token();
        wxString script = GUI::format_wxstr("window.setAccessToken(\'%1%\')", token);
        // TODO: should this be happening every OnShow?
        run_script(script);
    }
}

void ConnectWebViewPanel::on_script_message(wxWebViewEvent& evt)
{
    wxGetApp().handle_web_request(evt.GetString().ToUTF8().data());
}


WebViewDialog::WebViewDialog(wxWindow* parent, const wxString& url)
    : wxDialog(parent, wxID_ANY, "Webview Dialog", wxDefaultPosition, wxSize(1366, 768)/* wxSize(100 * wxGetApp().em_unit(), 100 * wxGetApp().em_unit())*/)
{
    ////std::string strlang = wxGetApp().app_config->get("language");
    ////if (strlang != "")
    ////    url = wxString::Format("file://%s/web/homepage/index.html?lang=%s", from_u8(resources_dir()), strlang);
    ////m_bbl_user_agent = wxString::Format("BBL-Slicer/v%s", SLIC3R_VERSION);

    wxBoxSizer* topsizer = new wxBoxSizer(wxVERTICAL);

    // Create the webview
    m_browser = WebView::CreateWebView(this, url);
    if (m_browser == nullptr) {
        wxLogError("Could not init m_browser");
        return;
    }

    SetSizer(topsizer);

    topsizer->Add(m_browser, wxSizerFlags().Expand().Proportion(1));

    Bind(wxEVT_SHOW, &WebViewDialog::on_show, this);
    Bind(wxEVT_WEBVIEW_SCRIPT_MESSAGE_RECEIVED, &WebViewDialog::on_script_message, this, m_browser->GetId());
}
WebViewDialog::~WebViewDialog()
{
}

void WebViewDialog::run_script(const wxString& javascript)
{
    if (!m_browser) 
        return;
    //bool res = WebView::run_script(m_browser, javascript);
}



PrinterPickWebViewDialog::PrinterPickWebViewDialog(wxWindow* parent, std::string& ret_val)
    : WebViewDialog(parent, L"https://dev.connect.prusa3d.com/prusa-slicer/printers")
    , m_ret_val(ret_val)
{
}
void PrinterPickWebViewDialog::on_show(wxShowEvent& evt)
{
    if (evt.IsShown()) {
        std::string token = wxGetApp().plater()->get_user_account()->get_access_token();
        wxString script = GUI::format_wxstr("window.setAccessToken(\'%1%\')", token);
        // TODO: should this be happening every OnShow?
        run_script(script);
    }
}
void PrinterPickWebViewDialog::on_script_message(wxWebViewEvent& evt)
{
    m_ret_val = evt.GetString().ToUTF8().data();
    this->EndModal(wxID_OK);
}

} // GUI
} // Slic3r
