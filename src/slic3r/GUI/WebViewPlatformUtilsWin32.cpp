#include "WebViewPlatformUtils.hpp"

#ifdef __WIN32__
#include "WebView2.h"
#include <wrl.h>
#include <atlbase.h>

#include "wx/msw/private/comptr.h"

#include "GUI_App.hpp"
#include "format.hpp"
#include "Mainframe.hpp"

namespace Slic3r::GUI {

void setup_webview_with_credentials(wxWebView* webview, const std::string& username, const std::string& password)
{
    ICoreWebView2 *webView2 = (ICoreWebView2 *) webview->GetNativeBackend();

    
    wxCOMPtr<ICoreWebView2_2> webview2Com;
    wxCOMPtr<ICoreWebView2Environment> environment;
    if(FAILED(webView2->QueryInterface(IID_PPV_ARGS(&webview2Com))))
    {
        return;
    }
    if (FAILED(webview2Com->get_Environment(&environment))) {
        return;
    }

    LPWSTR wideString;
    if (SUCCEEDED(environment->get_BrowserVersionString(&wideString))) {
        std::wcout << L"WebView2 runtime version: " << wideString << std::endl;
    }

    
    wxCOMPtr<ICoreWebView2_10> wv2_10;
    //if (!SUCCEEDED(webView2->QueryInterface(IID_PPV_ARGS(&wv2_10)))) {
    //wv2_10 = wil::com_ptr<ICoreWebView2>(m_appWindow->GetWebView()).query<ICoreWebView2_2>();
    // auto webView10 = webView2->query<ICoreWebView2_10>()
    HRESULT hr = webView2->QueryInterface(IID_PPV_ARGS(&wv2_10));
    if (FAILED(hr)) {
        return;        
    }
    // should it be stored?
    EventRegistrationToken basicAuthenticationRequestedToken = {};

    if (!SUCCEEDED(wv2_10->add_BasicAuthenticationRequested(
            Microsoft::WRL::Callback<ICoreWebView2BasicAuthenticationRequestedEventHandler>(
                [username, password](ICoreWebView2 *sender, ICoreWebView2BasicAuthenticationRequestedEventArgs *args) {
                    wxCOMPtr<ICoreWebView2BasicAuthenticationResponse> basicAuthenticationResponse;
                    if (!SUCCEEDED(args->get_Response(&basicAuthenticationResponse))) {
                        return -1;
                    }
                    if (!SUCCEEDED(basicAuthenticationResponse->put_UserName(GUI::from_u8(username).c_str()))) {
                        return -1;
                    }
                    if (!SUCCEEDED(basicAuthenticationResponse->put_Password(GUI::from_u8(password).c_str()))) {
                        return -1;
                    }
                    return 0;
                }
            ).Get(),
            &basicAuthenticationRequestedToken
        ))) 
    {

    }
    wv2_10->Release();
    
       
}
#endif
}
