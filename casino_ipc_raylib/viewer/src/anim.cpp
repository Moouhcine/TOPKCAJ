#include "anim.hpp"

#include <cmath>
#include <algorithm>
#include <raylib.h>
#include <raymath.h>

void update_scene(SceneState& scene, const CasinoSnap& snap, float dt) {
    scene.triggerWinSfx = false;
    scene.triggerEmptySfx = false;
    scene.jackpot = snap.jackpot;
    if (!scene.jackpotInitialized && snap.jackpot > 0) {
        scene.jackpotInitialized = true;
    }
    if (!scene.gameOver && scene.jackpotInitialized && snap.jackpot <= 0) {
        scene.triggerEmptySfx = true;
        scene.gameOver = true;
    }
    for (int i = 0; i < casino::MAX_PLAYERS; ++i) {
        auto& pv = scene.players[i];
        if (i < snap.playerCount) {
            int pid = snap.players[i].id;
            int slot = pid;
            if (slot < 0 || slot >= casino::MAX_PLAYERS) slot = i;
            int targetSlot = slot;
            if (snap.playerCount > 0) {
                targetSlot = (slot - 1 + snap.playerCount) % snap.playerCount; // décale la célébration sur le sprite précédent
            }
            pv.active = true;
            const float offsetX = 260.0f - 62.0f; // base offset minus left shift
            const float offsetY = 150.0f + 60.0f; // conserve le décalage vertical précédent
            pv.target = {snap.players[i].x + offsetX, snap.players[i].y + offsetY};
            if (pv.pos.x == 0.0f && pv.pos.y == 0.0f) {
                pv.pos = pv.target;
            } else {
                pv.pos = Vector2Lerp(pv.pos, pv.target, std::min(1.0f, dt * 6.0f));
            }
            pv.anim = static_cast<casino::AnimState>(snap.players[i].animState);
            pv.pulse = snap.players[i].pulse;
            pv.id = pid;
            pv.symbols[0] = snap.players[i].symbols[0];
            pv.symbols[1] = snap.players[i].symbols[1];
            pv.symbols[2] = snap.players[i].symbols[2];
            pv.lastDelta = snap.players[i].lastDelta;
            pv.spinning = snap.players[i].spinning != 0;
            pv.spinProgress = snap.players[i].spinProgress;

            bool prevSpin = scene.prevSpinning[slot];
            if (pv.spinning) {
                // reroll: reset win pose to base sprite immediately
                scene.showWinPose[slot] = false;
                scene.showWinPose[targetSlot] = false;
            }

            // détecter fin de spin pour historiser le delta
            if (prevSpin && !pv.spinning) {
                float now = GetTime();
                scene.lastResultTime[slot] = now;
                auto& hist = scene.history[slot];
                hist.push_back(pv.lastDelta);
                if (hist.size() > 4) {
                    hist.erase(hist.begin(), hist.end() - 4);
                }
                // cumul global + sprite victoire
                scene.totalBank += pv.lastDelta;
                scene.bankInitialized = true;
                scene.showWinPose[targetSlot] = pv.lastDelta > 0;
                if (pv.lastDelta > 0) {
                    scene.triggerWinSfx = true;
                }
            }
            scene.prevSpinning[slot] = pv.spinning;
        } else {
            pv.active = false;
        }
    }

    scene.glowPhase += dt * 2.0f;
    if (scene.glowPhase > 6.28318f) scene.glowPhase -= 6.28318f;

    // Confettis déclenchés à la fin d'un spin gagnant (pas dès le résultat backend)
    for (int i = 0; i < snap.playerCount; ++i) {
        int pid = snap.players[i].id;
        if (pid < 0 || pid >= casino::MAX_PLAYERS) pid = i;
        bool prevSpin = scene.prevSpinning[pid];
        bool nowSpin = scene.players[i].spinning;
        bool justEnded = prevSpin && !nowSpin;
        bool win = scene.players[i].lastDelta > 0;
        if (justEnded && win && scene.lastWinSeen != pid) {
            scene.lastWinSeen = pid;
            float baseX = scene.players[i].pos.x;
            float baseY = scene.players[i].pos.y - 40;
            for (auto& c : scene.confetti) {
                c.alive = true;
                c.life = 0.6f + GetRandomValue(0, 30) / 100.0f;
                c.pos = {baseX + GetRandomValue(-20, 20), baseY + GetRandomValue(-10, 10)};
                c.vel = {(float)GetRandomValue(-30, 30), (float)GetRandomValue(20, 80)};
                c.color = Color{(unsigned char)GetRandomValue(200, 255), (unsigned char)GetRandomValue(140, 240), (unsigned char)GetRandomValue(80, 220), 255};
            }
        }
    }

    for (auto& c : scene.confetti) {
        if (!c.alive) continue;
        c.pos.x += c.vel.x * dt;
        c.pos.y += c.vel.y * dt;
        c.vel.y += 40.0f * dt;
        c.life -= dt;
        if (c.life <= 0.0f) c.alive = false;
    }
}
