#include "WebViewDialog.hpp"

#include "slic3r/GUI/I18N.hpp"
#include "slic3r/GUI/GUI_App.hpp"
#include "slic3r/GUI/MainFrame.hpp"
#include "slic3r/GUI/Plater.hpp"
#include "slic3r/GUI/UserAccount.hpp"
#include "slic3r/GUI/format.hpp"
#include "slic3r/GUI/WebView.hpp"


#include <wx/webview.h>

#include <boost/log/trivial.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>


namespace pt = boost::property_tree;

namespace Slic3r {
namespace GUI {


WebViewPanel::WebViewPanel(wxWindow *parent, const wxString& default_url, const std::string& loading_html/* = "loading"*/)
        : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize)
        , m_default_url (default_url)
        , m_loading_html(loading_html)
 {
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

    SetSizer(topsizer);

    // Create the webview
    m_browser = WebView::CreateWebView(this, /*m_default_url*/ GUI::format_wxstr("file://%1%/web/%2%.html", boost::filesystem::path(resources_dir()).generic_string(), m_loading_html));
    if (!m_browser) {
        wxStaticText* text = new wxStaticText(this, wxID_ANY, _L("Failed to load a web browser."));
        topsizer->Add(text, 0, wxALIGN_LEFT | wxBOTTOM, 10);
        return;
    }
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

    m_context_menu = m_tools_menu->AppendCheckItem(wxID_ANY, _L("Enable Context Menu"));
    m_dev_tools = m_tools_menu->AppendCheckItem(wxID_ANY, _L("Enable Dev Tools"));

#endif
    //Zoom
    m_zoomFactor = 100;

    Bind(wxEVT_SHOW, &WebViewPanel::on_show, this);

    // Connect the webview events
    Bind(wxEVT_WEBVIEW_ERROR, &WebViewPanel::on_error, this, m_browser->GetId());
    Bind(wxEVT_WEBVIEW_SCRIPT_MESSAGE_RECEIVED, &WebViewPanel::on_script_message, this, m_browser->GetId());

#ifdef DEBUG_URL_PANEL
    // Connect the button events
    Bind(wxEVT_BUTTON, &WebViewPanel::on_back_button, this, m_button_back->GetId());
    Bind(wxEVT_BUTTON, &WebViewPanel::on_forward_button, this, m_button_forward->GetId());
    Bind(wxEVT_BUTTON, &WebViewPanel::on_stop_button, this, m_button_stop->GetId());
    Bind(wxEVT_BUTTON, &WebViewPanel::on_reload_button, this, m_button_reload->GetId());
    Bind(wxEVT_BUTTON, &WebViewPanel::on_tools_clicked, this, m_button_tools->GetId());
    Bind(wxEVT_TEXT_ENTER, &WebViewPanel::on_url, this, m_url->GetId());

    // Connect the menu events
    Bind(wxEVT_MENU, &WebViewPanel::on_view_source_request, this, viewSource->GetId());
    Bind(wxEVT_MENU, &WebViewPanel::on_view_text_request, this, viewText->GetId());
    Bind(wxEVT_MENU, &WebViewPanel::On_enable_context_menu, this, m_context_menu->GetId());
    Bind(wxEVT_MENU, &WebViewPanel::On_enable_dev_tools, this, m_dev_tools->GetId());

    Bind(wxEVT_MENU, &WebViewPanel::on_run_script_custom, this, m_script_custom->GetId());
    Bind(wxEVT_MENU, &WebViewPanel::on_add_user_script, this, addUserScript->GetId());
#endif
    //Connect the idle events
    Bind(wxEVT_IDLE, &WebViewPanel::on_idle, this);
    Bind(wxEVT_CLOSE_WINDOW, &WebViewPanel::on_close, this);

 }

WebViewPanel::~WebViewPanel()
{
    SetEvtHandlerEnabled(false);
#ifdef DEBUG_URL_PANEL
    delete m_tools_menu;
#endif
}


void WebViewPanel::load_url(const wxString& url)
{
    if (!m_browser)
        return;

    this->Show();
    this->Raise();
#ifdef DEBUG_URL_PANEL
    m_url->SetLabelText(url);
#endif
    m_browser->LoadURL(url);
    m_browser->SetFocus();
}

void WebViewPanel::load_default_url_delayed()
{
    assert(!m_default_url.empty());
    m_load_default_url = true;
}

void WebViewPanel::load_error_page()
{
    if (!m_browser)
        return;

    m_browser->Stop();
    m_load_error_page = true;    
}

void WebViewPanel::on_show(wxShowEvent& evt)
{
    m_shown = evt.IsShown();
    if (evt.IsShown() && m_load_default_url) {
        m_load_default_url = false;
        load_url(m_default_url);
    }
}

void WebViewPanel::on_idle(wxIdleEvent& WXUNUSED(evt))
{
    if (!m_browser)
        return;
    if (m_browser->IsBusy()) {
        wxSetCursor(wxCURSOR_ARROWWAIT);
    } else {
        wxSetCursor(wxNullCursor);

        if (m_load_error_page) {
            m_load_error_page = false;
            load_url(GUI::format_wxstr("file://%1%/web/connection_failed.html", boost::filesystem::path(resources_dir()).generic_string()));
        }
    }
#ifdef DEBUG_URL_PANEL
    m_button_stop->Enable(m_browser->IsBusy());
#endif
}

/**
    * Callback invoked when user entered an URL and pressed enter
    */
void WebViewPanel::on_url(wxCommandEvent& WXUNUSED(evt))
{
    if (!m_browser)
        return;
#ifdef DEBUG_URL_PANEL
    m_browser->LoadURL(m_url->GetValue());
    m_browser->SetFocus();
#endif
}

/**
    * Callback invoked when user pressed the "back" button
    */
void WebViewPanel::on_back_button(wxCommandEvent& WXUNUSED(evt))
{
    if (!m_browser)
        return;
    m_browser->GoBack();
}

/**
    * Callback invoked when user pressed the "forward" button
    */
void WebViewPanel::on_forward_button(wxCommandEvent& WXUNUSED(evt))
{
    if (!m_browser)
        return;
    m_browser->GoForward();
}

/**
    * Callback invoked when user pressed the "stop" button
    */
void WebViewPanel::on_stop_button(wxCommandEvent& WXUNUSED(evt))
{
    if (!m_browser)
        return;
    m_browser->Stop();
}

/**
    * Callback invoked when user pressed the "reload" button
    */
void WebViewPanel::on_reload_button(wxCommandEvent& WXUNUSED(evt))
{
    if (!m_browser)
        return;
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
    if (!m_browser)
        return;

    SourceViewDialog dlg(this, m_browser->GetPageSource());
    dlg.ShowModal();
}

/**
    * Invoked when user selects the "View Text" menu item
    */
void WebViewPanel::on_view_text_request(wxCommandEvent& WXUNUSED(evt))
{
    if (!m_browser)
        return;

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
    if (!m_browser)
        return;

#ifdef DEBUG_URL_PANEL
    m_context_menu->Check(m_browser->IsContextMenuEnabled());
    m_dev_tools->Check(m_browser->IsAccessToDevToolsEnabled());

    wxPoint position = ScreenToClient(wxGetMousePosition());
    PopupMenu(m_tools_menu, position.x, position.y);
#endif
}

void WebViewPanel::run_script(const wxString& javascript)
{
    if (!m_browser || !m_shown)
        return;
    // Remember the script we run in any case, so the next time the user opens
    // the "Run Script" dialog box, it is shown there for convenient updating.
    m_javascript = javascript;
    BOOST_LOG_TRIVIAL(debug) << "RunScript " << javascript;
    m_browser->RunScriptAsync(javascript);
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
    if (!m_browser)
        return;

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
    if (!m_browser)
        return;

    m_browser->ClearSelection();
}

void WebViewPanel::on_delete_selection(wxCommandEvent& WXUNUSED(evt))
{
    if (!m_browser)
        return;

    m_browser->DeleteSelection();
}

void WebViewPanel::on_select_all(wxCommandEvent& WXUNUSED(evt))
{
    if (!m_browser)
        return;

    m_browser->SelectAll();
}

void WebViewPanel::On_enable_context_menu(wxCommandEvent& evt)
{
    if (!m_browser)
        return;

    m_browser->EnableContextMenu(evt.IsChecked());
}
void WebViewPanel::On_enable_dev_tools(wxCommandEvent& evt)
{
    if (!m_browser)
        return;

    m_browser->EnableAccessToDevTools(evt.IsChecked());
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

    BOOST_LOG_TRIVIAL(error) << "WebView error: " << category;
    load_error_page();
#ifdef DEBUG_URL_PANEL
    m_info->ShowMessage(_L("An error occurred loading ") + evt.GetURL() + "\n" +
        "'" + category + "'", wxICON_ERROR);
#endif
}

void WebViewPanel::sys_color_changed()
{
#ifdef _WIN32
    wxGetApp().UpdateDarkUI(this);
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

ConnectRequestHandler::ConnectRequestHandler()
{
    m_actions["REQUEST_ACCESS_TOKEN"] = std::bind(&ConnectRequestHandler::on_request_access_token, this);
    m_actions["REQUEST_CONFIG"] = std::bind(&ConnectRequestHandler::on_request_config, this);
    m_actions["UPDATE_SELECTED_PRINTER"] = std::bind(&ConnectRequestHandler::on_request_update_selected_printer_action, this);
    m_actions["WEBAPP_READY"] = std::bind(&ConnectRequestHandler::request_compatible_printers, this);
}
ConnectRequestHandler::~ConnectRequestHandler()
{
}
void ConnectRequestHandler::handle_message(const std::string& message)
{
    // read msg and choose action
    /*
    v0:
    {"type":"request","detail":{"action":"requestAccessToken"}}
    v1:
    {"action":"REQUEST_ACCESS_TOKEN"}
    */
    std::string action_string;
    m_message_data = message;
    try {
        std::stringstream ss(m_message_data);
        pt::ptree ptree;
        pt::read_json(ss, ptree);
        // v1:
        if (const auto action = ptree.get_optional<std::string>("action"); action) {
            action_string = *action;
        }
    }
    catch (const std::exception& e) {
        BOOST_LOG_TRIVIAL(error) << "Could not parse _prusaConnect message. " << e.what();
        return;
    }

    if (action_string.empty()) {
        BOOST_LOG_TRIVIAL(error) << "Recieved invalid message from _prusaConnect (missing action). Message: " << message;
        return;
    }
    assert(m_actions.find(action_string) != m_actions.end()); // this assert means there is a action that has no handling.
    if (m_actions.find(action_string) != m_actions.end()) {
        m_actions[action_string]();
    }
}

void ConnectRequestHandler::resend_config()
{
    on_request_config();
}

void ConnectRequestHandler::on_request_access_token()
{
    std::string token = wxGetApp().plater()->get_user_account()->get_access_token();
    wxString script = GUI::format_wxstr("window._prusaConnect_v1.setAccessToken(\'%1%\')", token);
    run_script_bridge(script);
}

void ConnectRequestHandler::on_request_config()
{
    /*
    accessToken?: string;
    clientVersion?: string;
    colorMode?: "LIGHT" | "DARK";
    language?: ConnectLanguage;
    sessionId?: string;
    */
    
    const std::string token = wxGetApp().plater()->get_user_account()->get_access_token();
    //const std::string sesh = wxGetApp().plater()->get_user_account()->get_shared_session_key();
    const std::string dark_mode = wxGetApp().dark_mode() ? "DARK" : "LIGHT";
    wxString language = GUI::wxGetApp().current_language_code();
    language = language.SubString(0, 1);
    const std::string init_options = GUI::format("{\"accessToken\": \"%1%\" , \"clientVersion\": \"%2%\", \"colorMode\": \"%3%\", \"language\": \"%4%\"}", token, SLIC3R_VERSION, dark_mode, language);
    wxString script = GUI::format_wxstr("window._prusaConnect_v1.init(%1%)", init_options);
    run_script_bridge(script);
    
}

ConnectWebViewPanel::ConnectWebViewPanel(wxWindow* parent)
    : WebViewPanel(parent, L"https://connect.prusa3d.com/connect-slicer-app/", "connect_loading")
{  
}

void ConnectWebViewPanel::on_script_message(wxWebViewEvent& evt)
{
    BOOST_LOG_TRIVIAL(debug) << "recieved message from PrusaConnect FE: " << evt.GetString();
    handle_message(into_u8(evt.GetString()));
}

void ConnectWebViewPanel::logout()
{
    wxString script = L"window._prusaConnect_v1.logout()";
    run_script(script);
}

void ConnectWebViewPanel::sys_color_changed()
{
    resend_config();
}

void ConnectWebViewPanel::on_request_update_selected_printer_action()
{
    assert(!m_message_data.empty());
    wxGetApp().handle_connect_request_printer_pick(m_message_data);
}


PrinterWebViewPanel::PrinterWebViewPanel(wxWindow* parent, const wxString& default_url)
    : WebViewPanel(parent, default_url)
{
    if (!m_browser)
        return;

    m_browser->Bind(wxEVT_WEBVIEW_LOADED, &PrinterWebViewPanel::on_loaded, this);
}

void PrinterWebViewPanel::on_loaded(wxWebViewEvent& evt)
{
    if (evt.GetURL().IsEmpty())
        return;
    if (!m_api_key.empty()) {
        send_api_key();
    } else if (!m_usr.empty() && !m_psk.empty()) {
        send_credentials();
    }
}

void PrinterWebViewPanel::send_api_key()
{
    if (!m_browser || m_api_key_sent)
        return;
    m_api_key_sent = true;
    wxString key = from_u8(m_api_key);
    wxString script = wxString::Format(R"(
    // Check if window.fetch exists before overriding
    if (window.fetch) {
        const originalFetch = window.fetch;
        window.fetch = function(input, init = {}) {
            init.headers = init.headers || {};
            init.headers['X-API-Key'] = '%s';
            return originalFetch(input, init);
        };
    }
)",
    key);

    m_browser->RemoveAllUserScripts();
    m_browser->AddUserScript(script);
    m_browser->Reload();
    
}

void PrinterWebViewPanel::send_credentials()
{
    if (!m_browser || m_api_key_sent)
        return;
    m_api_key_sent = true;
    wxString usr = from_u8(m_usr);
    wxString psk = from_u8(m_psk);
    wxString script = wxString::Format(R"(
    // Check if window.fetch exists before overriding
    if (window.fetch) {
        const originalFetch = window.fetch;
        window.fetch = function(input, init = {}) {
            init.headers = init.headers || {};
            init.headers['X-API-Key'] = 'Basic ' + btoa(`%s:%s`);
            return originalFetch(input, init);
        };
    }
)", usr, psk);
    
    m_browser->RemoveAllUserScripts();
    m_browser->AddUserScript(script);
    m_browser->Reload();
    
}

void PrinterWebViewPanel::sys_color_changed()
{
}

WebViewDialog::WebViewDialog(wxWindow* parent, const wxString& url, const wxString& dialog_name, const wxSize& size, const std::string& loading_html/* = "loading"*/)
    : wxDialog(parent, wxID_ANY, dialog_name, wxDefaultPosition, size, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
    , m_loading_html(loading_html)
{
    wxBoxSizer* topsizer = new wxBoxSizer(wxVERTICAL);
    topsizer->SetMinSize(size);
    SetSizerAndFit(topsizer);

    // Create the webview
    m_browser = WebView::CreateWebView(this, GUI::format_wxstr("file://%1%/web/%2%.html", boost::filesystem::path(resources_dir()).generic_string(), m_loading_html));
    if (!m_browser) {
        wxStaticText* text = new wxStaticText(this, wxID_ANY, _L("Failed to load a web browser."));
        topsizer->Add(text, 0, wxALIGN_LEFT | wxBOTTOM, 10);
        return;
    }

    topsizer->Add(m_browser, wxSizerFlags().Expand().Proportion(1));

    Bind(wxEVT_SHOW, &WebViewDialog::on_show, this);
    Bind(wxEVT_WEBVIEW_SCRIPT_MESSAGE_RECEIVED, &WebViewDialog::on_script_message, this, m_browser->GetId());

    m_browser->LoadURL(url);   
}
WebViewDialog::~WebViewDialog()
{
}

void WebViewDialog::run_script(const wxString& javascript)
{
    if (!m_browser) 
        return;
    m_browser->RunScriptAsync(javascript);
}

PrinterPickWebViewDialog::PrinterPickWebViewDialog(wxWindow* parent, std::string& ret_val)
    : WebViewDialog(parent
        , L"https://connect.prusa3d.com/connect-slicer-app/printer-list"
        , _L("Choose a printer")
        , wxSize(std::max(parent->GetClientSize().x / 2, 100 * wxGetApp().em_unit()), std::max(parent->GetClientSize().y / 2, 50 * wxGetApp().em_unit()))
        , "connect_loading")
    , m_ret_val(ret_val)
{
    Centre();
}
void PrinterPickWebViewDialog::on_show(wxShowEvent& evt)
{
    /*
    if (evt.IsShown()) {
        std::string token = wxGetApp().plater()->get_user_account()->get_access_token();
        wxString script = GUI::format_wxstr("window.setAccessToken(\'%1%\')", token);
        // TODO: should this be happening every OnShow?
        run_script(script);
    }
    */
}
void PrinterPickWebViewDialog::on_script_message(wxWebViewEvent& evt)
{
    handle_message(into_u8(evt.GetString()));
}

void PrinterPickWebViewDialog::on_request_update_selected_printer_action()
{
    m_ret_val = m_message_data;
    this->EndModal(wxID_OK);
}

void PrinterPickWebViewDialog::request_compatible_printers()
{
    
    if (Preset::printer_technology(wxGetApp().preset_bundle->printers.get_selected_preset().config) == ptFFF) {
        request_compatible_printers_FFF();
    } else {
        request_compatible_printers_SLA();
    }
}

void PrinterPickWebViewDialog::request_compatible_printers_FFF()
{
    //PrinterParams: {
    //material: Material;
    //nozzleDiameter: number;
    //printerType: string;
    //}
    const Preset& selected_printer = wxGetApp().preset_bundle->printers.get_selected_preset();
    const Preset& selected_filament = wxGetApp().preset_bundle->filaments.get_selected_preset();
    std::string nozzle_diameter_serialized = dynamic_cast<const ConfigOptionFloats*>(selected_printer.config.option("nozzle_diameter"))->serialize();
    // Sending only first nozzle diamenter for now.
    if (size_t comma = nozzle_diameter_serialized.find(','); comma != std::string::npos)
        nozzle_diameter_serialized = nozzle_diameter_serialized.substr(0, comma);
    // Sending only first filament type for now. This should change to array of values
    const std::string filament_type_serialized = selected_filament.config.option("filament_type")->serialize();
    const std::string printer_model_serialized = selected_printer.config.option("printer_model")->serialize();
   

    const std::string request = GUI::format(
        "{"
        "\"printerModel\": \"%3%\", "
        "\"nozzleDiameter\": %2%, "
        "\"material\": \"%1%\" "
        "}", filament_type_serialized, nozzle_diameter_serialized, printer_model_serialized);

    wxString script = GUI::format_wxstr("window._prusaConnect_v1.requestCompatiblePrinter(%1%)", request);
    run_script(script);
}
void PrinterPickWebViewDialog::request_compatible_printers_SLA()
{
    const Preset& selected_printer = wxGetApp().preset_bundle->printers.get_selected_preset();
    const std::string printer_model_serialized = selected_printer.config.option("printer_model")->serialize();
    //const std::string printer_type = wxGetApp().plater()->get_user_account()->get_printer_type_from_name(printer_model_serialized);
    const Preset& selected_material = wxGetApp().preset_bundle->sla_materials.get_selected_preset();
    const std::string material_type_serialized = selected_material.config.option("material_type")->serialize();
    const std::string request = GUI::format(
        "{"
        "\"material\": \"%1%\", "
        "\"printerModel\": \"%2%\" "
        "}", material_type_serialized, printer_model_serialized);

    wxString script = GUI::format_wxstr("window._prusaConnect_v1.requestCompatiblePrinter(%1%)", request);
    run_script(script);
}

} // GUI
} // Slic3r
