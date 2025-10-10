#include "connection.h"

#include <arpa/inet.h>
#include <db/db.h>
#include <unistd.h>
#include <utils/exceptions.h>
#include <utils/response_codes.h>
#include <utils/utils.h>

#include <chrono>
#include <iostream>

int64_t Connection::parse_expiration(const std::string& expiration_str) {
    if (!std::regex_match(expiration_str, int_re)) {
        throw InvalidCommandException("expiration must be an integer");
    }

    int expiration_seconds = std::stoi(expiration_str);
    if (expiration_seconds < 0) {
        throw InvalidCommandException("expiration must be > 0");
    }

    auto now = std::chrono::system_clock::now();
    auto seconds_since_epoch =
        std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
    return (int64_t)seconds_since_epoch + expiration_seconds;
}

Request Connection::parse_request(const std::string& req_str) {
    std::vector<std::string> req_parts = split(req_str, ' ');
    if (req_parts.size() < 2) {
        throw InvalidCommandException("All commands require an action and key");
    }

    Operation operation = string_to_op(req_parts[0]);
    switch (operation) {
        case Operation::SET:
            if (req_parts.size() != 3) throw InvalidCommandException("'set' must have 3 operands");
            break;
        case Operation::SETEX:
            if (req_parts.size() != 4)
                throw InvalidCommandException("'setex' must have 4 operands");
            break;
        case Operation::EXPIRE:
            if (req_parts.size() != 3)
                throw InvalidCommandException("'expire' must have 3 operands");
            break;
    }

    return Request{.action = string_to_op(req_parts[0]),
                   .key = req_parts[1],
                   .value = req_parts.size() > 2 ? req_parts[2] : "",
                   .expiration = req_parts.size() > 3 ? parse_expiration(req_parts[3]) : -1

    };
}

std::string Connection::perform_operation(const Request& req) {
    switch (req.action) {
        case Operation::GET: {
            std::optional<DBEntry> entry = get(req.key);
            return entry ? entry->value + "\n" : ERR_NOT_FOUND;
        }
        case Operation::SET:
            return set(req) ? OK : ERR_UNKNOWN;
        case Operation::SETEX:
            return set(req) ? OK : ERR_UNKNOWN;
        case Operation::DELETE:
            return del(req.key) ? OK : ERR_UNKNOWN;
        case Operation::EXPIRE:
            return expire(req) ? OK : ERR_UNKNOWN;
        case Operation::PERSIST:
            return persist(req) ? OK : ERR_UNKNOWN;
        default:
            throw InvalidCommandException("unknown action");
    }
}

void Connection::handle_connection(const uint32_t client_fd) {
    char buffer[1024];
    while (1) {
        try {
            int bytes_read = recv(client_fd, buffer, sizeof(buffer), 0);
            if (bytes_read <= 0) break;

            uint16_t req_size = bytes_read;
            if (buffer[bytes_read - 1] == '\n') req_size--;

            Request req = parse_request(std::string(buffer, req_size));
            std::string response = perform_operation(req);
            send(client_fd, response.c_str(), response.size(), 0);

        } catch (InvalidCommandException& e) {
            send(client_fd, e.what(), e.get_message_size(), 0);
        } catch (std::exception& e) {
            send(client_fd, ERR_UNKNOWN.c_str(), ERR_UNKNOWN.size(), 0);
        }
    }
}

Connection::Connection(int client_fd, ConnectionInfo* connection_info) {
    connection_info_ = connection_info;
    client_fd_ = client_fd;
    handle_connection(client_fd);
}

Connection::~Connection() {
    std::lock_guard<std::mutex> lck(connection_info_->mutex);
    connection_info_->open_connections--;
    close(client_fd_);
}