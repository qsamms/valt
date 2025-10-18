#include "task_handler.h"

#include <db/db.h>
#include <utils/exceptions.h>
#include <utils/response_codes.h>
#include <utils/utils.h>

#include <chrono>
#include <iostream>
#include <cstdlib>

int64_t TaskHandler::parse_expiration(const std::string& expiration) {
    if (!std::regex_match(expiration, int_re)) {
        throw InvalidCommandException("expiration must be an integer");
    }
    return (int64_t)seconds_since_epoch() + std::abs(std::stoi(expiration));
}

Command TaskHandler::parse_request(const std::string& s) {
    std::vector<std::string> cmd_parts = split(s, ' ');

    Operation operation = string_to_op(cmd_parts[0]);
    std::string key;
    std::string value;
    int64_t expiration = -1;

    switch (operation) {
        case Operation::SET:
            if (cmd_parts.size() != 3) throw InvalidCommandException("'set' must have 3 operands");
            value = cmd_parts[2];
            break;
        case Operation::SETEX:
            if (cmd_parts.size() != 4)
                throw InvalidCommandException("'setex' must have 4 operands");
            value = cmd_parts[2];
            expiration = parse_expiration(cmd_parts[3]);
            break;
        case Operation::EXPIRE:
            if (cmd_parts.size() != 3)
                throw InvalidCommandException("'expire' must have 3 operands");
            expiration = parse_expiration(cmd_parts[2]);
            break;
    }

    return Command{.op = operation, .key = key, .value = value, .expiration = expiration};
}

std::string TaskHandler::perform_op(const Command& cmd) {
    switch (cmd.op) {
        case Operation::GET: {
            std::optional<DBEntry> maybe_entry = get(cmd.key);
            return maybe_entry ? maybe_entry->value + "\n" : ERR_NOT_FOUND;
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
        case Operation::FLUSH:
            return flush(cmd) ? OK : ERR_UNKNOWN;
        default:
            throw InvalidCommandException("unknown action");
    }
}

std::string TaskHandler::handle_task(const std::string& task) {
    try {
        Command r = parse_request(task);
        return perform_op(r);
    } catch (std::exception& e) {
        return std::string(e.what());
    }
}
