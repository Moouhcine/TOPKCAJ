#include "assets.hpp"

#include <raylib.h>
#include <filesystem>
#include <iostream>
#include <cctype>
#include <algorithm>

namespace fs = std::filesystem;

static Texture2D try_load_texture(const fs::path& path, bool& ok) {
    Texture2D tex{};
    if (fs::exists(path)) {
        tex = LoadTexture(path.string().c_str());
        if (tex.id != 0) {
            // If the texture is extremely large, downscale it to avoid GPU/driver problems
            const int MAX_DIM = 2048;
            if (tex.width > MAX_DIM || tex.height > MAX_DIM) {
                std::cout << "[assets] large texture detected (" << tex.width << "x" << tex.height << ") for " << path << ", downscaling to max " << MAX_DIM << "\n";
                UnloadTexture(tex);
                Image img = LoadImage(path.string().c_str());
                // compute scaled dimensions preserving aspect
                float scale = std::min(1.0f, (float)MAX_DIM / (float)std::max(img.width, img.height));
                int newW = std::max(1, (int)(img.width * scale));
                int newH = std::max(1, (int)(img.height * scale));
                ImageResize(&img, newW, newH);
                tex = LoadTextureFromImage(img);
                UnloadImage(img);
            }
            ok = true;
            SetTextureFilter(tex, TEXTURE_FILTER_BILINEAR);
        }
    }
    return tex;
}

static BitmapFont load_bitmap_font(const fs::path& baseDir) {
    BitmapFont font{};
    fs::path pngDir = baseDir / "png";
    if (!fs::exists(pngDir)) {
        return font;
    }

    auto crop_to_alpha = [](Image& img) {
        Color* pixels = LoadImageColors(img);
        if (!pixels) return;
        int minX = img.width, minY = img.height, maxX = 0, maxY = 0;
        bool found = false;
        for (int y = 0; y < img.height; ++y) {
            for (int x = 0; x < img.width; ++x) {
                Color c = pixels[y * img.width + x];
                if (c.a > 10) {
                    found = true;
                    minX = std::min(minX, x);
                    minY = std::min(minY, y);
                    maxX = std::max(maxX, x);
                    maxY = std::max(maxY, y);
                }
            }
        }
        UnloadImageColors(pixels);
        if (found) {
            Rectangle crop{(float)minX, (float)minY, (float)(maxX - minX + 1), (float)(maxY - minY + 1)};
            ImageCrop(&img, crop);
        }
    };

    const std::string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    for (char raw : chars) {
        char ch = raw;
        fs::path file = pngDir / (std::string(1, ch) + ".png");
        if (!fs::exists(file)) continue;
        Image img = LoadImage(file.string().c_str());
        if (!img.data) continue;
        crop_to_alpha(img);
        BitmapGlyph glyph{};
        glyph.width = img.width;
        glyph.height = img.height;
        glyph.advance = glyph.width + 8;
        glyph.texture = LoadTextureFromImage(img);
        SetTextureFilter(glyph.texture, TEXTURE_FILTER_BILINEAR);
        UnloadImage(img);
        font.glyphs[ch] = glyph;
        font.maxHeight = std::max(font.maxHeight, glyph.height);
        font.loaded = true;
    }
    return font;
}

Assets load_assets(const std::string& basePath) {
    Assets a{};
    bool texOk = false;
    fs::path base(basePath);
    fs::path baseDir = fs::path(basePath).parent_path();
    fs::path rootSprites = baseDir / "sprites";
    fs::path projectSprites = baseDir.parent_path() / "sprites";
    auto resolve = [&](const fs::path& rel) {
        fs::path candidates[] = {
            base / rel,
            baseDir / rel,
            rootSprites / rel.filename(),
            projectSprites / rel.filename()
        };
        for (auto& c : candidates) {
            if (fs::exists(c)) return c;
        }
        return candidates[0];
    };
    a.textures.floor = try_load_texture(resolve("sprites/casino/floor.png"), texOk);
    a.textures.wall = try_load_texture(resolve("sprites/casino/wall.png"), texOk);
    a.textures.table = try_load_texture(resolve("sprites/casino/table.png"), texOk);
    a.textures.slot = try_load_texture(resolve("sprites/custom/slot_machine.png"), texOk);
    if (!texOk) {
        a.textures.slot = try_load_texture(resolve("sprites/casino/slot_machine.png"), texOk);
    }
    if (a.textures.slot.id == 0) {
        a.textures.slot = try_load_texture(fs::path(basePath) / "machine.png", texOk);
    }
    a.textures.machineIdle = try_load_texture(fs::path(basePath) / "machine.png", texOk);
    a.textures.machineDown = try_load_texture(fs::path(basePath) / "machine_down.png", texOk);
    if (a.textures.machineIdle.id == 0) a.textures.machineIdle = a.textures.slot;
    if (a.textures.machineDown.id == 0) a.textures.machineDown = a.textures.machineIdle;
    a.textures.slotReel = try_load_texture(resolve("sprites/casino/slot_reel_symbols.png"), texOk);
    bool dummy = false;
    a.textures.slot7 = try_load_texture(resolve("sprites/custom/symbol_7.png"), dummy);
    a.textures.slotDiamond = try_load_texture(resolve("sprites/custom/symbol_diamond.png"), dummy);
    a.textures.slotBell = try_load_texture(resolve("sprites/custom/symbol_bell.png"), dummy);
    a.textures.slotStrawberry = try_load_texture(resolve("sprites/custom/symbol_strawberry.png"), dummy);
    if (a.textures.slot7.id == 0) a.textures.slot7 = try_load_texture(fs::path(basePath) / "symbole 7.png", dummy);
    if (a.textures.slotDiamond.id == 0) a.textures.slotDiamond = try_load_texture(fs::path(basePath) / "symbole diamant.png", dummy);
    if (a.textures.slotBell.id == 0) a.textures.slotBell = try_load_texture(fs::path(basePath) / "symbole cloche.png", dummy);
    if (a.textures.slotStrawberry.id == 0) a.textures.slotStrawberry = try_load_texture(fs::path(basePath) / "symbole fraise.png", dummy);
    // Symbols handled individually in render; keep texOk for main assets
    a.textures.panel = try_load_texture(resolve("sprites/ui/panel.png"), texOk);
    a.textures.goldPanel = try_load_texture(fs::path(basePath) / "gold_panel.jpeg", texOk);
    a.textures.panelCleanLarge = try_load_texture(fs::path(basePath) / "panel_clean_1880x120.png", texOk);
    a.textures.panelCleanRow = try_load_texture(fs::path(basePath) / "panel_clean_520x40.png", texOk);
    a.textures.panelCleanInfo = try_load_texture(fs::path(basePath) / "panel_clean_260x80.png", texOk);
    a.textures.panelCleanOverlay = try_load_texture(fs::path(basePath) / "panel_clean_640x240.png", texOk);
    a.textures.button = try_load_texture(resolve("sprites/ui/button.png"), texOk);
    a.textures.mainMenu = try_load_texture(fs::path(basePath) / "main_menu.png", texOk);
    a.textures.cursor = try_load_texture(fs::path(basePath) / "cursor.png", texOk);
    a.textures.tableau = try_load_texture(fs::path(basePath) / "tableau.png", texOk);
    a.textures.helpScreenshot = try_load_texture(fs::path(basePath) / "capture_ecran.png", texOk);
    if (a.textures.helpScreenshot.id != 0) {
        std::cout << "[assets] loaded help screenshot: " << (fs::path(basePath) / "capture_ecran.png") << "\n";
    } else {
        std::cout << "[assets] help screenshot not found or failed to load: " << (fs::path(basePath) / "capture_ecran.png") << "\n";
    }
    a.textures.coin = try_load_texture(resolve("sprites/ui/icon_coin.png"), texOk);
    a.textures.playerIdle = try_load_texture(resolve("sprites/players/player_idle.png"), texOk);
    a.textures.playerWalk = try_load_texture(resolve("sprites/players/player_walk.png"), texOk);
    a.textures.playerWin = try_load_texture(resolve("sprites/players/player_win.png"), texOk);
    a.textures.playerBack = try_load_texture(resolve("sprites/custom/player_back.png"), texOk);
    if (a.textures.playerBack.id == 0) {
        a.textures.playerBack = try_load_texture(fs::path(basePath) / "joueur.png", texOk);
    }
    if (a.textures.playerWin.id == 0) {
        a.textures.playerWin = try_load_texture(fs::path(basePath) / "joueur_win.png", texOk);
    }

    a.hasTextures = texOk;

    // bitmap font from PNG letters
    fs::path fontDir = base / "font_native_AZ_1-9_0_316x232";
    if (!fs::exists(fontDir)) {
        for (auto& entry : fs::directory_iterator(base)) {
            if (entry.is_directory() && entry.path().filename().string().find("font_native") != std::string::npos) {
                fontDir = entry.path();
                break;
            }
        }
    }
    if (fs::exists(fontDir)) {
        a.bitmapFont = load_bitmap_font(fontDir);
    }

    fs::path fontPath = base / "fonts/ui.ttf";
    if (fs::exists(fontPath)) {
        a.uiFont = LoadFont(fontPath.string().c_str());
        if (a.uiFont.baseSize > 0) {
            a.hasFont = true;
        }
    }
    if (!a.hasFont) {
        a.uiFont = GetFontDefault();
    }

    // Audio
    fs::path ambient = base / "ambient.mp3";
    if (fs::exists(ambient)) {
        a.audio.ambient = LoadMusicStream(ambient.string().c_str());
        a.audio.hasAudio = true;
    }
    fs::path win = base / "win.mp3";
    if (fs::exists(win)) {
        a.audio.win = LoadSound(win.string().c_str());
        SetSoundVolume(a.audio.win, 0.05f);
        a.audio.hasAudio = true;
    }
    fs::path empty = base / "empty.mp3";
    if (fs::exists(empty)) {
        a.audio.empty = LoadSound(empty.string().c_str());
        SetSoundVolume(a.audio.empty, 1.0f);
        a.audio.hasAudio = true;
    }

    return a;
}

void unload_assets(Assets& assets) {
    auto unload = [](Texture2D& t) {
        if (t.id != 0) {
            UnloadTexture(t);
            t.id = 0;
        }
    };

    unload(assets.textures.floor);
    unload(assets.textures.wall);
    unload(assets.textures.table);
    unload(assets.textures.slot);
    unload(assets.textures.machineIdle);
    unload(assets.textures.machineDown);
    unload(assets.textures.slotReel);
    unload(assets.textures.slot7);
    unload(assets.textures.slotDiamond);
    unload(assets.textures.slotBell);
    unload(assets.textures.slotStrawberry);
    unload(assets.textures.playerBack);
    unload(assets.textures.panel);
    unload(assets.textures.button);
    unload(assets.textures.coin);
    unload(assets.textures.playerIdle);
    unload(assets.textures.playerWalk);
    unload(assets.textures.playerWin);
    unload(assets.textures.goldPanel);
    unload(assets.textures.panelCleanLarge);
    unload(assets.textures.panelCleanRow);
    unload(assets.textures.panelCleanInfo);
    unload(assets.textures.panelCleanOverlay);
    unload(assets.textures.mainMenu);
    unload(assets.textures.cursor);
    unload(assets.textures.tableau);
    unload(assets.textures.helpScreenshot);

    if (assets.hasFont && assets.uiFont.baseSize > 0) {
        UnloadFont(assets.uiFont);
    }
    for (auto& kv : assets.bitmapFont.glyphs) {
        if (kv.second.texture.id != 0) {
            UnloadTexture(kv.second.texture);
            kv.second.texture.id = 0;
        }
    }
    if (assets.audio.hasAudio) {
        if (assets.audio.ambient.ctxData) UnloadMusicStream(assets.audio.ambient);
        if (assets.audio.win.frameCount > 0) UnloadSound(assets.audio.win);
        if (assets.audio.empty.frameCount > 0) UnloadSound(assets.audio.empty);
    }
}
