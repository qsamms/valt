#pragma once

#include <ev.h>
#include <openssl/ssl.h>

#include <string>

struct WatcherData {
    std::string buffer;
    uint32_t total_bytes;
    SSL* ssl;
};

void create_read_watcher(int fd, WatcherData* data);
void create_write_watcher(int fd, std::string data, SSL* ssl);
void create_accept_watcher(const int& fd, SSL_CTX* ssl_ctx);
