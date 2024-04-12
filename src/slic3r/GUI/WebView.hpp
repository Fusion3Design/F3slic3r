#ifndef slic3r_GUI_WebView_hpp_
#define slic3r_GUI_WebView_hpp_

class wxWebView;
class wxWindow;
class wxString;

namespace WebView
{
    wxWebView *CreateWebView(wxWindow *parent, const wxString& url);
    bool run_script(wxWebView * webView, const wxString& msg);
};

#endif // !slic3r_GUI_WebView_hpp_
