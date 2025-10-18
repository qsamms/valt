#pragma once

#include <utils/types.h>

#include <cstdint>
#include <regex>
#include <string>
#include <thread>
#include <vector>

class TaskHandler {
   private:
    Command parse_request(const std::string&);
    int64_t parse_expiration(const std::string&);
    std::string perform_op(const Command&);

   public:
    std::string handle_task(const std::string&);
};
