#include "utils.h"

#include <arpa/inet.h>
#include <conf/valt_config.h>
#include <fcntl.h>
#include <types/types.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <sstream>

#include "exceptions.h"

using RuntimeError = std::runtime_error;

namespace utils {

std::string to_lower(const std::string& str) {
    std::string out(str);
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return out;
}

Operation to_operation(const std::string& s) {
    auto it = op_mapping.find(utils::to_lower(s));
    if (it == op_mapping.end()) {
        throw InvalidCommandException();
    }
    return it->second;
}

std::vector<std::string> split(const std::string& s, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::stringstream ss(s);

    while (std::getline(ss, token, delimiter)) {
        tokens.push_back(token);
    }

    return tokens;
}

uint64_t seconds_since_epoch() {
    auto now = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
}

void set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) throw RuntimeError("set nonblocking failed");
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) throw RuntimeError("set nonblocking failed");
}

std::string escape_string(const std::string& s) {
    std::string result;
    for (char c : s) {
        switch (c) {
            case '\n':
                result += "\\n";
                break;
            case '\t':
                result += "\\t";
                break;
            case '\r':
                result += "\\r";
                break;
            case '\\':
                result += "\\\\";
                break;
            case '\"':
                result += "\\\"";
                break;
            default:
                result += c;
                break;
        }
    }
    return result;
}

int init_server(int port, int max_pending_connections) {
    struct sockaddr_in address;
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        throw std::runtime_error("failed to create socket");
    }

    utils::set_nonblocking(server_fd);

    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;          // IPv4
    address.sin_addr.s_addr = INADDR_ANY;  // Bind to all interfaces
    address.sin_port = htons(port);

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        close(server_fd);
        throw std::runtime_error("setsockopt failed");
    }

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) == -1) {
        close(server_fd);
        throw std::runtime_error("bind failed");
    }

    if (listen(server_fd, max_pending_connections) == -1) {
        close(server_fd);
        throw std::runtime_error("listen failed");
    }

    return server_fd;
}

SSL_CTX* create_ssl_context(const std::string& cert_path, const std::string& key_path) {
    SSL_CTX* ctx = SSL_CTX_new(TLS_server_method());
    if (!ctx) {
        ERR_print_errors_fp(stderr);
        return nullptr;
    }

    if (SSL_CTX_use_certificate_file(ctx, cert_path.c_str(), SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        SSL_CTX_free(ctx);
        return nullptr;
    }

    if (SSL_CTX_use_PrivateKey_file(ctx, key_path.c_str(), SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        SSL_CTX_free(ctx);
        return nullptr;
    }

    return ctx;
}

}  // namespace utils
