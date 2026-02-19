#include "database_mixin.h"

DataBaseMixin::DataBaseMixin(const ValtConfig* cfg) {
    valt_config = cfg;
    db = std::unordered_map<std::string, std::unique_ptr<DBNode>>();

    head = new DBNode{.value = "", .expiration = 0, .next = nullptr, .prev = nullptr};
    tail = new DBNode{.value = "", .expiration = 0, .next = nullptr, .prev = nullptr};

    head->next = tail;
    tail->prev = head;
}

DataBaseMixin::~DataBaseMixin() {
    delete head;
    delete tail;
}

void DataBaseMixin::move_to_front(DBNode* node) {
    if (head->next == node) {
        return;
    }

    if (node->prev && node->next) {
        DBNode* prev = node->prev;
        DBNode* next = node->next;

        prev->next = next;
        next->prev = prev;

        node->next = nullptr;
        node->prev = nullptr;
    }

    DBNode* firstNode = head->next;
    head->next = node;

    node->next = firstNode;
    firstNode->prev = node;
    node->prev = head;
}

void DataBaseMixin::remove_node(DBNode* node) {
    unlink(node);
    db.erase(node->key);
}

void DataBaseMixin::unlink(DBNode* node) {
    if (!node || !node->prev || !node->next) return;
    node->prev->next = node->next;
    node->next->prev = node->prev;
}

std::string DataBaseMixin::set(const Request& req) {
    if (!db.contains(req.key)) {
        std::unique_ptr<DBNode> newNode = std::make_unique<DBNode>();
        newNode->expiration = req.expiration;
        newNode->value = req.value;
        newNode->key = req.key;
        move_to_front(newNode.get());
        db[req.key] = std::move(newNode);
    } else {
        DBNode* node = db[req.key].get();
        node->value = req.value;
        node->expiration = req.expiration;
        move_to_front(node);
    }
    return OK;
}

std::string DataBaseMixin::get(const Request& req) {
    const std::string& key = req.key;

    auto it = db.find(key);
    if (it == db.end()) {
        throw KeyNotFoundException();
    }
    DBNode* node = it->second.get();
    int64_t expiration = node->expiration;
    if (expiration > 0) {
        if (utils::seconds_since_epoch() > expiration) {
            remove_node(node);
            throw KeyNotFoundException();
        }
    }

    move_to_front(node);

    return node->value;
}

std::string DataBaseMixin::del(const Request& req) {
    const std::string& key = req.key;
    if (db.contains(key)) {
        DBNode* node = db[key].get();
        remove_node(node);
    }
    return OK;
}

std::string DataBaseMixin::persist(const Request& req) {
    auto it = db.find(req.key);

    if (it != db.end()) {
        DBNode* node = it->second.get();
        if (node->expiration > 0 && utils::seconds_since_epoch() > node->expiration) {
            remove_node(node);
            throw KeyNotFoundException();
        }
        node->expiration = -1;
        move_to_front(node);
    }
    return OK;
}

std::string DataBaseMixin::expire(const Request& req) {
    auto it = db.find(req.key);

    if (it != db.end()) {
        DBNode* node = it->second.get();
        node->expiration = req.expiration;
        move_to_front(node);
        return OK;
    }
    throw KeyNotFoundException();
}

std::string DataBaseMixin::evict(const Request& req) {
    if (db.size() > 0) {
        DBNode* node = tail->prev;
        remove_node(node);
    }
    return OK;
}

std::string DataBaseMixin::flush() {
    head->next = tail;
    head->prev = nullptr;
    tail->next = nullptr;
    tail->prev = head;
    db.clear();
    return OK;
}
