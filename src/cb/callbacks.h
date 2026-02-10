#pragma once

#include <ev.h>

#include <string>

void create_read_watcher(int fd, void* data);
void create_write_watcher(int fd, const std::string& data);
void create_accept_watcher(const int& fd);
