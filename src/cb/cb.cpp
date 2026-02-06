#include "cb.h"

#include <arpa/inet.h>
#include <cb/cb.h>
#include <task_handler/task_handler.h>
#include <utils/utils.h>

#include <iostream>

#include "spdlog/spdlog.h"

struct read_data {
    std::string buffer;
    uint32_t total_bytes;
};

std::unordered_map<int, read_data> reads;
std::unordered_map<int, std::string> writes;

enum class ConnectionPolicy {
    CLOSE,
    KEEPALIVE,
};

enum class WatcherType {
    READ,
    WRITE,
};

void cleanup_watcher(EV_P_ ev_io* watcher, WatcherType wt, ConnectionPolicy policy) {
    switch (wt) {
        case WatcherType::READ:
            reads.erase(watcher->fd);
            break;
        case WatcherType::WRITE:
            writes.erase(watcher->fd);
            break;
    }
    ev_io_stop(EV_A_ watcher);
    if (policy == ConnectionPolicy::CLOSE) close(watcher->fd);
    delete watcher;
}

void client_write_cb(EV_P_ ev_io* watcher, int revents) {
    if (revents & EV_WRITE) {
        if (!writes.contains(watcher->fd)) {
            cleanup_watcher(EV_A_ watcher, WatcherType::WRITE, ConnectionPolicy::KEEPALIVE);
            return;
        }
        std::string content = writes[watcher->fd];

        int bytes_sent = send(watcher->fd, content.c_str(), content.size(), 0);
        if (bytes_sent <= 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) return;
            cleanup_watcher(EV_A_ watcher, WatcherType::WRITE, ConnectionPolicy::CLOSE);
            return;
        }

        content = content.substr(bytes_sent);
        if (content.empty()) {
            cleanup_watcher(EV_A_ watcher, WatcherType::WRITE, ConnectionPolicy::KEEPALIVE);
        } else {
            writes[watcher->fd] = content;
        }
    }
}

void client_read_cb(EV_P_ ev_io* watcher, int revents) {
    char buffer[1024];
    if (revents & EV_READ) {
        int bytes_read = recv(watcher->fd, buffer, sizeof(buffer), 0);
        if (bytes_read <= 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) return;
            cleanup_watcher(EV_A_ watcher, WatcherType::READ, ConnectionPolicy::CLOSE);
            return;
        }

        if (!reads.contains(watcher->fd)) {
            if (bytes_read >= 4) {
                uint32_t message_length;
                memcpy(&message_length, buffer, 4);
                message_length = ntohl(message_length);
                std::string message = std::string(buffer + 4, bytes_read - 4);

                if (message_length == message.size()) {
                    TaskHandler th;
                    writes[watcher->fd] = th.handle_task(message);
                    ev_io* write_watcher = new ev_io;
                    ev_io_init(write_watcher, client_write_cb, watcher->fd, EV_WRITE);
                    ev_io_start(EV_DEFAULT, write_watcher);
                    return;
                }
                read_data rd = {.buffer = std::string(buffer + 4, bytes_read - 4),
                                .total_bytes = message_length};
                reads[watcher->fd] = rd;
            } else {
                read_data rd = {.buffer = std::string(buffer, bytes_read), .total_bytes = 0};
                reads[watcher->fd] = rd;
            }
        } else {
            read_data& rd = reads[watcher->fd];
            if (rd.total_bytes == 0) {
                // don't know data length yet, still network byte order
                const char* read_buffer = rd.buffer.c_str();
                uint32_t total_bytes_so_far = rd.buffer.size() + bytes_read;
                if (total_bytes_so_far >= 4) {
                    uint32_t message_length;
                    memcpy(&message_length, read_buffer, rd.buffer.size());
                    memcpy(&message_length + rd.buffer.size(), buffer, 4 - rd.buffer.size());
                    message_length = ntohl(message_length);
                    rd.total_bytes = message_length;

                    uint32_t message_bytes = total_bytes_so_far - 4;
                    if (message_bytes > 0) {
                        uint32_t offset = bytes_read - message_bytes;
                        rd.buffer = std::string(buffer + offset, bytes_read - offset);
                    }
                } else {
                    rd.buffer = rd.buffer + std::string(buffer, bytes_read);
                }
            } else {
                // we know the data length we just haven't received it all yet
                if (rd.buffer.size() + bytes_read >= rd.total_bytes) {
                    rd.buffer = rd.buffer + std::string(buffer, rd.total_bytes - rd.buffer.size());
                } else {
                    rd.buffer = rd.buffer + std::string(buffer);
                }
            }
            if (rd.total_bytes == rd.buffer.size()) {
                TaskHandler th;
                writes[watcher->fd] = th.handle_task(rd.buffer);
                ev_io* write_watcher = new ev_io;
                ev_io_init(write_watcher, client_write_cb, watcher->fd, EV_WRITE);
                ev_io_start(EV_DEFAULT, write_watcher);
                reads.erase(watcher->fd);
                return;
            }
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
