#include "assets.hpp"

#include <raylib.h>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

static Texture2D try_load_texture(const fs::path& path, bool& ok) {
    Texture2D tex{};
    if (fs::exists(path)) {
        tex = LoadTexture(path.string().c_str());
        if (tex.id != 0) {
            ok = true;
            SetTextureFilter(tex, TEXTURE_FILTER_BILINEAR);
        }
    }
    return tex;
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
    a.textures.button = try_load_texture(resolve("sprites/ui/button.png"), texOk);
    a.textures.coin = try_load_texture(resolve("sprites/ui/icon_coin.png"), texOk);
    a.textures.playerIdle = try_load_texture(resolve("sprites/players/player_idle.png"), texOk);
    a.textures.playerWalk = try_load_texture(resolve("sprites/players/player_walk.png"), texOk);
    a.textures.playerWin = try_load_texture(resolve("sprites/players/player_win.png"), texOk);
    a.textures.playerBack = try_load_texture(resolve("sprites/custom/player_back.png"), texOk);
    if (a.textures.playerBack.id == 0) {
        a.textures.playerBack = try_load_texture(fs::path(basePath) / "joueur.png", texOk);
    }

    a.hasTextures = texOk;

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

    if (assets.hasFont && assets.uiFont.baseSize > 0) {
        UnloadFont(assets.uiFont);
    }
}
