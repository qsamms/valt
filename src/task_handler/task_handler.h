#pragma once

#include <utils/types.h>

#include <cstdint>
#include <regex>
#include <string>
#include <thread>
#include <vector>

class TaskHandler {
   private:
    Command parse_request(const std::string& req_str);
    int64_t parse_expiration(const std::string& expiration_str);
    std::string perform_op(const Command& req);

   public:
    std::string handle_task(const Task task);
};
