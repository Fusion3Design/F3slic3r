#include "WebView.hpp"
#include "slic3r/GUI/GUI_App.hpp"

#include <wx/uri.h>
#include <wx/webview.h>

#include <boost/log/trivial.hpp>

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

#ifdef __WIN32_
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
        BOOST_LOG_TRIVIAL(error) << "Failed to create wxWebView object.";
    }
    return webView;
}


