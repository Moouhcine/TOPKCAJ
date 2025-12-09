#include "ipc_attach.hpp"
#include "ipc_shared.hpp" // for safe_mutex_lock and open_shared_memory

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <thread>
#include <chrono>

std::optional<SharedAttachment> attach_shared_state() {
    // Retry opening shared memory for a short period to avoid race at startup
    const int maxMs = 3000; // total wait time
    const int stepMs = 100; // wait step
    int waited = 0;
    int fd = -1;
    while (waited <= maxMs) {
        fd = shm_open(casino::SHM_NAME, O_RDWR, 0666);
        if (fd >= 0) break;
        // sleep and retry
        std::this_thread::sleep_for(std::chrono::milliseconds(stepMs));
        waited += stepMs;
    }
    if (fd < 0) {
        std::cerr << "[viewer] shm_open failed after retries: " << std::strerror(errno) << "\n";
        return std::nullopt;
    }
    void* addr = mmap(nullptr, sizeof(casino::SharedState), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (addr == MAP_FAILED) {
        std::cerr << "[viewer] mmap failed: " << std::strerror(errno) << "\n";
        close(fd);
        return std::nullopt;
    }
    SharedAttachment att{};
    att.fd = fd;
    att.state = reinterpret_cast<casino::SharedState*>(addr);
    att.valid = true;
    return att;
}

void detach_shared_state(SharedAttachment& att) {
    if (att.state) {
        munmap(att.state, sizeof(casino::SharedState));
        att.state = nullptr;
    }
    if (att.fd >= 0) {
        close(att.fd);
        att.fd = -1;
    }
    att.valid = false;
}

bool copy_snapshot(SharedAttachment& att, CasinoSnap& out) {
    if (!att.valid || !att.state) return false;
    // try a few times to lock the mutex, with short backoff
    const int maxAttempts = 6;
    int attempt = 0;
    bool locked = false;
    while (attempt < maxAttempts) {
        if (casino::safe_mutex_lock(&att.state->mutex)) {
            locked = true;
            break;
        }
        ++attempt;
        std::this_thread::sleep_for(std::chrono::milliseconds(20 * attempt));
    }
    if (!locked) {
        std::cerr << "[viewer] failed to lock shared mutex after retries\n";
        return false;
    }
    out.tick = att.state->tick;
    out.jackpot = att.state->jackpot;
    out.rounds = att.state->rounds;
    out.lastWinnerId = att.state->lastWinnerId;
    out.lastWinAmount = att.state->lastWinAmount;
    out.playerCount = att.state->playerCount;
    for (int i = 0; i < att.state->playerCount && i < casino::MAX_PLAYERS; ++i) {
        out.players[i] = att.state->players[i];
    }
    // copy instrumentation
    out.mutex_held = att.state->mutex_held;
    out.sem_value = att.state->sem_value;
    out.mq_count = att.state->mq_count;
    out.mutex_last_held_ts = att.state->mutex_last_held_ts;
    pthread_mutex_unlock(&att.state->mutex);
    return true;
}
