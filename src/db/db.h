#pragma once

#include <utils/types.h>

#include <ctime>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>

extern std::map<std::string, DBEntry> db;
extern std::mutex global_mutex;

std::optional<DBEntry> get(const std::string& key);
int set(const Request& cmd);
int del(const std::string& key);
int persist(const Request& cmd);
int expire(const Request& cmd);
