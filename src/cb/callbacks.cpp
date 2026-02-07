#include "callbacks.h"

#include <arpa/inet.h>
#include <task_handler/task_handler.h>
#include <utils/utils.h>

#include "spdlog/spdlog.h"

struct ReadData {
    std::string buffer;
    uint32_t total_bytes;
};

enum class ConnectionPolicy {
    CLOSE,
    KEEPALIVE,
};

void cleanup_watcher(EV_P_ ev_io* watcher, ConnectionPolicy policy) {
    ev_io_stop(EV_A_ watcher);
    if (policy == ConnectionPolicy::CLOSE) close(watcher->fd);
    if (watcher->data != nullptr) delete (std::string*)watcher->data;
    delete watcher;
}

void client_write_cb(EV_P_ ev_io* watcher, int revents) {
    if (revents & EV_WRITE) {
        if (watcher->data == nullptr) {
            cleanup_watcher(EV_A_ watcher, ConnectionPolicy::KEEPALIVE);
            return;
        }

        std::string* content = (std::string*)watcher->data;
        int bytes_sent = send(watcher->fd, (*content).c_str(), (*content).size(), 0);
        if (bytes_sent <= 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) return;
            cleanup_watcher(EV_A_ watcher, ConnectionPolicy::CLOSE);
            return;
        }

        *content = (*content).substr(bytes_sent);
        if ((*content).empty()) {
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
            ReadData* rd;
            if (bytes_read >= 4) {
                uint32_t message_length;
                memcpy(&message_length, buffer, 4);
                message_length = ntohl(message_length);
                std::string message = std::string(buffer + 4, bytes_read - 4);
                rd = new ReadData{.buffer = message, .total_bytes = message_length};
            } else {
                rd = new ReadData{.buffer = std::string(buffer, bytes_read), .total_bytes = 0};
            }
            watcher->data = rd;
        } else {
            ReadData* rd = (ReadData*)watcher->data;
            std::string& read_buffer = rd->buffer;

            if (rd->total_bytes == 0) {
                // We still don't know the data length
                uint32_t total_bytes_so_far = read_buffer.size() + bytes_read;
                if (total_bytes_so_far >= 4) {
                    uint32_t message_length;
                    memcpy(&message_length, read_buffer.c_str(), read_buffer.size());
                    memcpy(&message_length + read_buffer.size(), buffer, 4 - read_buffer.size());
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

        ReadData* rd = (ReadData*)watcher->data;
        std::string read_buffer = rd->buffer;

        if (rd->total_bytes == read_buffer.size()) {
            TaskHandler& th = TaskHandler::get_instance();
            std::string response = th.handle_task(read_buffer);
            uint32_t length_prefix = response.size();
            length_prefix = htonl(length_prefix);

            char length_prefix_arr[4];
            memcpy(length_prefix_arr, &length_prefix, 4);

            std::string* write_data = new std::string(std::string(length_prefix_arr, 4) + response);

            ev_io* write_watcher = new ev_io;
            write_watcher->data = (void*)(write_data);
            ev_io_init(write_watcher, client_write_cb, watcher->fd, EV_WRITE);
            ev_io_start(EV_DEFAULT, write_watcher);
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
        set_nonblocking(client_fd);

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &address.sin_addr, client_ip, sizeof(client_ip));
        int client_port = ntohs(address.sin_port);
        spdlog::debug("Accepted connection from: {}", client_ip);

        struct ev_loop* loop = EV_DEFAULT;
        ev_io* client_watcher = new ev_io;
        ev_io_init(client_watcher, client_read_cb, client_fd, EV_READ);
        ev_io_start(loop, client_watcher);
    }
}
