#pragma once

#include <utils/types.h>

#include <ctime>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>

extern std::map<std::string, DBEntry> db;

std::optional<DBEntry> get(const std::string& key);
int set(const Command& cmd);
int del(const std::string& key);
int persist(const Command& cmd);
int expire(const Command& cmd);
