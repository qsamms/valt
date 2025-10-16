#include "cb.h"

#include <arpa/inet.h>
#include <cb/cb.h>
#include <db/db.h>
#include <ev.h>
#include <task_handler/task_handler.h>
#include <utils/utils.h>

std::unordered_map<int, std::string> reads;
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
        if (content.empty())
            cleanup_watcher(EV_A_ watcher, WatcherType::WRITE, ConnectionPolicy::KEEPALIVE);
        else {
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

        uint16_t req_size = bytes_read;
        if (buffer[req_size - 1] == '\n') {
            TaskHandler th;
            std::string content;

            if (reads.contains(watcher->fd))
                content = reads[watcher->fd] + std::string(buffer, --req_size);
            else
                content = std::string(buffer, --req_size);

            reads.erase(watcher->fd);
            Task task = Task{
                .client_fd = watcher->fd,
                .content = content,
            };
            writes[watcher->fd] = th.handle_task(task);

            ev_io* write_watcher = new ev_io;
            ev_io_init(write_watcher, client_write_cb, watcher->fd, EV_WRITE);
            ev_io_start(EV_DEFAULT, write_watcher);
        } else {
            // Add the new content to the in-progress read
            if (!reads.contains(watcher->fd))
                reads[watcher->fd] = std::string(buffer);
            else {
                std::string cur = reads[watcher->fd];
                reads[watcher->fd] = cur + std::string(buffer);
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
            printf("Failed to accept connection\n");
            return;
        }
        set_nonblocking(client_fd);

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &address.sin_addr, client_ip, sizeof(client_ip));
        int client_port = ntohs(address.sin_port);
        printf("Accepted connection from %s:%d\n", client_ip, client_port);

        struct ev_loop* loop = EV_DEFAULT;
        ev_io* client_watcher = new ev_io;
        ev_io_init(client_watcher, client_read_cb, client_fd, EV_READ);
        ev_io_start(loop, client_watcher);
    }
}
