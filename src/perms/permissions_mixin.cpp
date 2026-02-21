#include "permissions_mixin.h"

#include <net/network_layer.h>
#include <utils/exceptions.h>

PermissionsMixin::PermissionsMixin(const ValtConfig* cfg) : valt_config(cfg), authentication_enabled(true) {
    if (char* key = std::getenv("VALT_MASTER_KEY"); key != nullptr) {
        master_key = std::string(key);
    }
}

std::string PermissionsMixin::authenticate(const Request& req) {
    auto& conns = get_connections();
    if (req.value == master_key) {
        Connection* c = conns.at(req.client_fd).get();
        c->authenticated = true;
        return OK;
    }
    return "ERR_AUTHENTICATION_FAILED";
}

void PermissionsMixin::disable_authentication() {
    authentication_enabled = false;
}

void PermissionsMixin::validate_authenticated(const Request& req) {
    auto& conns = get_connections();
    Connection* c = conns.at(req.client_fd).get();

    if (authentication_enabled && req.op != Operation::AUTHENTICATE && !c->authenticated) {
        throw UnauthenticatedException();
    }
}

void PermissionsMixin::validate_session_mode(const Request& req) {
    auto& conns = get_connections();
    Connection* c = conns.at(req.client_fd).get();

    if (c->mode == SessionMode::PUBSUB && req.op != Operation::UNSUBSCRIBE) {
        throw InvalidCommandException();
    }
}

void PermissionsMixin::set_client_mode(const int& client_fd, const SessionMode& mode) {
    auto& conns = get_connections();
    Connection* c = conns.at(client_fd).get();
    c->mode = mode;
}
