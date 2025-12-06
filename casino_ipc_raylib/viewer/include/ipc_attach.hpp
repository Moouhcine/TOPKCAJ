#pragma once

#include <optional>
#include <pthread.h>
#include "snapshot.hpp"

struct SharedAttachment {
    int fd = -1;
    casino::SharedState* state = nullptr;
    bool valid = false;
};

std::optional<SharedAttachment> attach_shared_state();
void detach_shared_state(SharedAttachment&);
bool copy_snapshot(SharedAttachment&, CasinoSnap& out);
