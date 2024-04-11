#include "UserAccount.hpp"

#include "format.hpp"

#include "libslic3r/Utils.hpp"

#include <boost/regex.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/log/trivial.hpp>

#include <wx/stdpaths.h>

namespace pt = boost::property_tree;

namespace Slic3r {
namespace GUI {

UserAccount::UserAccount(wxEvtHandler* evt_handler, AppConfig* app_config, const std::string& instance_hash)
    : m_communication(std::make_unique<UserAccountCommunication>(evt_handler, app_config))
    , m_instance_hash(instance_hash)
{}

UserAccount::~UserAccount()
{}

void UserAccount::set_username(const std::string& username)
{
    m_username = username;
    m_communication->set_username(username);
}

void UserAccount::clear()
{
    m_username = {};
    m_account_user_data.clear();
    m_printer_map.clear();
    m_communication->do_clear();
}

void UserAccount::set_remember_session(bool remember)
{
    m_communication->set_remember_session(remember);
}
void UserAccount::toggle_remember_session()
{
    m_communication->set_remember_session(!m_communication->get_remember_session());
}
bool UserAccount::get_remember_session()
{
    return m_communication->get_remember_session();
}

bool UserAccount::is_logged()
{
    return m_communication->is_logged();
}
void UserAccount::do_login()
{
    m_communication->do_login();
}
void UserAccount::do_logout()
{
    m_communication->do_logout();
}

std::string UserAccount::get_access_token()
{
    return m_communication->get_access_token();
}

std::string UserAccount::get_shared_session_key()
{
    return m_communication->get_shared_session_key();
}

boost::filesystem::path UserAccount::get_avatar_path(bool logged) const
{
    if (logged) {
        const std::string filename = "prusaslicer-avatar-" + m_instance_hash + m_avatar_extension;
        return boost::filesystem::path(wxStandardPaths::Get().GetTempDir().utf8_str().data()) / filename;
    } else {
        return  boost::filesystem::path(resources_dir()) / "icons" / "user.svg";
    }
}


void UserAccount::enqueue_connect_printers_action()
{
    m_communication->enqueue_connect_printers_action();
}
void UserAccount::enqueue_avatar_action()
{
    m_communication->enqueue_avatar_action(m_account_user_data["avatar"]);
}

bool UserAccount::on_login_code_recieved(const std::string& url_message)
{
    m_communication->on_login_code_recieved(url_message);
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
    m_account_user_data.clear();
    for (const auto& section : ptree) {
        const auto opt = ptree.get_optional<std::string>(section.first);
        if (opt) {
            BOOST_LOG_TRIVIAL(debug) << static_cast<std::string>(section.first) << "    " << *opt;
            m_account_user_data[section.first] = *opt;
        }
       
    }
    if (m_account_user_data.find("public_username") == m_account_user_data.end()) {
        BOOST_LOG_TRIVIAL(error) << "User ID message from PrusaAuth did not contain public_username. Login failed. Message data: " << data;
        return false;
    }
    std::string public_username = m_account_user_data["public_username"];
    set_username(public_username);
    out_username = public_username;
    // equeue GET with avatar url
    if (m_account_user_data.find("avatar") != m_account_user_data.end()) {
        const boost::filesystem::path server_file(m_account_user_data["avatar"]);
        m_avatar_extension = server_file.extension().string();
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
        m_communication->enqueue_test_connection();
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

    pt::ptree parse_tree_for_subtree(const pt::ptree& tree, const std::string& param)
    {
        for (const auto& section : tree) {
            if (section.first == param) {
                return section.second;
            }
            else {
                if (pt::ptree res = parse_tree_for_subtree(section.second, param); !res.empty())
                    return res;
            }

        }
        return pt::ptree();
    }
}

bool UserAccount::on_connect_printers_success(const std::string& data, AppConfig* app_config, bool& out_printers_changed)
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
    // tree string is in format {"result": [{"printer_type": "1.2.3", "states": [{"printer_state": "OFFLINE", "count": 1}, ...]}, {..}]}

    ConnectPrinterStateMap new_printer_map;

    assert(ptree.front().first == "result");
    for (const auto& printer_tree : ptree.front().second) {
        // printer_tree is {"printer_type": "1.2.3", "states": [..]}
        std::string name;
        ConnectPrinterState state;

        const auto type_opt = printer_tree.second.get_optional<std::string>("printer_model");
        if (!type_opt) {
            continue;
        }
        // printer_type is actually printer_name for now
        name = *type_opt;
        // printer should not appear twice
        assert(new_printer_map.find(name) == new_printer_map.end());
        // prepare all states on 0
        new_printer_map[name].reserve(static_cast<size_t>(ConnectPrinterState::CONNECT_PRINTER_STATE_COUNT));
        for (size_t i = 0; i < static_cast<size_t>(ConnectPrinterState::CONNECT_PRINTER_STATE_COUNT); i++) {
            new_printer_map[name].push_back(0);
        }

        for (const auto& section : printer_tree.second) {
            // section is "printer_type": "1.2.3" OR "states": [..]}
            if (section.first == "states") {
                for (const auto& subsection : section.second) {
                    // subsection is {"printer_state": "OFFLINE", "count": 1}
                    const auto state_opt = subsection.second.get_optional<std::string>("printer_state");
                    const auto count_opt = subsection.second.get_optional<int>("count");
                    if (!state_opt || ! count_opt) {
                        continue;
                    }
                    if (auto pair = printer_state_table.find(*state_opt); pair != printer_state_table.end()) {
                        state = pair->second;
                    } else {
                        assert(true); // On this assert, printer_state_table needs to be updated with *state_opt and correct ConnectPrinterState
                        continue;
                    }
                    new_printer_map[name][static_cast<size_t>(state)] = *count_opt;
                }
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

std::string UserAccount::get_keyword_from_json(const std::string& json, const std::string& keyword) const
{
    std::string out;
    try {
        std::stringstream ss(json);
        pt::ptree ptree;
        pt::read_json(ss, ptree);

        out = parse_tree_for_param(ptree, keyword);
        //assert(!out.empty());
    }
    catch (const std::exception& e) {
        BOOST_LOG_TRIVIAL(error) << "Could not parse prusaconnect message. " << e.what();
    }
    return out;
}

void UserAccount::fill_compatible_printers_from_json(const std::string& json, std::vector<std::string>& result) const
{
    try {
        std::stringstream ss(json);
        pt::ptree ptree;
        pt::read_json(ss, ptree);

        pt::ptree out = parse_tree_for_subtree(ptree, "printer_type_compatible");
        if (out.empty()) {
            BOOST_LOG_TRIVIAL(error) << "Failed to find compatible_printer_type in printer detail.";
            return;
        }
        for (const auto& sub : out) {
            result.emplace_back(sub.second.data());
        }
    }
    catch (const std::exception& e) {
        BOOST_LOG_TRIVIAL(error) << "Could not parse prusaconnect message. " << e.what();
    }
}


std::string UserAccount::get_printer_type_from_name(const std::string& printer_name) const
{
    
    for (const auto& pair : printer_type_and_name_table) {
        if (pair.second == printer_name) {
            return pair.first;
        }
    }
    assert(true); // This assert means printer_type_and_name_table needs a update
    return {};
}


}} // namespace slic3r::GUI