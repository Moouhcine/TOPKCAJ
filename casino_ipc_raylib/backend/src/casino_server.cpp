#include "ipc_shared.hpp"
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <fcntl.h>
#include <iostream>
#include <mqueue.h>
#include <cmath>
#include <random>
#include <string>
#include <thread>
#include <algorithm>
#include <cstring>
#include <fstream>
#include <vector>
#include <semaphore.h>

using namespace std::chrono_literals;

namespace {
volatile std::sig_atomic_t g_running = 1;

void handle_sigint(int) { g_running = 0; }

struct TargetPos {
    float x = 0.0f;
    float y = 0.0f;
    std::chrono::steady_clock::time_point nextChange;
};

constexpr int SPIN_COST = 20;
constexpr int SYMBOL_COUNT = 4;

struct SpinTimers {
    std::chrono::steady_clock::time_point nextAllowed;
    float cooldownMin = 2.0f;
    float cooldownMax = 5.0f;
    std::chrono::steady_clock::time_point nextRandomStart;
};

static bool load_layout(const std::string& path, std::vector<TargetPos>& out, int playerCount) {
    std::ifstream f(path);
    if (!f.is_open()) return false;
    int idx = 0;
    std::string tag;
    while (f >> tag) {
        if (tag == "P") {
            float x, y;
            if (!(f >> x >> y)) break;
            if (idx < playerCount && idx < (int)out.size()) {
                out[idx].x = x;
                out[idx].y = y;
            }
            idx++;
        } else if (tag == "S") {
            // skip scale line
            std::string rest;
            std::getline(f, rest);
        }
    }
    return true;
}

} // namespace

int main(int argc, char** argv) {
    int playerCount = 6;
    unsigned int seed = static_cast<unsigned int>(std::random_device{}());

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--players" && i + 1 < argc) {
            playerCount = std::atoi(argv[++i]);
            if (playerCount < 1) playerCount = 1;
            if (playerCount > casino::MAX_PLAYERS) playerCount = casino::MAX_PLAYERS;
        } else if (arg == "--seed" && i + 1 < argc) {
            seed = static_cast<unsigned int>(std::strtoul(argv[++i], nullptr, 10));
        }
    }

    std::cout << "[server] starting with players=" << playerCount << " seed=" << seed << "\n";
    std::signal(SIGINT, handle_sigint);

    auto handleOpt = casino::open_shared_memory(true);
    if (!handleOpt) {
        return 1;
    }
    casino::SharedHandle shm = *handleOpt;
    if (!casino::initialize_state(shm.state)) {
        std::cerr << "[server] failed to init shared state\n";
        casino::close_shared_memory(shm);
        return 1;
    }

    sem_unlink(casino::SEM_NAME);
    sem_t* sem = sem_open(casino::SEM_NAME, O_CREAT, 0666, 0);
    bool semOk = sem != SEM_FAILED;
    if (!semOk) {
        std::cerr << "[server] sem_open failed, continuing without semaphore wakeup\n";
    }

    mq_unlink(casino::MQ_NAME);
    struct mq_attr attr{};
    attr.mq_maxmsg = 10; // stay within typical /proc/sys/fs/mqueue/msg_max default
    attr.mq_msgsize = sizeof(casino::BetMessage);
    attr.mq_flags = 0;
    attr.mq_curmsgs = 0;
    mqd_t mq = mq_open(casino::MQ_NAME, O_CREAT | O_RDONLY | O_NONBLOCK, 0666, &attr);
    if (mq == static_cast<mqd_t>(-1)) {
        std::cerr << "[server] mq_open failed: " << std::strerror(errno) << "\n";
        std::cerr << "Hint: ensure /dev/mqueue is mounted (sudo mount -t mqueue none /dev/mqueue) or run scripts/clean_ipc.sh then retry.\n";
        casino::close_shared_memory(shm);
        return 1;
    }

    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> winChance(1, 100);

    std::vector<TargetPos> targets(casino::MAX_PLAYERS);
    const float cx = 960.0f;
    const float cy = 540.0f + 60.0f;
    const float radius = 300.0f;
    for (int i = 0; i < playerCount; ++i) {
        float angle = 3.14159f / 2 + (6.28318f * i / std::max(6, playerCount));
        targets[i].x = cx + radius * std::cos(angle);
        targets[i].y = cy + radius * std::sin(angle);
    }
    // Load layout.txt if present to override positions
    if (!load_layout("viewer/layout.txt", targets, playerCount)) {
        load_layout("layout.txt", targets, playerCount);
    }

    pthread_mutex_lock(&shm.state->mutex);
    shm.state->playerCount = playerCount;
    shm.state->jackpot = 600; // banque initiale: 6 joueurs * 5 tours * coût fixe
    for (int i = 0; i < playerCount; ++i) {
        shm.state->players[i].id = i;
        shm.state->players[i].x = targets[i].x;
        shm.state->players[i].y = targets[i].y;
        shm.state->players[i].animState = casino::ANIM_IDLE;
        shm.state->players[i].pulse = 0.0f;
    }
    pthread_mutex_unlock(&shm.state->mutex);

    auto lastPulseDecay = std::chrono::steady_clock::now();
    const auto spinDuration = 2s;
    std::vector<SpinTimers> timers(casino::MAX_PLAYERS);
    auto nowInit = std::chrono::steady_clock::now();
    constexpr float MIN_COOLDOWN = 2.2f;
    std::uniform_int_distribution<int> initialJitter(0, 800);
    std::uniform_int_distribution<int> randomStart(1000, 4000);
    for (int i = 0; i < casino::MAX_PLAYERS; ++i) {
        timers[i].nextAllowed = nowInit + std::chrono::milliseconds(200 * i + initialJitter(rng));
        timers[i].cooldownMin = MIN_COOLDOWN + (i * 0.1f);
        timers[i].cooldownMax = 4.5f + (i * 0.2f);
        timers[i].nextRandomStart = nowInit + std::chrono::milliseconds(randomStart(rng));
    }

    // weights for symbols: 7 (rare), diamond, bell, strawberry (common)
    int weights[SYMBOL_COUNT] = {1, 3, 5, 8};
    int payouts[SYMBOL_COUNT] = {400, 220, 140, 90};

    auto pick_symbol = [&]() {
        int total = 0;
        for (int w : weights) total += w;
        std::uniform_int_distribution<int> dist(1, total);
        int r = dist(rng);
        int acc = 0;
        for (int i = 0; i < SYMBOL_COUNT; ++i) {
            acc += weights[i];
            if (r <= acc) return i;
        }
        return SYMBOL_COUNT - 1;
    };

    auto run_spin = [&](int playerId) {
        int symA = pick_symbol();
        int symB = pick_symbol();
        int symC = pick_symbol();
        bool win = (symA == symB && symB == symC);
        int payout = win ? payouts[symA] : 0;
        int delta = payout - SPIN_COST;

        pthread_mutex_lock(&shm.state->mutex);
        shm.state->tick++;
        shm.state->rounds++;
        shm.state->jackpot += delta;
        if (shm.state->jackpot < 0) shm.state->jackpot = 0;
        auto& p = shm.state->players[playerId];
        p.symbols[0] = symA;
        p.symbols[1] = symB;
        p.symbols[2] = symC;
        p.lastDelta = delta;
        p.lastPayout = payout;
        p.spinning = 1;
        p.spinProgress = 0.0f;
        p.animState = win ? casino::ANIM_WIN : casino::ANIM_LOSE;
        p.pulse = win ? 1.0f : 0.3f;
        shm.state->lastWinnerId = win ? playerId : -1;
        shm.state->lastWinAmount = payout;
        pthread_mutex_unlock(&shm.state->mutex);
    };

    while (g_running) {
        // Attendre un signal léger (semaphore) ou timeout pour éviter le busy loop
        if (semOk) {
            struct timespec ts{};
            auto nowWake = std::chrono::steady_clock::now() + 16ms;
            auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(nowWake.time_since_epoch()).count();
            ts.tv_sec = static_cast<time_t>(ns / 1000000000);
            ts.tv_nsec = static_cast<long>(ns % 1000000000);
            sem_timedwait(sem, &ts); // ignore ETIMEDOUT/errors
        } else {
            std::this_thread::sleep_for(2ms);
        }

        casino::BetMessage msg{};
        while (true) {
            ssize_t r = mq_receive(mq, reinterpret_cast<char*>(&msg), sizeof(msg), nullptr);
            if (r >= 0) {
                int pid = msg.playerId % casino::MAX_PLAYERS;
                auto now = std::chrono::steady_clock::now();
                if (pid < 0 || pid >= playerCount) continue;
                if (now >= timers[pid].nextAllowed) {
                    float cd = std::uniform_real_distribution<float>(timers[pid].cooldownMin, timers[pid].cooldownMax)(rng);
                    timers[pid].nextAllowed = now + std::chrono::milliseconds((int)(cd * 1000));
                    run_spin(pid);
                }
            } else {
                break;
            }
        }

        // Lancer des spins aléatoires même sans message, pour désynchroniser encore plus
        auto nowRandom = std::chrono::steady_clock::now();
        for (int pid = 0; pid < playerCount; ++pid) {
            auto& t = timers[pid];
            if (nowRandom >= t.nextRandomStart && nowRandom >= t.nextAllowed) {
                float cd = std::uniform_real_distribution<float>(t.cooldownMin, t.cooldownMax)(rng);
                t.nextAllowed = nowRandom + std::chrono::milliseconds((int)(cd * 1000));
                t.nextRandomStart = nowRandom + std::chrono::milliseconds(randomStart(rng));
                run_spin(pid);
            }
        }

        auto now = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(now - lastPulseDecay).count();
        lastPulseDecay = now;

        pthread_mutex_lock(&shm.state->mutex);
        shm.state->tick++;
        for (int i = 0; i < shm.state->playerCount; ++i) {
            auto& p = shm.state->players[i];
            if (p.spinning) {
                p.spinProgress += dt / std::chrono::duration<float>(spinDuration).count();
                if (p.spinProgress >= 1.0f) {
                    p.spinning = 0;
                    p.spinProgress = 1.0f;
                }
            }
            p.pulse = std::max(0.0f, p.pulse - 0.6f * dt);
        }
        pthread_mutex_unlock(&shm.state->mutex);

        std::this_thread::sleep_for(16ms);
    }

    mq_close(mq);
    if (semOk) {
        sem_close(sem);
        sem_unlink(casino::SEM_NAME);
    }
    casino::close_shared_memory(shm);
    std::cout << "[server] stopped" << std::endl;
    return 0;
}
