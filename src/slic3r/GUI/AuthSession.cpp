#include "AuthSession.hpp"
#include "GUI_App.hpp"
#include "format.hpp"
#include "../Utils/Http.hpp"
#include "I18N.hpp"

#include <boost/log/trivial.hpp>
#include <boost/regex.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/beast/core/detail/base64.hpp>
#include <curl/curl.h>
#include <string>

namespace fs = boost::filesystem;
namespace pt = boost::property_tree;

namespace Slic3r {
namespace GUI {

wxDEFINE_EVENT(EVT_OPEN_PRUSAAUTH, OpenPrusaAuthEvent);
wxDEFINE_EVENT(EVT_LOGGEDOUT_PRUSAAUTH, PrusaAuthSuccessEvent);
wxDEFINE_EVENT(EVT_PA_ID_USER_SUCCESS, PrusaAuthSuccessEvent);
wxDEFINE_EVENT(EVT_PA_ID_USER_FAIL, PrusaAuthFailEvent);
wxDEFINE_EVENT(EVT_PRUSAAUTH_SUCCESS, PrusaAuthSuccessEvent);
wxDEFINE_EVENT(EVT_PRUSACONNECT_PRINTERS_SUCCESS, PrusaAuthSuccessEvent);
wxDEFINE_EVENT(EVT_PRUSAAUTH_FAIL, PrusaAuthFailEvent);

void UserActionPost::perform(const std::string& access_token, UserActionSuccessFn success_callback, UserActionFailFn fail_callback, const std::string& input)
{
    std::string url = m_url;
    BOOST_LOG_TRIVIAL(info) << m_action_name <<" POST " << url << " body: " << input;
    auto http = Http::post(std::move(url));
    if (!input.empty())
        http.set_post_body(input);
    http.header("Content-type", "application/x-www-form-urlencoded");
    http.on_error([&](std::string body, std::string error, unsigned status) {
        BOOST_LOG_TRIVIAL(error) << m_action_name << " action failed. status: " << status << " Body: " << body;
        if (fail_callback)
            fail_callback(body);
    });
    http.on_complete([&, this](std::string body, unsigned status) {
        BOOST_LOG_TRIVIAL(info) << m_action_name << "action success. Status: " << status << " Body: " << body;
        if (success_callback)
            success_callback(body);
    });
    http.perform_sync();
}

void UserActionGetWithEvent::perform(const std::string& access_token, UserActionSuccessFn success_callback, UserActionFailFn fail_callback, /*UNUSED*/ const std::string& input)
{
    std::string url = m_url;
    BOOST_LOG_TRIVIAL(info) << m_action_name << " GET " << url;
    auto http = Http::get(url);
    http.header("Authorization", "Bearer " + access_token);
    http.on_error([&](std::string body, std::string error, unsigned status) {
        BOOST_LOG_TRIVIAL(error) << m_action_name << " action failed. status: " << status << " Body: " << body;
        if (fail_callback)
            fail_callback(body);
        std::string message = GUI::format("%1% action failed (%2%): %3%", m_action_name, std::to_string(status), body);
        wxQueueEvent(m_evt_handler, new PrusaAuthFailEvent(m_fail_evt_type, std::move(message)));
    });
    http.on_complete([&, this](std::string body, unsigned status) {
        BOOST_LOG_TRIVIAL(info) << m_action_name << " action success. Status: " << status << " Body: " << body;
        if (success_callback)
            success_callback(body);
        wxQueueEvent(m_evt_handler, new PrusaAuthSuccessEvent(m_succ_evt_type, body));
    });

    http.perform_sync();
}

void AuthSession::process_action_queue()
{
    if (m_priority_action_queue.empty() && m_action_queue.empty())
        return;

    if (this->is_initialized()) {
        // if priority queue already has some action f.e. to exchange tokens, the test should not be neccessary but also shouldn't be problem
        enqueue_test_with_refresh();
    }

    while (!m_priority_action_queue.empty()) {
        m_actions[m_priority_action_queue.front().action_id]->perform(m_access_token, m_priority_action_queue.front().success_callback, m_priority_action_queue.front().fail_callback, m_priority_action_queue.front().input);
        if (!m_priority_action_queue.empty())
            m_priority_action_queue.pop();
    }

    if (!this->is_initialized())
        return;

    while (!m_action_queue.empty()) {
        m_actions[m_action_queue.front().action_id]->perform(m_access_token, m_action_queue.front().success_callback, m_action_queue.front().fail_callback, m_action_queue.front().input);
        if (!m_action_queue.empty())
            m_action_queue.pop();
    }
}

void AuthSession::enqueue_action(UserActionID id, UserActionSuccessFn success_callback, UserActionFailFn fail_callback, const std::string& input)
{
    m_action_queue.push({ id, success_callback, fail_callback, input });
}

void AuthSession::init_with_code(const std::string& code, const std::string& code_verifier)
{
    // Data we have       
    const std::string REDIRECT_URI = "prusaslicer://login";
    std::string post_fields = "code=" + code +
        "&client_id=" + client_id() +
        "&grant_type=authorization_code" +
        "&redirect_uri=" + REDIRECT_URI +
        "&code_verifier="+ code_verifier;

    auto succ_fn = [&](const std::string& body){
        // Data we need
        std::string access_token, refresh_token, shared_session_key;
        try {
            std::stringstream ss(body);
            pt::ptree ptree;
            pt::read_json(ss, ptree);

            const auto access_token_optional = ptree.get_optional<std::string>("access_token");
            const auto refresh_token_optional = ptree.get_optional<std::string>("refresh_token");
            const auto shared_session_key_optional = ptree.get_optional<std::string>("shared_session_key");

            if (access_token_optional)
                access_token = *access_token_optional;
            if (refresh_token_optional)
                refresh_token = *refresh_token_optional;
            if (shared_session_key_optional)
                shared_session_key = *shared_session_key_optional;
        }
        catch (const std::exception&) {
            BOOST_LOG_TRIVIAL(error) << "Auth::http_access Could not parse server response.";
        }

        assert(!access_token.empty() && !refresh_token.empty() && !shared_session_key.empty());
        if (access_token.empty() || refresh_token.empty() || shared_session_key.empty()) {
            BOOST_LOG_TRIVIAL(error) << "Refreshing token has failed.";
            m_access_token = std::string();
            m_refresh_token = std::string();
            m_shared_session_key = std::string();
            return;
        }

        BOOST_LOG_TRIVIAL(info) << "access_token: " << access_token;
        BOOST_LOG_TRIVIAL(info) << "refresh_token: " << refresh_token;
        BOOST_LOG_TRIVIAL(info) << "shared_session_key: " << shared_session_key;

        m_access_token = access_token;
        m_refresh_token = refresh_token;
        m_shared_session_key = shared_session_key;
    };

    // fail fn might be cancel_queue here
    m_priority_action_queue.push({ UserActionID::CODE_FOR_TOKEN, succ_fn, std::bind(&AuthSession::enqueue_refresh, this, std::placeholders::_1), post_fields });
}

void AuthSession::enqueue_test_with_refresh()
{
    // on test fail - try refresh
    m_priority_action_queue.push({ UserActionID::TEST_CONNECTION, nullptr, std::bind(&AuthSession::enqueue_refresh, this, std::placeholders::_1), {} });
}

void AuthSession::enqueue_refresh(const std::string& body)
{
    std::string post_fields = "grant_type=refresh_token" 
        "&client_id=" + client_id() +
        "&refresh_token=" + m_refresh_token;

    auto succ_callback = [&](const std::string& body){
        std::string new_access_token;
        std::string new_refresh_token;
        std::string new_shared_session_key;
        try {
            std::stringstream ss(body);
            pt::ptree ptree;
            pt::read_json(ss, ptree);

            const auto access_token_optional = ptree.get_optional<std::string>("access_token");
            const auto refresh_token_optional = ptree.get_optional<std::string>("refresh_token");
            const auto shared_session_key_optional = ptree.get_optional<std::string>("shared_session_key");

            if (access_token_optional)
                new_access_token = *access_token_optional;
            if (refresh_token_optional)
                new_refresh_token = *refresh_token_optional;
            if (shared_session_key_optional)
                new_shared_session_key = *shared_session_key_optional;
        }
        catch (const std::exception&) {
            BOOST_LOG_TRIVIAL(error) << "Could not parse server response.";
        }

        assert(!new_access_token.empty() && !new_refresh_token.empty() && !new_shared_session_key.empty());
        if (new_access_token.empty() || new_refresh_token.empty() || new_shared_session_key.empty()) {
            BOOST_LOG_TRIVIAL(error) << "Refreshing token has failed.";
            m_access_token = std::string();
            m_refresh_token = std::string();
            m_shared_session_key = std::string();
            // TODO: cancel following queue
        }

        BOOST_LOG_TRIVIAL(info) << "access_token: " << new_access_token;
        BOOST_LOG_TRIVIAL(info) << "refresh_token: " << new_refresh_token;
        BOOST_LOG_TRIVIAL(info) << "shared_session_key: " << new_shared_session_key;

        m_access_token = new_access_token;
        m_refresh_token = new_refresh_token;
        m_shared_session_key = new_shared_session_key;
        m_priority_action_queue.push({ UserActionID::TEST_CONNECTION, nullptr, std::bind(&AuthSession::refresh_failed_callback, this, std::placeholders::_1), {} });
    };

    m_priority_action_queue.push({ UserActionID::REFRESH_TOKEN, succ_callback, std::bind(&AuthSession::refresh_failed_callback, this, std::placeholders::_1), post_fields });
}

void AuthSession::refresh_failed_callback(const std::string& body)
{
    clear();
    cancel_queue();
}
void AuthSession::cancel_queue()
{
    while (!m_priority_action_queue.empty()) {
        m_priority_action_queue.pop();
    }
    while (!m_action_queue.empty()) {
        m_action_queue.pop();
    }
}

}} // Slic3r::GUI