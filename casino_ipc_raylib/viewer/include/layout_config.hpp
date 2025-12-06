#pragma once

#include <string>
#include <vector>
#include <raylib.h>

struct LayoutParams {
    float slotScale = 1.0f;
    float symbolScale = 1.0f;
    float playerScale = 1.0f;
    float windowW = 62.0f;
    float windowH = 110.0f;
    float windowOffsetX = 40.0f;
    float windowOffsetY = 40.0f;
    float panelScale = 1.0f;
    float panelOffsetX = 0.0f;
    float panelOffsetY = 0.0f;
    float barOffsetY = 0.0f;
    float logOffsetY = 0.0f;
};

struct SlotLayout {
    float slotScale = 1.0f;
    float symbolScale = 1.0f;
    float playerScale = 1.0f;
    float windowW = 62.0f;
    float windowH = 110.0f;
    float windowOffsetX = 40.0f;
    float windowOffsetY = 40.0f;
    bool set = false;
};

// Load layout parameters (line starting with 'S') and per-slot overrides from layout.txt; returns true if loaded.
bool load_layout_params(const std::string& path, LayoutParams& out);
bool load_layout_file(const std::string& path, LayoutParams& out, std::vector<SlotLayout>& slots);

// Load scene.json (slots + ui) into params/slots/positions (positions optional).
bool load_scene_file(const std::string& path, LayoutParams& params, std::vector<SlotLayout>& slots, std::vector<Vector2>* positions);

// Load Tiled .tmj exported map with object layer "slots" and map properties for UI.
bool load_tiled_tmj(const std::string& path, LayoutParams& params, std::vector<SlotLayout>& slots, std::vector<Vector2>* positions);
