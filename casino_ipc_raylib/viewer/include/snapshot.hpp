#pragma once

#include <cstdint>
#include <array>
#include "protocol.hpp"

struct CasinoSnap {
    uint64_t tick = 0;
    int64_t jackpot = 0;
    int32_t rounds = 0;
    int32_t lastWinnerId = -1;
    int32_t lastWinAmount = 0;
    int32_t playerCount = 0;
    casino::PlayerState players[casino::MAX_PLAYERS]{};
    // mirrored instrumentation from shared state
    int32_t mutex_held = 0;
    uint64_t mutex_last_held_ts = 0;
    int32_t sem_value = 0;
    int32_t mq_count = 0;
};
