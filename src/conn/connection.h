#pragma once

#include <ev.h>
#include <openssl/ssl.h>
#include <types/types.h>

#include <deque>
#include <memory>

struct WatcherState {
    std::string buffer;
    uint32_t message_length;
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

    Connection(int client_fd);
    Connection(int client_fd, SSL* ssl);
    Connection(Connection&&) noexcept = default;
    Connection& operator=(Connection&&) noexcept = default;
    ~Connection();

    void add_to_queue(const std::string& s);
    void reset_state(WatcherState* state);
    void start_write_watcher(void (*callback)(struct ev_loop*, ev_io*, int));
    void start_read_watcher(void (*callback)(struct ev_loop*, ev_io*, int));
    void stop_write_watcher();
    void stop_read_watcher();
};
