#ifndef slic3r_UserAccount_hpp_
#define slic3r_UserAccount_hpp_

#include "Auth.hpp"
#include "libslic3r/AppConfig.hpp"

#include <string>
#include <memory>

namespace Slic3r{
namespace GUI{

enum class ConnectPrinterState {
    CONNECT_PRINTER_OFFLINE,
    CONNECT_PRINTER_PRINTING,
    CONNECT_PRINTER_PAUSED,//?
    CONNECT_PRINTER_STOPED,//?
    CONNECT_PRINTER_IDLE,
    CONNECT_PRINTER_FINISHED,
    CONNECT_PRINTER_READY, //?
    CONNECT_PRINTER_STATE_COUNT
};

typedef std::map<std::string, std::vector<size_t>> ConnectPrinterStateMap;
// Class UserAccount should handle every request for entities outside PrusaSlicer like PrusaAuth or PrusaConnect.
// Outside communication is implemented in class PrusaAuthCommunication in Auth.hpp.
// All incoming data shoud be stored in UserAccount.
class UserAccount {
public:
    UserAccount(wxEvtHandler* evt_handler, Slic3r::AppConfig* app_config);
    ~UserAccount();

    bool is_logged();
    void do_login();
    void do_logout();

    void set_remember_session(bool remember);

#if 0
    void enqueue_user_id_action();
    void enqueue_connect_dummy_action();
#endif
    void enqueue_connect_printers_action();

    void on_login_code_recieved(const std::string& url_message);
    std::string on_user_id_success(const std::string data, AppConfig* app_config);
    void on_communication_fail(const std::string data, AppConfig* app_config);
    void on_logout(AppConfig* app_config);
    std::string on_connect_printers_success(const std::string data, AppConfig* app_config);

    std::string get_username() const { return m_username; }
    std::string get_access_token();
    const ConnectPrinterStateMap& get_printer_state_map() const { return m_printer_map; }

    // standalone utility methods
    std::string get_model_from_json(const std::string& message) const;
    std::string get_nozzle_from_json(const std::string& message) const;
private:
    void set_username(const std::string& username, AppConfig* app_config);

    std::unique_ptr<Slic3r::GUI::PrusaAuthCommunication> m_auth_communication;
    
    std::string             m_username;
    ConnectPrinterStateMap  m_printer_map;

    const std::map<std::string, std::string> printer_type_and_name_table = {
        {"1.3.0", "MK3"         },
        {"1.3.1", "MK3S"        },
        {"1.4.0", "MK4"         },
        {"2.1.0", "MINI"        },
        // ysFIXME : needs to add Connect ids for next printers
        {"0.0.0", "MK4IS"       },
        {"0.0.0", "MK3SMMU2S"   },
        {"0.0.0", "MK3MMU2"     },
        {"0.0.0", "XL"          },
        {"0.0.0", "MK2.5S"      },
        {"0.0.0", "MK2.5"       },
        {"0.0.0", "MK2.5SMMU2S" },
        {"0.0.0", "MK2.5MMU2"   },
        {"0.0.0", "MK2S"        },
        {"0.0.0", "MK2SMM"      },
        {"0.0.0", "SL1"         },
        {"0.0.0", "SL1S"        },
    };

    const std::map<std::string, ConnectPrinterState> printer_state_table = {
        {"OFFLINE"  , ConnectPrinterState::CONNECT_PRINTER_OFFLINE},
        {"PRINTING" , ConnectPrinterState::CONNECT_PRINTER_PRINTING},
        {"PAUSED"   , ConnectPrinterState::CONNECT_PRINTER_PAUSED},
        {"STOPED"   , ConnectPrinterState::CONNECT_PRINTER_STOPED},
        {"IDLE"     , ConnectPrinterState::CONNECT_PRINTER_IDLE},
        {"FINISHED" , ConnectPrinterState::CONNECT_PRINTER_FINISHED},
        {"READY"    , ConnectPrinterState::CONNECT_PRINTER_READY},
    };
};
}} // namespace slic3r::GUI
#endif // slic3r_UserAccount_hpp_
