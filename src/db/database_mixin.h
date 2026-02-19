#pragma once

#include <types/types.h>
#include <utils/constants.h>
#include <utils/exceptions.h>
#include <utils/utils.h>
#include <valt/valt_config.h>

#include <memory>
#include <string>
#include <unordered_map>

struct DBNode {
    std::string key;
    std::string value;
    int64_t expiration;

    DBNode* next;
    DBNode* prev;
};

class DataBaseMixin {
    /*
    Core database is a least recently used (LRU) cache implemented with a map and doubly linked
    list. On use, database entries (nodes) are moved to the front of the list, so the last node in
    the list is always the LRU. The map links a key directly to its node for quick lookups.
    */
   private:
    const ValtConfig* valt_config;
    DBNode* head;
    DBNode* tail;
    std::unordered_map<std::string, std::unique_ptr<DBNode>> db;

    void move_to_front(DBNode* node);
    void remove_node(DBNode* node);
    void unlink(DBNode* node);

   public:
    DataBaseMixin() = delete;
    DataBaseMixin(const ValtConfig* cfg);
    ~DataBaseMixin();

    std::string set(const Request& req);
    std::string get(const Request& req);
    std::string del(const Request& req);
    std::string persist(const Request& req);
    std::string expire(const Request& req);
    std::string evict(const Request& req);
    std::string flush();
};
