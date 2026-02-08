#pragma once

#include <ev.h>

void create_read_watcher(int fd, void* data);
void create_write_watcher(int fd, void* data);

void client_write_cb(EV_P_ ev_io* watcher, int revents);
void client_read_cb(EV_P_ ev_io* watcher, int revents);
void accept_connection_cb(EV_P_ ev_io* watcher, int revents);
