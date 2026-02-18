#include "valt.h"

#include <db/database_mixin.h>
#include <utils/exceptions.h>
#include <utils/utils.h>

#include <cstdlib>
#include <regex>
#include <vector>

const std::regex int_re(R"(^[+-]?\d+$)");

std::string Valt::execute(const std::string& request_string, const int& client_fd, SSL* ssl) {
    try {
        Request req = parseRequest(request_string);
        req.client_fd = client_fd;
        req.ssl = ssl;

        validate_authenticated(req);
        validate_session_mode(req);

        std::string response = performRequest(req);
        return response;
    } catch (std::exception& e) {
        return std::string(e.what());
    }
}

std::string Valt::performRequest(const Request& req) {
    switch (req.op) {
        case Operation::AUTHENTICATE:
            return authenticate(req);
        case Operation::GET:
            return get(req);
        case Operation::SET:
            return set(req);
        case Operation::SETEX:
            return set(req);
        case Operation::DELETE:
            return del(req);
        case Operation::EXPIRE:
            return expire(req);
        case Operation::PERSIST:
            return persist(req);
        case Operation::FLUSH:
            flush_db();
            flush_queues();
            return OK;
        case Operation::CREATE_QUEUE:
            return createQueue(req);
        case Operation::DELETE_QUEUE:
            return deleteQueue(req);
        case Operation::SUBSCRIBE:
            set_client_mode(req.client_fd, SessionMode::PUBSUB);
            return subscribe(req);
        case Operation::UNSUBSCRIBE:
            set_client_mode(req.client_fd, SessionMode::DB);
            return unsubscribe(req);
        case Operation::PUBLISH:
            return publish(req);
        default:
            throw UnknownCommandException();
    }
}

Request Valt::parseRequest(const std::string& request_string) {
    std::vector<std::string> args = utils::split(request_string, ' ');
    uint8_t num_args = args.size();

    Operation operation = utils::to_operation(args[0]);
    std::string key;
    std::string value;
    int64_t expiration = -1;

    switch (operation) {
        case Operation::AUTHENTICATE:
            if (num_args != 2) throw InvalidCommandException();
            value = args[1];
            break;
        case Operation::GET:
            if (num_args != 2) throw InvalidCommandException();
            key = args[1];
            break;
        case Operation::SET:
            if (num_args != 3) throw InvalidCommandException();
            key = args[1];
            value = args[2];
            break;
        case Operation::SETEX:
            if (num_args != 4) throw InvalidCommandException();
            key = args[1];
            value = args[2];
            expiration = parseExpiration(args[3]);
            break;
        case Operation::DELETE:
            if (num_args != 2) throw InvalidCommandException();
            key = args[1];
            break;
        case Operation::PERSIST:
            if (num_args != 2) throw InvalidCommandException();
            key = args[1];
            break;
        case Operation::EXPIRE:
            if (num_args != 3) throw InvalidCommandException();
            key = args[1];
            expiration = parseExpiration(args[2]);
            break;
        case Operation::FLUSH:
            if (num_args != 1) throw InvalidCommandException();
            break;
        case Operation::CREATE_QUEUE:
            if (num_args != 2) throw InvalidCommandException();
            key = args[1];
            break;
        case Operation::DELETE_QUEUE:
            if (num_args != 2) throw InvalidCommandException();
            key = args[1];
            break;
        case Operation::SUBSCRIBE:
            if (num_args != 2) throw InvalidCommandException();
            key = args[1];
            break;
        case Operation::UNSUBSCRIBE:
            if (num_args != 2) throw InvalidCommandException();
            key = args[1];
            break;
        case Operation::PUBLISH:
            if (num_args != 3) throw InvalidCommandException();
            key = args[1];
            value = args[2];
            break;
    }

    return Request{.op = operation, .key = key, .value = value, .expiration = expiration};
}

int64_t Valt::parseExpiration(const std::string& expiration) {
    if (!std::regex_match(expiration, int_re)) {
        throw InvalidCommandException();
    }
    return static_cast<int64_t>(utils::seconds_since_epoch()) + std::abs(std::stoi(expiration));
}
