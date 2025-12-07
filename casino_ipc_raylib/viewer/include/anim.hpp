#pragma once

#include <array>
#include <vector>
#include <raylib.h>
#include "snapshot.hpp"

struct PlayerVisual {
    bool active = false;
    Vector2 pos{0.0f, 0.0f};
    Vector2 target{0.0f, 0.0f};
    float pulse = 0.0f;
    int id = 0;
    casino::AnimState anim = casino::ANIM_IDLE;
    int symbols[3]{0,1,2};
    int lastDelta = 0;
    bool spinning = false;
    float spinProgress = 0.0f;
};

struct Confetto {
    Vector2 pos{};
    Vector2 vel{};
    Color color{};
    float life = 0.0f;
    bool alive = false;
};

struct SceneState {
    std::array<PlayerVisual, casino::MAX_PLAYERS> players;
    std::array<Confetto, 64> confetti;
    float glowPhase = 0.0f;
    int lastWinSeen = -1;
    std::array<std::vector<int>, casino::MAX_PLAYERS> history; // dernières variations
    std::array<float, casino::MAX_PLAYERS> lastResultTime{};   // timestamp (GetTime) du dernier résultat
    std::array<bool, casino::MAX_PLAYERS> prevSpinning{};      // pour détecter fin de spin
    std::array<bool, casino::MAX_PLAYERS> showWinPose{};       // sprite victoire actif ?
    int totalBank = 0;                                         // cumul gains/pertes
    bool bankInitialized = false;
    bool jackpotInitialized = false;
    int64_t jackpot = 0;
    bool gameOver = false;
    bool triggerWinSfx = false;
    bool triggerEmptySfx = false;
};

void update_scene(SceneState& scene, const CasinoSnap& snap, float dt);
