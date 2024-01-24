#include "UserAccount.hpp"

#include "format.hpp"

#include <boost/regex.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/log/trivial.hpp>

namespace pt = boost::property_tree;

namespace Slic3r {
namespace GUI {

UserAccount::UserAccount(wxEvtHandler* evt_handler, AppConfig* app_config)
    : m_auth_communication(std::make_unique<PrusaAuthCommunication>(evt_handler, app_config))
{}

UserAccount::~UserAccount()
{}

void UserAccount::set_username(const std::string& username)
{
    m_username = username;
    m_auth_communication->set_username(username);
}

void UserAccount::clear()
{
    m_username = {};
    m_user_data.clear();
    m_printer_map.clear();
    m_auth_communication->do_clear();
}

void UserAccount::set_remember_session(bool remember)
{
    m_auth_communication->set_remember_session(remember);
}
void UserAccount::toggle_remember_session()
{
    m_auth_communication->set_remember_session(!m_auth_communication->get_remember_session());
}
bool UserAccount::get_remember_session()
{
    return m_auth_communication->get_remember_session();
}

bool UserAccount::is_logged()
{
    return m_auth_communication->is_logged();
}
void UserAccount::do_login()
{
    m_auth_communication->do_login();
}
void UserAccount::do_logout()
{
    m_auth_communication->do_logout();
}

std::string UserAccount::get_access_token()
{
    return m_auth_communication->get_access_token();
}

#if 0
void UserAccount::enqueue_user_id_action()
{
    m_auth_communication->enqueue_user_id_action();
}
void UserAccount::enqueue_connect_dummy_action()
{
    m_auth_communication->enqueue_connect_dummy_action();
}
#endif

void UserAccount::enqueue_connect_printers_action()
{
    m_auth_communication->enqueue_connect_printers_action();
}
void UserAccount::enqueue_avatar_action()
{
    m_auth_communication->enqueue_avatar_action(m_user_data["avatar"]);
}

bool UserAccount::on_login_code_recieved(const std::string& url_message)
{
    m_auth_communication->on_login_code_recieved(url_message);
    return true;
}

bool UserAccount::on_user_id_success(const std::string data, std::string& out_username)
{
    boost::property_tree::ptree ptree;
    try {
        std::stringstream ss(data);
        boost::property_tree::read_json(ss, ptree);
    }
    catch (const std::exception&) {
        BOOST_LOG_TRIVIAL(error) << "UserIDUserAction Could not parse server response.";
        return false;
    }
    m_user_data.clear();
    for (const auto& section : ptree) {
        const auto opt = ptree.get_optional<std::string>(section.first);
        if (opt) {
            BOOST_LOG_TRIVIAL(debug) << static_cast<std::string>(section.first) << "    " << *opt;
            m_user_data[section.first] = *opt;
        }
       
    }
    if (m_user_data.find("public_username") == m_user_data.end()) {
        BOOST_LOG_TRIVIAL(error) << "User ID message from PrusaAuth did not contain public_username. Login failed. Message data: " << data;
        return false;
    }
    std::string public_username = m_user_data["public_username"];
    set_username(public_username);
    out_username = public_username;
    // equeue GET with avatar url
    if (m_user_data.find("avatar") != m_user_data.end()) {
        enqueue_avatar_action();
    }
    else {
        BOOST_LOG_TRIVIAL(error) << "User ID message from PrusaAuth did not contain avatar.";
    }
    // update printers list
    enqueue_connect_printers_action();
    return true;
}

void UserAccount::on_communication_fail()
{
    m_fail_counter++;
    if (m_fail_counter > 5) // there is no deeper reason why 5
    {
        m_auth_communication->enqueue_test_connection();
        m_fail_counter = 0;
    }
}

namespace {
    std::string parse_tree_for_param(const pt::ptree& tree, const std::string& param)
    {
        for (const auto& section : tree) {
            if (section.first == param) {
                return section.second.data();
            } else {
                if (std::string res = parse_tree_for_param(section.second, param); !res.empty())
                    return res;
            }

        }
        return {};
    }
}

bool UserAccount::on_connect_printers_success(const std::string data, AppConfig* app_config, bool& out_printers_changed)
{
    BOOST_LOG_TRIVIAL(debug) << "PrusaConnect printers message: " << data;
    pt::ptree ptree;
    try {
        std::stringstream ss(data);
        pt::read_json(ss, ptree);
    }
    catch (const std::exception& e) {
        BOOST_LOG_TRIVIAL(error) << "Could not parse prusaconnect message. " << e.what();
        return false;
    }
    // fill m_printer_map with data from ptree
    // tree string is in format {"printers": [{..}, {..}]}

    ConnectPrinterStateMap new_printer_map;

    assert(ptree.front().first == "printers");
    for (const auto& printer_tree : ptree.front().second) {
        std::string name;
        ConnectPrinterState state;
        std::string type_string = parse_tree_for_param(printer_tree.second, "printer_type");
        std::string state_string = parse_tree_for_param(printer_tree.second, "connect_state");

        assert(!type_string.empty());
        assert(!state_string.empty());
        if (type_string.empty() || state_string.empty())
            continue;
        // name of printer needs to be taken from translate table, if missing
        if (auto pair = printer_type_and_name_table.find(type_string); pair != printer_type_and_name_table.end()) {
            name = pair->second;
        } else {
            assert(true); // On this assert, printer_type_and_name_table needs to be updated with type_string and correct printer name
            continue;
        }
        // translate state string to enum value
        if (auto pair = printer_state_table.find(state_string); pair != printer_state_table.end()) {
            state = pair->second;
        } else {
            assert(true); // On this assert, printer_state_table and ConnectPrinterState needs to be updated
            continue;
        }
        if (auto counter = new_printer_map.find(name); counter != new_printer_map.end()) {
            new_printer_map[name][static_cast<size_t>(state)]++;
        }  else {
            new_printer_map[name].reserve(static_cast<size_t>(ConnectPrinterState::CONNECT_PRINTER_STATE_COUNT));
            for (size_t i = 0; i < static_cast<size_t>(ConnectPrinterState::CONNECT_PRINTER_STATE_COUNT); i++) {
                new_printer_map[name].push_back(i == static_cast<size_t>(state) ? 1 : 0);
            }
        }
    }

    // compare new and old printer map and update old map into new
    out_printers_changed = false;
    for (const auto& it : new_printer_map) {
        if (m_printer_map.find(it.first) == m_printer_map.end()) {
            // printer is not in old map, add it by copying data from new map
            out_printers_changed = true;
            m_printer_map[it.first].reserve(static_cast<size_t>(ConnectPrinterState::CONNECT_PRINTER_STATE_COUNT));
            for (size_t i = 0; i < static_cast<size_t>(ConnectPrinterState::CONNECT_PRINTER_STATE_COUNT); i++) {
                m_printer_map[it.first].push_back(new_printer_map[it.first][i]);
            }
        } else {
            // printer is in old map, check state by state
            for (size_t i = 0; i < static_cast<size_t>(ConnectPrinterState::CONNECT_PRINTER_STATE_COUNT); i++) {
                if (m_printer_map[it.first][i] != new_printer_map[it.first][i])  {
                    out_printers_changed = true;
                    m_printer_map[it.first][i] = new_printer_map[it.first][i];
                }
            }
        }
    }
    return true;
}

std::string UserAccount::get_model_from_json(const std::string& message) const
{
    std::string out;
    try {
        std::stringstream ss(message);
        pt::ptree ptree;
        pt::read_json(ss, ptree);

        std::string printer_type = parse_tree_for_param(ptree, "printer_type");
        if (auto pair = printer_type_and_name_table.find(printer_type); pair != printer_type_and_name_table.end()) {
            out = pair->second;
        }
        //assert(!out.empty());
    }
    catch (const std::exception& e) {
        BOOST_LOG_TRIVIAL(error) << "Could not parse prusaconnect message. " << e.what();
    }
    return out;
}

std::string UserAccount::get_nozzle_from_json(const std::string& message) const
{
    std::string out;
    try {
        std::stringstream ss(message);
        pt::ptree ptree;
        pt::read_json(ss, ptree);

        out = parse_tree_for_param(ptree, "nozzle_diameter");
        //assert(!out.empty());
    }
    catch (const std::exception& e) {
        BOOST_LOG_TRIVIAL(error) << "Could not parse prusaconnect message. " << e.what();
    }
    return out;
}

std::string UserAccount::get_apikey_from_json(const std::string& message) const
{
    std::string out;
    try {
        std::stringstream ss(message);
        pt::ptree ptree;
        pt::read_json(ss, ptree);

        out = parse_tree_for_param(ptree, "prusaconnect_api_key");
        //assert(!out.empty());
    }
    catch (const std::exception& e) {
        BOOST_LOG_TRIVIAL(error) << "Could not parse prusaconnect message. " << e.what();
    }
    return out;
}

}} // namespace slic3r::GUI