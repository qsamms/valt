#pragma once

#include <openssl/err.h>
#include <openssl/ssl.h>
#include <types/types.h>

#include <string>
#include <vector>

namespace utils {

std::string to_lower(const std::string& str);

Operation to_operation(const std::string& str);

std::vector<std::string> split(const std::string& s, char delimiter);

uint64_t seconds_since_epoch();

void set_nonblocking(int client_fd);

std::string escape_string(const std::string& s);

int init_server(int port, int max_pending_connections);

SSL_CTX* create_ssl_context(const std::string& cert_path, const std::string& key_path);

}  // namespace utils
