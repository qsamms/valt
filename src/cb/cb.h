#include <ev.h>

void client_write_cb(EV_P_ ev_io* watcher, int revents);
void client_read_cb(EV_P_ ev_io* watcher, int revents);
void accept_connection_cb(EV_P_ ev_io* watcher, int revents);
