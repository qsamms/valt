#pragma once

#include <CLI/CLI.hpp>
#include <string>

struct ValtConfig {
    bool no_auth = false;
    int port = 1738;
    int tls_port = 6767;
    std::string cert_path = "";
    std::string private_key = "";
    int max_connections = 10000;
    int max_pending_connections = 10;
};
