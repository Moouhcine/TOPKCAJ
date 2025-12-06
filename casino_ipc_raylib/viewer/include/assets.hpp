#pragma once

#include <raylib.h>
#include <string>
#include <vector>

struct TexturePack {
    Texture2D floor{};
    Texture2D wall{};
    Texture2D table{};
    Texture2D slot{};
    Texture2D slotReel{};
    Texture2D playerBack{};
    Texture2D slot7{};
    Texture2D slotDiamond{};
    Texture2D slotBell{};
    Texture2D slotStrawberry{};
    Texture2D panel{};
    Texture2D button{};
    Texture2D coin{};
    Texture2D playerIdle{};
    Texture2D playerWalk{};
    Texture2D playerWin{};
    bool loaded = false;
};

struct Assets {
    TexturePack textures;
    Font uiFont{};
    bool hasFont = false;
    bool hasTextures = false;
};

Assets load_assets(const std::string& basePath);
void unload_assets(Assets& assets);
