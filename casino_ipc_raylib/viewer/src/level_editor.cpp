#include <raylib.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#define RAYGUI_IMPLEMENTATION
#include "../raygui.h"

#include "assets.hpp"
#include "layout_config.hpp"
#include "render.hpp"
#include "protocol.hpp"

struct Scene {
    std::vector<Vector2> pos;
    LayoutParams params;
    std::vector<SlotLayout> slots;
};

static Scene default_scene(int count) {
    Scene s;
    s.pos.resize(count);
    s.slots.resize(count);
    float cx = 960.0f;
    float cy = 600.0f;
    float radius = 300.0f;
    for (int i = 0; i < count; ++i) {
        float angle = PI / 2.0f + (2.0f * PI * i / (float)count);
        s.pos[i].x = cx + radius * cosf(angle);
        s.pos[i].y = cy + radius * sinf(angle);
        s.slots[i].slotScale = 1.0f;
        s.slots[i].symbolScale = 1.0f;
        s.slots[i].playerScale = 1.0f;
        s.slots[i].windowW = s.params.windowW;
        s.slots[i].windowH = s.params.windowH;
        s.slots[i].windowOffsetX = s.params.windowOffsetX;
        s.slots[i].windowOffsetY = s.params.windowOffsetY;
        s.slots[i].set = true;
    }
    return s;
}

static bool load_scene(Scene& s, const std::string& path) {
    LayoutParams p = s.params;
    std::vector<SlotLayout> slots;
    std::vector<Vector2> pos;
    bool ok = load_scene_file(path, p, slots, &pos);
    if (ok) {
        s.params = p;
        if (!slots.empty()) {
            s.slots = slots;
            s.pos = pos;
        }
    }
    return ok;
}

static bool save_scene(const Scene& s, const std::string& path) {
    std::ofstream f(path);
    if (!f.is_open()) return false;
    f << "{\n  \"ui\": {\"slotScale\": " << s.params.slotScale << ", \"symbolScale\": " << s.params.symbolScale
      << ", \"playerScale\": " << s.params.playerScale << ", \"windowW\": " << s.params.windowW
      << ", \"windowH\": " << s.params.windowH << ", \"windowOffsetX\": " << s.params.windowOffsetX
      << ", \"windowOffsetY\": " << s.params.windowOffsetY << ", \"panelScale\": " << s.params.panelScale
      << ", \"panelOffsetX\": " << s.params.panelOffsetX << ", \"panelOffsetY\": " << s.params.panelOffsetY
      << ", \"barOffsetY\": " << s.params.barOffsetY << ", \"logOffsetY\": " << s.params.logOffsetY << "},\n";
    f << "  \"slots\": [\n";
    for (size_t i = 0; i < s.pos.size(); ++i) {
        const SlotLayout& sl = (i < s.slots.size()) ? s.slots[i] : SlotLayout{};
        f << "    {\"id\": " << i << ", \"x\": " << s.pos[i].x << ", \"y\": " << s.pos[i].y
          << ", \"slotScale\": " << sl.slotScale << ", \"symbolScale\": " << sl.symbolScale
          << ", \"playerScale\": " << sl.playerScale << ", \"windowW\": " << sl.windowW
          << ", \"windowH\": " << sl.windowH << ", \"windowOffsetX\": " << sl.windowOffsetX
          << ", \"windowOffsetY\": " << sl.windowOffsetY << "}";
        if (i + 1 < s.pos.size()) f << ",";
        f << "\n";
    }
    f << "  ]\n}\n";
    return true;
}

int main() {
    int playerCount = 6;
    Scene scene = default_scene(playerCount);
    load_scene(scene, "scene.json");
    if ((int)scene.pos.size() < playerCount) {
        Scene def = default_scene(playerCount);
        scene = def;
    }

    RenderSettings cfg{};
    const char* envW = std::getenv("EDITOR_W");
    const char* envH = std::getenv("EDITOR_H");
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

    InitWindow(cfg.width, cfg.height, "Level Editor (raygui)");
    SetTargetFPS(60);

    Assets assets = load_assets("viewer/assets");

    int dragging = -1;
    Vector2 dragOffset{};
    int selected = 0;

    while (!WindowShouldClose()) {
        Vector2 m = GetMousePosition();
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            for (int i = 0; i < playerCount && i < (int)scene.pos.size(); ++i) {
                if (CheckCollisionPointCircle(m, scene.pos[i], 32)) {
                    dragging = i;
                    dragOffset = {scene.pos[i].x - m.x, scene.pos[i].y - m.y};
                    selected = i;
                    break;
                }
            }
        }
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) dragging = -1;
        if (dragging >= 0 && dragging < (int)scene.pos.size()) {
            scene.pos[dragging] = {m.x + dragOffset.x, m.y + dragOffset.y};
        }

        if (GuiButton({20, 20, 120, 24}, "Save")) save_scene(scene, "scene.json");
        if (GuiButton({150, 20, 120, 24}, "Load")) load_scene(scene, "scene.json");

        SlotLayout& sl = scene.slots[selected];
        sl.slotScale = GuiSlider({20, 60, 220, 16}, "SlotScale", "", sl.slotScale, 0.2f, 2.5f);
        sl.symbolScale = GuiSlider({20, 84, 220, 16}, "SymbolScale", "", sl.symbolScale, 0.2f, 2.5f);
        sl.playerScale = GuiSlider({20, 108, 220, 16}, "PlayerScale", "", sl.playerScale, 0.2f, 2.0f);
        sl.windowW = GuiSlider({20, 132, 220, 16}, "WinW", "", sl.windowW, 30.0f, 220.0f);
        sl.windowH = GuiSlider({20, 156, 220, 16}, "WinH", "", sl.windowH, 30.0f, 240.0f);
        sl.windowOffsetX = GuiSlider({20, 180, 220, 16}, "WinOffX", "", sl.windowOffsetX, 0.0f, 200.0f);
        sl.windowOffsetY = GuiSlider({20, 204, 220, 16}, "WinOffY", "", sl.windowOffsetY, 0.0f, 200.0f);

        scene.params.panelScale = GuiSlider({20, 240, 220, 16}, "PanelScale", "", scene.params.panelScale, 0.5f, 2.0f);
        scene.params.panelOffsetY = GuiSlider({20, 264, 220, 16}, "PanelOffY", "", scene.params.panelOffsetY, -300.0f, 300.0f);
        scene.params.barOffsetY = GuiSlider({20, 288, 220, 16}, "BarOffY", "", scene.params.barOffsetY, -200.0f, 200.0f);
        scene.params.logOffsetY = GuiSlider({20, 312, 220, 16}, "LogOffY", "", scene.params.logOffsetY, -200.0f, 200.0f);

        BeginDrawing();
        ClearBackground(Color{10, 20, 30, 255});

        // preview machines + symbols
        for (int i = 0; i < playerCount; ++i) {
            Vector2 p = scene.pos[i];
            const SlotLayout& slot = scene.slots[i];
            float scale = slot.slotScale * 0.35f;
            Rectangle dest{p.x - assets.textures.slot.width * scale * 0.5f, p.y - assets.textures.slot.height * scale * 0.5f,
                           assets.textures.slot.width * scale, assets.textures.slot.height * scale};
            if (assets.textures.slot.id != 0) {
                DrawTexturePro(assets.textures.slot, {0,0,(float)assets.textures.slot.width,(float)assets.textures.slot.height}, dest, {0,0}, 0.0f, WHITE);
            } else {
                DrawRectangleRounded(dest, 0.2f, 8, Color{120, 60, 60, 255});
            }
            float windowW = slot.windowW * slot.symbolScale;
            float windowH = slot.windowH * slot.symbolScale;
            float startX = dest.x + slot.windowOffsetX;
            float startY = dest.y + slot.windowOffsetY;
            const Texture2D* icons[3] = {&assets.textures.slot7, &assets.textures.slotDiamond, &assets.textures.slotBell};
            for (int s = 0; s < 3; ++s) {
                Rectangle box = {startX + s * (windowW + 12.0f * slot.symbolScale), startY, windowW, windowH};
                DrawRectangleRounded(box, 0.25f, 6, Color{50, 50, 70, 180});
                if (icons[s] && icons[s]->id != 0) {
                    Rectangle src{0,0,(float)icons[s]->width,(float)icons[s]->height};
                    Rectangle dst{box.x + 4, box.y + 4, box.width - 8, box.height - 8};
                    DrawTexturePro(*icons[s], src, dst, {0,0}, 0.0f, WHITE);
                }
            }
            DrawCircleLines(p.x, p.y, 36, (i == selected) ? GOLD : YELLOW);
            DrawText(TextFormat("P%d", i), p.x - 10, p.y - 10, 20, RAYWHITE);
        }

        // player preview
        if (assets.textures.playerBack.id != 0) {
            float ps = scene.slots[selected].playerScale;
            Rectangle dst{20, (float)cfg.height - 200, assets.textures.playerBack.width * 0.18f * ps, assets.textures.playerBack.height * 0.18f * ps};
            DrawTexturePro(assets.textures.playerBack, {0,0,(float)assets.textures.playerBack.width,(float)assets.textures.playerBack.height}, dst, {0,0}, 0.0f, WHITE);
            DrawText(TextFormat("Player P%d sprite", selected), 20, (int)(dst.y - 18), 14, LIGHTGRAY);
        }

        DrawText("Level editor: select Pn, drag to move, adjust sliders, Save/Load scene.json", 20, cfg.height - 40, 18, RAYWHITE);
        EndDrawing();
    }

    unload_assets(assets);
    CloseWindow();
    return 0;
}
