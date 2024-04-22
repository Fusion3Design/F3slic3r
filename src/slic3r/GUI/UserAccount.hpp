#ifndef slic3r_UserAccount_hpp_
#define slic3r_UserAccount_hpp_

#include "UserAccountCommunication.hpp"
#include "libslic3r/AppConfig.hpp"

#include <string>
#include <memory>
#include <boost/filesystem.hpp>

namespace Slic3r{
namespace GUI{

enum class ConnectPrinterState {
    CONNECT_PRINTER_OFFLINE,
    CONNECT_PRINTER_PRINTING,
    CONNECT_PRINTER_PAUSED,//?
    CONNECT_PRINTER_STOPPED,//?
    CONNECT_PRINTER_IDLE,
    CONNECT_PRINTER_FINISHED,
    CONNECT_PRINTER_READY, //?
    CONNECT_PRINTER_ATTENTION,
    CONNECT_PRINTER_BUSY,
    CONNECT_PRINTER_ERROR,
    CONNECT_PRINTER_STATE_COUNT
};

typedef std::map<std::string, std::vector<size_t>> ConnectPrinterStateMap;
typedef std::map<std::string, std::string> ConnectUUIDToModelMap;
// Class UserAccount should handle every request for entities outside PrusaSlicer like PrusaAuth or PrusaConnect.
// Outside communication is implemented in class UserAccountCommunication that runs separate thread. Results come back in events to Plater.
// All incoming data shoud be stored in UserAccount.
class UserAccount {
public:
    UserAccount(wxEvtHandler* evt_handler, Slic3r::AppConfig* app_config, const std::string& instance_hash);
    ~UserAccount();

    bool is_logged();
    void do_login();
    void do_logout();

    void set_remember_session(bool remember);
    void toggle_remember_session();
    bool get_remember_session();
    void enqueue_connect_status_action();
    void enqueue_connect_printer_models_action();
    void enqueue_avatar_action();

    // Clears all data and connections, called on logout or EVT_UA_RESET
    void clear();

    // Functions called from UI where events emmited from UserAccountSession are binded
    // Returns bool if data were correctly proccessed
    bool on_login_code_recieved(const std::string& url_message);
    bool on_user_id_success(const std::string data, std::string& out_username);
    // Called on EVT_UA_FAIL, triggers test after several calls
    void on_communication_fail();
    bool on_connect_printers_success(const std::string& data, AppConfig* app_config, bool& out_printers_changed);
    bool on_connect_uiid_map_success(const std::string& data, AppConfig* app_config, bool& out_printers_changed);

    std::string get_username() const { return m_username; }
    std::string get_access_token();
    std::string get_shared_session_key();
    const ConnectPrinterStateMap& get_printer_state_map() const { return m_printer_map; }
    boost::filesystem::path get_avatar_path(bool logged) const;

    // standalone utility methods
    std::string get_model_from_json(const std::string& message) const;
    std::string get_nozzle_from_json(const std::string& message) const;
    std::string get_keyword_from_json(const std::string& json, const std::string& keyword) const;
    void fill_compatible_printers_from_json(const std::string& json, std::vector<std::string>& result) const;
    void fill_compatible_printers_from_json_old(const std::string& json, std::vector<std::string>& result) const;
    void fill_material_from_json(const std::string& json, std::vector<std::string>& result) const;

    const std::map<std::string, ConnectPrinterState>& get_printer_state_table() const { return printer_state_table; }

    std::string get_printer_type_from_name(const std::string& printer_name) const;
private:
    void set_username(const std::string& username);
   
    std::string m_instance_hash; // used in avatar path

    std::unique_ptr<Slic3r::GUI::UserAccountCommunication> m_communication;
    
    ConnectPrinterStateMap              m_printer_map;
    ConnectUUIDToModelMap               m_printer_uuid_map;
    std::map<std::string, std::string>  m_account_user_data;
    std::string                         m_username;
    size_t                              m_fail_counter { 0 };
    std::string                         m_avatar_extension;

    // first string is "printer_type" code from Connect edpoints
    const std::map<std::string, std::string> printer_type_and_name_table = {
        {"1.2.5", "MK2.5"       },
        {"1.2.6", "MK2.5S"      },
        {"1.3.0", "MK3"         },
        {"1.3.1", "MK3S"        },
        {"1.3.5", "MK3.5"       },
        {"1.3.9", "MK3.9"       },
        {"1.4.0", "MK4"         },
        {"2.1.0", "MINI"        },
        {"3.1.0", "XL"          },
        {"5.1.0", "SL1"         },
        {"5.1.1", "SL1S"        },
        // ysFIXME : needs to add Connect ids for next printers
        /*{"0.0.0", "MK4IS"       },
        {"0.0.0", "MK3SMMU2S"   },
        {"0.0.0", "MK3MMU2"     },
        {"0.0.0", "MK2.5SMMU2S" },
        {"0.0.0", "MK2.5MMU2"   },
        {"0.0.0", "MK2S"        },
        {"0.0.0", "MK2SMM"      },
        {"0.0.0", "SL1S"        },*/
    };
    /* TODO: 
        4	1	0	iXL
        6	2	0	Trilab DeltiQ 2
        6	2	1	Trilab DelriQ 2 Plus
        7	1	0	Trilab AzteQ
        7	2	0	Trilab AzteQ Industrial
        7	2	1	Trilab AzteQ Industrial Plus
    */

    const std::map<std::string, ConnectPrinterState> printer_state_table = {
        {"OFFLINE"  , ConnectPrinterState::CONNECT_PRINTER_OFFLINE},
        {"PRINTING" , ConnectPrinterState::CONNECT_PRINTER_PRINTING},
        {"PAUSED"   , ConnectPrinterState::CONNECT_PRINTER_PAUSED},
        {"STOPPED"  , ConnectPrinterState::CONNECT_PRINTER_STOPPED},
        {"IDLE"     , ConnectPrinterState::CONNECT_PRINTER_IDLE},
        {"FINISHED" , ConnectPrinterState::CONNECT_PRINTER_FINISHED},
        {"READY"    , ConnectPrinterState::CONNECT_PRINTER_READY},
        {"ATTENTION", ConnectPrinterState::CONNECT_PRINTER_ATTENTION},
        {"BUSY"     , ConnectPrinterState::CONNECT_PRINTER_BUSY},
    };
};
}} // namespace slic3r::GUI
#endif // slic3r_UserAccount_hpp_
