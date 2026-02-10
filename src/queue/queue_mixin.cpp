#include "queue_mixin.h"

QueueMixin::QueueMixin() {
    q_clients = std::unordered_map<std::string, std::unordered_set<int>>();
}

std::string QueueMixin::deleteQueue(const Request& req) {
    q_clients.erase(req.key);
    return OK;
}

std::string QueueMixin::createQueue(const Request& req) {
    if (q_clients.contains(req.key)) {
        throw QueueExistsException();
    }
    q_clients[req.key] = std::unordered_set<int>();
    return OK;
}

std::string QueueMixin::subscribe(const Request& req) {
    if (!q_clients.contains(req.key)) {
        throw QueueNotFoundException();
    }
    std::unordered_set<int>& clients = q_clients[req.key];
    clients.insert(req.client_fd);
    return OK;
}

std::string QueueMixin::publish(const Request& req) {
    if (!q_clients.contains(req.key)) {
        throw QueueNotFoundException();
    }
    const std::unordered_set<int>& fds = q_clients[req.key];
    for (const int& fd : fds) {
        create_write_watcher(fd, std::string(req.value));
    }

    return OK;
}

std::string QueueMixin::flush_queues() {
    q_clients.clear();
    return OK;
}
