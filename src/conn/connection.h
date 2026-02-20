#pragma once

#include <ev.h>
#include <net/network_layer.h>
#include <openssl/ssl.h>
#include <types/types.h>

#include <deque>
#include <memory>

struct WatcherState {
    std::string buffer;
    uint32_t total_bytes;
};

struct Connection {
    /*
    Some info about a client's session, cleans up all memory / IO on destruct.
    */
    int fd;
    bool authenticated;
    SessionMode mode;
    SSL* ssl;
    std::unique_ptr<ev_io> read_watcher;
    std::unique_ptr<ev_io> write_watcher;
    std::unique_ptr<WatcherState> write_state;
    std::unique_ptr<WatcherState> read_state;
    std::deque<std::string> write_queue;

    Connection(int client_fd)
        : fd(client_fd),
          ssl(nullptr),
          authenticated(false),
          mode(SessionMode::DB),
          read_watcher(std::make_unique<ev_io>()),
          write_watcher(std::make_unique<ev_io>()),
          write_state(std::make_unique<WatcherState>()),
          read_state(std::make_unique<WatcherState>()) {}

    Connection(int client_fd, SSL* ssl)
        : fd(client_fd),
          ssl(ssl),
          authenticated(false),
          mode(SessionMode::DB),
          read_watcher(std::make_unique<ev_io>()),
          write_watcher(std::make_unique<ev_io>()),
          write_state(std::make_unique<WatcherState>()),
          read_state(std::make_unique<WatcherState>()) {}

    Connection(Connection&&) noexcept = default;
    Connection& operator=(Connection&&) noexcept = default;

    ~Connection() {
        if (read_watcher) ev_io_stop(EV_DEFAULT, read_watcher.get());
        if (write_watcher) ev_io_stop(EV_DEFAULT, write_watcher.get());

        if (ssl != nullptr) {
            SSL_shutdown(ssl);
            SSL_free(ssl);
        }
        close(fd);
    }

    void add_to_queue(const std::string& s) { write_queue.push_back(std::move(s)); }

    void reset_state(WatcherState* state) {
        state->buffer.clear();
        state->total_bytes = 0;
    }

    void start_write_watcher(void (*callback)(struct ev_loop*, ev_io*, int)) {
        ev_io_stop(EV_DEFAULT, write_watcher.get());
        ev_io_init(write_watcher.get(), callback, fd, EV_WRITE);
        ev_io_start(EV_DEFAULT, write_watcher.get());
    }
    void start_read_watcher(void (*callback)(struct ev_loop*, ev_io*, int)) {
        ev_io_stop(EV_DEFAULT, read_watcher.get());
        ev_io_init(read_watcher.get(), callback, fd, EV_READ);
        ev_io_start(EV_DEFAULT, read_watcher.get());
    }

    void stop_write_watcher() { ev_io_stop(EV_DEFAULT, write_watcher.get()); }
    void stop_read_watcher() { ev_io_stop(EV_DEFAULT, read_watcher.get()); }
};
