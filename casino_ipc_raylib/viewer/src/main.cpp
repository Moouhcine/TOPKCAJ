#include <chrono>
#include <filesystem>
#include <cmath>
#include <iostream>
#include <raylib.h>
#include <algorithm>
#include <cstdlib>
#include <cstdlib>

#include "assets.hpp"
#include "ipc_attach.hpp"
#include "render.hpp"
#include "anim.hpp"
#include "layout_config.hpp"

int main() {
    RenderSettings cfg{};
    const char* envW = std::getenv("VIEWER_W");
    const char* envH = std::getenv("VIEWER_H");
    if (envW) cfg.width = std::max(640, std::atoi(envW));
    if (envH) cfg.height = std::max(360, std::atoi(envH));
    int monCount = GetMonitorCount();
    int mon = monCount > 0 ? GetCurrentMonitor() : 0;
    int screenW = monCount > 0 ? GetMonitorWidth(mon) : 1920;
    int screenH = monCount > 0 ? GetMonitorHeight(mon) : 1080;
    int marginW = 80;
    int marginH = 120;
    int maxW = std::max(640, screenW - marginW);
    int maxH = std::max(480, screenH - marginH);
    cfg.width = std::clamp(cfg.width, 800, std::max(800, maxW));
    cfg.height = std::clamp(cfg.height, 600, std::max(600, maxH));
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_ALWAYS_RUN);
    InitWindow(cfg.width, cfg.height, "Casino IPC Viewer");
    SetTargetFPS(60);

    std::string assetBase = std::filesystem::exists("assets") ? "assets" : "viewer/assets";
    if (!std::filesystem::exists(assetBase)) {
        assetBase = "assets"; // fallback if executed from viewer/
    }
    Assets assets = load_assets(assetBase);

    LayoutParams lp{};
    std::vector<SlotLayout> slots(casino::MAX_PLAYERS);
    std::vector<Vector2> slotPositions;
    if (!load_scene_file("scene.json", lp, slots, &slotPositions)) {
        if (!load_tiled_tmj("scene.tmj", lp, slots, &slotPositions)) {
            if (!load_layout_file("viewer/layout.txt", lp, slots)) {
                load_layout_params("layout.txt", lp);
            }
        }
    }
    set_layout_params(lp);
    for (int i = 0; i < (int)slots.size(); ++i) {
        if (slots[i].set) set_slot_layout(i, slots[i]);
    }
    for (int i = 0; i < (int)slotPositions.size() && i < casino::MAX_PLAYERS; ++i) {
        set_slot_position(i, slotPositions[i]);
    }

    auto attachmentOpt = attach_shared_state();
    bool attached = attachmentOpt.has_value();
    if (!attached) {
        std::cerr << "[viewer] Could not attach SHM; running in fallback demo mode." << std::endl;
    }

    CasinoSnap snap{};
    SceneState scene{};

    auto lastFallback = std::chrono::steady_clock::now();
    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        if (attached) {
            if (!copy_snapshot(*attachmentOpt, snap)) {
                attached = false;
            }
        } else {
            // fallback animation if no SHM
            auto now = std::chrono::steady_clock::now();
            float t = std::chrono::duration<float>(now - lastFallback).count();
            lastFallback = now;
            snap.playerCount = slotPositions.empty() ? 4 : (int)std::min<size_t>(slotPositions.size(), casino::MAX_PLAYERS);
            snap.tick++;
            snap.jackpot = 1000 + (int)(200 * std::sin(GetTime()));
            snap.rounds++;
            for (int i = 0; i < snap.playerCount; ++i) {
                snap.players[i].id = i;
                if (i < (int)slotPositions.size()) {
                    snap.players[i].x = slotPositions[i].x;
                    snap.players[i].y = slotPositions[i].y;
                } else {
                    snap.players[i].x = 400 + 200 * std::cos(GetTime() * 0.8f + i);
                    snap.players[i].y = 400 + 140 * std::sin(GetTime() * 0.9f + i);
                }
                snap.players[i].animState = (i % 2 == 0) ? casino::ANIM_WALK : casino::ANIM_IDLE;
                snap.players[i].pulse = 0.2f + 0.2f * std::sin(t + i);
                snap.players[i].symbols[0] = i;
                snap.players[i].symbols[1] = (i + 1) % 6;
                snap.players[i].symbols[2] = (i + 2) % 6;
                snap.players[i].lastDelta = (i % 2 == 0) ? 80 : -20;
                snap.players[i].spinning = (std::fmod(GetTime() + i, 3.0) < 1.5);
                snap.players[i].spinProgress = std::fmod(GetTime() + i, 3.0f) / 3.0f;
            }
        }

        update_scene(scene, snap, dt);
        render_frame(assets, scene, snap, cfg);
    }

    if (attached) {
        detach_shared_state(*attachmentOpt);
    }
    unload_assets(assets);
    CloseWindow();
    return 0;
}
