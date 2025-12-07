#include "render.hpp"

#include <algorithm>
#include <cstdio>
#include <string>
#include <cmath>
#include <vector>
#include <cctype>
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

static float draw_bitmap_text(const Assets& assets, const std::string& text, Vector2 pos, float fontSize, float spacing, Color tint) {
    if (!assets.bitmapFont.loaded) {
        DrawTextEx(assets.uiFont, text.c_str(), pos, fontSize, spacing, tint);
        return MeasureTextEx(assets.uiFont, text.c_str(), fontSize, spacing).x;
    }
    float scale = fontSize / (float)assets.bitmapFont.maxHeight;
    float x = pos.x;
    for (char raw : text) {
        if (raw == ' ') {
            x += fontSize * 0.45f;
            continue;
        }
        char ch = (char)std::toupper((unsigned char)raw);
        auto it = assets.bitmapFont.glyphs.find(ch);
        if (it != assets.bitmapFont.glyphs.end() && it->second.texture.id != 0) {
            const auto& g = it->second;
            Rectangle src{0, 0, (float)g.texture.width, (float)g.texture.height};
            float w = g.width * scale;
            float h = g.height * scale;
            Rectangle dst{x, pos.y, w, h};
            DrawTexturePro(g.texture, src, dst, {0, 0}, 0.0f, tint);
            x += g.advance * scale + spacing;
        } else {
            std::string tmp(1, raw);
            Vector2 sz = MeasureTextEx(assets.uiFont, tmp.c_str(), fontSize, spacing);
            DrawTextEx(assets.uiFont, tmp.c_str(), {x, pos.y}, fontSize, spacing, tint);
            x += sz.x + spacing;
        }
    }
    return x - pos.x;
}

static void draw_panel_tex(const Texture2D& tex, Rectangle dest, Color tint, Color fallback, bool preserveRatio = false) {
    if (tex.id != 0) {
        if (preserveRatio) {
            float scale = std::min(dest.width / (float)tex.width, dest.height / (float)tex.height);
            float w = tex.width * scale;
            float h = tex.height * scale;
            Rectangle actual{dest.x + (dest.width - w) * 0.5f, dest.y + (dest.height - h) * 0.5f, w, h};
            DrawTexturePro(tex, {0, 0, (float)tex.width, (float)tex.height}, actual, {0, 0}, 0.0f, tint);
        } else {
            DrawTexturePro(tex, {0, 0, (float)tex.width, (float)tex.height}, dest, {0, 0}, 0.0f, tint);
        }
    } else {
        DrawRectangleRounded(dest, 0.2f, 8, fallback);
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
        const Texture2D* machine = pv.spinning ? &assets.textures.machineDown : &assets.textures.machineIdle;
        if (!machine || machine->id == 0) machine = &assets.textures.slot;
        draw_texture_or_rect(*machine, dest, WHITE, Color{140, 70, 70, 255});
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
        int pid = pv.id;
        bool winPose = (pid >= 0 && pid < casino::MAX_PLAYERS) ? scene.showWinPose[pid] : false;
        const Texture2D* sheet = &assets.textures.playerIdle;
        int frameCount = 4;
        bool useFullFrame = false;
        if (winPose && assets.textures.playerWin.id != 0) {
            sheet = &assets.textures.playerWin;
            frameCount = 1;
            useFullFrame = true;
        } else if (assets.textures.playerBack.id != 0) {
            sheet = &assets.textures.playerBack;
            frameCount = 1; // sprite statique de dos, évite le clignotement
        } else if (pv.anim == casino::ANIM_WALK) {
            sheet = &assets.textures.playerWalk;
        }

        int frame = (GetFrameTime() <= 0.0f) ? 0 : (int)(GetTime() * 6.0f) % frameCount;
        Rectangle src;
        if (useFullFrame || sheet == &assets.textures.playerBack) {
            src = {0, 0, (float)sheet->width, (float)sheet->height};
        } else {
            src = { (float)(frame * 128), 0, 128, 256 };
        }
        Rectangle dest;
        if (useFullFrame) {
            // Garde l'empreinte visuelle identique au sprite de base
            dest = center_rect(pv.pos.x, pv.pos.y, 96 * ps, 184 * ps);
        } else if (sheet == &assets.textures.playerBack) {
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
        draw_bitmap_text(assets, TextFormat("P%d", labelId + 1), {pv.pos.x - 35.0f, pv.pos.y - 224.0f}, 22 * ps, 1, Color{255, 230, 200, 255});
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
    float rowH = 40.0f * scale;
    float width = 520.0f * scale;
    float startX = 20.0f + gLayout.panelOffsetX;
    float gap = 12.0f * scale;
    float startY = cfg.height - (rowH + gap) * std::min(snap.playerCount, casino::MAX_PLAYERS) - 120.0f * scale + gLayout.panelOffsetY;
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
        const Texture2D& panelTex = (assets.textures.goldPanel.id != 0) ? assets.textures.goldPanel : assets.textures.panel;
        draw_panel_tex(panelTex, row, WHITE, Color{16, 32, 48, 180}, false);
        int pid = scene.players[i].id;
        Color nameCol = pv.spinning ? YELLOW : RAYWHITE;
        draw_bitmap_text(assets, TextFormat("P%d", rowIdx + 1), {row.x + 8, row.y + 6 * scale}, 16 * scale, 1, nameCol);
        Color deltaCol = pv.lastDelta >= 0 ? Color{120, 255, 120, 255} : Color{255, 120, 120, 255};
        float bounce = 0.0f;
        if (scene.lastResultTime[pid] > 0.0f) {
            float age = GetTime() - scene.lastResultTime[pid];
            if (age < 1.2f) {
                bounce = std::sin(age * 10.0f) * 3.0f * (1.0f - age / 1.2f);
            }
        }
        draw_bitmap_text(assets, TextFormat("%+d", pv.lastDelta), {row.x + 60 * scale, row.y + (6 + bounce) * scale}, 14 * scale, 1, deltaCol);
        // Historique 5 blocs
        auto& hist = scene.history[pid];
        float blockW = 48.0f * scale;
        float blockH = rowH - 10.0f * scale;
        float hx = row.x + 130 * scale;
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
            draw_bitmap_text(assets, TextFormat("%+d", val), {b.x + 6 * scale, b.y + 6 * scale}, 16 * scale, 1, BLACK);
        }

        float symW = 42.0f * scale;
        float gapSym = 8.0f * scale;
        float sx = row.x + row.width - (symW * 3 + gap * 2) - 12.0f * scale;
        for (int s = 0; s < 3; ++s) {
            Rectangle box = {sx + s * (symW + gapSym), row.y + 4 * scale, symW, rowH - 8 * scale};
            int sym = displayed_symbol(pv, s, i);
            draw_symbol_box(box, sym, pv.spinning, assets.textures);
        }
    }
}

static void draw_ui(const Assets& assets, const CasinoSnap& snap, const SceneState& scene, const RenderSettings& cfg) {
    float scale = gLayout.panelScale;
    Color woodDark{84, 60, 36, 240};
    Color woodLight{128, 92, 48, 230};
    Color gold{220, 180, 80, 255};
    const Texture2D& panelTex = (assets.textures.goldPanel.id != 0) ? assets.textures.goldPanel : assets.textures.panel;

    Rectangle bar = {20, 20 + gLayout.barOffsetY, (float)cfg.width - 40, 120 * scale};
    draw_panel_tex(panelTex, bar, WHITE, woodDark, false); // stretch le bandeau du haut
    DrawRectangleGradientV(bar.x, bar.y, bar.width, bar.height, ColorAlpha(woodLight, 0.25f), ColorAlpha(woodDark, 0.3f));
    char buf[128];
    draw_bitmap_text(assets, "TABLEAU", {bar.x + 24, bar.y + 14 * scale}, 20 * scale, 1, Color{250, 230, 210, 255});

    std::snprintf(buf, sizeof(buf), "Jackpot: %lld", static_cast<long long>(snap.jackpot));
    draw_bitmap_text(assets, buf, {bar.x + 24, bar.y + 38 * scale}, 30 * scale, 1, gold);
    std::snprintf(buf, sizeof(buf), "Rounds: %d", snap.rounds);
    draw_bitmap_text(assets, buf, {bar.x + 24, bar.y + 72 * scale}, 22 * scale, 1, Color{240, 230, 210, 255});

    float rightX = bar.x + bar.width - 320 * scale;
    if (snap.lastWinnerId >= 0) {
        std::snprintf(buf, sizeof(buf), "Last win: P%d +%d", snap.lastWinnerId, snap.lastWinAmount);
        draw_bitmap_text(assets, buf, {rightX, bar.y + 34 * scale}, 22 * scale, 1, Color{255, 220, 120, 255});
    } else {
        draw_bitmap_text(assets, "WAITING WINNERS", {rightX, bar.y + 34 * scale}, 20 * scale, 1, Color{240, 230, 210, 255});
    }
    draw_bitmap_text(assets, "CUMUL", {rightX, bar.y + 64 * scale}, 20 * scale, 1, Color{250, 230, 200, 255});
    std::snprintf(buf, sizeof(buf), "%+d", scene.totalBank);
    Color cumulCol = scene.totalBank >= 0 ? Color{140, 255, 140, 255} : Color{255, 120, 120, 255};
    draw_bitmap_text(assets, buf, {rightX + 120 * scale, bar.y + 64 * scale}, 26 * scale, 1, cumulCol);

    float logScale = gLayout.panelScale;
    Rectangle infoPanel = {(float)cfg.width - 260 * logScale - 20, (float)cfg.height - 100 * logScale + gLayout.logOffsetY, 260 * logScale, 80 * logScale};
    draw_panel_tex(panelTex, infoPanel, WHITE, Color{90, 70, 40, 220}, false);
    std::snprintf(buf, sizeof(buf), "Tick %llu", static_cast<unsigned long long>(snap.tick));
    draw_bitmap_text(assets, buf, {infoPanel.x + 14, infoPanel.y + 16 * logScale}, 20 * logScale, 1, Color{240, 230, 210, 255});
    std::snprintf(buf, sizeof(buf), "Players %d", snap.playerCount);
    draw_bitmap_text(assets, buf, {infoPanel.x + 14, infoPanel.y + 42 * logScale}, 18 * logScale, 1, Color{220, 200, 170, 255});
}

void render_frame(const Assets& assets, SceneState& scene, const CasinoSnap& snap, const RenderSettings& cfg) {
    BeginDrawing();
    ClearBackground(Color{10, 20, 30, 255});

    draw_room(assets, cfg, scene);
    draw_slots(assets, cfg, snap, scene);
    draw_players(assets, scene);
    draw_confetti(scene);
    draw_slot_panel(assets, cfg, scene, snap);
    draw_ui(assets, snap, scene, cfg);
    if (scene.gameOver) {
        DrawRectangle(0, 0, cfg.width, cfg.height, ColorAlpha(BLACK, 0.45f));
        Rectangle panel = center_rect(cfg.width * 0.5f, cfg.height * 0.5f, 640, 240);
        const Texture2D& panelTex = (assets.textures.goldPanel.id != 0) ? assets.textures.goldPanel : assets.textures.panel;
        draw_panel_tex(panelTex, panel, WHITE, Color{40, 30, 20, 240}, false);
        draw_bitmap_text(assets, "PERDU", {panel.x + 190, panel.y + 84}, 64, 2, RED);
        draw_bitmap_text(assets, "BANQUE VIDE", {panel.x + 150, panel.y + 140}, 32, 1, WHITE);
    }

    EndDrawing();
}
