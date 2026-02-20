#pragma once

#include <net/network_layer.h>
#include <openssl/ssl.h>
#include <types/types.h>
#include <utils/constants.h>
#include <utils/exceptions.h>
#include <valt/valt_config.h>

#include <string>
#include <unordered_set>

class PubSubMixin {
   private:
    const ValtConfig* valt_config;
    std::unordered_map<std::string, std::unordered_set<int>> subs;

   public:
    PubSubMixin() = delete;
    PubSubMixin(const ValtConfig* cfg);

    std::string deleteQueue(const Request& req);
    std::string createQueue(const Request& req);
    std::string subscribe(const Request& req);
    std::string unsubscribe(const Request& req);
    std::string publish(const Request& req);
    std::string flush();
};
