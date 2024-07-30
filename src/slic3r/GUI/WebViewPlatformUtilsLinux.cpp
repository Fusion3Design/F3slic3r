#include "WebViewPlatformUtils.hpp"
#include <unordered_map>
#include <memory>
#include <webkit2/webkit2.h>


namespace Slic3r::GUI {

struct Credentials {
    std::string username;
    std::string password;
};

gboolean webkit_authorize_handler(WebKitWebView *web_view, WebKitAuthenticationRequest *request, gpointer user_data)
{
    const Credentials& creds = *static_cast<const Credentials*>(user_data);
    webkit_authentication_request_authenticate(request, webkit_credential_new(creds.username.c_str(), creds.password.c_str(), WEBKIT_CREDENTIAL_PERSISTENCE_PERMANENT));
    return TRUE;
}

void free_credentials(gpointer user_data, GClosure* closure)
{
    Credentials* creds = static_cast<Credentials*>(user_data);
    delete creds;
}

void setup_webview_with_credentials(wxWebView* web_view, const std::string& username, const std::string& password)
{
    WebKitWebView* native_backend = static_cast<WebKitWebView *>(web_view->GetNativeBackend());
    Credentials* user_data = new Credentials{username, password};

    g_signal_connect_data(
        native_backend,
        "authenticate",
        G_CALLBACK(webkit_authorize_handler),
        user_data,
        &free_credentials,
        static_cast<GConnectFlags>(0)
    );

}

}
