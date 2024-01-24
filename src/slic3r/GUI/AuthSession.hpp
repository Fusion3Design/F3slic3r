#ifndef slic3r_AuthSession_hpp_
#define slic3r_AuthSession_hpp_

#include "Event.hpp"
#include "libslic3r/AppConfig.hpp"

#include <queue>
#include <map>
#include <thread>
#include <mutex>
#include <memory>

namespace Slic3r {
namespace GUI {

using OpenPrusaAuthEvent = Event<wxString>;
using PrusaAuthSuccessEvent = Event<std::string>;
using PrusaAuthFailEvent = Event<std::string>;
wxDECLARE_EVENT(EVT_OPEN_PRUSAAUTH, OpenPrusaAuthEvent);
wxDECLARE_EVENT(EVT_LOGGEDOUT_PRUSAAUTH, PrusaAuthSuccessEvent);
wxDECLARE_EVENT(EVT_PA_ID_USER_SUCCESS, PrusaAuthSuccessEvent);
wxDECLARE_EVENT(EVT_PA_ID_USER_FAIL, PrusaAuthFailEvent);
wxDECLARE_EVENT(EVT_PRUSAAUTH_SUCCESS, PrusaAuthSuccessEvent);
wxDECLARE_EVENT(EVT_PRUSACONNECT_PRINTERS_SUCCESS, PrusaAuthSuccessEvent);
wxDECLARE_EVENT(EVT_PA_AVATAR_SUCCESS, PrusaAuthSuccessEvent);
wxDECLARE_EVENT(EVT_PRUSAAUTH_FAIL, PrusaAuthFailEvent); // Soft fail - clears only after some number of fails
wxDECLARE_EVENT(EVT_PA_AVATAR_FAIL, PrusaAuthFailEvent); // Soft fail - clears only after some number of fails
wxDECLARE_EVENT(EVT_PRUSAAUTH_RESET, PrusaAuthFailEvent); // Hard fail - clears all

typedef std::function<void(const std::string& body)> UserActionSuccessFn;
typedef std::function<void(const std::string& body)> UserActionFailFn;

// UserActions implements different operations via trigger() method. Stored in m_actions.
enum class UserActionID {
    AUTH_ACTION_DUMMY,
    AUTH_ACTION_REFRESH_TOKEN,
    AUTH_ACTION_CODE_FOR_TOKEN,
    AUTH_ACTION_USER_ID,
    AUTH_ACTION_TEST_ACCESS_TOKEN,
    AUTH_ACTION_TEST_CONNECTION,
    AUTH_ACTION_CONNECT_DUMMY,
    AUTH_ACTION_CONNECT_PRINTERS,
    AUTH_ACTION_AVATAR,
};
class UserAction
{
public:
    UserAction(const std::string name, const std::string url) : m_action_name(name), m_url(url){}
    ~UserAction() {}
    virtual void perform(wxEvtHandler* evt_handler, const std::string& access_token, UserActionSuccessFn success_callback, UserActionFailFn fail_callback, const std::string& input) = 0;
protected:
    std::string m_action_name;
    std::string m_url;
};

class UserActionGetWithEvent : public UserAction
{
public:
    UserActionGetWithEvent(const std::string name, const std::string url, wxEventType succ_event_type, wxEventType fail_event_type)
        : m_succ_evt_type(succ_event_type)
        , m_fail_evt_type(fail_event_type)
        , UserAction(name, url)
    {}
    ~UserActionGetWithEvent() {}
    void perform(wxEvtHandler* evt_handler, const std::string& access_token, UserActionSuccessFn success_callback, UserActionFailFn fail_callback, const std::string& input) override;
private:
    wxEventType   m_succ_evt_type;
    wxEventType   m_fail_evt_type;
};

class UserActionPost : public UserAction
{
public:
    UserActionPost(const std::string name, const std::string url) : UserAction(name, url) {}
    ~UserActionPost() {}
    void perform(wxEvtHandler* evt_handler, const std::string& access_token, UserActionSuccessFn success_callback, UserActionFailFn fail_callback, const std::string& input) override;
};

class DummyUserAction : public UserAction
{
public:
    DummyUserAction() : UserAction("Dummy", {}) {}
    ~DummyUserAction() {}
    void perform(wxEvtHandler* evt_handler, const std::string& access_token, UserActionSuccessFn success_callback, UserActionFailFn fail_callback, const std::string& input) override { }
};

struct ActionQueueData
{
    UserActionID            action_id;
    UserActionSuccessFn     success_callback;
    UserActionFailFn        fail_callback;
    std::string             input;
};

class AuthSession
{
public:
    AuthSession(wxEvtHandler* evt_handler, const std::string& access_token, const std::string& refresh_token, const std::string& shared_session_key, bool polling_enabled)
        : p_evt_handler(evt_handler)
        , m_access_token(access_token)
        , m_refresh_token(refresh_token)
        , m_shared_session_key(shared_session_key)
        , m_polling_enabled(polling_enabled)
       
    {
        
        // do not forget to add delete to destructor
        m_actions[UserActionID::AUTH_ACTION_DUMMY] = std::make_unique<DummyUserAction>();
        m_actions[UserActionID::AUTH_ACTION_REFRESH_TOKEN] = std::make_unique<UserActionPost>("EXCHANGE_TOKENS", "https://test-account.prusa3d.com/o/token/");
        m_actions[UserActionID::AUTH_ACTION_CODE_FOR_TOKEN] = std::make_unique<UserActionPost>("EXCHANGE_TOKENS", "https://test-account.prusa3d.com/o/token/");
        m_actions[UserActionID::AUTH_ACTION_USER_ID] = std::make_unique<UserActionGetWithEvent>("USER_ID", "https://test-account.prusa3d.com/api/v1/me/", EVT_PA_ID_USER_SUCCESS, EVT_PRUSAAUTH_RESET);
        m_actions[UserActionID::AUTH_ACTION_TEST_ACCESS_TOKEN] = std::make_unique<UserActionGetWithEvent>("TEST_ACCESS_TOKEN", "https://test-account.prusa3d.com/api/v1/me/", EVT_PA_ID_USER_SUCCESS, EVT_PRUSAAUTH_FAIL);
        m_actions[UserActionID::AUTH_ACTION_TEST_CONNECTION] = std::make_unique<UserActionGetWithEvent>("TEST_CONNECTION", "https://test-account.prusa3d.com/api/v1/me/", wxEVT_NULL, EVT_PRUSAAUTH_RESET);
        m_actions[UserActionID::AUTH_ACTION_CONNECT_DUMMY] = std::make_unique<UserActionGetWithEvent>("CONNECT_DUMMY", "https://dev.connect.prusa3d.com/slicer/dummy"/*"dev.connect.prusa:8000/slicer/dummy"*/, EVT_PRUSAAUTH_SUCCESS, EVT_PRUSAAUTH_FAIL);
        m_actions[UserActionID::AUTH_ACTION_CONNECT_PRINTERS] = std::make_unique<UserActionGetWithEvent>("CONNECT_PRINTERS", "https://dev.connect.prusa3d.com/slicer/printers"/*"dev.connect.prusa:8000/slicer/printers"*/, EVT_PRUSACONNECT_PRINTERS_SUCCESS, EVT_PRUSAAUTH_FAIL);
        m_actions[UserActionID::AUTH_ACTION_AVATAR] = std::make_unique<UserActionGetWithEvent>("AVATAR", "https://test-media.printables.com/media/", EVT_PA_AVATAR_SUCCESS, EVT_PA_AVATAR_FAIL);
    }
    ~AuthSession()
    {
        m_actions[UserActionID::AUTH_ACTION_DUMMY].reset(nullptr);
        m_actions[UserActionID::AUTH_ACTION_REFRESH_TOKEN].reset(nullptr);
        m_actions[UserActionID::AUTH_ACTION_CODE_FOR_TOKEN].reset(nullptr);
        m_actions[UserActionID::AUTH_ACTION_USER_ID].reset(nullptr);
        m_actions[UserActionID::AUTH_ACTION_TEST_ACCESS_TOKEN].reset(nullptr);
        m_actions[UserActionID::AUTH_ACTION_TEST_CONNECTION].reset(nullptr);
        m_actions[UserActionID::AUTH_ACTION_CONNECT_DUMMY].reset(nullptr);
        m_actions[UserActionID::AUTH_ACTION_CONNECT_PRINTERS].reset(nullptr);
        m_actions[UserActionID::AUTH_ACTION_AVATAR].reset(nullptr);
        //assert(m_actions.empty());
    }
    void clear() {
        m_access_token.clear();
        m_refresh_token.clear();
        m_shared_session_key.clear();
        m_proccessing_enabled = false;
    }

    // Functions that automatically enable action queu processing
    void init_with_code(const std::string& code, const std::string& code_verifier);
    void enqueue_action(UserActionID id, UserActionSuccessFn success_callback, UserActionFailFn fail_callback, const std::string& input);
    // Special enques, that sets callbacks.
    void enqueue_test_with_refresh();

    void process_action_queue();
    bool is_initialized() { return !m_access_token.empty() || !m_refresh_token.empty(); }
    std::string get_access_token() const { return m_access_token; }
    std::string get_refresh_token() const { return m_refresh_token; }
    std::string get_shared_session_key() const { return m_shared_session_key; }
    
    void set_polling_enabled(bool enabled) {m_polling_enabled = enabled; }
private:
    
    void enqueue_refresh(const std::string& body);
    void refresh_fail_callback(const std::string& body);
    void cancel_queue();
    void code_exchange_fail_callback(const std::string& body);
    void token_success_callback(const std::string& body);
    std::string client_id() const { return "UfTRUm5QjWwaQEGpWQBHGHO3reAyuzgOdBaiqO52"; }

    // false prevents action queu to be processed - no communication is done
    // sets to true by init_with_code or enqueue_action call
    bool        m_proccessing_enabled {false}; 
    // triggers CONNECT_PRINTERS action when woken up on idle
    bool        m_polling_enabled;

    std::string m_access_token;
    std::string m_refresh_token;
    std::string m_shared_session_key;

    std::queue<ActionQueueData>                             m_action_queue;
    std::queue<ActionQueueData>                             m_priority_action_queue;
    std::map<UserActionID, std::unique_ptr<UserAction>>     m_actions;

    wxEvtHandler* p_evt_handler;
};

}
}
#endif