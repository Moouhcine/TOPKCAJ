#pragma once

#include <raylib.h>
#include "snapshot.hpp"
#include "assets.hpp"
#include "anim.hpp"
#include "layout_config.hpp"

struct RenderSettings {
    int width = 1920;
    int height = 1080;
};

void set_layout_params(const LayoutParams& params);
void set_slot_layout(int idx, const SlotLayout& slot);
void set_slot_position(int idx, Vector2 pos);
struct SharedAttachment;
void render_frame(const Assets& assets, SceneState& scene, const CasinoSnap& snap, const RenderSettings& cfg, SharedAttachment* att);
// Render scene contents without calling BeginDrawing()/EndDrawing().
void render_scene_no_begin(const Assets& assets, SceneState& scene, const CasinoSnap& snap, const RenderSettings& cfg, SharedAttachment* att);
