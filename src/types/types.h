#pragma once

#include <openssl/ssl.h>

#include <cstdint>
#include <string>
#include <unordered_map>

enum class Operation {
    AUTHENTICATE,
    GET,
    SET,
    SETEX,
    DELETE,
    PERSIST,
    EXPIRE,
    FLUSH,
    SUBSCRIBE,
    UNSUBSCRIBE,
    PUBLISH,
    CREATE_QUEUE,
    DELETE_QUEUE,
};

enum class SessionMode {
    DB,
    PUBSUB,
};

struct Connection {
    bool authenticated;
    SessionMode mode;
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
    {"authenticate", Operation::AUTHENTICATE},
    {"get", Operation::GET},
    {"set", Operation::SET},
    {"setex", Operation::SETEX},
    {"persist", Operation::PERSIST},
    {"expire", Operation::EXPIRE},
    {"delete", Operation::DELETE},
    {"flush", Operation::FLUSH},
    {"subscribe", Operation::SUBSCRIBE},
    {"unsubscribe", Operation::UNSUBSCRIBE},
    {"publish", Operation::PUBLISH},
    {"create_queue", Operation::CREATE_QUEUE},
    {"delete_queue", Operation::DELETE_QUEUE}};
