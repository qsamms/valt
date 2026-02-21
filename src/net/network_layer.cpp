#include "network_layer.h"

#include <arpa/inet.h>
#include <openssl/err.h>
#include <utils/utils.h>
#include <valt/valt.h>

#include "spdlog/spdlog.h"

#define LENGTH_PREFIX_SIZE 4
#define MAX_MESSAGE_LENGTH_BYTES 67108864  // 64MB

std::unordered_map<int, std::unique_ptr<Connection>>& get_connections() {
    static std::unordered_map<int, std::unique_ptr<Connection>> conns;
    return conns;
}

void pubsub_write_cb(EV_P_ ev_io* watcher, int revents) {
    if (!(revents & EV_WRITE)) return;

    auto& conns = get_connections();
    if (!conns.contains(watcher->fd)) {
        ev_io_stop(EV_A_ watcher);
        delete watcher;
        return;
    }
    Connection* c = conns.at(watcher->fd).get();
    std::deque<std::string>& q = c->write_queue;

    while (!q.empty()) {
        std::string& front = q.front();

        SSL* ssl = c->ssl;
        int bytes_sent = 0;

        if (ssl != nullptr) {
            bytes_sent = SSL_write(ssl, front.c_str(), front.size());
            if (bytes_sent <= 0) {
                int err = SSL_get_error(ssl, bytes_sent);
                if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) return;
                conns.erase(watcher->fd);
                return;
            }
        } else {
            bytes_sent = send(watcher->fd, front.c_str(), front.size(), 0);
            if (bytes_sent <= 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) return;
                conns.erase(watcher->fd);
                return;
            }
        }

        front = front.substr(bytes_sent);
        if (front.empty()) {
            q.pop_front();
        }
    }

    c->stop_write_watcher();
}

void client_write_cb(EV_P_ ev_io* watcher, int revents) {
    if (!(revents & EV_WRITE)) return;

    auto& conns = get_connections();
    if (!conns.contains(watcher->fd)) {
        ev_io_stop(EV_A_ watcher);
        delete watcher;
        return;
    }
    Connection* c = conns.at(watcher->fd).get();
    SSL* ssl = c->ssl;
    WatcherState* state = c->write_state.get();

    int bytes_sent = 0;
    if (ssl != nullptr) {
        bytes_sent = SSL_write(ssl, state->buffer.c_str(), state->buffer.size());
        if (bytes_sent <= 0) {
            int err = SSL_get_error(ssl, bytes_sent);
            if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) return;
            conns.erase(watcher->fd);
            return;
        }
    } else {
        bytes_sent = send(watcher->fd, state->buffer.c_str(), state->buffer.size(), 0);
        if (bytes_sent <= 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) return;
            conns.erase(watcher->fd);
            return;
        }
    }

    state->buffer = state->buffer.substr(bytes_sent);
    if (state->buffer.empty()) {
        c->stop_write_watcher();
    }
}

void client_read_cb(EV_P_ ev_io* watcher, int revents) {
    if (!(revents & EV_READ)) return;

    auto& conns = get_connections();
    if (!conns.contains(watcher->fd)) {
        ev_io_stop(EV_A_ watcher);
        delete watcher;
        return;
    }
    Connection* c = conns.at(watcher->fd).get();
    WatcherState* state = c->read_state.get();
    std::string& read_buffer = state->buffer;
    uint32_t& message_length = state->message_length;
    SSL* ssl = c->ssl;

    char buffer[1024];
    int bytes_read = 0;

    if (ssl != nullptr) {
        bytes_read = SSL_read(ssl, buffer, sizeof(buffer));
        if (bytes_read <= 0) {
            int err = SSL_get_error(ssl, bytes_read);
            if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) return;
            conns.erase(watcher->fd);
            return;
        }
    } else {
        bytes_read = recv(watcher->fd, buffer, sizeof(buffer), 0);
        if (bytes_read <= 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) return;
            conns.erase(watcher->fd);
            return;
        }
    }

    bool first_read = read_buffer.size() == 0;
    if (first_read) {
        if (bytes_read >= LENGTH_PREFIX_SIZE) {
            memcpy(&message_length, buffer, LENGTH_PREFIX_SIZE);
            message_length = ntohl(message_length);
            std::string message = std::string(buffer + LENGTH_PREFIX_SIZE, bytes_read - LENGTH_PREFIX_SIZE);
            read_buffer = std::move(message);
        } else {
            std::string message = std::string(buffer, bytes_read);
            read_buffer = std::move(message);
            message_length = 0;
        }
    } else {
        // Data length unknown
        if (state->message_length == 0) {
            uint32_t total_bytes = read_buffer.size() + bytes_read;
            if (total_bytes >= LENGTH_PREFIX_SIZE) {
                memcpy(&message_length, read_buffer.c_str(), read_buffer.size());
                memcpy(&message_length + read_buffer.size(), buffer, LENGTH_PREFIX_SIZE - read_buffer.size());
                message_length = ntohl(message_length);

                if (message_length > MAX_MESSAGE_LENGTH_BYTES) {
                    conns.erase(watcher->fd);
                    return;
                }

                uint32_t message_length_bytes = total_bytes - LENGTH_PREFIX_SIZE;
                if (message_length_bytes > 0) {
                    uint32_t message_offset = bytes_read - message_length_bytes;
                    read_buffer = std::move(std::string(buffer + message_offset, bytes_read - message_offset));
                }
            } else {
                read_buffer.append(std::string(buffer, bytes_read));
            }
        }
        // Data length known
        else {
            if (read_buffer.size() + bytes_read >= message_length) {
                read_buffer.append(buffer, message_length - read_buffer.size());
            } else {
                read_buffer.append(buffer, bytes_read);
            }
        }
    }

    if (read_buffer.size() == message_length) {
        Valt& valt = Valt::getInstance();
        std::string response = valt.execute(read_buffer, watcher->fd);
        WatcherState* write_state = c->write_state.get();
        write_state->buffer = std::move(utils::length_prefixed(response));
        write_state->message_length = 0;
        c->start_write_watcher(client_write_cb);
        c->reset_state(c->read_state.get());
    }
}

void tls_handshake_cb(EV_P_ ev_io* watcher, int revents) {
    auto& conns = get_connections();
    if (!conns.contains(watcher->fd)) {
        ev_io_stop(EV_A_ watcher);
        delete watcher;
        return;
    }
    Connection* c = conns.at(watcher->fd).get();
    SSL* ssl = c->ssl;

    int result = SSL_do_handshake(ssl);
    if (result == 1) {
        // Handshake complete — switch to normal read watcher
        c->stop_write_watcher();
        c->stop_read_watcher();
        c->start_read_watcher(client_read_cb);
        return;
    }

    int err = SSL_get_error(ssl, result);
    if (err == SSL_ERROR_WANT_READ) {
        c->stop_write_watcher();
        c->start_read_watcher(tls_handshake_cb);
    } else if (err == SSL_ERROR_WANT_WRITE) {
        c->stop_read_watcher();
        c->start_write_watcher(tls_handshake_cb);
    } else {
        ERR_print_errors_fp(stderr);
        conns.erase(watcher->fd);
    }
}

void accept_connection_cb(EV_P_ ev_io* watcher, int revents) {
    if (!(revents & EV_READ)) return;

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

    auto& conns = get_connections();
    SSL_CTX* ssl_ctx = static_cast<SSL_CTX*>(watcher->data);
    if (ssl_ctx == nullptr) {
        conns.try_emplace(client_fd, std::make_unique<Connection>(client_fd));
        Connection* c = conns.at(client_fd).get();
        c->start_read_watcher(client_read_cb);
    } else {
        SSL* ssl = SSL_new(ssl_ctx);
        SSL_set_fd(ssl, client_fd);
        SSL_set_accept_state(ssl);  // server mode
        conns.try_emplace(client_fd, std::make_unique<Connection>(client_fd, ssl));
        Connection* c = conns.at(client_fd).get();
        c->start_read_watcher(tls_handshake_cb);
    }
}

void create_accept_watcher(const int& fd, SSL_CTX* ssl_ctx) {
    ev_io* watcher = new ev_io{};
    watcher->data = ssl_ctx;
    ev_io_init(watcher, accept_connection_cb, fd, EV_READ);
    ev_io_start(EV_DEFAULT, watcher);
}
