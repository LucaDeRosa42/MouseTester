#pragma once
#include "imgui.h"

// Intensità di illuminazione (0..1) di ogni parte del mouse.
struct MouseVisual {
    float button[5] = {};  // sinistro, destro, centrale, laterale 4 (indietro), laterale 5 (avanti)
    float wheelUp = 0;
    float wheelDown = 0;
};

// Disegna un mouse vettoriale centrato nell'area indicata.
void DrawMouse(ImDrawList* dl, ImVec2 pos, ImVec2 size, const MouseVisual& v, ImU32 accent);
