#include "pubsub_mixin.h"

#include <net/network_layer.h>
#include <spdlog/spdlog.h>
#include <utils/utils.h>

PubSubMixin::PubSubMixin(const ValtConfig* cfg) {
    valt_config = cfg;
    subs = std::unordered_map<std::string, std::unordered_set<int>>();
}

std::string PubSubMixin::deleteQueue(const Request& req) {
    subs.erase(req.key);
    return OK;
}

std::string PubSubMixin::createQueue(const Request& req) {
    if (subs.contains(req.key)) {
        throw QueueExistsException();
    }
    subs[req.key] = std::unordered_set<int>();
    return OK;
}

std::string PubSubMixin::unsubscribe(const Request& req) {
    if (!subs.contains(req.key)) {
        throw QueueNotFoundException();
    }
    std::unordered_set<int>& clients = subs[req.key];
    clients.erase(req.client_fd);

    auto& conns = get_connections();
    if (conns.contains(req.client_fd)) {
        Connection* c = conns.at(req.client_fd).get();
        c->stop_write_watcher();
    }
    return OK;
}

std::string PubSubMixin::subscribe(const Request& req) {
    if (!subs.contains(req.key)) {
        throw QueueNotFoundException();
    }
    std::unordered_set<int>& clients = subs[req.key];
    clients.insert(req.client_fd);

    auto& conns = get_connections();
    if (conns.contains(req.client_fd)) {
        Connection* c = conns.at(req.client_fd).get();
        spdlog::info("subbing");
        c->stop_write_watcher();
        c->start_write_watcher(pubsub_write_cb);
    }
    return OK;
}

std::string PubSubMixin::publish(const Request& req) {
    if (!subs.contains(req.key)) {
        throw QueueNotFoundException();
    }
    std::unordered_set<int>& fds = subs[req.key];
    auto& conns = get_connections();
    for (auto it = fds.begin(); it != fds.end();) {
        if (conns.contains(*it)) {
            Connection* c = conns.at(*it).get();
            c->add_to_queue(utils::length_prefixed(req.value));
            c->start_write_watcher(pubsub_write_cb);
            ++it;
        } else {
            it = fds.erase(it);
        }
    }

    return OK;
}

std::string PubSubMixin::flush() {
    subs.clear();
    return OK;
}
