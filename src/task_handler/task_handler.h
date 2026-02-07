#pragma once

#include <db/database_mixin.h>
#include <types/types.h>

#include <cstdint>
#include <string>

class TaskHandler : DataBaseMixin {
   private:
    TaskHandler() = default;
    ~TaskHandler() = default;

    TaskHandler(const TaskHandler&) = delete;
    TaskHandler& operator=(const TaskHandler&) = delete;
    TaskHandler(TaskHandler&&) = delete;
    TaskHandler& operator=(TaskHandler&&) = delete;

    Command parse_request(const std::string&);
    int64_t parse_expiration(const std::string&);
    std::string perform_op(const Command&);

   public:
    std::string handle_task(const std::string&);

    static TaskHandler& get_instance() {
        static TaskHandler instance;
        return instance;
    }
};
