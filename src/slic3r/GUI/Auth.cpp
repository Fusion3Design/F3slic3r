#include "Auth.hpp"
#include "GUI_App.hpp"
#include "format.hpp"
#include "../Utils/Http.hpp"
#include "slic3r/GUI/I18N.hpp"

#include <boost/log/trivial.hpp>
#include <boost/beast/core/detail/base64.hpp>
#include <curl/curl.h>
#include <string>

#include <iostream>
#include <random>
#include <algorithm>
#include <iterator>
#include <regex>
#include <iomanip>
#include <cstring>
#include <cstdint>

#if wxUSE_SECRETSTORE 
#include <wx/secretstore.h>
#endif

#ifdef WIN32
#include <wincrypt.h>
#endif // WIN32

#ifdef __APPLE__
#include <CommonCrypto/CommonDigest.h>
#endif

#ifdef __linux__
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#endif // __linux__



namespace fs = boost::filesystem;


namespace Slic3r {
namespace GUI {

namespace {

std::string get_code_from_message(const std::string& url_message)
{
    size_t pos = url_message.rfind("code=");
    std::string out;
    for (size_t i = pos + 5; i < url_message.size(); i++) {
        const char& c = url_message[i];
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'))
            out+= c;
        else
            break;  
    }
    return out;
}

bool is_secret_store_ok()
{
#if wxUSE_SECRETSTORE 
    wxSecretStore store = wxSecretStore::GetDefault();
    wxString errmsg;
    if (!store.IsOk(&errmsg)) {
        BOOST_LOG_TRIVIAL(warning) << "wxSecretStore is not supported: " << errmsg;
        return false;
    }
    return true;
#else
    return false;
#endif
}
bool save_secret(const std::string& opt, const std::string& usr, const std::string& psswd)
{
#if wxUSE_SECRETSTORE 
    wxSecretStore store = wxSecretStore::GetDefault();
    wxString errmsg;
    if (!store.IsOk(&errmsg)) {
        std::string msg = GUI::format("%1% (%2%).", _u8L("This system doesn't support storing passwords securely"), errmsg);
        BOOST_LOG_TRIVIAL(error) << msg;
        //show_error(nullptr, msg);
        return false;
    }
    const wxString service = GUI::format_wxstr(L"%1%/PrusaAuth/%2%", SLIC3R_APP_NAME, opt);
    const wxString username = boost::nowide::widen(usr);
    const wxSecretValue password(boost::nowide::widen(psswd));
    if (!store.Save(service, username, password)) {
        std::string msg(_u8L("Failed to save credentials to the system secret store."));
        BOOST_LOG_TRIVIAL(error) << msg;
        //show_error(nullptr, msg);
        return false;
    }
    return true;
#else
    BOOST_LOG_TRIVIAL(error) << "wxUSE_SECRETSTORE not supported. Cannot save password to the system store.";
    return false;
#endif // wxUSE_SECRETSTORE 
}
bool load_secret(const std::string& opt, std::string& usr, std::string& psswd)
{
#if wxUSE_SECRETSTORE
    wxSecretStore store = wxSecretStore::GetDefault();
    wxString errmsg;
    if (!store.IsOk(&errmsg)) {
        std::string msg = GUI::format("%1% (%2%).", _u8L("This system doesn't support storing passwords securely"), errmsg);
        BOOST_LOG_TRIVIAL(error) << msg;
        //show_error(nullptr, msg);
        return false;
    }
    const wxString service = GUI::format_wxstr(L"%1%/PrusaAuth/%2%", SLIC3R_APP_NAME, opt);
    wxString username;
    wxSecretValue password;
    if (!store.Load(service, username, password)) {
        std::string msg(_u8L("Failed to load credentials from the system secret store."));
        BOOST_LOG_TRIVIAL(error) << msg;
        //show_error(nullptr, msg);
        return false;
    }
    usr = into_u8(username);
    psswd = into_u8(password.GetAsString());
    return true;
#else
    BOOST_LOG_TRIVIAL(error) << "wxUSE_SECRETSTORE not supported. Cannot load password from the system store.";
    return false;
#endif // wxUSE_SECRETSTORE 
}
}

PrusaAuthCommunication::PrusaAuthCommunication(wxEvtHandler* evt_handler, AppConfig* app_config)
    : m_evt_handler(evt_handler)
{
    std::string access_token, refresh_token, shared_session_key;
    if (is_secret_store_ok()) {
        std::string key0, key1;
        load_secret("access_token", key0, access_token);
        load_secret("refresh_token", key1, refresh_token);
        assert(key0 == key1);
        shared_session_key = key0;
    } else {
        access_token = app_config->get("access_token");
        refresh_token = app_config->get("refresh_token");
        shared_session_key = app_config->get("shared_session_key");
    }
    if (!access_token.empty() || !refresh_token.empty())
        m_remember_session = true;
    m_session = std::make_unique<AuthSession>(evt_handler, access_token, refresh_token, shared_session_key, app_config->get_bool("connect_polling"));
    init_session_thread();
    // perform login at the start
    if (m_remember_session)
        do_login();
}

PrusaAuthCommunication::~PrusaAuthCommunication() {
    if (m_thread.joinable()) {
        // Stop the worker thread, if running.
        {
            // Notify the worker thread to cancel wait on detection polling.
            std::lock_guard<std::mutex> lck(m_thread_stop_mutex);
            m_thread_stop = true;
        }
        m_thread_stop_condition.notify_all();
        // Wait for the worker thread to stop.
        m_thread.join();
    }
}

void PrusaAuthCommunication::set_username(const std::string& username, AppConfig* app_config)
{
    m_username = username;
    {
        std::lock_guard<std::mutex> lock(m_session_mutex);
        if (is_secret_store_ok()) {
            save_secret("access_token", m_session->get_shared_session_key(), m_remember_session ? m_session->get_access_token() : std::string());
            save_secret("refresh_token", m_session->get_shared_session_key(), m_remember_session ? m_session->get_refresh_token() : std::string());
        }
        else {
            app_config->set("access_token", m_remember_session ? m_session->get_access_token() : std::string());
            app_config->set("refresh_token", m_remember_session ? m_session->get_refresh_token() : std::string());
            app_config->set("shared_session_key", m_remember_session ? m_session->get_shared_session_key() : std::string());
        }
    }
}

std::string PrusaAuthCommunication::get_access_token()
{
    {
        std::lock_guard<std::mutex> lock(m_session_mutex);
        return m_session->get_access_token();
    }
}

void PrusaAuthCommunication::set_polling_enabled(bool enabled)
{
    {
        std::lock_guard<std::mutex> lock(m_session_mutex);
        return m_session->set_polling_enabled(enabled);
    }
}

void PrusaAuthCommunication::login_redirect()
{
    const std::string AUTH_HOST = "https://test-account.prusa3d.com";
    const std::string CLIENT_ID = client_id();
    const std::string REDIRECT_URI = "prusaslicer://login";
    CodeChalengeGenerator ccg;
    m_code_verifier = ccg.generate_verifier();
    std::string code_challenge = ccg.generate_chalenge(m_code_verifier);
    BOOST_LOG_TRIVIAL(info) << "code verifier: " << m_code_verifier;
    BOOST_LOG_TRIVIAL(info) << "code challenge: " << code_challenge;
    wxString url = GUI::format_wxstr(L"%1%/o/authorize/?client_id=%2%&response_type=code&code_challenge=%3%&code_challenge_method=S256&scope=basic_info&redirect_uri=%4%", AUTH_HOST, CLIENT_ID, code_challenge, REDIRECT_URI);

    wxQueueEvent(m_evt_handler,new OpenPrusaAuthEvent(GUI::EVT_OPEN_PRUSAAUTH, std::move(url)));
}

bool PrusaAuthCommunication::is_logged()
{
    return !m_username.empty();
}
void PrusaAuthCommunication::do_login()
{
    {
        std::lock_guard<std::mutex> lock(m_session_mutex);
        if (!m_session->is_initialized()) {
            login_redirect();
        }
        m_session->enqueue_action(UserActionID::LOGIN_USER_ID, nullptr, nullptr, {});
    }
    wakeup_session_thread();
}
void PrusaAuthCommunication::do_logout(AppConfig* app_config)
{
    do_clear(app_config);
    wxQueueEvent(m_evt_handler, new PrusaAuthSuccessEvent(GUI::EVT_LOGGEDOUT_PRUSAAUTH, {}));
}

void PrusaAuthCommunication::do_clear(AppConfig* app_config)
{
    {
        std::lock_guard<std::mutex> lock(m_session_mutex);
        m_session->clear();
    }
    set_username({}, app_config);
}

void PrusaAuthCommunication::on_login_code_recieved(const std::string& url_message)
{
    {
        std::lock_guard<std::mutex> lock(m_session_mutex);
        const std::string code = get_code_from_message(url_message);
        m_session->init_with_code(code, m_code_verifier);
    }
    wakeup_session_thread();
}

#if 0
void PrusaAuthCommunication::enqueue_user_id_action()
{
    {
        std::lock_guard<std::mutex> lock(m_session_mutex);
        if (!m_session->is_initialized()) {
            return;
        }
        m_session->enqueue_action(UserActionID::USER_ID, nullptr, nullptr, {});
    }
    wakeup_session_thread();
}

void PrusaAuthCommunication::enqueue_connect_dummy_action()
{
    {
        std::lock_guard<std::mutex> lock(m_session_mutex);
        if (!m_session->is_initialized()) {
            BOOST_LOG_TRIVIAL(error) << "Connect Dummy endpoint connection failed - Not Logged in.";
            return;
        }
        m_session->enqueue_action(UserActionID::CONNECT_DUMMY, nullptr, nullptr, {});
    }
    wakeup_session_thread();
}
#endif

void PrusaAuthCommunication::enqueue_connect_printers_action()
{
    {
        std::lock_guard<std::mutex> lock(m_session_mutex);
        if (!m_session->is_initialized()) {
            BOOST_LOG_TRIVIAL(error) << "Connect Printers endpoint connection failed - Not Logged in.";
            return;
        }
        m_session->enqueue_action(UserActionID::CONNECT_PRINTERS, nullptr, nullptr, {});
    }
    wakeup_session_thread();
}

void PrusaAuthCommunication::enqueue_avatar_action(const std::string url)
{
    {
        std::lock_guard<std::mutex> lock(m_session_mutex);
        if (!m_session->is_initialized()) {
            BOOST_LOG_TRIVIAL(error) << "Connect Printers endpoint connection failed - Not Logged in.";
            return;
        }
        m_session->enqueue_action(UserActionID::AVATAR, nullptr, nullptr, url);
    }
    wakeup_session_thread();
}
void PrusaAuthCommunication::init_session_thread()
{
    m_thread = std::thread([this]() {
        for (;;) {
            // Wait for 1 second 
            // Cancellable.
            {
                std::unique_lock<std::mutex> lck(m_thread_stop_mutex);
                m_thread_stop_condition.wait_for(lck, std::chrono::seconds(1), [this] { return m_thread_stop; });
            }
            if (m_thread_stop)
                // Stop the worker thread.
                break;
            {
                std::lock_guard<std::mutex> lock(m_session_mutex);
                m_session->process_action_queue();
            }
        }
    });
}

void PrusaAuthCommunication::wakeup_session_thread()
{
    m_thread_stop_condition.notify_all();
}





std::string CodeChalengeGenerator::generate_chalenge(const std::string& verifier)
{
    std::string code_challenge;
    try
    {
        code_challenge = sha256(verifier);
        code_challenge = base64_encode(code_challenge);
    }
    catch (const std::exception& e)
    {
        BOOST_LOG_TRIVIAL(error) << "Code Chalenge Generator failed: " << e.what();
    }
    assert(!code_challenge.empty());
    return code_challenge;
    
}
std::string CodeChalengeGenerator::generate_verifier()
{
    size_t length = 40;
    std::string code_verifier = generate_code_verifier(length);
    assert(code_verifier.size() == length);
    return code_verifier;
}
std::string CodeChalengeGenerator::base64_encode(const std::string& input)
{
    std::string output;
    output.resize(boost::beast::detail::base64::encoded_size(input.size()));
    boost::beast::detail::base64::encode(&output[0], input.data(), input.size());
    // save encode - replace + and / with - and _
    std::replace(output.begin(), output.end(), '+', '-');
    std::replace(output.begin(), output.end(), '/', '_');
    // remove last '=' sign 
    while (output.back() == '=')
        output.pop_back();
    return output;
}
std::string CodeChalengeGenerator::generate_code_verifier(size_t length)
{
    const std::string                   chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    std::random_device                  rd;
    std::mt19937                        gen(rd());
    std::uniform_int_distribution<int>  distribution(0, chars.size() - 1);
    std::string                         code_verifier;
    for (int i = 0; i < length; ++i) {
        code_verifier += chars[distribution(gen)];
    }
    return code_verifier;
}

#ifdef WIN32
std::string CodeChalengeGenerator::sha256(const std::string& input)
{
    HCRYPTPROV          prov_handle = NULL;
    HCRYPTHASH          hash_handle = NULL;
    DWORD               hash_size = 0;
    DWORD               buffer_size = sizeof(DWORD);
    std::string         output;

    if (!CryptAcquireContext(&prov_handle, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) {
        throw std::exception("CryptAcquireContext failed.");
    }
    if (!CryptCreateHash(prov_handle, CALG_SHA_256, 0, 0, &hash_handle)) {
        CryptReleaseContext(prov_handle, 0);
        throw std::exception("CryptCreateHash failed.");
    }
    if (!CryptHashData(hash_handle, reinterpret_cast<const BYTE*>(input.c_str()), input.length(), 0)) {
        CryptDestroyHash(hash_handle);
        CryptReleaseContext(prov_handle, 0);
        throw std::exception("CryptCreateHash failed.");
    }
    if (!CryptGetHashParam(hash_handle, HP_HASHSIZE, reinterpret_cast<BYTE*>(&hash_size), &buffer_size, 0)) {
        CryptDestroyHash(hash_handle);
        CryptReleaseContext(prov_handle, 0);
        throw std::exception("CryptGetHashParam HP_HASHSIZE failed.");
    }
    output.resize(hash_size);
    if (!CryptGetHashParam(hash_handle, HP_HASHVAL, reinterpret_cast<BYTE*>(&output[0]), &hash_size, 0)) {
        CryptDestroyHash(hash_handle);
        CryptReleaseContext(prov_handle, 0);
        throw std::exception("CryptGetHashParam HP_HASHVAL failed.");
    }
    return output;
}
#elif __APPLE__
std::string CodeChalengeGenerator::sha256(const std::string& input) {
    // Initialize the context
    CC_SHA256_CTX sha256;
    CC_SHA256_Init(&sha256);

    // Update the context with the input data
    CC_SHA256_Update(&sha256, input.c_str(), static_cast<CC_LONG>(input.length()));

    // Finalize the hash and retrieve the result
    unsigned char digest[CC_SHA256_DIGEST_LENGTH];
    CC_SHA256_Final(digest, &sha256);

    return std::string(reinterpret_cast<char*>(digest), CC_SHA256_DIGEST_LENGTH);
}
#else
std::string CodeChalengeGenerator::sha256(const std::string& input) {
    EVP_MD_CTX* mdctx;
    const EVP_MD* md;
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digestLen;

    md = EVP_sha256();
    mdctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(mdctx, md, NULL);
    EVP_DigestUpdate(mdctx, input.c_str(), input.length());
    EVP_DigestFinal_ex(mdctx, digest, &digestLen);
    EVP_MD_CTX_free(mdctx);

    return std::string(reinterpret_cast<char*>(digest), digestLen);
}
#endif // __linux__
}} // Slic3r::GUI