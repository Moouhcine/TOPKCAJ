#pragma once

#include <cstdint>
#include <pthread.h>

namespace casino {

constexpr const char* SHM_NAME = "/casino_ipc_shared";
constexpr const char* MQ_NAME = "/casino_ipc_mq";
constexpr const char* SEM_NAME = "/casino_ipc_sem";
constexpr int MAX_PLAYERS = 16;

enum AnimState : int32_t {
    ANIM_IDLE = 0,
    ANIM_WALK = 1,
    ANIM_WIN = 2,
    ANIM_LOSE = 3,
};

struct PlayerState {
    int32_t id = -1;
    float x = 0.0f;
    float y = 0.0f;
    int32_t animState = ANIM_IDLE;
    float pulse = 0.0f; // 0..1 for VFX
    int32_t symbols[3] = {0, 1, 2}; // last slot reel symbols
    int32_t lastDelta = 0;          // win (+) or cost (-)
    int32_t spinning = 0;           // 1 while animating
    float spinProgress = 0.0f;      // 0..1 over 3s window
    int32_t lastPayout = 0;
};

struct SharedState {
    pthread_mutex_t mutex; // process-shared
    uint64_t tick = 0;
    int64_t jackpot = 0;
    int32_t rounds = 0;
    int32_t lastWinnerId = -1;
    int32_t lastWinAmount = 0;
    int32_t playerCount = 0;
    PlayerState players[MAX_PLAYERS];
};

struct BetMessage {
    int32_t playerId;
    int32_t amount;
};

} // namespace casino
