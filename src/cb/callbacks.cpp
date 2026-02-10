#include "callbacks.h"

#include <arpa/inet.h>
#include <rh/request_handler.h>
#include <utils/utils.h>

#include "spdlog/spdlog.h"

namespace {

struct WatcherData {
    std::string buffer;
    uint32_t total_bytes;
};

enum class ConnectionPolicy {
    CLOSE,
    KEEPALIVE,
};

void cleanup_watcher(EV_P_ ev_io* watcher, const ConnectionPolicy& policy) {
    ev_io_stop(EV_A_ watcher);
    if (policy == ConnectionPolicy::CLOSE) close(watcher->fd);
    if (watcher->data != nullptr) delete watcher->data;
    delete watcher;
}

void client_write_cb(EV_P_ ev_io* watcher, int revents) {
    if (revents & EV_WRITE) {
        if (watcher->data == nullptr) {
            cleanup_watcher(EV_A_ watcher, ConnectionPolicy::KEEPALIVE);
            return;
        }

        WatcherData* data = static_cast<WatcherData*>(watcher->data);
        int bytes_sent = send(watcher->fd, data->buffer.c_str(), data->buffer.size(), 0);
        if (bytes_sent <= 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) return;
            cleanup_watcher(EV_A_ watcher, ConnectionPolicy::CLOSE);
            return;
        }

        data->buffer = data->buffer.substr(bytes_sent);
        if (data->buffer.empty()) {
            cleanup_watcher(EV_A_ watcher, ConnectionPolicy::KEEPALIVE);
        }
    }
}

void client_read_cb(EV_P_ ev_io* watcher, int revents) {
    char buffer[1024];

    if (revents & EV_READ) {
        int bytes_read = recv(watcher->fd, buffer, sizeof(buffer), 0);
        if (bytes_read <= 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) return;
            cleanup_watcher(EV_A_ watcher, ConnectionPolicy::CLOSE);
            return;
        }

        if (watcher->data == nullptr) {
            WatcherData* rd;
            if (bytes_read >= 4) {
                uint32_t message_length = 0;
                memcpy(reinterpret_cast<char*>(&message_length), buffer, 4);
                message_length = ntohl(message_length);
                std::string message = std::string(buffer + 4, bytes_read - 4);
                rd = new WatcherData{.buffer = message, .total_bytes = message_length};
            } else {
                rd = new WatcherData{.buffer = std::string(buffer, bytes_read), .total_bytes = 0};
            }
            watcher->data = rd;
        } else {
            WatcherData* rd = static_cast<WatcherData*>(watcher->data);
            std::string& read_buffer = rd->buffer;

            if (rd->total_bytes == 0) {
                // We still don't know the data length
                uint32_t total_bytes_so_far = read_buffer.size() + bytes_read;
                if (total_bytes_so_far >= 4) {
                    uint32_t message_length = 0;
                    memcpy(reinterpret_cast<char*>(&message_length), read_buffer.c_str(),
                           read_buffer.size());
                    memcpy(reinterpret_cast<char*>(&message_length) + read_buffer.size(), buffer,
                           4 - read_buffer.size());
                    message_length = ntohl(message_length);
                    rd->total_bytes = message_length;

                    uint32_t message_bytes = total_bytes_so_far - 4;
                    if (message_bytes > 0) {
                        uint32_t offset = bytes_read - message_bytes;
                        rd->buffer = std::string(buffer + offset, bytes_read - offset);
                    }
                } else {
                    rd->buffer = std::string(read_buffer + std::string(buffer, bytes_read));
                }
            } else {
                // We know the data length we just haven't received it all yet
                if (read_buffer.size() + bytes_read >= rd->total_bytes) {
                    rd->buffer = std::string(
                        read_buffer + std::string(buffer, rd->total_bytes - read_buffer.size()));
                } else {
                    rd->buffer = std::string(read_buffer + std::string(buffer, bytes_read));
                }
            }
        }

        WatcherData* rd = static_cast<WatcherData*>(watcher->data);
        std::string read_buffer = rd->buffer;
        if (rd->total_bytes == read_buffer.size()) {
            RequestHandler& request_handler = RequestHandler::getInstance();
            std::string response = request_handler.execute(read_buffer, watcher->fd);
            create_write_watcher(watcher->fd, response);
            watcher->data = nullptr;
        }
    }
}

void accept_connection_cb(EV_P_ ev_io* watcher, int revents) {
    if (revents & EV_READ) {
        struct sockaddr_in address;
        socklen_t addrlen = sizeof(address);
        int client_fd = accept(watcher->fd, (struct sockaddr*)&address, &addrlen);
        if (client_fd == -1) {
            spdlog::debug("Failed to accept connection");
            return;
        }
        utils::set_nonblocking(client_fd);

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &address.sin_addr, client_ip, sizeof(client_ip));
        int client_port = ntohs(address.sin_port);
        spdlog::debug("Accepted connection from: {}", client_ip);

        create_read_watcher(client_fd, nullptr);
    }
}

}  // namespace

void create_read_watcher(int fd, void* data) {
    ev_io* watcher = new ev_io{};
    watcher->data = data;
    ev_io_init(watcher, client_read_cb, fd, EV_READ);
    ev_io_start(EV_DEFAULT, watcher);
}

void create_write_watcher(int fd, const std::string& response) {
    ev_io* watcher = new ev_io{};

    uint32_t response_size = htonl(response.size());
    char length_prefix[4];
    memcpy(length_prefix, &response_size, 4);

    WatcherData* wd = new WatcherData{
        .buffer = std::string(std::string(length_prefix, 4) + response), .total_bytes = 0};
    watcher->data = wd;
    ev_io_init(watcher, client_write_cb, fd, EV_WRITE);
    ev_io_start(EV_DEFAULT, watcher);
}

void create_accept_watcher(const int& fd) {
    ev_io* watcher = new ev_io{};
    ev_io_init(watcher, accept_connection_cb, fd, EV_READ);
    ev_io_start(EV_DEFAULT, watcher);
}
