#include "WebViewPlatformUtils.hpp"

#ifdef __WIN32__
#include "WebView2.h"
#include <wrl.h>
#include <atlbase.h>
#include <boost/log/trivial.hpp>
#include <unordered_map>

#include "wx/msw/private/comptr.h"

#include "GUI_App.hpp"
#include "format.hpp"
#include "Mainframe.hpp"

namespace Slic3r::GUI {

std::unordered_map<ICoreWebView2*,EventRegistrationToken> g_basic_auth_handler_tokens;

void setup_webview_with_credentials(wxWebView* webview, const std::string& username, const std::string& password)
{
    ICoreWebView2 *webView2 = static_cast<ICoreWebView2 *>(webview->GetNativeBackend());
    wxCOMPtr<ICoreWebView2_10> wv2_10;
    HRESULT hr = webView2->QueryInterface(IID_PPV_ARGS(&wv2_10));
    if (FAILED(hr)) {
        return;        
    }

    // should it be stored?
    EventRegistrationToken basicAuthenticationRequestedToken = {};

    if (FAILED(wv2_10->add_BasicAuthenticationRequested(
            Microsoft::WRL::Callback<ICoreWebView2BasicAuthenticationRequestedEventHandler>(
                [username, password](ICoreWebView2 *sender, ICoreWebView2BasicAuthenticationRequestedEventArgs *args) {
                    wxCOMPtr<ICoreWebView2BasicAuthenticationResponse> basicAuthenticationResponse;
                    if (FAILED(args->get_Response(&basicAuthenticationResponse))) {
                        return -1;
                    }
                    if (FAILED(basicAuthenticationResponse->put_UserName(GUI::from_u8(username).c_str()))) {
                        return -1;
                    }
                    if (FAILED(basicAuthenticationResponse->put_Password(GUI::from_u8(password).c_str()))) {
                        return -1;
                    }
                    return 0;
                }
            ).Get(),
            &basicAuthenticationRequestedToken
        ))) {

        BOOST_LOG_TRIVIAL(error) << "WebView: Cannot register authentication request handler";
    } else {
        g_basic_auth_handler_tokens[webView2] = basicAuthenticationRequestedToken;
    }
       
}

void remove_webview_credentials(wxWebView* webview)
{
    ICoreWebView2 *webView2 = static_cast<ICoreWebView2 *>(webview->GetNativeBackend());
    wxCOMPtr<ICoreWebView2_10> wv2_10;
    HRESULT hr = webView2->QueryInterface(IID_PPV_ARGS(&wv2_10));
    if (FAILED(hr)) {
        return;
    }

    auto it = g_basic_auth_handler_tokens.find(webView2);
    if (it != g_basic_auth_handler_tokens.end()) {
        if (FAILED(wv2_10->remove_BasicAuthenticationRequested(it->second))) {
            BOOST_LOG_TRIVIAL(error) << "WebView: Unregistering authentication request handler failed";
        }
    } else {
        BOOST_LOG_TRIVIAL(error) << "WebView: Cannot unregister authentication request handler";
    }

}

} // namespace Slic3r::GUI
#endif // WIN32
