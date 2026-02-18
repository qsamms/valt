#include "permissions_mixin.h"

#include <utils/exceptions.h>

PermissionsMixin::PermissionsMixin() {
    authentication_enabled = true;
    conns = std::unordered_map<int, Connection>();
    char* key = std::getenv("VALT_MASTER_KEY");
    if (key != nullptr) {
        master_key = std::string(key);
    }
};

void PermissionsMixin::disable_authentication() {
    authentication_enabled = false;
}

void PermissionsMixin::create_session(int client_fd) {
    if (!conns.contains(client_fd)) {
        conns.emplace(client_fd, Connection{.authenticated = false});
    }
}

void PermissionsMixin::end_session(int client_fd) {
    if (conns.contains(client_fd)) {
        conns.erase(client_fd);
    }
}

void PermissionsMixin::validate_authenticated(const Request& req) {
    if (authentication_enabled && req.op != Operation::AUTHENTICATE &&
        (!conns.contains(req.client_fd) || !conns[req.client_fd].authenticated)) {
        throw UnauthenticatedException();
    }
}

std::string PermissionsMixin::authenticate(const Request& req) {
    if (req.value == master_key) {
        conns[req.client_fd].authenticated = true;
        return "OK";
    }
    return "ERR_AUTHENTICATION_FAILED";
}
