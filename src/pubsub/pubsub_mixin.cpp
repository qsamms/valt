#include "pubsub_mixin.h"

PubSubMixin::PubSubMixin() {
    subs = std::unordered_map<std::string, std::unordered_set<Subscription>>();
}

std::string PubSubMixin::deleteQueue(const Request& req) {
    subs.erase(req.key);
    return OK;
}

std::string PubSubMixin::createQueue(const Request& req) {
    if (subs.contains(req.key)) {
        throw QueueExistsException();
    }
    subs[req.key] = std::unordered_set<Subscription>();
    return OK;
}

std::string PubSubMixin::unsubscribe(const Request& req) {
    if (!subs.contains(req.key)) {
        throw QueueNotFoundException();
    }
    std::unordered_set<Subscription>& clients = subs[req.key];
    clients.erase(Subscription{.fd = req.client_fd});
    return OK;
}

std::string PubSubMixin::subscribe(const Request& req) {
    if (!subs.contains(req.key)) {
        throw QueueNotFoundException();
    }
    std::unordered_set<Subscription>& clients = subs[req.key];
    clients.insert(Subscription{.fd = req.client_fd, .ssl = req.ssl});
    return OK;
}

std::string PubSubMixin::publish(const Request& req) {
    if (!subs.contains(req.key)) {
        throw QueueNotFoundException();
    }
    const std::unordered_set<Subscription>& qos = subs[req.key];
    for (const Subscription& qo : qos) {
        create_write_watcher(qo.fd, req.value, qo.ssl);
    }

    return OK;
}

std::string PubSubMixin::flush() {
    subs.clear();
    return OK;
}
