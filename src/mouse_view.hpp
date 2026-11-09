#pragma once
#include "imgui.h"

// Highlight intensity (0..1) of each part of the mouse.
struct MouseVisual {
    float button[5] = {};  // left, right, middle, side 4 (back), side 5 (forward)
    float wheelUp = 0;
    float wheelDown = 0;
};

// Draws a vector mouse centred in the given area.
void DrawMouse(ImDrawList* dl, ImVec2 pos, ImVec2 size, const MouseVisual& v, ImU32 accent);
