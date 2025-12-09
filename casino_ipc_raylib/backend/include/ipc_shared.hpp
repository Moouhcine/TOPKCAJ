#pragma once

#include "protocol.hpp"
#include <optional>
#include <string>

namespace casino {

struct SharedHandle {
    int fd = -1;
    SharedState* state = nullptr;
    bool owner = false;
};

// Create or open shared memory; owner=true creates/initialises with mutex attributes.
std::optional<SharedHandle> open_shared_memory(bool owner);

// Closes fd + unmaps (does not unlink).
void close_shared_memory(SharedHandle& handle);

// Unlink SHM and MQ names.
void unlink_ipc();

// Initialize mutex/process-shared and zero state. Only call once (owner path).
bool initialize_state(SharedState* state);

// Lock helper that handles owner-dead robust mutexes.
inline bool safe_mutex_lock(pthread_mutex_t* m) {
    if (!m) return false;
    int rv = pthread_mutex_lock(m);
    if (rv == 0) return true;
#ifdef EOWNERDEAD
    if (rv == EOWNERDEAD) {
        // If owner died, try to mark mutex consistent and continue.
        if (pthread_mutex_consistent(m) == 0) return true;
    }
#endif
    return false;
}

} // namespace casino
