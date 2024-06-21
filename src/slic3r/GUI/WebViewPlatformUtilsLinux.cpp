#include "WebViewPlatformUtils.hpp"
#include <unordered_map>
#include <memory>
#include <webkit2/webkit2.h>


namespace Slic3r::GUI {

struct Credentials {
    std::string username;
    std::string password;
};

using WebViewCredentialsMap = std::unordered_map<void*, std::unique_ptr<Credentials>>;
WebViewCredentialsMap g_credentials;

gboolean webkit_authorize_handler(WebKitWebView *web_view, WebKitAuthenticationRequest *request, gpointer user_data)
{
    const Credentials& creds = *static_cast<const Credentials*>(user_data);
    webkit_authentication_request_authenticate(request, webkit_credential_new(creds.username.c_str(), creds.password.c_str()));
}

    void setup_webview_with_credentials(wxWebView* web_view, const std::string& username, const std::string& password)
{
    std::unique_ptr<Credentials> ptr(new Credentials{username, password});
    g_credentials[web_view->GetNativeBackend()] = std::move(ptr);
    g_signal_connect(web_view, "authorize",
                     G_CALLBACK(webkit_authorize_handler), g_credentials[web_view].get());

}

}
