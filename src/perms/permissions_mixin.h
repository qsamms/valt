#include <types/types.h>
#include <valt/valt_config.h>

#include <unordered_map>

class PermissionsMixin {
   private:
    const ValtConfig* valt_config;
    std::string master_key;
    bool authentication_enabled;
    std::unordered_map<int, Connection> conns;

   public:
    PermissionsMixin() = delete;
    PermissionsMixin(const ValtConfig* cfg);

    std::string authenticate(const Request& req);

    void disable_authentication();
    void validate_authenticated(const Request& req);
    void create_session(const int& client_fd);
    void end_session(const int& client_fd);

    void set_client_mode(const int& client_fd, const SessionMode& mode);
    void validate_session_mode(const Request& req);
};
