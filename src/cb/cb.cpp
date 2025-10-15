#include "cb.h"
#include <arpa/inet.h>
#include <cb/cb.h>
#include <db/db.h>
#include <ev.h>
#include <utils/utils.h>


void client_write_cb(EV_P_ ev_io* watcher, int revents) {
    if (revents & EV_WRITE) {
        // write to client socket

    }
}

void client_read_cb(EV_P_ ev_io* watcher, int revents) {
    char buffer[1024];
    if (revents & EV_READ) {
        // TODO: if not all data is read, save the state and return 
        int bytes_read = recv(watcher->fd, buffer, sizeof(buffer), 0);
        if (bytes_read <= 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return;
            }
            ev_io_stop(EV_A_ watcher);
            close(watcher->fd);
            delete watcher;
            return;
        }
        uint16_t req_size = bytes_read;
        if (buffer[req_size - 1] == '\n') req_size--;

        Task task = Task{
            .client_fd = watcher->fd,
            .content = std::string(buffer, req_size),
        };
        

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