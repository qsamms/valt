#include "task_handler.h"

#include <arpa/inet.h>
#include <db/db.h>
#include <ev.h>
#include <unistd.h>
#include <utils/exceptions.h>
#include <utils/response_codes.h>
#include <utils/utils.h>

#include <chrono>
#include <iostream>

int64_t TaskHandler::parse_expiration(const std::string& expiration_str) {
    if (!std::regex_match(expiration_str, int_re)) {
        throw InvalidCommandException("expiration must be an integer");
    }

    int expiration_seconds = std::stoi(expiration_str);
    if (expiration_seconds < 0) {
        throw InvalidCommandException("expiration must be > 0");
    }

    return (int64_t)seconds_since_epoch() + expiration_seconds;
}

Command TaskHandler::parse_request(const std::string& s) {
    std::vector<std::string> cmd_parts = split(s, ' ');
    if (cmd_parts.size() < 2) {
        throw InvalidCommandException("All commands cuire an action and key");
    }

    Operation operation = string_to_op(cmd_parts[0]);
    switch (operation) {
        case Operation::SET:
            if (cmd_parts.size() != 3) throw InvalidCommandException("'set' must have 3 operands");
            break;
        case Operation::SETEX:
            if (cmd_parts.size() != 4)
                throw InvalidCommandException("'setex' must have 4 operands");
            break;
        case Operation::EXPIRE:
            if (cmd_parts.size() != 3)
                throw InvalidCommandException("'expire' must have 3 operands");
            break;
    }

    return Command{.op = operation,
                   .key = cmd_parts[1],
                   .value = cmd_parts.size() > 2 ? cmd_parts[2] : "",
                   .expiration = cmd_parts.size() > 3 ? parse_expiration(cmd_parts[3]) : -1};
}

std::string TaskHandler::perform_op(const Command& cmd) {
    switch (cmd.op) {
        case Operation::GET: {
            std::optional<DBEntry> entry = get(cmd.key);
            return entry ? entry->value + "\n" : ERR_NOT_FOUND;
        }
        case Operation::SET:
            return set(cmd) ? OK : ERR_UNKNOWN;
        case Operation::SETEX:
            return set(cmd) ? OK : ERR_UNKNOWN;
        case Operation::DELETE:
            return del(cmd.key) ? OK : ERR_UNKNOWN;
        case Operation::EXPIRE:
            return expire(cmd) ? OK : ERR_UNKNOWN;
        case Operation::PERSIST:
            return persist(cmd) ? OK : ERR_UNKNOWN;
        default:
            throw InvalidCommandException("unknown action");
    }
}

std::string TaskHandler::handle_task(const Task task) {
    try {
        Command r = parse_request(task.content);
        return perform_op(r);
    } catch (std::exception& e) {
        return std::string(e.what());
    }
}
