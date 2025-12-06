#include "ipc_shared.hpp"
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <mqueue.h>
#include <random>
#include <thread>

using namespace std::chrono_literals;

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: player <id>" << std::endl;
        return 1;
    }
    int id = std::atoi(argv[1]);
    if (id < 0 || id >= casino::MAX_PLAYERS) {
        std::cerr << "[player] invalid id" << std::endl;
        return 1;
    }

    mqd_t mq = mq_open(casino::MQ_NAME, O_WRONLY);
    if (mq == static_cast<mqd_t>(-1)) {
        std::cerr << "[player] mq_open failed (server not running?)" << std::endl;
        return 1;
    }

    std::mt19937 rng(static_cast<unsigned int>(std::chrono::steady_clock::now().time_since_epoch().count()) + id * 31);
    std::uniform_int_distribution<int> betDist(10, 120);
    std::uniform_int_distribution<int> startJitter(0, 1200);     // désynchronise le premier envoi
    std::uniform_int_distribution<int> pauseJitter(600, 1800);   // cadence variable par joueur

    // Décalage initial pour éviter que tous les joueurs envoient en même temps
    int initialDelayMs = startJitter(rng) + id * 150;
    std::this_thread::sleep_for(std::chrono::milliseconds(initialDelayMs));

    // Pause de base différente par joueur pour casser la synchro
    int basePauseMs = 1200 + id * 320;

    casino::BetMessage msg{};
    msg.playerId = id;

    while (true) {
        msg.amount = betDist(rng);
        if (mq_send(mq, reinterpret_cast<const char*>(&msg), sizeof(msg), 0) != 0) {
            std::cerr << "[player] mq_send failed" << std::endl;
        }
        int pause = basePauseMs + pauseJitter(rng);
        std::this_thread::sleep_for(std::chrono::milliseconds(pause));
    }

    mq_close(mq);
    return 0;
}
