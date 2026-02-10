#pragma once

#include <cb/callbacks.h>
#include <types/types.h>
#include <utils/constants.h>
#include <utils/exceptions.h>

#include <algorithm>
#include <string>
#include <unordered_set>
#include <vector>

class QueueMixin {
   private:
    std::unordered_map<std::string, std::unordered_set<int>> q_clients;

   public:
    QueueMixin() { q_clients = std::unordered_map<std::string, std::unordered_set<int>>(); }

    std::string deleteQueue(const Request& req) {
        q_clients.erase(req.key);
        return OK;
    }

    std::string createQueue(const Request& req) {
        if (q_clients.contains(req.key)) {
            throw QueueExistsException();
        }
        q_clients[req.key] = std::unordered_set<int>();
        return OK;
    }

    std::string subscribe(const Request& req) {
        if (!q_clients.contains(req.key)) {
            throw QueueNotFoundException();
        }
        std::unordered_set<int>& clients = q_clients[req.key];
        clients.insert(req.client_fd);
        return OK;
    }

    std::string publish(const Request& req) {
        if (!q_clients.contains(req.key)) {
            throw QueueNotFoundException();
        }
        const std::unordered_set<int>& fds = q_clients[req.key];
        for (const int& fd : fds) {
            create_write_watcher(fd, std::string(req.value));
        }

        return OK;
    }

    std::string flush_queues() {
        q_clients.clear();
        return OK;
    }
};
