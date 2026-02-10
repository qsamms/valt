#include "database_mixin.h"

DataBaseMixin::DataBaseMixin() {
    db = std::unordered_map<std::string, DBEntry>();
}

std::string DataBaseMixin::set(const Request& req) {
    DBEntry entry;
    entry.expiration = req.expiration;
    entry.value = req.value;
    db[req.key] = entry;
    return OK;
}

std::string DataBaseMixin::get(const Request& req) {
    const std::string& key = req.key;

    auto it = db.find(key);
    if (it == db.end()) {
        throw KeyNotFoundException();
    }
    DBEntry entry = it->second;
    int64_t expiration = entry.expiration;
    if (expiration > 0) {
        if (utils::seconds_since_epoch() > expiration) {
            db.erase(key);
            throw KeyNotFoundException();
        }
    }
    return entry.value;
}

std::string DataBaseMixin::del(const Request& req) {
    const std::string& key = req.key;
    if (db.contains(key)) {
        db.erase(key);
    }
    return OK;
}

std::string DataBaseMixin::persist(const Request& req) {
    auto it = db.find(req.key);

    if (it != db.end()) {
        DBEntry entry = it->second;
        if (entry.expiration > 0 && utils::seconds_since_epoch() > entry.expiration) {
            db.erase(req.key);
            throw KeyNotFoundException();
        }
        entry.expiration = -1;
    }
    return OK;
}

std::string DataBaseMixin::expire(const Request& req) {
    auto it = db.find(req.key);

    if (it != db.end()) {
        it->second.expiration = req.expiration;
        return OK;
    }
    throw KeyNotFoundException();
}

std::string DataBaseMixin::flush_db() {
    db.clear();
    return OK;
}
