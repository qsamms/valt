#pragma once

#include <db/database_mixin.h>
#include <queue/queue_mixin.h>
#include <types/types.h>

#include <cstdint>
#include <string>

class RequestHandler : DataBaseMixin, QueueMixin {
   private:
    RequestHandler() = default;
    ~RequestHandler() = default;

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;
    RequestHandler(RequestHandler&&) = delete;
    RequestHandler& operator=(RequestHandler&&) = delete;

    Request parseRequest(const std::string&);
    int64_t parseExpiration(const std::string&);
    std::string performRequest(const Request&);

   public:
    std::string execute(const std::string&, const int& client_fd, SSL* ssl);

    static RequestHandler& getInstance() {
        static RequestHandler instance;
        return instance;
    }
};
