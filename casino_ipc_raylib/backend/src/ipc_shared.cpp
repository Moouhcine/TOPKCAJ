#include "ipc_shared.hpp"

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <mqueue.h>
#include <semaphore.h>

namespace casino {

std::optional<SharedHandle> open_shared_memory(bool owner) {
    int flags = O_RDWR;
    if (owner) {
        flags |= O_CREAT;
    }
    int fd = shm_open(SHM_NAME, flags, 0666);
    if (fd < 0) {
        std::cerr << "[ipc] shm_open failed: " << std::strerror(errno) << "\n";
        return std::nullopt;
    }

    if (owner) {
        if (ftruncate(fd, sizeof(SharedState)) != 0) {
            std::cerr << "[ipc] ftruncate failed: " << std::strerror(errno) << "\n";
            close(fd);
            return std::nullopt;
        }
    }

    void* addr = mmap(nullptr, sizeof(SharedState), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (addr == MAP_FAILED) {
        std::cerr << "[ipc] mmap failed: " << std::strerror(errno) << "\n";
        close(fd);
        return std::nullopt;
    }

    SharedHandle handle{};
    handle.fd = fd;
    handle.state = reinterpret_cast<SharedState*>(addr);
    handle.owner = owner;
    return handle;
}

void close_shared_memory(SharedHandle& handle) {
    if (handle.state) {
        munmap(handle.state, sizeof(SharedState));
        handle.state = nullptr;
    }
    if (handle.fd >= 0) {
        close(handle.fd);
        handle.fd = -1;
    }
}

void unlink_ipc() {
    shm_unlink(SHM_NAME);
    mq_unlink(MQ_NAME);
    sem_unlink(SEM_NAME);
}

bool initialize_state(SharedState* state) {
    if (!state) return false;
    pthread_mutexattr_t attr;
    if (pthread_mutexattr_init(&attr) != 0) {
        return false;
    }
    if (pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED) != 0) {
        pthread_mutexattr_destroy(&attr);
        return false;
    }
    if (pthread_mutex_init(&state->mutex, &attr) != 0) {
        pthread_mutexattr_destroy(&attr);
        return false;
    }
    pthread_mutexattr_destroy(&attr);

    state->tick = 0;
    state->jackpot = 0;
    state->rounds = 0;
    state->lastWinnerId = -1;
    state->lastWinAmount = 0;
    state->playerCount = 0;
    for (auto& p : state->players) {
        p = PlayerState{};
    }
    return true;
}

} // namespace casino
