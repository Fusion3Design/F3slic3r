#include "WebView.hpp"
#include "slic3r/GUI/GUI_App.hpp"
#include "slic3r/Utils/MacDarkMode.hpp"

#include <wx/webviewarchivehandler.h>
#include <wx/webviewfshandler.h>
#include <wx/msw/webview_edge.h>
#include <wx/uri.h>
#include "wx/private/jsscriptwrapper.h"

#include <boost/log/trivial.hpp>

#ifdef __WIN32__
#include <WebView2.h>
#elif defined __linux__
#include <gtk/gtk.h>
#define WEBKIT_API
struct WebKitWebView;
struct WebKitJavascriptResult;
extern "C" {
WEBKIT_API void
webkit_web_view_run_javascript                       (WebKitWebView             *web_view,
                                                      const gchar               *script,
                                                      GCancellable              *cancellable,
                                                      GAsyncReadyCallback       callback,
                                                      gpointer                  user_data);
WEBKIT_API WebKitJavascriptResult *
webkit_web_view_run_javascript_finish                (WebKitWebView             *web_view,
                                                      GAsyncResult              *result,
						      GError                    **error);
WEBKIT_API void
webkit_javascript_result_unref              (WebKitJavascriptResult *js_result);
}
#endif

class FakeWebView : public wxWebView
{
    virtual bool Create(wxWindow* parent, wxWindowID id, const wxString& url, const wxPoint& pos, const wxSize& size, long style, const wxString& name) override { return false; }
    virtual wxString GetCurrentTitle() const override { return wxString(); }
    virtual wxString GetCurrentURL() const override { return wxString(); }
    virtual bool IsBusy() const override { return false; }
    virtual bool IsEditable() const override { return false; }
    virtual void LoadURL(const wxString& url) override { }
    virtual void Print() override { }
    virtual void RegisterHandler(wxSharedPtr<wxWebViewHandler> handler) override { }
    virtual void Reload(wxWebViewReloadFlags flags = wxWEBVIEW_RELOAD_DEFAULT) override { }
    virtual bool RunScript(const wxString& javascript, wxString* output = NULL) const override { return false; }
    virtual void SetEditable(bool enable = true) override { }
    virtual void Stop() override { }
    virtual bool CanGoBack() const override { return false; }
    virtual bool CanGoForward() const override { return false; }
    virtual void GoBack() override { }
    virtual void GoForward() override { }
    virtual void ClearHistory() override { }
    virtual void EnableHistory(bool enable = true) override { }
    virtual wxVector<wxSharedPtr<wxWebViewHistoryItem>> GetBackwardHistory() override { return {}; }
    virtual wxVector<wxSharedPtr<wxWebViewHistoryItem>> GetForwardHistory() override { return {}; }
    virtual void LoadHistoryItem(wxSharedPtr<wxWebViewHistoryItem> item) override { }
    virtual bool CanSetZoomType(wxWebViewZoomType type) const override { return false; }
    virtual float GetZoomFactor() const override { return 0.0f; }
    virtual wxWebViewZoomType GetZoomType() const override { return wxWebViewZoomType(); }
    virtual void SetZoomFactor(float zoom) override { }
    virtual void SetZoomType(wxWebViewZoomType zoomType) override { }
    virtual bool CanUndo() const override { return false; }
    virtual bool CanRedo() const override { return false; }
    virtual void Undo() override { }
    virtual void Redo() override { }
    virtual void* GetNativeBackend() const override { return nullptr; }
    virtual void DoSetPage(const wxString& html, const wxString& baseUrl) override { }
};

wxWebView* WebView::CreateWebView(wxWindow * parent, const wxString& url)
{
#if wxUSE_WEBVIEW_EDGE
    bool backend_available = wxWebView::IsBackendAvailable(wxWebViewBackendEdge);
#else
    bool backend_available = wxWebView::IsBackendAvailable(wxWebViewBackendWebKit);
#endif

    wxWebView* webView = nullptr;
    if (backend_available)
        webView = wxWebView::New();
    
    if (webView) {
        wxString correct_url = url.empty() ? wxString("") : wxURI(url).BuildURI();

#ifdef __WIN32__
        webView->SetUserAgent(wxString::Format("PrusaSlicer/v%s", SLIC3R_VERSION));
        webView->Create(parent, wxID_ANY, correct_url, wxDefaultPosition, wxDefaultSize);
        //We register the wxfs:// protocol for testing purposes
        //webView->RegisterHandler(wxSharedPtr<wxWebViewHandler>(new wxWebViewArchiveHandler("wxfs")));
        //And the memory: file system
        //webView->RegisterHandler(wxSharedPtr<wxWebViewHandler>(new wxWebViewFSHandler("memory")));
#else
        // With WKWebView handlers need to be registered before creation
        //webView->RegisterHandler(wxSharedPtr<wxWebViewHandler>(new wxWebViewArchiveHandler("wxfs")));
        // And the memory: file system
        //webView->RegisterHandler(wxSharedPtr<wxWebViewHandler>(new wxWebViewFSHandler("memory")));
        webView->Create(parent, wxID_ANY, correct_url, wxDefaultPosition, wxDefaultSize);
        webView->SetUserAgent(wxString::Format("PrusaSlicer/v%s", SLIC3R_VERSION));
#endif
#ifndef __WIN32__
        Slic3r::GUI::wxGetApp().CallAfter([webView] {
#endif
        if (!webView->AddScriptMessageHandler("_prusaSlicer")) {
            // TODO: dialog to user !!!
            //wxLogError("Could not add script message handler");
            BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << "Could not add script message handler";
        }
#ifndef __WIN32__
        });
#endif
        webView->EnableContextMenu(false);
    } else {
        // TODO: dialog to user !!!
        BOOST_LOG_TRIVIAL(error) << "Failed to create wxWebView object. Using Dummy object instead. Webview won't be working.";
        webView = new FakeWebView;
    }
    return webView;
}

void WebView::LoadUrl(wxWebView * webView, wxString const &url)
{
    auto url2  = url;
#ifdef __WIN32__
    url2.Replace("\\", "/");
#endif
    if (!url2.empty()) { url2 = wxURI(url2).BuildURI(); }
    BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << url2.ToUTF8();
    webView->LoadURL(url2);
}

bool WebView::run_script(wxWebView *webView, wxString const &javascript)
{
    try {
#ifdef __WIN32__
        ICoreWebView2 *   webView2 = (ICoreWebView2 *) webView->GetNativeBackend();
        if (webView2 == nullptr)
            return false;
        int               count   = 0;
        wxJSScriptWrapper wrapJS(javascript, wxJSScriptWrapper::OutputType::JS_OUTPUT_STRING);
        wxString wrapped_code = wrapJS.GetWrappedCode();
        return webView2->ExecuteScript(wrapJS.GetWrappedCode(), NULL) == 0;
#elif defined __WXMAC__
        WKWebView * wkWebView = (WKWebView *) webView->GetNativeBackend();
        wxJSScriptWrapper wrapJS(javascript, wxJSScriptWrapper::OutputType::JS_OUTPUT_STRING);
        Slic3r::GUI::WKWebView_evaluateJavaScript(wkWebView, wrapJS.GetWrappedCode(), nullptr);
        return true;
#else
        WebKitWebView *wkWebView = (WebKitWebView *) webView->GetNativeBackend();
        webkit_web_view_run_javascript(
            wkWebView, javascript.utf8_str(), NULL,
            [](GObject *wkWebView, GAsyncResult *res, void *) {
                GError * error = NULL;
                auto result = webkit_web_view_run_javascript_finish((WebKitWebView*)wkWebView, res, &error);
                if (!result)
                    g_error_free (error);
                else
                    webkit_javascript_result_unref (result);
        }, NULL);
        return true;
#endif
    } catch (std::exception &) {
        return false;
    }
}
