#include "cb.h"

#include <arpa/inet.h>
#include <cb/cb.h>
#include <db/db.h>
#include <ev.h>
#include <task_handler/task_handler.h>
#include <utils/utils.h>

std::map<int, std::string> reads;
std::map<int, std::string> writes;

void client_write_cb(EV_P_ ev_io* watcher, int revents) {
    if (revents & EV_WRITE) {
        auto it = writes.find(watcher->fd);
        if (it == writes.end()) {
            writes.erase(watcher->fd);
            ev_io_stop(EV_A_ watcher);
            delete watcher;
            return;
        }
        std::string content = writes[watcher->fd];

        int bytes_sent = send(watcher->fd, content.c_str(), content.size() + 1, 0);
        if (bytes_sent <= 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) return;
            writes.erase(watcher->fd);
            ev_io_stop(EV_A_ watcher);
            close(watcher->fd);
            delete watcher;
            return;
        }

        content = content.substr(bytes_sent);

        if (content.empty()) {
            writes.erase(watcher->fd);
            ev_io_stop(EV_A_ watcher);
            delete watcher;
        }
    }
}

void client_read_cb(EV_P_ ev_io* watcher, int revents) {
    char buffer[1024];
    if (revents & EV_READ) {
        int bytes_read = recv(watcher->fd, buffer, sizeof(buffer), 0);
        if (bytes_read <= 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) return;
            reads.erase(watcher->fd);
            ev_io_stop(EV_A_ watcher);
            close(watcher->fd);
            delete watcher;
            return;
        }

        uint16_t req_size = bytes_read;
        if (buffer[req_size - 1] == '\n') {
            // Reached a newline, process the request
            std::string content;
            auto it = reads.find(watcher->fd);
            if (it == reads.end()) {
                content = std::string(buffer, --req_size);
            } else {
                content = reads[watcher->fd] + std::string(buffer, --req_size);
            }
            reads.erase(watcher->fd);
            Task task = Task{
                .client_fd = watcher->fd,
                .content = content,
            };
            TaskHandler th;
            writes[watcher->fd] = th.handle_task(task);

            ev_io* write_watcher = new ev_io;
            ev_io_init(write_watcher, client_write_cb, watcher->fd, EV_WRITE);
            ev_io_start(EV_DEFAULT, write_watcher);
        } else {
            // add new content to in_progress string
            auto it = reads.find(watcher->fd);
            if (it == reads.end()) {
                reads[watcher->fd] = std::string(buffer);
            } else {
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
