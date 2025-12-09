#pragma once

#include <raylib.h>
#include <string>
#include <vector>
#include <unordered_map>

struct TexturePack {
    Texture2D floor{};
    Texture2D wall{};
    Texture2D table{};
    Texture2D slot{};           // legacy slot texture
    Texture2D machineIdle{};    // machine.png
    Texture2D machineDown{};    // machine_down.png
    Texture2D slotReel{};
    Texture2D playerBack{};
    Texture2D slot7{};
    Texture2D slotDiamond{};
    Texture2D slotBell{};
    Texture2D slotStrawberry{};
    Texture2D panel{};
    Texture2D goldPanel{};
    Texture2D panelCleanLarge{};
    Texture2D panelCleanRow{};
    Texture2D panelCleanInfo{};
    Texture2D panelCleanOverlay{};
    Texture2D button{};
    Texture2D mainMenu{};     // full-screen main menu image (clickable zones overlayed)
    Texture2D cursor{};      // tutorial cursor image
    Texture2D tableau{};     // right-side tableau panel image
    Texture2D helpScreenshot{}; // optional help screenshot (capture_ecran.png)
    Texture2D coin{};
    Texture2D playerIdle{};
    Texture2D playerWalk{};
    Texture2D playerWin{};
    bool loaded = false;
};

struct BitmapGlyph {
    Texture2D texture{};
    int width = 0;
    int height = 0;
    int advance = 0;
};

struct BitmapFont {
    std::unordered_map<char, BitmapGlyph> glyphs;
    int maxHeight = 1;
    bool loaded = false;
};

struct AudioPack {
    Music ambient{};
    Sound win{};
    Sound empty{};
    bool hasAudio = false;
};

struct Assets {
    TexturePack textures;
    Font uiFont{};
    BitmapFont bitmapFont{};
    AudioPack audio{};
    bool hasFont = false;
    bool hasTextures = false;
};

Assets load_assets(const std::string& basePath);
void unload_assets(Assets& assets);
