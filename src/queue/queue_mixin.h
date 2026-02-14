#pragma once

#include <cb/callbacks.h>
#include <openssl/ssl.h>
#include <types/types.h>
#include <utils/constants.h>
#include <utils/exceptions.h>

#include <algorithm>
#include <string>
#include <unordered_set>
#include <vector>

struct QueueObject {
    int fd;
    SSL* ssl;

    bool operator==(const QueueObject& other) const { return fd == other.fd; }
};

namespace std {
template <>
struct hash<QueueObject> {
    size_t operator()(const QueueObject& qo) const { return std::hash<int>{}(qo.fd); }
};
}  // namespace std

class QueueMixin {
   private:
    std::unordered_map<std::string, std::unordered_set<QueueObject>> q_clients;

   public:
    QueueMixin();

    std::string deleteQueue(const Request& req);
    std::string createQueue(const Request& req);
    std::string subscribe(const Request& req);
    std::string publish(const Request& req);
    std::string flush_queues();
};
