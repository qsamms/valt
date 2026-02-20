#pragma once

#include <conn/connection.h>
#include <ev.h>
#include <openssl/ssl.h>

#include <memory>
#include <string>
#include <unordered_map>

std::unordered_map<int, std::unique_ptr<Connection>>& get_connections();
void create_accept_watcher(const int& fd, SSL_CTX* ssl_ctx);
void pubsub_write_cb(EV_P_ ev_io* watcher, int revents);
void client_write_cb(EV_P_ ev_io* watcher, int revents);
