#include "network_layer.h"

#include <arpa/inet.h>
#include <openssl/err.h>
#include <utils/utils.h>
#include <valt/valt.h>

#include "spdlog/spdlog.h"

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
    if (revents & EV_WRITE) {
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
}

void client_read_cb(EV_P_ ev_io* watcher, int revents) {
    char buffer[1024];

    if (revents & EV_READ) {
        auto& conns = get_connections();
        if (!conns.contains(watcher->fd)) {
            ev_io_stop(EV_A_ watcher);
            delete watcher;
            return;
        }
        Connection* c = conns.at(watcher->fd).get();
        WatcherState* state = c->read_state.get();
        SSL* ssl = c->ssl;

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

        bool first_read = state->buffer.size() == 0;
        if (first_read) {
            if (bytes_read >= 4) {
                uint32_t message_length = 0;
                memcpy(reinterpret_cast<char*>(&message_length), buffer, 4);
                message_length = ntohl(message_length);
                std::string message = std::string(buffer + 4, bytes_read - 4);
                state->buffer = message;
                state->total_bytes = message_length;
            } else {
                state->buffer = std::string(buffer, bytes_read);
                state->total_bytes = 0;
            }
        } else {
            std::string& read_buffer = state->buffer;

            if (state->total_bytes == 0) {
                // We still don't know the data length
                uint32_t total_bytes_so_far = read_buffer.size() + bytes_read;
                if (total_bytes_so_far >= 4) {
                    uint32_t message_length = 0;
                    memcpy(reinterpret_cast<char*>(&message_length), read_buffer.c_str(),
                           read_buffer.size());
                    memcpy(reinterpret_cast<char*>(&message_length) + read_buffer.size(), buffer,
                           4 - read_buffer.size());
                    message_length = ntohl(message_length);
                    state->total_bytes = message_length;

                    uint32_t message_bytes = total_bytes_so_far - 4;
                    if (message_bytes > 0) {
                        uint32_t offset = bytes_read - message_bytes;
                        state->buffer = std::string(buffer + offset, bytes_read - offset);
                    }
                } else {
                    state->buffer = std::string(read_buffer + std::string(buffer, bytes_read));
                }
            } else {
                // We know the data length we just haven't received it all yet
                if (read_buffer.size() + bytes_read >= state->total_bytes) {
                    state->buffer = std::string(
                        read_buffer + std::string(buffer, state->total_bytes - read_buffer.size()));
                } else {
                    state->buffer = std::string(read_buffer + std::string(buffer, bytes_read));
                }
            }
        }

        if (state->buffer.size() == state->total_bytes) {
            Valt& valt = Valt::getInstance();
            spdlog::info("{}", state->buffer);
            std::string response = valt.execute(state->buffer, watcher->fd, ssl);
            WatcherState* write_state = c->write_state.get();
            write_state->buffer = std::move(utils::length_prefixed(response));
            write_state->total_bytes = 0;
            c->start_write_watcher(client_write_cb);
            c->reset_state(c->read_state.get());
        }
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
    if (revents & EV_READ) {
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
}

void create_accept_watcher(const int& fd, SSL_CTX* ssl_ctx) {
    ev_io* watcher = new ev_io{};
    watcher->data = ssl_ctx;
    ev_io_init(watcher, accept_connection_cb, fd, EV_READ);
    ev_io_start(EV_DEFAULT, watcher);
}
