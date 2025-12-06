#include "ipc_attach.hpp"

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

std::optional<SharedAttachment> attach_shared_state() {
    int fd = shm_open(casino::SHM_NAME, O_RDWR, 0666);
    if (fd < 0) {
        std::cerr << "[viewer] shm_open failed: " << std::strerror(errno) << "\n";
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
    if (pthread_mutex_lock(&att.state->mutex) != 0) return false;
    out.tick = att.state->tick;
    out.jackpot = att.state->jackpot;
    out.rounds = att.state->rounds;
    out.lastWinnerId = att.state->lastWinnerId;
    out.lastWinAmount = att.state->lastWinAmount;
    out.playerCount = att.state->playerCount;
    for (int i = 0; i < att.state->playerCount && i < casino::MAX_PLAYERS; ++i) {
        out.players[i] = att.state->players[i];
    }
    pthread_mutex_unlock(&att.state->mutex);
    return true;
}
