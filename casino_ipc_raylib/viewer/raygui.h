// Minimal raygui-like header (self contained) used for the layout editor.
// Not the full raygui, just the controls we need: Label, Button, Slider, ValueBox, CheckBox.
// License: zlib (same spirit as raygui).

#ifndef MINI_RAYGUI_H
#define MINI_RAYGUI_H

#include "raylib.h"
#include <cstring>

#define RAYGUI_VERSION "mini-1.0"

typedef enum {
    GUI_STATE_NORMAL = 0,
    GUI_STATE_FOCUSED,
    GUI_STATE_PRESSED,
    GUI_STATE_DISABLED,
} GuiState;

static GuiState rgState = GUI_STATE_NORMAL;

static inline void GuiSetState(int state) { rgState = (GuiState)state; }
static inline int GuiGetState(void) { return (int)rgState; }

static inline void GuiLabel(Rectangle bounds, const char* text) {
    DrawText(text, (int)bounds.x, (int)bounds.y, (int)bounds.height - 4, RAYWHITE);
}

static inline bool GuiButton(Rectangle bounds, const char* text) {
    Color bg = (rgState == GUI_STATE_PRESSED) ? Color{60, 120, 200, 255} : Color{40, 80, 140, 255};
    DrawRectangleRounded(bounds, 0.2f, 6, bg);
    DrawRectangleRoundedLines(bounds, 0.2f, 6, Color{200, 220, 255, 255});
    int textSize = (int)bounds.height - 6;
    int tx = (int)(bounds.x + bounds.width / 2 - MeasureText(text, textSize) / 2);
    int ty = (int)(bounds.y + bounds.height / 2 - textSize / 2);
    DrawText(text, tx, ty, textSize, RAYWHITE);
    bool clicked = false;
    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        Vector2 m = GetMousePosition();
        if (CheckCollisionPointRec(m, bounds)) clicked = true;
    }
    return clicked;
}

static inline float GuiSlider(Rectangle bounds, const char* left, const char* right, float value, float minValue, float maxValue) {
    DrawRectangleRec(bounds, Color{30, 50, 80, 255});
    float t = (value - minValue) / (maxValue - minValue);
    t = fminf(fmaxf(t, 0.0f), 1.0f);
    Rectangle knob{bounds.x + t * (bounds.width - 12), bounds.y, 12, bounds.height};
    DrawRectangleRec(knob, Color{120, 200, 255, 255});
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        Vector2 m = GetMousePosition();
        if (CheckCollisionPointRec(m, bounds)) {
            t = (m.x - bounds.x) / bounds.width;
            t = fminf(fmaxf(t, 0.0f), 1.0f);
            value = minValue + t * (maxValue - minValue);
        }
    }
    int ty = (int)(bounds.y - bounds.height - 4);
    DrawText(left, (int)bounds.x, ty, (int)bounds.height - 4, RAYWHITE);
    DrawText(right, (int)(bounds.x + bounds.width - MeasureText(right, (int)bounds.height - 4)), ty, (int)bounds.height - 4, RAYWHITE);
    return value;
}

static inline bool GuiValueBox(Rectangle bounds, const char* text, float* value, float minValue, float maxValue, bool editMode) {
    DrawRectangleRec(bounds, Color{30, 50, 70, 255});
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%.2f", *value);
    DrawText(buf, (int)bounds.x + 6, (int)bounds.y + 4, (int)bounds.height - 8, RAYWHITE);
    DrawText(text, (int)bounds.x, (int)bounds.y - (int)bounds.height, (int)bounds.height - 6, GRAY);
    if (editMode && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        Vector2 m = GetMousePosition();
        if (CheckCollisionPointRec(m, bounds)) {
            *value += GetMouseWheelMove() * ((maxValue - minValue) / 50.0f);
            if (*value < minValue) *value = minValue;
            if (*value > maxValue) *value = maxValue;
        }
    }
    return false;
}

static inline bool GuiCheckBox(Rectangle bounds, const char* text, bool* checked) {
    DrawRectangleRec(bounds, Color{30, 50, 70, 255});
    if (*checked) DrawRectangleRec({bounds.x + 4, bounds.y + 4, bounds.width - 8, bounds.height - 8}, Color{120, 200, 255, 255});
    DrawText(text, (int)(bounds.x + bounds.width + 6), (int)bounds.y, (int)bounds.height - 4, RAYWHITE);
    bool toggled = false;
    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        Vector2 m = GetMousePosition();
        Rectangle hit = {bounds.x, bounds.y, bounds.width + MeasureText(text, (int)bounds.height - 4) + 8, bounds.height};
        if (CheckCollisionPointRec(m, hit)) { *checked = !*checked; toggled = true; }
    }
    return toggled;
}

#endif // MINI_RAYGUI_H
