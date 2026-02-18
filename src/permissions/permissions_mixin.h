#include <types/types.h>

#include <unordered_map>

class PermissionsMixin {
   private:
    std::string master_key;
    bool authentication_enabled;
    std::unordered_map<int, Connection> conns;

   public:
    PermissionsMixin();

    std::string authenticate(const Request& req);

    void disable_authentication();
    void validate_authenticated(const Request& req);
    void create_session(int client_fd);
    void end_session(int client_fd);
};
