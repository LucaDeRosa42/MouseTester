// Mouse Tester - Copyright (C) 2026 lcrd42
// SPDX-License-Identifier: GPL-3.0-or-later
#include "mouse_view.hpp"

#include <algorithm>

namespace {

ImU32 Mix(ImU32 a, ImU32 b, float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    const ImVec4 ca = ImGui::ColorConvertU32ToFloat4(a);
    const ImVec4 cb = ImGui::ColorConvertU32ToFloat4(b);
    return ImGui::ColorConvertFloat4ToU32(ImVec4(ca.x + (cb.x - ca.x) * t, ca.y + (cb.y - ca.y) * t,
                                                 ca.z + (cb.z - ca.z) * t, ca.w + (cb.w - ca.w) * t));
}

ImU32 WithAlpha(ImU32 c, float a) {
    ImVec4 v = ImGui::ColorConvertU32ToFloat4(c);
    v.w *= a;
    return ImGui::ColorConvertFloat4ToU32(v);
}

// Glow around a highlighted part.
void Glow(ImDrawList* dl, ImVec2 a, ImVec2 b, float rounding, ImDrawFlags flags, ImU32 accent, float t) {
    if (t <= 0.01f) return;
    for (int i = 3; i >= 1; --i) {
        const float g = float(i) * 3.0f;
        dl->AddRect(ImVec2(a.x - g, a.y - g), ImVec2(b.x + g, b.y + g), WithAlpha(accent, 0.12f * t), rounding + g, flags, 2.0f);
    }
}

} // namespace

void DrawMouse(ImDrawList* dl, ImVec2 pos, ImVec2 size, const MouseVisual& v, ImU32 accent) {
    const ImU32 bodyCol    = IM_COL32(30, 32, 38, 255);
    const ImU32 outlineCol = IM_COL32(70, 75, 88, 255);
    const ImU32 buttonCol  = IM_COL32(44, 47, 56, 255);
    const ImU32 grooveCol  = IM_COL32(18, 19, 23, 255);
    const ImU32 dimCol     = IM_COL32(80, 85, 98, 255);

    // Mouse proportions: width w, height 1.7w
    const float w = std::min(size.x * 0.55f, size.y / 1.85f);
    const float h = w * 1.7f;
    const float cx = pos.x + size.x * 0.5f;
    const float x0 = cx - w * 0.5f, x1 = cx + w * 0.5f;
    const float y0 = pos.y + (size.y - h) * 0.5f, y1 = y0 + h;
    const float r = w * 0.48f;

    // Body
    dl->AddRectFilled(ImVec2(x0, y0), ImVec2(x1, y1), bodyCol, r);

    // Main buttons
    const float inset = w * 0.03f;
    const float gap = w * 0.03f;
    const float yB = y0 + h * 0.42f;
    const float br = r - inset;

    const ImVec2 lA(x0 + inset, y0 + inset), lB(cx - gap, yB);
    const ImVec2 rA(cx + gap, y0 + inset), rB(x1 - inset, yB);
    Glow(dl, lA, lB, br, ImDrawFlags_RoundCornersTopLeft, accent, v.button[0]);
    Glow(dl, rA, rB, br, ImDrawFlags_RoundCornersTopRight, accent, v.button[1]);
    dl->AddRectFilled(lA, lB, Mix(buttonCol, accent, v.button[0]), br, ImDrawFlags_RoundCornersTopLeft);
    dl->AddRectFilled(rA, rB, Mix(buttonCol, accent, v.button[1]), br, ImDrawFlags_RoundCornersTopRight);

    // Wheel
    const float ww = w * 0.13f;
    const ImVec2 wA(cx - ww * 0.5f, y0 + h * 0.10f), wB(cx + ww * 0.5f, y0 + h * 0.27f);
    dl->AddRectFilled(ImVec2(wA.x - inset, wA.y - inset), ImVec2(wB.x + inset, wB.y + inset), grooveCol, ww * 0.5f);
    Glow(dl, wA, wB, ww * 0.5f, 0, accent, v.button[2]);
    dl->AddRectFilled(wA, wB, Mix(IM_COL32(58, 62, 72, 255), accent, v.button[2]), ww * 0.5f);
    for (float y = wA.y + ww * 0.45f; y < wB.y - ww * 0.3f; y += ww * 0.32f)
        dl->AddLine(ImVec2(wA.x + ww * 0.2f, y), ImVec2(wB.x - ww * 0.2f, y), WithAlpha(grooveCol, 0.7f), 1.5f);

    // Scroll arrows to the right of the mouse
    const float ax = x1 + w * 0.16f;
    const float as = w * 0.07f;
    const float wMid = (wA.y + wB.y) * 0.5f;
    const ImU32 upCol = Mix(dimCol, accent, v.wheelUp);
    const ImU32 downCol = Mix(dimCol, accent, v.wheelDown);
    dl->AddTriangleFilled(ImVec2(ax, wMid - as * 2.4f), ImVec2(ax - as, wMid - as * 0.8f), ImVec2(ax + as, wMid - as * 0.8f), upCol);
    dl->AddTriangleFilled(ImVec2(ax, wMid + as * 2.4f), ImVec2(ax - as, wMid + as * 0.8f), ImVec2(ax + as, wMid + as * 0.8f), downCol);

    // Side buttons (left side): 5 = forward on top, 4 = back below
    const float sx0 = x0 - w * 0.035f, sx1 = x0 + w * 0.05f;
    const ImVec2 s5A(sx0, y0 + h * 0.46f), s5B(sx1, y0 + h * 0.56f);
    const ImVec2 s4A(sx0, y0 + h * 0.58f), s4B(sx1, y0 + h * 0.68f);
    const float sr = w * 0.03f;
    Glow(dl, s5A, s5B, sr, 0, accent, v.button[4]);
    Glow(dl, s4A, s4B, sr, 0, accent, v.button[3]);
    dl->AddRectFilled(s5A, s5B, Mix(buttonCol, accent, v.button[4]), sr);
    dl->AddRectFilled(s4A, s4B, Mix(buttonCol, accent, v.button[3]), sr);
    dl->AddRect(s5A, s5B, outlineCol, sr, 0, 1.5f);
    dl->AddRect(s4A, s4B, outlineCol, sr, 0, 1.5f);

    const float fs = ImGui::GetFontSize();
    dl->AddText(ImVec2(sx0 - fs * 0.9f, (s5A.y + s5B.y) * 0.5f - fs * 0.5f), Mix(dimCol, accent, v.button[4]), "5");
    dl->AddText(ImVec2(sx0 - fs * 0.9f, (s4A.y + s4B.y) * 0.5f - fs * 0.5f), Mix(dimCol, accent, v.button[3]), "4");

    // Outline and details
    dl->AddRect(ImVec2(x0, y0), ImVec2(x1, y1), outlineCol, r, 0, 2.0f);
    dl->AddCircleFilled(ImVec2(cx, y0 + h * 0.74f), w * 0.035f, WithAlpha(accent, 0.35f));
}
