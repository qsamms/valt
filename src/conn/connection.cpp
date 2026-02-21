#include "connection.h"

Connection::Connection(int client_fd)
    : fd(client_fd),
      ssl(nullptr),
      authenticated(false),
      mode(SessionMode::DB),
      read_watcher(std::make_unique<ev_io>()),
      write_watcher(std::make_unique<ev_io>()),
      write_state(std::make_unique<WatcherState>()),
      read_state(std::make_unique<WatcherState>()) {
}

Connection::Connection(int client_fd, SSL* ssl)
    : fd(client_fd),
      ssl(ssl),
      authenticated(false),
      mode(SessionMode::DB),
      read_watcher(std::make_unique<ev_io>()),
      write_watcher(std::make_unique<ev_io>()),
      write_state(std::make_unique<WatcherState>()),
      read_state(std::make_unique<WatcherState>()) {
}

Connection::~Connection() {
    if (read_watcher) ev_io_stop(EV_DEFAULT, read_watcher.get());
    if (write_watcher) ev_io_stop(EV_DEFAULT, write_watcher.get());

    if (ssl != nullptr) {
        SSL_shutdown(ssl);
        SSL_free(ssl);
    }
    close(fd);
}

void Connection::add_to_queue(const std::string& s) {
    write_queue.push_back(std::move(s));
}

void Connection::reset_state(WatcherState* state) {
    state->buffer.clear();
    state->message_length = 0;
}

void Connection::start_write_watcher(void (*callback)(struct ev_loop*, ev_io*, int)) {
    ev_io_stop(EV_DEFAULT, write_watcher.get());
    ev_io_init(write_watcher.get(), callback, fd, EV_WRITE);
    ev_io_start(EV_DEFAULT, write_watcher.get());
}

void Connection::start_read_watcher(void (*callback)(struct ev_loop*, ev_io*, int)) {
    ev_io_stop(EV_DEFAULT, read_watcher.get());
    ev_io_init(read_watcher.get(), callback, fd, EV_READ);
    ev_io_start(EV_DEFAULT, read_watcher.get());
}

void Connection::stop_write_watcher() {
    ev_io_stop(EV_DEFAULT, write_watcher.get());
}

void Connection::stop_read_watcher() {
    ev_io_stop(EV_DEFAULT, read_watcher.get());
}
