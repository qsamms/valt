#pragma once

#include <server/server.h>
#include <utils/types.h>

#include <cstdint>
#include <regex>
#include <string>
#include <vector>

class Connection {
   private:
    ConnectionInfo* connection_info_;
    int client_fd_;

    Request parse_request(const std::string& req_str);
    int64_t parse_expiration(const std::string& expiration_str);
    std::string perform_operation(const Request& req);

   public:
    void handle_connection(const uint32_t client_fd);

    Connection(int client_fd, ConnectionInfo* connection_info);
    ~Connection();
};