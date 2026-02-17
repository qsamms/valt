#include "callbacks.h"

#include <arpa/inet.h>
#include <openssl/err.h>
#include <utils/utils.h>
#include <valt/valt.h>

#include "spdlog/spdlog.h"

namespace {

struct WatcherData {
    std::string buffer;
    uint32_t total_bytes;
    SSL* ssl;
};

enum class ConnectionPolicy {
    CLOSE,
    KEEPALIVE,
};

void cleanup_watcher(EV_P_ ev_io* watcher, const ConnectionPolicy& policy) {
    ev_io_stop(EV_A_ watcher);
    if (policy == ConnectionPolicy::CLOSE) {
        WatcherData* wd = static_cast<WatcherData*>(watcher->data);
        if (wd != nullptr && wd->ssl) {
            SSL_shutdown(wd->ssl);
            SSL_free(wd->ssl);
        }
        close(watcher->fd);
        Valt& valt = Valt::getInstance();
        valt.end_session(watcher->fd);
    }
    if (watcher->data != nullptr) delete static_cast<WatcherData*>(watcher->data);
    delete watcher;
}

void client_write_cb(EV_P_ ev_io* watcher, int revents) {
    if (revents & EV_WRITE) {
        if (watcher->data == nullptr) {
            cleanup_watcher(EV_A_ watcher, ConnectionPolicy::KEEPALIVE);
            return;
        }

        WatcherData* data = static_cast<WatcherData*>(watcher->data);
        int bytes_sent = 0;
        if (data->ssl != nullptr) {
            bytes_sent = SSL_write(data->ssl, data->buffer.c_str(), data->buffer.size());
            if (bytes_sent <= 0) {
                int err = SSL_get_error(data->ssl, bytes_sent);
                if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) return;
                cleanup_watcher(EV_A_ watcher, ConnectionPolicy::CLOSE);
                return;
            }
        } else {
            bytes_sent = send(watcher->fd, data->buffer.c_str(), data->buffer.size(), 0);
            if (bytes_sent <= 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) return;
                cleanup_watcher(EV_A_ watcher, ConnectionPolicy::CLOSE);
                return;
            }
        }

        data->buffer = data->buffer.substr(bytes_sent);
        if (data->buffer.empty()) {
            cleanup_watcher(EV_A_ watcher, ConnectionPolicy::KEEPALIVE);
        }
    }
}

void client_read_cb(EV_P_ ev_io* watcher, int revents) {
    char buffer[1024];

    if (revents & EV_READ) {
        WatcherData* rd = static_cast<WatcherData*>(watcher->data);
        int bytes_read = 0;
        if (rd->ssl != nullptr) {
            bytes_read = SSL_read(rd->ssl, buffer, sizeof(buffer));
            if (bytes_read <= 0) {
                int err = SSL_get_error(rd->ssl, bytes_read);
                if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) return;
                cleanup_watcher(EV_A_ watcher, ConnectionPolicy::CLOSE);
                return;
            }
        } else {
            bytes_read = recv(watcher->fd, buffer, sizeof(buffer), 0);
            if (bytes_read <= 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) return;
                cleanup_watcher(EV_A_ watcher, ConnectionPolicy::CLOSE);
                return;
            }
        }

        bool first_read = rd->buffer.size() == 0;
        if (first_read) {
            WatcherData* rd;
            if (bytes_read >= 4) {
                uint32_t message_length = 0;
                memcpy(reinterpret_cast<char*>(&message_length), buffer, 4);
                message_length = ntohl(message_length);
                std::string message = std::string(buffer + 4, bytes_read - 4);
                rd->buffer = message;
                rd->total_bytes = message_length;
            } else {
                rd->buffer = std::string(buffer, bytes_read);
                rd->total_bytes = 0;
            }
            watcher->data = rd;
        } else {
            std::string& read_buffer = rd->buffer;

            if (rd->total_bytes == 0) {
                // We still don't know the data length
                uint32_t total_bytes_so_far = read_buffer.size() + bytes_read;
                if (total_bytes_so_far >= 4) {
                    uint32_t message_length = 0;
                    memcpy(reinterpret_cast<char*>(&message_length), read_buffer.c_str(),
                           read_buffer.size());
                    memcpy(reinterpret_cast<char*>(&message_length) + read_buffer.size(), buffer,
                           4 - read_buffer.size());
                    message_length = ntohl(message_length);
                    rd->total_bytes = message_length;

                    uint32_t message_bytes = total_bytes_so_far - 4;
                    if (message_bytes > 0) {
                        uint32_t offset = bytes_read - message_bytes;
                        rd->buffer = std::string(buffer + offset, bytes_read - offset);
                    }
                } else {
                    rd->buffer = std::string(read_buffer + std::string(buffer, bytes_read));
                }
            } else {
                // We know the data length we just haven't received it all yet
                if (read_buffer.size() + bytes_read >= rd->total_bytes) {
                    rd->buffer = std::string(
                        read_buffer + std::string(buffer, rd->total_bytes - read_buffer.size()));
                } else {
                    rd->buffer = std::string(read_buffer + std::string(buffer, bytes_read));
                }
            }
        }

        std::string read_buffer = rd->buffer;
        if (rd->total_bytes == read_buffer.size()) {
            Valt& valt = Valt::getInstance();
            std::string response = valt.execute(read_buffer, watcher->fd, rd->ssl);
            create_write_watcher(watcher->fd, response, rd->ssl);
            watcher->data = nullptr;
        }
    }
}

void tls_handshake_cb(EV_P_ ev_io* watcher, int revents) {
    WatcherData* data = static_cast<WatcherData*>(watcher->data);
    SSL* ssl = data->ssl;

    int result = SSL_do_handshake(ssl);
    if (result == 1) {
        // Handshake complete — switch to normal read watcher
        ev_io_stop(EV_A_ watcher);
        ev_io_init(watcher, client_read_cb, watcher->fd, EV_READ);
        ev_io_start(EV_A_ watcher);
        return;
    }

    int err = SSL_get_error(ssl, result);
    if (err == SSL_ERROR_WANT_READ) {
        ev_set_cb(watcher, tls_handshake_cb);
        ev_io_stop(EV_A_ watcher);
        ev_io_set(watcher, watcher->fd, EV_READ);
        ev_io_start(EV_A_ watcher);
    } else if (err == SSL_ERROR_WANT_WRITE) {
        ev_io_stop(EV_A_ watcher);
        ev_io_set(watcher, watcher->fd, EV_WRITE);
        ev_io_start(EV_A_ watcher);
    } else {
        ERR_print_errors_fp(stderr);
        cleanup_watcher(EV_A_ watcher, ConnectionPolicy::CLOSE);
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

        Valt& valt = Valt::getInstance();
        valt.create_session(client_fd);

        SSL_CTX* ssl_ctx = static_cast<SSL_CTX*>(watcher->data);
        if (ssl_ctx == nullptr) {
            WatcherData* wd = new WatcherData{.buffer = "", .total_bytes = 0, .ssl = nullptr};
            create_read_watcher(client_fd, static_cast<void*>(wd));
        } else {
            SSL* ssl = SSL_new(ssl_ctx);
            SSL_set_fd(ssl, client_fd);
            SSL_set_accept_state(ssl);  // server mode

            WatcherData* wd = new WatcherData{.buffer = "", .total_bytes = 0, .ssl = ssl};

            ev_io* tls_handshake_watcher = new ev_io{};
            tls_handshake_watcher->data = wd;
            ev_io_init(tls_handshake_watcher, tls_handshake_cb, client_fd, EV_READ);
            ev_io_start(EV_DEFAULT, tls_handshake_watcher);
        }
    }
}

}  // namespace

void create_read_watcher(int fd, void* data) {
    ev_io* watcher = new ev_io{};
    watcher->data = data;
    ev_io_init(watcher, client_read_cb, fd, EV_READ);
    ev_io_start(EV_DEFAULT, watcher);
}

void create_write_watcher(int fd, std::string response, SSL* ssl) {
    ev_io* watcher = new ev_io{};

    uint32_t response_size = htonl(response.size());
    char length_prefix[4];
    memcpy(length_prefix, &response_size, 4);

    WatcherData* wd =
        new WatcherData{.buffer = std::string(std::string(length_prefix, 4) + response),
                        .total_bytes = 0,
                        .ssl = ssl};
    watcher->data = wd;
    ev_io_init(watcher, client_write_cb, fd, EV_WRITE);
    ev_io_start(EV_DEFAULT, watcher);
}

void create_accept_watcher(const int& fd, SSL_CTX* ssl_ctx) {
    ev_io* watcher = new ev_io{};
    watcher->data = ssl_ctx;
    ev_io_init(watcher, accept_connection_cb, fd, EV_READ);
    ev_io_start(EV_DEFAULT, watcher);
}
