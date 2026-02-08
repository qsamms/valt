#pragma once

#include <cb/callbacks.h>
#include <types/types.h>
#include <utils/constants.h>
#include <utils/exceptions.h>

#include <algorithm>
#include <string>
#include <unordered_map>
#include <vector>

class QueueMixin {
   private:
    std::unordered_map<std::string, std::vector<int>> queues;

   public:
    QueueMixin() { queues = std::unordered_map<std::string, std::vector<int>>(); }

    std::string deleteQueue(const Request& req) {
        queues.erase(req.key);
        return OK;
    }

    std::string createQueue(const Request& req) {
        if (queues.contains(req.key)) {
            throw QueueExistsException();
        }
        queues[req.key] = std::vector<int>();
        return OK;
    }

    std::string subscribe(const Request& req) {
        if (!queues.contains(req.key)) {
            throw QueueNotFoundException();
        }
        std::vector<int>& q = queues[req.key];
        if (std::find(q.begin(), q.end(), req.client_fd) != q.end()) {
            q.push_back(req.client_fd);
        }
        return OK;
    }

    std::string publish(const Request& req) {
        if (!queues.contains(req.key)) {
            throw QueueNotFoundException();
        }
        const std::vector<int>& fds = queues[req.key];
        for (int i = 0; i < fds.size(); i++) {
            std::string* watcher_message = new std::string(req.value);
            create_write_watcher(fds[i], (void*)watcher_message);
        }

        return OK;
    }
};
