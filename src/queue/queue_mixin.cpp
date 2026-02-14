#include "queue_mixin.h"

QueueMixin::QueueMixin() {
    q_clients = std::unordered_map<std::string, std::unordered_set<QueueObject>>();
}

std::string QueueMixin::deleteQueue(const Request& req) {
    q_clients.erase(req.key);
    return OK;
}

std::string QueueMixin::createQueue(const Request& req) {
    if (q_clients.contains(req.key)) {
        throw QueueExistsException();
    }
    q_clients[req.key] = std::unordered_set<QueueObject>();
    return OK;
}

std::string QueueMixin::subscribe(const Request& req) {
    if (!q_clients.contains(req.key)) {
        throw QueueNotFoundException();
    }
    std::unordered_set<QueueObject>& clients = q_clients[req.key];
    clients.insert(QueueObject{.fd = req.client_fd, .ssl = req.ssl});
    return OK;
}

std::string QueueMixin::publish(const Request& req) {
    if (!q_clients.contains(req.key)) {
        throw QueueNotFoundException();
    }
    const std::unordered_set<QueueObject>& qos = q_clients[req.key];
    for (const QueueObject& qo : qos) {
        create_write_watcher(qo.fd, req.value, qo.ssl);
    }

    return OK;
}

std::string QueueMixin::flush_queues() {
    q_clients.clear();
    return OK;
}
