#pragma once

#include <ev.h>
#include <openssl/ssl.h>

#include <string>

void create_read_watcher(int fd, void* data);
void create_write_watcher(int fd, std::string data, SSL* ssl);
void create_accept_watcher(const int& fd, SSL_CTX* ssl_ctx);
