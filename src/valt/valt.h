#pragma once

#include <db/database_mixin.h>
#include <permissions/permissions_mixin.h>
#include <queue/queue_mixin.h>
#include <types/types.h>

#include <cstdint>
#include <string>

class Valt : public PermissionsMixin, DataBaseMixin, QueueMixin {
   private:
    Valt() = default;
    ~Valt() = default;

    Valt(const Valt&) = delete;
    Valt& operator=(const Valt&) = delete;
    Valt(Valt&&) = delete;
    Valt& operator=(Valt&&) = delete;

    Request parseRequest(const std::string&);
    int64_t parseExpiration(const std::string&);
    std::string performRequest(const Request&);

   public:
    std::string execute(const std::string&, const int& client_fd, SSL* ssl);

    static Valt& getInstance() {
        static Valt instance;
        return instance;
    }
};
