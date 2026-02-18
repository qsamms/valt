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

std::string PermissionsMixin::authenticate(const Request& req) {
    if (req.value == master_key) {
        conns[req.client_fd].authenticated = true;
        return OK;
    }
    return "ERR_AUTHENTICATION_FAILED";
}

void PermissionsMixin::disable_authentication() {
    authentication_enabled = false;
}

void PermissionsMixin::create_session(const int& client_fd) {
    if (!conns.contains(client_fd)) {
        conns.emplace(client_fd, Connection{.authenticated = false, .mode = SessionMode::DB});
    }
}

void PermissionsMixin::end_session(const int& client_fd) {
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

void PermissionsMixin::validate_session_mode(const Request& req) {
    if (conns.contains(req.client_fd) && conns[req.client_fd].mode == SessionMode::PUBSUB &&
        req.op != Operation::UNSUBSCRIBE) {
        throw InvalidCommandException();
    }
}

void PermissionsMixin::set_client_mode(const int& client_fd, const SessionMode& mode) {
    if (conns.contains(client_fd)) {
        conns[client_fd].mode = mode;
    }
}
