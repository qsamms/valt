#pragma once

#include <cstdint>
#include <regex>
#include <string>
#include <unordered_map>

const std::regex int_re(R"(^[+-]?\d+$)");

struct DBEntry {
    std::string value;
    int64_t expiration;
};

enum class Operation { GET, SET, SETEX, DELETE, PERSIST, EXPIRE, FLUSH };

struct Command {
    Operation op;
    std::string key;
    std::string value;
    int64_t expiration;
};

const std::unordered_map<std::string, Operation> op_mapping = {
    {"get", Operation::GET},         {"set", Operation::SET},       {"setex", Operation::SETEX},
    {"persist", Operation::PERSIST}, {"expire", Operation::EXPIRE}, {"delete", Operation::DELETE},
    {"flush", Operation::FLUSH}};
