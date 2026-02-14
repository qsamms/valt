#pragma once

#include <openssl/ssl.h>

#include <cstdint>
#include <string>
#include <unordered_map>

struct DBEntry {
    std::string value;
    int64_t expiration;
};

enum class Operation {
    GET,
    SET,
    SETEX,
    DELETE,
    PERSIST,
    EXPIRE,
    FLUSH,
    SUBSCRIBE,
    PUBLISH,
    CREATE_QUEUE,
    DELETE_QUEUE,
};

struct Request {
    int client_fd;
    SSL* ssl;
    Operation op;
    std::string key;
    std::string value;
    int64_t expiration;
};

const std::unordered_map<std::string, Operation> op_mapping = {
    {"get", Operation::GET},
    {"set", Operation::SET},
    {"setex", Operation::SETEX},
    {"persist", Operation::PERSIST},
    {"expire", Operation::EXPIRE},
    {"delete", Operation::DELETE},
    {"flush", Operation::FLUSH},
    {"subscribe", Operation::SUBSCRIBE},
    {"publish", Operation::PUBLISH},
    {"create_queue", Operation::CREATE_QUEUE},
    {"delete_queue", Operation::DELETE_QUEUE}};
