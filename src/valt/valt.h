#pragma once

#include <db/database_mixin.h>
#include <perms/permissions_mixin.h>
#include <pubsub/pubsub_mixin.h>

#include <cstdint>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>

#include "valt_config.h"

class Valt : public PermissionsMixin, DataBaseMixin, PubSubMixin {
   private:
    std::thread bg_mem_update;
    std::atomic<bool> stop;
    const ValtConfig* cfg;

    Valt() = delete;
    Valt(const ValtConfig* valt_config)
        : cfg(valt_config), PermissionsMixin(valt_config), DataBaseMixin(valt_config), PubSubMixin(valt_config) {
        stop.store(false);
        bg_mem_update = std::thread(&Valt::update_memory_in_bg, this);
    }
    ~Valt() {
        stop.store(true);
        bg_mem_update.join();
    }

    Valt(const Valt&) = delete;
    Valt& operator=(const Valt&) = delete;
    Valt(Valt&&) = delete;
    Valt& operator=(Valt&&) = delete;

    Request parseRequest(const std::string&);
    int64_t parseExpiration(const std::string&);
    std::string performRequest(const Request&);
    void update_memory_in_bg();

   public:
    std::string execute(const std::string&, const int& client_fd);

    static Valt& getInstance(const ValtConfig* cfg = nullptr) {
        static std::mutex mtx;
        static Valt* instance = nullptr;

        std::lock_guard<std::mutex> lock(mtx);

        if (instance == nullptr) {
            if (cfg == nullptr) {
                throw std::runtime_error("Valt must be initialized with its config");
            }
            instance = new Valt(cfg);
        }

        return *instance;
    }
};
