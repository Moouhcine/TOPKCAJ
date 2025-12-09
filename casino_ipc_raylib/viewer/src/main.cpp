#include <chrono>
#include <filesystem>
#include <cmath>
#include <iostream>
#include <raylib.h>
#include <algorithm>
#include <cstdlib>
#include <cstdlib>
#include <thread>
#include <sstream>

#include "assets.hpp"
#include "ipc_attach.hpp"
#include "render.hpp"
#include "anim.hpp"
#include "layout_config.hpp"

// Display main menu with invisible clickable zones over `assets/main_menu.png`.
// Zones are defined in normalized coordinates relative to the drawn image rectangle
// (x,y,w,h) where x,y are offset from left/top and w,h are fractions of image size.
static bool show_main_menu(const Assets& assets, const RenderSettings& cfg) {
    const Texture2D& menuTex = assets.textures.mainMenu;
    if (menuTex.id == 0) return true; // no menu image -> skip immediately

    // normalized hit boxes (tweak these to match your `main_menu.png` layout)
    // (helpRel unused because we place Help absolutely below Play)

    // compute destination rect preserving aspect ratio and centered
    float winW = (float)cfg.width;
    float winH = (float)cfg.height;
    float texW = (float)menuTex.width;
    float texH = (float)menuTex.height;
    float scale = std::min(winW / texW, winH / texH);
    float dstW = texW * scale;
    float dstH = texH * scale;
    Rectangle dst{(winW - dstW) * 0.5f, (winH - dstH) * 0.5f, dstW, dstH};

    // Local lambda to convert rel rect to screen rect
    auto rel_to_abs = [&](const Rectangle& r) {
        return Rectangle{dst.x + r.x * dst.width, dst.y + r.y * dst.height, r.width * dst.width, r.height * dst.height};
    };

    bool running = true;
    bool skipEndDrawing = false;
    while (running && !WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(BLACK);
        // draw menu image centered
        DrawTexturePro(menuTex, {0,0,texW,texH}, dst, {0,0}, 0.0f, WHITE);

        Vector2 m = GetMousePosition();
        // Use absolute coordinates for the Play button as requested
        // Origin (0,0) is top-left of the window
        // move left by 50px and up by 150px from previous placement
        Rectangle playAbs = {980.0f, 310.0f, 490.0f, 150.0f};
        // If menu image is scaled/centered, we still clamp to window bounds
        if (playAbs.x < 0) playAbs.x = 0;
        if (playAbs.y < 0) playAbs.y = 0;
        if (playAbs.x + playAbs.width > winW) playAbs.x = std::max(0.0f, winW - playAbs.width);
        if (playAbs.y + playAbs.height > winH) playAbs.y = std::max(0.0f, winH - playAbs.height);
        // Help button: same size as Play and positioned just below by 300px
        Rectangle helpAbs = { playAbs.x, playAbs.y + 230.0f, playAbs.width, playAbs.height };

        // Play/Help buttons are invisible; keep hitboxes but don't draw any chrome.
        static bool tutorialActive = false;

        // hover cursor change
        if (CheckCollisionPointRec(m, playAbs) || CheckCollisionPointRec(m, helpAbs)) {
            SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
        } else {
            SetMouseCursor(MOUSE_CURSOR_DEFAULT);
        }

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            if (!tutorialActive && CheckCollisionPointRec(m, playAbs)) {
                // Play: exit menu and start viewer
                EndDrawing();
                return true;
            } else if (!tutorialActive && CheckCollisionPointRec(m, helpAbs)) {
                // Start tutorial/help mode
                tutorialActive = true;
                std::cout << "[viewer] entering Help mode. helpScreenshot id=" << assets.textures.helpScreenshot.id << "\n";
                // Build a list of elements to explain (positions + text) in window coordinates
                struct HelpItem { Vector2 target; std::string text; };
                std::vector<HelpItem> items;
                // top banner (jackpot/rounds) - left of top bar
                items.push_back({{40.0f, 20.0f}, "Jackpot: montant global partagé entre les joueurs. Mise/Coeur du jeu."});
                items.push_back({{40.0f, 60.0f}, "Rounds: nombre total de tours joués par le serveur."});
                // center slot area (screen center)
                items.push_back({{(float)cfg.width * 0.5f, (float)cfg.height * 0.5f - 60.0f}, "Machines: chaque joueur a une machine. Le serveur anime et met à jour les symboles."});
                // player panel area (left bottom)
                items.push_back({{40.0f, (float)cfg.height - 200.0f}, "Panneau joueur: montre le dernier gain/perte, historique et symboles."});
                // right info/jackpot summary
                items.push_back({{(float)cfg.width - 220.0f, 80.0f}, "Informations: affiche le dernier gagnant, le jackpot et le cumul global."});
                // IPC/server note (bottom-center)
                items.push_back({{(float)cfg.width * 0.5f, (float)cfg.height - 60.0f}, "Architecture: serveur (process) met à jour la mémoire partagée; les joueurs envoient des messages via une MQ; sémaphores réveillent le serveur."});

                size_t step = 0;
                Texture2D cursorTex = assets.textures.cursor;
                // configurable hotspot (in texture pixels). If not set, default is (0,0)=top-left.
                Vector2 cursorHotspotTex = {0.0f, 0.0f};
                if (const char* hs = std::getenv("VIEWER_CURSOR_HOTSPOT")) {
                    // parse format "x,y"
                    float ox = 0.0f, oy = 0.0f;
                    if (std::sscanf(hs, "%f,%f", &ox, &oy) == 2) {
                        cursorHotspotTex.x = ox;
                        cursorHotspotTex.y = oy;
                    }
                }
                bool helpDebug = (std::getenv("HELP_DEBUG") != nullptr);
                // start cursor at the first help target (center point)
                Vector2 cursorPos = items.empty() ? Vector2{40.0f, 40.0f} : items[0].target;
                // panel size (640x240) as requested
                const float panelW = 640.0f;
                const float panelH = 240.0f;
                // tutorial loop: show a static play-screen background (no animations),
                // then display a help panel that appears to the bottom-right of the cursor.
                // To avoid nested BeginDrawing/EndDrawing we end the current frame before
                // entering the tutorial inner loop and skip the outer EndDrawing for that
                // iteration.
                EndDrawing();
                skipEndDrawing = true;
                int lastLoggedStep = -1;
                while (!WindowShouldClose() && tutorialActive) {
                    // build a static snapshot (freeze animations) from scratch
                    CasinoSnap staticSnap{};
                    staticSnap.playerCount = 6;
                    staticSnap.jackpot = 1200;
                    staticSnap.rounds = 0;
                    const float cx = cfg.width * 0.5f;
                    const float cy = cfg.height * 0.5f + 60.0f;
                    const float radius = std::min(cfg.width, cfg.height) * 0.25f;
                    for (int i = 0; i < staticSnap.playerCount && i < casino::MAX_PLAYERS; ++i) {
                        float angle = 3.14159f / 2 + (6.28318f * i / std::max(6, staticSnap.playerCount));
                        staticSnap.players[i].id = i;
                        staticSnap.players[i].x = cx + radius * std::cos(angle);
                        staticSnap.players[i].y = cy + radius * std::sin(angle);
                        staticSnap.players[i].symbols[0] = i % 4;
                        staticSnap.players[i].symbols[1] = (i + 1) % 4;
                        staticSnap.players[i].symbols[2] = (i + 2) % 4;
                        staticSnap.players[i].lastDelta = (i % 2 == 0) ? 80 : -20;
                        staticSnap.players[i].spinning = 0;
                        staticSnap.players[i].spinProgress = 1.0f;
                        staticSnap.players[i].pulse = 0.2f;
                    }
                    SceneState staticScene{};
                    for (int i = 0; i < casino::MAX_PLAYERS; ++i) {
                        staticScene.players[i].active = (i < staticSnap.playerCount);
                        staticScene.players[i].pos.x = staticSnap.players[i].x;
                        staticScene.players[i].pos.y = staticSnap.players[i].y;
                        staticScene.players[i].spinning = false;
                        staticScene.players[i].spinProgress = 1.0f;
                        staticScene.players[i].symbols[0] = staticSnap.players[i].symbols[0];
                        staticScene.players[i].symbols[1] = staticSnap.players[i].symbols[1];
                        staticScene.players[i].symbols[2] = staticSnap.players[i].symbols[2];
                        staticScene.players[i].pulse = staticSnap.players[i].pulse;
                        staticScene.players[i].id = staticSnap.players[i].id;
                    }

                    if (lastLoggedStep != (int)step) {
                        std::cout << "[help] entering step " << step << " / " << items.size() << "\n";
                        lastLoggedStep = (int)step;
                    }
                    // compute actual pixel targets for this snapshot so the cursor
                    // points at the live scene elements (avoid using menu dst coords)
                    std::vector<Vector2> targets;
                    targets.reserve(items.size());
                    // top banner positions
                    targets.push_back({40.0f, 20.0f});
                    targets.push_back({40.0f, 60.0f});
                    // center machine: use the scene center we computed above
                    targets.push_back({cx, cy - 60.0f});
                    // player panel (left bottom)
                    targets.push_back({40.0f, (float)cfg.height - 200.0f});
                    // right info/jackpot summary
                    targets.push_back({(float)cfg.width - 220.0f, 80.0f});
                    // IPC/server note (bottom-center)
                    targets.push_back({(float)cfg.width * 0.5f, (float)cfg.height - 60.0f});
                    BeginDrawing();
                    // If a help screenshot is provided in assets, draw it fullscreen
                    if (assets.textures.helpScreenshot.id != 0) {
                        Rectangle src{0, 0, (float)assets.textures.helpScreenshot.width, (float)assets.textures.helpScreenshot.height};
                        Rectangle dstFull{0, 0, (float)cfg.width, (float)cfg.height};
                        DrawTexturePro(assets.textures.helpScreenshot, src, dstFull, {0,0}, 0.0f, WHITE);
                    } else {
                        // render the frozen play screen using the scene renderer that does not call Begin/End
                        render_scene_no_begin(assets, staticScene, staticSnap, cfg, nullptr);
                    }

                    // Cursor: move smoothly toward the help item's target position
                    Vector2 desired = (step < (int)targets.size()) ? targets[step] : items[step].target;
                    const float lerp = 0.25f; // smoothing factor
                    cursorPos.x += (desired.x - cursorPos.x) * lerp;
                    cursorPos.y += (desired.y - cursorPos.y) * lerp;
                    float cscale = 0.22f; // smaller
                    float cursorW = (cursorTex.id != 0) ? cursorTex.width * cscale : 12.0f;
                    float cursorH = (cursorTex.id != 0) ? cursorTex.height * cscale : 12.0f;
                    // Use cursorPos as the hotspot (top-left) so the cursor points exactly
                    // to the provided target coordinates. This avoids scale affecting
                    // the perceived target location.
                    float cursorX = cursorPos.x;
                    float cursorY = cursorPos.y;

                    // Compute panel position using quadrant logic:
                    // - if cursor is on the right half -> place panel to the left
                    // - if cursor is on the left half -> place panel to the right
                    // - if cursor is on the bottom half -> place panel above
                    // - if cursor is on the top half -> place panel below
                    const float margin = 12.0f;
                    float panelX;
                    float panelY;
                    if (cursorPos.x > (float)cfg.width * 0.5f) {
                        // place to the left
                        panelX = cursorPos.x - panelW - margin;
                    } else {
                        // place to the right
                        panelX = cursorPos.x + margin;
                    }
                    if (cursorPos.y > (float)cfg.height * 0.5f) {
                        // place above
                        panelY = cursorPos.y - panelH - margin;
                    } else {
                        // place below
                        panelY = cursorPos.y + margin;
                    }
                    // clamp to window bounds with a small inset
                    const float inset = 20.0f;
                    if (panelX < inset) panelX = inset;
                    if (panelX + panelW > (float)cfg.width - inset) panelX = (float)cfg.width - panelW - inset;
                    if (panelY < inset) panelY = inset;
                    if (panelY + panelH > (float)cfg.height - inset) panelY = (float)cfg.height - panelH - inset;
                    Rectangle helpPanel{panelX, panelY, panelW, panelH};
                    // Ensure layout params and slot positions are set so symbols and players render
                    LayoutParams helpLP{};
                    helpLP.slotScale = 0.85f;
                    helpLP.symbolScale = 0.75f; // reduce symbol draw size for help
                    helpLP.playerScale = 0.9f;
                    helpLP.windowW = 56.0f;
                    helpLP.windowH = 96.0f;
                    helpLP.panelScale = 1.0f;
                    set_layout_params(helpLP);
                    // Position slots to match the static snapshot
                    for (int i = 0; i < staticSnap.playerCount && i < casino::MAX_PLAYERS; ++i) {
                        SlotLayout sl{};
                        sl.slotScale = helpLP.slotScale;
                        sl.symbolScale = helpLP.symbolScale;
                        sl.playerScale = helpLP.playerScale;
                        sl.windowW = helpLP.windowW;
                        sl.windowH = helpLP.windowH;
                        sl.set = true;
                        set_slot_layout(i, sl);
                        set_slot_position(i, {staticSnap.players[i].x, staticSnap.players[i].y});
                    }
                    if (assets.textures.panelCleanOverlay.id != 0) {
                        DrawTexturePro(assets.textures.panelCleanOverlay, {0,0,(float)assets.textures.panelCleanOverlay.width,(float)assets.textures.panelCleanOverlay.height}, helpPanel, {0,0}, 0.0f, WHITE);
                    } else {
                        DrawRectangleRounded(helpPanel, 0.05f, 8, Color{20,20,30,220});
                    }

                    // draw cursor (at base position) using hotspot (in texture pixels)
                    if (cursorTex.id != 0) {
                        Rectangle srcC{0,0,(float)cursorTex.width,(float)cursorTex.height};
                        // hotspot is provided in original texture pixels; scale it to dst size
                        float hotX = cursorHotspotTex.x * cscale;
                        float hotY = cursorHotspotTex.y * cscale;
                        Rectangle dstC{cursorX - hotX, cursorY - hotY, cursorW, cursorH};
                        DrawTexturePro(cursorTex, srcC, dstC, {0,0}, 0.0f, WHITE);
                        if (helpDebug) {
                            // draw small cross at the logical hotspot (where cursor points)
                            DrawLine((int)cursorX - 6, (int)cursorY, (int)cursorX + 6, (int)cursorY, RED);
                            DrawLine((int)cursorX, (int)cursorY - 6, (int)cursorX, (int)cursorY + 6, RED);
                        }
                    } else {
                        DrawCircleV({cursorX + 8.0f, cursorY + 8.0f}, 8, Color{255,200,80,220});
                    }

                    if (helpDebug) {
                        // draw target cross and small debug box with coords
                        Vector2 tv = desired;
                        DrawLine((int)tv.x - 6, (int)tv.y, (int)tv.x + 6, (int)tv.y, GREEN);
                        DrawLine((int)tv.x, (int)tv.y - 6, (int)tv.x, (int)tv.y + 6, GREEN);
                        char dbuf[128];
                        std::snprintf(dbuf, sizeof(dbuf), "T:(%.0f,%.0f) C:(%.0f,%.0f) P:(%.0f,%.0f)", tv.x, tv.y, cursorPos.x, cursorPos.y, panel.x, panel.y);
                        DrawText(dbuf, (int)(helpPanel.x + 8), (int)(helpPanel.y + helpPanel.height - 48), 14, Color{220,220,220,200});
                    }

                    // draw the current help text (wrap inside panel) with larger font but wrapping
                    std::string text = items[step].text;
                    int fontSize = 20; // slightly larger
                    float textMargin = 16.0f;
                    float x = helpPanel.x + textMargin;
                    float y = helpPanel.y + textMargin;
                    std::string word;
                    std::istringstream iss(text);
                    while (iss >> word) {
                        std::string out = word + " ";
                        float w = MeasureText(out.c_str(), fontSize);
                        if (x + w > helpPanel.x + helpPanel.width - textMargin) {
                            x = helpPanel.x + textMargin;
                            y += fontSize + 6;
                        }
                        DrawText(out.c_str(), (int)x, (int)y, fontSize, WHITE);
                        x += w;
                        // if text reaches bottom, stop drawing
                        if (y > helpPanel.y + helpPanel.height - textMargin - fontSize) break;
                    }

                    EndDrawing();

                    // wait for click/keypress to advance. We must continue polling events
                    // to keep the window responsive. Call BeginDrawing/EndDrawing each
                    // iteration so raylib can poll OS events; draw a small hint.
                    bool advanced = false;
                    while (!advanced && !WindowShouldClose()) {
                        BeginDrawing();
                        // small hint at bottom of panel
                        const char* hint = "Click / Space / Enter to advance, Esc to exit";
                        DrawText(hint, (int)(helpPanel.x + 18), (int)(helpPanel.y + helpPanel.height - 28), 18, Color{220,220,220,200});

                        // Process input while inside a valid frame so raylib updates internal state
                        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) || IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER)) {
                            std::cout << "[help] input detected for step " << step << "\n";
                            step++;
                            if (step >= items.size()) {
                                tutorialActive = false;
                                step = items.size() - 1;
                            }
                            advanced = true;
                        } else if (IsKeyPressed(KEY_ESCAPE)) {
                            std::cout << "[help] escape pressed, exiting help\n";
                            tutorialActive = false;
                            advanced = true;
                        }

                        EndDrawing();
                        // small sleep to avoid busy-looping; events were polled during EndDrawing
                        std::this_thread::sleep_for(std::chrono::milliseconds(8));
                    }
                }
                // after tutorial we continue the outer loop normally

            }
        }

        if (!skipEndDrawing) {
            EndDrawing();
        } else {
            // we already ended drawing to enter the tutorial inner loop
            skipEndDrawing = false;
        }
    }
    return false;
}

int main() {
    // VM / faible framerate : augmente le buffer audio pour éviter les underflows/grésillements
    SetAudioStreamBufferSizeDefault(8192);
    InitAudioDevice();
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
    // Show main menu if provided; returns false if user closed window
    if (!show_main_menu(assets, cfg)) {
        unload_assets(assets);
        CloseAudioDevice();
        CloseWindow();
        return 0;
    }
    if (assets.audio.hasAudio && assets.audio.ambient.ctxData) {
        SetMusicVolume(assets.audio.ambient, 0.35f);
        PlayMusicStream(assets.audio.ambient);
    }

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

        if (!scene.gameOver) {
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
        }

        if (assets.audio.hasAudio) {
            if (assets.audio.ambient.ctxData) {
                UpdateMusicStream(assets.audio.ambient); // garder le stream fluide même en game over
            }
            if (scene.triggerWinSfx && assets.audio.win.frameCount > 0) {
                PlaySound(assets.audio.win);
            }
            if (scene.triggerEmptySfx) {
                if (assets.audio.ambient.ctxData) StopMusicStream(assets.audio.ambient);
                if (assets.audio.empty.frameCount > 0) {
                    PlaySound(assets.audio.empty);
                }
            }
        }
        scene.triggerWinSfx = false;
        scene.triggerEmptySfx = false;
        render_frame(assets, scene, snap, cfg, attached ? &*attachmentOpt : nullptr);
    }

    if (attached) {
        detach_shared_state(*attachmentOpt);
    }
    unload_assets(assets);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}
