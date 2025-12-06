#include "render.hpp"

#include <algorithm>
#include <cstdio>
#include <string>
#include <cmath>
#include <vector>
#include "layout_config.hpp"

static Rectangle center_rect(float cx, float cy, float w, float h) {
    return {cx - w * 0.5f, cy - h * 0.5f, w, h};
}

static void draw_texture_or_rect(const Texture2D& tex, Rectangle dest, Color tint, Color fallback) {
    if (tex.id != 0) {
        DrawTexturePro(tex, {0, 0, (float)tex.width, (float)tex.height}, dest, {0, 0}, 0.0f, tint);
    } else {
        DrawRectangleRec(dest, fallback);
    }
}

static void draw_room(const Assets& assets, const RenderSettings& cfg, const SceneState& scene) {
    const float w = (float)cfg.width;
    const float h = (float)cfg.height;
    draw_texture_or_rect(assets.textures.wall, {0, 0, w, h * 0.22f}, WHITE, Color{30, 30, 50, 255});
    draw_texture_or_rect(assets.textures.floor, {0, h * 0.05f, w, h * 0.95f}, WHITE, Color{20, 120, 100, 255});

    float glow = 0.25f + 0.25f * std::sin(scene.glowPhase * 2.0f);
    DrawRectangleGradientV(0, 0, cfg.width, 40, ColorAlpha(YELLOW, glow), BLANK);
}

static void draw_symbol_box(Rectangle box, int sym, bool spinning, const TexturePack& tex);

static LayoutParams gLayout{};
static SlotLayout gSlots[casino::MAX_PLAYERS]{};
void set_layout_params(const LayoutParams& params) { gLayout = params; }
void set_slot_layout(int idx, const SlotLayout& slot) {
    if (idx >= 0 && idx < casino::MAX_PLAYERS) gSlots[idx] = slot;
}
static Vector2 gSlotPos[casino::MAX_PLAYERS]{};
static bool gSlotPosSet[casino::MAX_PLAYERS]{};
void set_slot_position(int idx, Vector2 pos) {
    if (idx >= 0 && idx < casino::MAX_PLAYERS) {
        gSlotPos[idx] = pos;
        gSlotPosSet[idx] = true;
    }
}

static int displayed_symbol(const PlayerVisual& pv, int slotIdx, int playerIdx) {
    if (pv.spinning) {
        return ((int)(GetTime() * 25) + slotIdx + playerIdx * 5) % 4;
    }
    return pv.symbols[slotIdx];
}

static void draw_slots(const Assets& assets, const RenderSettings&, const CasinoSnap& snap, const SceneState& scene) {
    for (int i = 0; i < snap.playerCount && i < casino::MAX_PLAYERS; ++i) {
        const auto& pv = scene.players[i];
        SlotLayout sl = gSlots[i].set ? gSlots[i] : SlotLayout{};
        int sid = pv.id;
        if (sid < 0 || sid >= casino::MAX_PLAYERS) sid = i;
        if (gSlots[sid].set) sl = gSlots[sid];
        if (!gSlots[sid].set) {
            sl.slotScale = gLayout.slotScale;
            sl.symbolScale = gLayout.symbolScale;
            sl.playerScale = gLayout.playerScale;
            sl.windowW = gLayout.windowW;
            sl.windowH = gLayout.windowH;
            sl.windowOffsetX = gLayout.windowOffsetX;
            sl.windowOffsetY = gLayout.windowOffsetY;
        }
        float sx = gSlotPosSet[sid] ? gSlotPos[sid].x : scene.players[i].pos.x;
        float sy = gSlotPosSet[sid] ? gSlotPos[sid].y : scene.players[i].pos.y;
        float baseW = 256.0f * sl.slotScale;
        float baseH = 256.0f * sl.slotScale;
        Rectangle dest;
        if (gSlotPosSet[sid]) {
            dest = {sx, sy, baseW, baseH}; // LDtk coords are top-left
        } else {
            dest = center_rect(sx, sy, baseW, baseH);
        }
        draw_texture_or_rect(assets.textures.slot, dest, WHITE, Color{140, 70, 70, 255});
        // reel strip
        float windowW = sl.windowW * sl.symbolScale;
        float windowH = sl.windowH * sl.symbolScale;
        float startX = dest.x + sl.windowOffsetX;
        float startY = dest.y + sl.windowOffsetY;
        for (int s = 0; s < 3; ++s) {
            float bx = startX + s * (windowW + 19.0f * sl.symbolScale);
            float by = startY;
            float jitter = pv.spinning ? (float)(fmod(GetTime() * 4.0 + s * 0.25, 1.0) * 10.0) : 0.0f;
            Rectangle box = {bx, by + jitter, windowW, windowH};
            int sym = displayed_symbol(pv, s, i);
            draw_symbol_box(box, sym, pv.spinning, assets.textures);
        }
    }
}

static void draw_players(const Assets& assets, const SceneState& scene) {
    for (int i = 0; i < casino::MAX_PLAYERS; ++i) {
        const auto& pv = scene.players[i];
        if (!pv.active) continue;
        SlotLayout sl = gSlots[i].set ? gSlots[i] : SlotLayout{};
        if (!gSlots[i].set) {
            sl.playerScale = gLayout.playerScale;
        }
        float ps = sl.playerScale;
        const Texture2D* sheet = &assets.textures.playerIdle;
        int frameCount = 4;
        if (assets.textures.playerBack.id != 0) {
            sheet = &assets.textures.playerBack;
            frameCount = 1; // sprite statique de dos, évite le clignotement
        } else if (pv.anim == casino::ANIM_WIN) sheet = &assets.textures.playerWin;
        else if (pv.anim == casino::ANIM_WALK) sheet = &assets.textures.playerWalk;

        int frame = (GetFrameTime() <= 0.0f) ? 0 : (int)(GetTime() * 6.0f) % frameCount;
        Rectangle src;
        if (sheet == &assets.textures.playerBack) {
            src = {0, 0, (float)sheet->width, (float)sheet->height};
        } else {
            src = { (float)(frame * 128), 0, 128, 256 };
        }
        Rectangle dest;
        if (sheet == &assets.textures.playerBack) {
            float scale = 0.2f * ps;
            dest = center_rect(pv.pos.x, pv.pos.y, sheet->width * scale, sheet->height * scale);
        } else {
            dest = center_rect(pv.pos.x, pv.pos.y, 96 * ps, 184 * ps);
        }
        if (sheet->id != 0) {
            DrawTexturePro(*sheet, src, dest, {dest.width * 0.5f, dest.height * 0.5f}, 0.0f, WHITE);
        } else {
            Color base = Color{50, 140, 220, 255};
            Color accent = Color{20, 60, 140, 255};
            DrawRectangleRounded(dest, 0.25f, 10, base);
            DrawRectangleRoundedLines(dest, 0.25f, 10, accent);
            DrawCircleGradient(pv.pos.x, pv.pos.y - 70 * ps, 30 * ps, Color{245, 225, 200, 255}, Color{200, 170, 140, 255});
        }
        // Label Pn au-dessus (offset 15px gauche, 62px haut) basé sur l'id réel
        int labelId = pv.id;
        DrawText(TextFormat("P%d", labelId + 1), pv.pos.x - 35.0f, pv.pos.y - 224.0f, (int)(22 * ps), Color{255, 230, 200, 255});
        if (pv.pulse > 0.01f) {
            float alpha = std::min(1.0f, pv.pulse);
            DrawCircleGradient(pv.pos.x, pv.pos.y - 40, 36 + pv.pulse * 20.0f, ColorAlpha(YELLOW, alpha * 0.6f), BLANK);
        }
    }
}

static void draw_confetti(const SceneState& scene) {
    for (const auto& c : scene.confetti) {
        if (!c.alive) continue;
        Color col = c.color;
        col.a = (unsigned char)(255 * std::clamp(c.life, 0.0f, 1.0f));
        DrawRectangleV(c.pos, {4, 8}, col);
    }
}

static Color symbol_color(int sym) {
    switch (sym % 4) {
        case 0: return Color{240, 70, 70, 255};      // 7
        case 1: return Color{80, 180, 240, 255};     // diamond
        case 2: return Color{240, 180, 60, 255};     // bell
        default: return Color{220, 80, 120, 255};    // strawberry
    }
}

static void draw_symbol_box(Rectangle box, int sym, bool spinning, const TexturePack& tex) {
    Rectangle inner = box;
    if (spinning && tex.slotReel.id != 0) {
        float reelH = (float)tex.slotReel.height;
        float scroll = std::fmod((float)GetTime() * 320.0f + sym * 37.0f, reelH);
        Rectangle src{0.0f, scroll, (float)tex.slotReel.width, inner.height};
        DrawTexturePro(tex.slotReel, src, inner, {0, 0}, 0.0f, WHITE);
        if (scroll + inner.height > reelH) {
            float remaining = scroll + inner.height - reelH;
            Rectangle src2{0.0f, 0.0f, (float)tex.slotReel.width, remaining};
            Rectangle dst2{inner.x, inner.y + inner.height - remaining, inner.width, remaining};
            DrawTexturePro(tex.slotReel, src2, dst2, {0, 0}, 0.0f, WHITE);
        }
        return;
    }

    const Texture2D* icon = nullptr;
    switch (sym % 4) {
        case 0: icon = &tex.slot7; break;
        case 1: icon = &tex.slotDiamond; break;
        case 2: icon = &tex.slotBell; break;
        case 3: icon = &tex.slotStrawberry; break;
    }
    float bob = spinning ? std::sin((float)GetTime() * 10.0f + sym) * 6.0f : 0.0f;
    float scale = 2.0f;
    float dstW = inner.width * scale;
    float dstH = inner.height * scale;
    float cx = inner.x + inner.width * 0.5f;
    float cy = inner.y + inner.height * 0.5f + bob;
    Rectangle dst{cx - dstW * 0.5f, cy - dstH * 0.5f, dstW, dstH};
    if (icon && icon->id != 0) {
        Rectangle src{0, 0, (float)icon->width, (float)icon->height};
        DrawTexturePro(*icon, src, dst, {0,0}, 0.0f, WHITE);
    } else {
        char buf[8];
        std::snprintf(buf, sizeof(buf), "%d", sym % 10);
        DrawText(buf, dst.x + dst.width / 2 - 6, dst.y + dst.height / 2 - 8, 18, BLACK);
    }
}

static void draw_slot_panel(const Assets& assets, const RenderSettings& cfg, const SceneState& scene, const CasinoSnap& snap) {
    float scale = gLayout.panelScale;
    float rowH = 32.0f * scale;
    float width = 420.0f * scale;
    float startX = 20.0f + gLayout.panelOffsetX;
    float gap = 10.0f * scale;
    float startY = cfg.height - (rowH + gap) * std::min(snap.playerCount, casino::MAX_PLAYERS) - 100.0f * scale + gLayout.panelOffsetY;
    int count = std::min(snap.playerCount, casino::MAX_PLAYERS);
    std::vector<int> order;
    order.reserve(count);
    if (count == 1) {
        order.push_back(0);
    } else {
        for (int i = 1; i < count; ++i) order.push_back(i);
        order.push_back(0);
    }
    for (int rowIdx = 0; rowIdx < (int)order.size(); ++rowIdx) {
        int i = order[rowIdx];
        const auto& pv = scene.players[i];
        Rectangle row = {startX, startY + rowIdx * (rowH + gap), width, rowH};
        draw_texture_or_rect(assets.textures.panel, row, WHITE, Color{16, 32, 48, 180});
        int pid = scene.players[i].id;
        Color nameCol = pv.spinning ? YELLOW : RAYWHITE;
        DrawTextEx(assets.uiFont, TextFormat("P%d", rowIdx + 1), {row.x + 8, row.y + 6 * scale}, 16 * scale, 1, nameCol);
        Color deltaCol = pv.lastDelta >= 0 ? Color{120, 255, 120, 255} : Color{255, 120, 120, 255};
        float bounce = 0.0f;
        if (scene.lastResultTime[pid] > 0.0f) {
            float age = GetTime() - scene.lastResultTime[pid];
            if (age < 1.2f) {
                bounce = std::sin(age * 10.0f) * 3.0f * (1.0f - age / 1.2f);
            }
        }
        DrawTextEx(assets.uiFont, TextFormat("%+d", pv.lastDelta), {row.x + 60 * scale, row.y + (6 + bounce) * scale}, 14 * scale, 1, deltaCol);
        // Historique 5 blocs
        auto& hist = scene.history[pid];
        float blockW = 36.0f * scale;
        float blockH = rowH - 8.0f * scale;
        float hx = row.x + 120 * scale;
        int startIdx = (int)std::max(0, (int)hist.size() - 4);
        for (int h = 0; h < 4; ++h) {
            int idx = startIdx + h;
            Color bcol = Color{60, 60, 60, 180};
            int val = 0;
            if (idx < (int)hist.size()) {
                val = hist[idx];
                bcol = val >= 0 ? Color{80, 180, 80, 220} : Color{200, 80, 80, 220};
            }
            Rectangle b = {hx + h * (blockW + 4 * scale), row.y + 4 * scale, blockW, blockH};
            DrawRectangleRounded(b, 0.2f, 6, bcol);
            DrawTextEx(assets.uiFont, TextFormat("%+d", val), {b.x + 6 * scale, b.y + 6 * scale}, 14 * scale, 1, BLACK);
        }

        float symW = 34.0f * scale;
        float gapSym = 6.0f * scale;
        float sx = row.x + row.width - (symW * 3 + gap * 2) - 12.0f * scale;
        for (int s = 0; s < 3; ++s) {
            Rectangle box = {sx + s * (symW + gapSym), row.y + 4 * scale, symW, rowH - 8 * scale};
            int sym = displayed_symbol(pv, s, i);
            draw_symbol_box(box, sym, pv.spinning, assets.textures);
        }
    }
}

static void draw_ui(const Assets& assets, const CasinoSnap& snap, const RenderSettings& cfg) {
    float scale = gLayout.panelScale;
    Color woodDark{84, 60, 36, 240};
    Color woodLight{128, 92, 48, 230};
    Color gold{220, 180, 80, 255};

    Rectangle bar = {20, 20 + gLayout.barOffsetY, (float)cfg.width - 40, 86 * scale};
    DrawRectangleGradientV(bar.x, bar.y, bar.width, bar.height, woodLight, woodDark);
    DrawRectangleLinesEx(bar, 2, Color{40, 30, 20, 220});
    char buf[128];
    std::snprintf(buf, sizeof(buf), "Jackpot: %lld", static_cast<long long>(snap.jackpot));
    DrawTextEx(assets.uiFont, buf, {bar.x + 24, bar.y + 20 * scale}, 28 * scale, 1, gold);
    std::snprintf(buf, sizeof(buf), "Rounds: %d", snap.rounds);
    DrawTextEx(assets.uiFont, buf, {bar.x + 24, bar.y + 52 * scale}, 22 * scale, 1, Color{240, 230, 210, 255});

    Rectangle winnerPanel = {bar.x + bar.width - 260 * scale, bar.y + 8 * scale, 240 * scale, bar.height - 16 * scale};
    DrawRectangleRounded(winnerPanel, 0.18f, 8, Color{140, 110, 40, 240});
    DrawRectangleRoundedLines(winnerPanel, 0.18f, 8, Color{80, 60, 24, 240});
    if (snap.lastWinnerId >= 0) {
        std::snprintf(buf, sizeof(buf), "Last win: P%d +%d", snap.lastWinnerId, snap.lastWinAmount);
        DrawTextEx(assets.uiFont, buf, {winnerPanel.x + 12, winnerPanel.y + 22}, 22, 1, Color{255, 220, 120, 255});
    } else {
        DrawTextEx(assets.uiFont, "Waiting winners", {winnerPanel.x + 12, winnerPanel.y + 22}, 20, 1, Color{240, 230, 210, 255});
    }

    float logScale = gLayout.panelScale;
    Rectangle infoPanel = {(float)cfg.width - 260 * logScale - 20, (float)cfg.height - 100 * logScale + gLayout.logOffsetY, 260 * logScale, 80 * logScale};
    DrawRectangleRounded(infoPanel, 0.2f, 8, Color{90, 70, 40, 220});
    DrawRectangleRoundedLines(infoPanel, 0.2f, 8, Color{60, 45, 24, 240});
    std::snprintf(buf, sizeof(buf), "Tick %llu", static_cast<unsigned long long>(snap.tick));
    DrawTextEx(assets.uiFont, buf, {infoPanel.x + 14, infoPanel.y + 16 * logScale}, 20 * logScale, 1, Color{240, 230, 210, 255});
    std::snprintf(buf, sizeof(buf), "Players %d", snap.playerCount);
    DrawTextEx(assets.uiFont, buf, {infoPanel.x + 14, infoPanel.y + 42 * logScale}, 18 * logScale, 1, Color{220, 200, 170, 255});
}

void render_frame(const Assets& assets, SceneState& scene, const CasinoSnap& snap, const RenderSettings& cfg) {
    BeginDrawing();
    ClearBackground(Color{10, 20, 30, 255});

    draw_room(assets, cfg, scene);
    draw_slots(assets, cfg, snap, scene);
    draw_players(assets, scene);
    draw_confetti(scene);
    draw_slot_panel(assets, cfg, scene, snap);
    draw_ui(assets, snap, cfg);

    EndDrawing();
}
