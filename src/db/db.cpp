#include "db.h"

#include <utils/utils.h>

std::unordered_map<std::string, DBEntry> db;

int set(const Command& req) {
    DBEntry entry;
    entry.expiration = req.expiration;
    entry.value = req.value;
    db[req.key] = entry;
    return 1;
}

std::optional<DBEntry> get(const std::string& key) {
    auto it = db.find(key);
    if (it == db.end()) {
        return std::nullopt;
    }
    DBEntry entry = it->second;
    int64_t expiration = entry.expiration;
    if (expiration > 0) {
        if (seconds_since_epoch() > expiration) {
            db.erase(key);
            return std::nullopt;
        }
    }
    return entry;
}

int del(const std::string& key) {
    if (db.contains(key)) {
        db.erase(key);
        return 1;
    }
    return 0;
}

int persist(const Command& cmd) {
    auto it = db.find(cmd.key);
    if (it != db.end()) {
        DBEntry entry = it->second;
        if (entry.expiration > 0 && seconds_since_epoch() > entry.expiration) {
            db.erase(cmd.key);
            return 0;
        }
        entry.expiration = -1;
        return 1;
    }
    return 0;
}

int expire(const Command& cmd) {
    auto it = db.find(cmd.key);
    if (it != db.end()) {
        it->second.expiration = cmd.expiration;
        return 1;
    }
    return 0;
}

int flush(const Command& cmd) {
    std::unordered_map<std::string, DBEntry>().swap(db);
    return 1;
}
