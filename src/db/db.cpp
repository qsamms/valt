#include "db.h"

#include <utils/types.h>
#include <utils/utils.h>

#include <chrono>
#include <exception>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>

std::map<std::string, DBEntry> db;
std::mutex global_mutex;

int set(const Request& req) {
    std::lock_guard<std::mutex> g(global_mutex);
    DBEntry entry;
    entry.expiration = req.expiration;
    entry.value = req.value;
    db[req.key] = entry;
    return 1;
}

std::optional<DBEntry> get(const std::string& key) {
    std::lock_guard<std::mutex> g(global_mutex);

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
    std::lock_guard<std::mutex> g(global_mutex);
    if (db.contains(key)) {
        db.erase(key);
        return 1;
    }
    return 0;
}

int persist(const Request& req) {
    std::lock_guard<std::mutex> g(global_mutex);
    auto it = db.find(req.key);
    if (!(it == db.end())) {
        DBEntry entry = it->second;
        if (entry.expiration > 0 && seconds_since_epoch() > entry.expiration) {
            db.erase(req.key);
            return 0;
        }
        entry.expiration = -1;
        return 1;
    }
    return 0;
}

int expire(const Request& req) {
    std::lock_guard<std::mutex> g(global_mutex);
    auto it = db.find(req.key);
    if (!(it == db.end())) {
        it->second.expiration = req.expiration;
        return 1;
    }
    return 0;
}