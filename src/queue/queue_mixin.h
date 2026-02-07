#pragma once

#include <string>
#include <unordered_map>
#include <vector>

class QueueMixin {
   private:
    static std::unordered_map<std::string, std::vector<int>> queues;

   public:
    void broadcast(std::string queue, std::string message) {}
};
