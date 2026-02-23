#include <conn/connection.h>

#include <mutex>

class ValtState {
    /*
    Global state for the program
    */
   private:
    std::unordered_map<int, std::unique_ptr<Connection>> conns;

    ValtState() : memory_used(0) {};
    ValtState(const ValtState&) = delete;
    ValtState& operator=(const ValtState&) = delete;
    ValtState(ValtState&&) = delete;
    ValtState& operator=(ValtState&&) = delete;

   public:
    std::mutex mem_mtx;
    size_t memory_used;

    static ValtState* getInstance() {
        static ValtState instance;
        return &instance;
    }

    std::unordered_map<int, std::unique_ptr<Connection>>& get_connections() { return conns; }
};
