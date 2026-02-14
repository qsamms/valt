#pragma once

#include <types/types.h>
#include <utils/constants.h>
#include <utils/exceptions.h>
#include <utils/utils.h>

#include <string>
#include <unordered_map>

class DataBaseMixin {
   private:
    std::unordered_map<std::string, DBEntry> db;

   public:
    DataBaseMixin();

    std::string set(const Request& req);
    std::string get(const Request& req);
    std::string del(const Request& req);
    std::string persist(const Request& req);
    std::string expire(const Request& req);
    std::string flush_db();
};
