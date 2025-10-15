#include <arpa/inet.h>
#include <cb/cb.h>
#include <db/db.h>
#include <ev.h>
#include <sys/socket.h>
#include <unistd.h>
#include <utils/utils.h>

#include <cstdio>
#include <cstring>
#include <iostream>
#include <thread>
#include <vector>

#define SERVER_PORT 9999
#define MAX_PENDING_CONNECTIONS 10
#define MAX_CONCURRENT_CONNECTIONS 10000

using RuntimeError = std::runtime_error;

int main(int argc, char* argv[]) {
    struct sockaddr_in address;
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        throw RuntimeError("failed to create socket");
    }

    set_nonblocking(server_fd);

    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;          // IPv4
    address.sin_addr.s_addr = INADDR_ANY;  // Bind to all interfaces
    address.sin_port = htons(SERVER_PORT);

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        close(server_fd);
        throw RuntimeError("setsockopt failed");
    }

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) == -1) {
        close(server_fd);
        throw RuntimeError("bind failed");
    }

    if (listen(server_fd, MAX_PENDING_CONNECTIONS) == -1) {
        close(server_fd);
        throw RuntimeError("listen failed");
    }

    struct ev_loop* loop = EV_DEFAULT;
    ev_io accept_watcher;
    ev_io_init(&accept_watcher, accept_connection_cb, server_fd, EV_READ);
    ev_io_start(loop, &accept_watcher);

    ev_run(loop, 0);

    close(server_fd);
    return 0;
}