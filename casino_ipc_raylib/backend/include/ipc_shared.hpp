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

} // namespace casino
