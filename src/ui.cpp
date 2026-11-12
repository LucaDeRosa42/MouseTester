// Mouse Tester - Copyright (C) 2026 lcrd42
// SPDX-License-Identifier: GPL-3.0-or-later
#include "ui.hpp"

#include "export.hpp"
#include "i18n.hpp"
#include "resource.h"

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstdio>

namespace {

const ImVec4 kAccent(0.30f, 0.76f, 1.00f, 1.00f);
const ImVec4 kGood(0.35f, 0.85f, 0.55f, 1.00f);
const ImVec4 kWarn(1.00f, 0.72f, 0.30f, 1.00f);
const ImVec4 kBad(1.00f, 0.40f, 0.40f, 1.00f);

// Translated label with a fixed ImGui ID ("text###tag"), so tabs and widgets keep
// their state when the language changes.
const char* L(Str id, const char* tag) {
    static char buf[8][600];
    static int next = 0;
    char* b = buf[next++ % 8];
    std::snprintf(b, sizeof(buf[0]), "%s###%s", T(id), tag);
    return b;
}

void SectionTitle(const char* text) {
    ImGui::PushStyleColor(ImGuiCol_Text, kAccent);
    ImGui::TextUnformatted(text);
    ImGui::PopStyleColor();
    ImGui::Separator();
    ImGui::Spacing();
}

void Hint(const char* text) {
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
    ImGui::TextWrapped("%s", text);
    ImGui::PopStyleColor();
}

// Explanatory popup shown while the previous item is hovered.
void Explain(const char* text) {
    if (!ImGui::BeginItemTooltip()) return;
    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 26.0f);
    ImGui::TextUnformatted(text);
    ImGui::PopTextWrapPos();
    ImGui::EndTooltip();
}

// "Small label + value" cell in a statistics table, with an optional explanation on hover.
void Stat(const char* label, const char* value, const ImVec4* color = nullptr, const char* tip = nullptr) {
    ImGui::TableNextColumn();
    ImGui::BeginGroup();
    ImGui::TextDisabled("%s", label);
    if (color) ImGui::TextColored(*color, "%s", value);
    else ImGui::TextUnformatted(value);
    ImGui::EndGroup();
    if (tip) Explain(tip);
}

void StatF(const char* label, const char* fmt, double v, const ImVec4* color = nullptr) {
    char buf[64];
    std::snprintf(buf, sizeof(buf), fmt, v);
    Stat(label, buf, color);
}

// ---------------------------------------------------------------- plot scales
// "Round" step (1, 2, 5 × 10^n) that splits `range` into about `ticks` parts.
double NiceStep(double range, int ticks) {
    const double raw = range / ticks;
    const double p = std::pow(10.0, std::floor(std::log10(raw)));
    const double f = raw / p;
    return (f < 1.5 ? 1.0 : f < 3.5 ? 2.0 : f < 7.5 ? 5.0 : 10.0) * p;
}

// Inner area of the last ImGui plot (without FramePadding).
struct PlotArea { ImVec2 min, max; };
PlotArea LastPlotArea() {
    const ImVec2 pad = ImGui::GetStyle().FramePadding;
    const ImVec2 a = ImGui::GetItemRectMin(), b = ImGui::GetItemRectMax();
    return {ImVec2(a.x + pad.x, a.y + pad.y), ImVec2(b.x - pad.x, b.y - pad.y)};
}

// Label of the reference value: "125 µs · 8000 Hz".
void RefLabel(char* buf, size_t size, double refUs) {
    std::snprintf(buf, size, "%.0f µs · %.0f Hz", refUs, 1e6 / refUs);
}

// Horizontal axis in µs below the last plot (histogram), with a vertical grid
// and a highlighted line on the reference interval.
void XAxisMicros(double maxUs, double refUs) {
    const PlotArea r = LastPlotArea();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImU32 grid = ImGui::GetColorU32(ImGuiCol_TextDisabled, 0.30f);
    const ImU32 text = ImGui::GetColorU32(ImGuiCol_TextDisabled);
    const float w = r.max.x - r.min.x;

    ImGui::Dummy(ImVec2(w, ImGui::GetTextLineHeight())); // row reserved for the numbers
    const float ty = ImGui::GetItemRectMin().y;

    const double step = NiceStep(maxUs, 8);
    for (double v = 0; v <= maxUs + step * 0.01; v += step) {
        const float x = r.min.x + float(v / maxUs) * w;
        dl->AddLine(ImVec2(x, r.min.y), ImVec2(x, r.max.y), grid);
        char t[32];
        std::snprintf(t, sizeof(t), v + step > maxUs + step * 0.01 ? "%.0f µs" : "%.0f", v);
        const float tw = ImGui::CalcTextSize(t).x;
        dl->AddText(ImVec2(std::clamp(x - tw * 0.5f, r.min.x, r.max.x - tw), ty), text, t);
    }

    if (refUs > 0 && refUs < maxUs) {
        const ImU32 accent = ImGui::ColorConvertFloat4ToU32(kAccent);
        const float x = r.min.x + float(refUs / maxUs) * w;
        dl->AddLine(ImVec2(x, r.min.y), ImVec2(x, r.max.y), accent, 1.5f);
        char t[48];
        RefLabel(t, sizeof(t), refUs);
        const float tw = ImGui::CalcTextSize(t).x;
        dl->AddText(ImVec2(std::min(x + 4.0f, r.max.x - tw), r.min.y), accent, t);
    }
}

// Vertical axis in µs inside the last plot (timeline), with a horizontal grid
// and a highlighted line on the reference interval.
void YAxisMicros(double maxUs, double refUs) {
    const PlotArea r = LastPlotArea();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImU32 grid = ImGui::GetColorU32(ImGuiCol_TextDisabled, 0.30f);
    const ImU32 text = ImGui::GetColorU32(ImGuiCol_TextDisabled);
    const float h = r.max.y - r.min.y;
    const float lh = ImGui::GetTextLineHeight();

    const double step = NiceStep(maxUs, 4);
    for (double v = step; v <= maxUs + step * 0.01; v += step) {
        const float y = r.max.y - float(v / maxUs) * h;
        dl->AddLine(ImVec2(r.min.x, y), ImVec2(r.max.x, y), grid);
        char t[32];
        std::snprintf(t, sizeof(t), "%.0f µs", v);
        dl->AddText(ImVec2(r.min.x + 4.0f, std::max(r.min.y, y - lh)), text, t);
    }

    if (refUs > 0 && refUs < maxUs) {
        const ImU32 accent = ImGui::ColorConvertFloat4ToU32(kAccent);
        const float y = r.max.y - float(refUs / maxUs) * h;
        dl->AddLine(ImVec2(r.min.x, y), ImVec2(r.max.x, y), accent, 1.5f);
        char t[48];
        RefLabel(t, sizeof(t), refUs);
        const float tw = ImGui::CalcTextSize(t).x;
        dl->AddText(ImVec2(r.max.x - tw - 4.0f, std::max(r.min.y, y - lh)), accent, t);
    }
}

// ---------------------------------------------------------------- left panel
void DrawLeftPanel(Analyzer& a, UiState& s, double dt) {
    ImGui::PushFont(s.fontBig);
    ImGui::TextColored(kAccent, "MOUSE TESTER");
    ImGui::PopFont();
    ImGui::SameLine();
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + s.fontBig->FontSize * 0.4f);
    ImGui::TextDisabled("v" APP_VERSION_STR);
    ImGui::Spacing();

    // Device selection
    for (auto& [h, count] : a.devicePackets)
        if (!s.deviceNames.count(h)) s.deviceNames[h] = RawInputCapture::deviceName(h);

    const char* current = a.deviceFilter ? s.deviceNames[a.deviceFilter].c_str() : T(Str::AllDevices);
    ImGui::SetNextItemWidth(-FLT_MIN);
    if (ImGui::BeginCombo("##device", current)) {
        if (ImGui::Selectable(T(Str::AllDevices), a.deviceFilter == nullptr)) {
            a.deviceFilter = nullptr;
            a.resetAll();
        }
        for (auto& [h, count] : a.devicePackets) {
            char label[320];
            const int len = std::snprintf(label, sizeof(label), T(Str::DevicePackets), s.deviceNames[h].c_str(), (unsigned long long)count);
            std::snprintf(label + len, sizeof(label) - len, "##%p", h);
            if (ImGui::Selectable(label, a.deviceFilter == h)) {
                a.deviceFilter = h;
                a.resetAll();
            }
        }
        ImGui::EndCombo();
    }

    // Interface language (remembered for the next start)
    ImGui::AlignTextToFramePadding();
    ImGui::TextDisabled("%s", T(Str::Language));
    ImGui::SameLine();
    ImGui::SetNextItemWidth(-FLT_MIN);
    if (ImGui::BeginCombo("##language", LanguageName(CurrentLanguage()))) {
        for (int i = 0; i < int(Lang::Count); ++i)
            if (ImGui::Selectable(LanguageName(Lang(i)), CurrentLanguage() == Lang(i))) SetLanguage(Lang(i));
        ImGui::EndCombo();
    }

    // Highlight: full while pressed, then fades out in ~200 ms
    const float decay = float(dt) * 5.0f;
    for (int i = 0; i < 5; ++i)
        s.visual.button[i] = a.buttons[i].down ? 1.0f : std::max(0.0f, s.visual.button[i] - decay);
    s.visual.wheelUp = std::max(0.0f, s.visual.wheelUp - decay);
    s.visual.wheelDown = std::max(0.0f, s.visual.wheelDown - decay);
    if (a.wheelLastQpc != s.seenWheelQpc) {
        s.seenWheelQpc = a.wheelLastQpc;
        (a.wheelLastDir > 0 ? s.visual.wheelUp : s.visual.wheelDown) = 1.0f;
    }

    // Mouse drawing: it takes the room left by the button table and the bottom buttons, so the
    // panel never needs a scrollbar (between 160 and 330 px at 100% scale).
    const ImGuiStyle& st = ImGui::GetStyle();
    const float rowH = ImGui::GetTextLineHeight() + st.CellPadding.y * 2;
    const float below = rowH * 6 + ImGui::GetTextLineHeightWithSpacing() * 2 + ImGui::GetFrameHeight() * 2 +
                        st.ItemSpacing.y * 6;
    const float mouseH = std::clamp(ImGui::GetContentRegionAvail().y - below, 160.0f * s.scale, 330.0f * s.scale);
    const ImVec2 area(ImGui::GetContentRegionAvail().x, mouseH);
    const ImVec2 pos = ImGui::GetCursorScreenPos();
    DrawMouse(ImGui::GetWindowDrawList(), pos, area, s.visual, ImGui::ColorConvertFloat4ToU32(kAccent));
    ImGui::Dummy(area);

    // Button summary
    if (ImGui::BeginTable("buttons", 3, ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn(T(Str::ColButton));
        ImGui::TableSetupColumn(T(Str::ColPresses));
        ImGui::TableSetupColumn(T(Str::ColChatter));
        ImGui::TableHeadersRow();
        for (int i = 0; i < 5; ++i) {
            const ButtonInfo& b = a.buttons[i];
            const char* name = T(Str(int(Str::BtnLeft) + i));
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            if (b.down) ImGui::TextColored(kAccent, "%s", name);
            else ImGui::TextUnformatted(name);
            ImGui::TableNextColumn();
            ImGui::Text("%llu", (unsigned long long)b.presses);
            ImGui::TableNextColumn();
            if (b.chatter) ImGui::TextColored(kBad, "%llu", (unsigned long long)b.chatter);
            else ImGui::TextDisabled("0");
        }
        ImGui::EndTable();
    }

    ImGui::Spacing();
    ImGui::Text(T(Str::WheelSummary), a.wheelUp / 120.0, a.wheelDown / 120.0);

    // Buttons at the bottom of the panel, with the outcome of the last export above them
    const ImGuiStyle& style = ImGui::GetStyle();
    const float btnH = ImGui::GetFrameHeight();
    const float bottom = btnH * 2 + style.ItemSpacing.y + ImGui::GetTextLineHeightWithSpacing() + style.WindowPadding.y;
    ImGui::SetCursorPosY(std::max(ImGui::GetCursorPosY(), ImGui::GetWindowHeight() - bottom));
    if (s.exportStatus >= 0) ImGui::TextColored(s.exportOk ? kGood : kBad, "%s", T(Str(s.exportStatus)));
    else ImGui::NewLine();
    if (ImGui::Button(L(Str::ExportReport, "export"), ImVec2(-FLT_MIN, 0))) {
        const std::wstring path = AskSavePath(L"mouse-report.txt", L"Text file (*.txt)\0*.txt\0", L"txt");
        if (!path.empty()) {
            s.exportOk = ExportReport(a, s, path);
            s.exportStatus = int(s.exportOk ? Str::ReportSaved : Str::ReportFailed);
        }
    }
    if (ImGui::Button(L(Str::ResetAll, "resetall"), ImVec2(-FLT_MIN, 0))) a.resetAll();
}

// ---------------------------------------------------------------- polling tab
void DrawPollingTab(Analyzer& a, UiState& s) {
    const IntervalStats& st = a.stats;
    // From the mean, not the median: see Analyzer::refresh().
    const double hz = st.meanUs > 0 ? 1e6 / st.meanUs : 0.0;
    const int nominal = NearestStandardRate(hz);

    // The number turns grey while measuring is paused: it shows the last valid value.
    const bool live = a.collecting();
    ImGui::PushFont(s.fontBig);
    if (st.count > 100 && live) ImGui::Text("%.0f Hz", hz);
    else if (st.count > 100) ImGui::TextDisabled("%.0f Hz", hz);
    else ImGui::TextDisabled("--- Hz");
    ImGui::PopFont();
    ImGui::SameLine();
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + s.fontBig->FontSize * 0.35f);
    if (nominal && st.count > 100) ImGui::TextDisabled(T(Str::Nominal), nominal);
    else ImGui::TextDisabled("%s", T(Str::MoveCircles));

    // Measuring state: current speed compared with the required minimum.
    const double v = a.speedPerPacket();
    if (live)
        ImGui::TextColored(kGood, T(Str::Measuring), v);
    else if (a.moving())
        ImGui::TextColored(kWarn, T(Str::TooSlow), v, Analyzer::kFastSpeed);
    else
        ImGui::TextDisabled("%s", T(Str::NotMoving));

    ImGui::Spacing();
    if (ImGui::BeginTable("pstats", 4, ImGuiTableFlags_SizingStretchSame)) {
        StatF(T(Str::Mean), "%.1f µs", st.meanUs);
        StatF(T(Str::Median), "%.1f µs", st.medianUs);
        const double jitterPct = st.meanUs > 0 ? 100.0 * st.stddevUs / st.meanUs : 0.0;
        const ImVec4* jc = jitterPct < 10 ? &kGood : jitterPct < 25 ? &kWarn : &kBad;
        StatF(T(Str::Jitter), "%.1f µs", st.stddevUs, st.count > 100 ? jc : nullptr);
        StatF(T(Str::PacketsPerSec), "%.0f", a.packetsLastSecond);

        char buf[64];
        std::snprintf(buf, sizeof(buf), "%.0f / %.0f µs", st.minUs, st.maxUs);
        Stat(T(Str::MinMax), buf);
        std::snprintf(buf, sizeof(buf), "%.0f / %.0f µs", st.p01Us, st.p99Us);
        Stat(T(Str::Percentiles), buf);
        const ImVec4* oc = st.outlierPct < 1 ? &kGood : st.outlierPct < 5 ? &kWarn : &kBad;
        StatF(T(Str::Outliers), "%.2f %%", st.outlierPct, st.count > 100 ? oc : nullptr);
        StatF(T(Str::Batched), "%.2f %%", st.batchedPct);
        ImGui::EndTable();
    }

    // Interval histogram
    ImGui::Spacing();
    SectionTitle(T(Str::IntervalDistribution));
    const double maxUs = std::max(st.meanUs * 3.0, 400.0);
    // Reference: the interval of the detected standard rate, otherwise the mean.
    const double refUs = st.count > 100 ? (nominal ? 1e6 / nominal : st.meanUs) : 0.0;
    a.histogram(120, maxUs, s.histogram);
    const float plotH = 170.0f * s.scale;
    ImGui::PlotHistogram("##hist", s.histogram.data(), (int)s.histogram.size(), 0, nullptr, 0.0f, FLT_MAX, ImVec2(-FLT_MIN, plotH));
    XAxisMicros(maxUs, refUs);

    // Over time
    ImGui::Spacing();
    SectionTitle(T(Str::LastIntervals));
    const auto& iv = a.intervals();
    const size_t n = std::min<size_t>(iv.size(), 2000);
    s.timeline.resize(n);
    for (size_t i = 0; i < n; ++i) s.timeline[i] = iv[iv.size() - n + i];
    ImGui::PlotLines("##timeline", s.timeline.data(), (int)n, 0, nullptr, 0.0f, (float)maxUs, ImVec2(-FLT_MIN, plotH));
    YAxisMicros(maxUs, refUs);

    ImGui::Spacing();
    Hint(T(Str::HintPollSpeed));
    Hint(T(Str::HintBatched));
    if (ImGui::Button(L(Str::ResetPolling, "resetpolling"))) a.resetPolling();
    ImGui::SameLine();
    if (ImGui::Button(L(Str::ExportIntervals, "exportcsv"))) {
        const std::wstring path = AskSavePath(L"polling-intervals.csv", L"CSV file (*.csv)\0*.csv\0", L"csv");
        if (!path.empty()) {
            s.exportOk = ExportIntervalsCsv(a, path);
            s.exportStatus = int(s.exportOk ? Str::IntervalsSaved : Str::IntervalsFailed);
        }
    }
}

// ---------------------------------------------------------------- sensor tab
constexpr size_t kRecentPoints = 3000; // "recent movements": highlighted and used by "Recent only"

// Explanation and legend name of each sensor anomaly (index = MarkKind).
const char* MarkTip(int i) { return T(Str(int(Str::TipMissing) + i)); }
const char* MarkName(int i) { return T(Str(int(Str::MarkMissing) + i)); }

const ImU32 kMarkColors[4] = {
    IM_COL32(255, 90, 90, 255),   // missing packet
    IM_COL32(255, 184, 77, 255),  // spike
    IM_COL32(200, 120, 255, 255), // dip
    IM_COL32(255, 90, 200, 255),  // reversal
};

// Sensor path with zoom (wheel), panning (drag) and automatic fit (double click).
// Coordinates are in sensor counts.
void DrawPath(const Analyzer& a, UiState& s, ImVec2 size) {
    UiState::PathView& v = s.pathView;
    const auto& path = a.path();
    const size_t n = path.size();

    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 p0 = ImGui::GetCursorScreenPos();
    const ImVec2 p1(p0.x + size.x, p0.y + size.y);
    const ImVec2 c((p0.x + p1.x) * 0.5f, (p0.y + p1.y) * 0.5f);

    ImGui::InvisibleButton("##path", size);
    const bool hovered = ImGui::IsItemHovered();
    ImGui::SetItemKeyOwner(ImGuiKey_MouseWheelY); // the wheel zooms instead of scrolling the window
    const ImGuiIO& io = ImGui::GetIO();

    // --- Automatic fit ---
    if (v.autoFit && n >= 2) {
        const size_t from = v.recentOnly && n > kRecentPoints ? n - kRecentPoints : 0;
        float minX = FLT_MAX, minY = FLT_MAX, maxX = -FLT_MAX, maxY = -FLT_MAX;
        for (size_t i = from; i < n; ++i) {
            minX = std::min(minX, path[i].x); maxX = std::max(maxX, path[i].x);
            minY = std::min(minY, path[i].y); maxY = std::max(maxY, path[i].y);
        }
        const float margin = 16.0f;
        v.pxPerCount = std::min((size.x - 2 * margin) / std::max(maxX - minX, 1.0f),
                                (size.y - 2 * margin) / std::max(maxY - minY, 1.0f));
        v.centerX = (minX + maxX) * 0.5f;
        v.centerY = (minY + maxY) * 0.5f;
    }

    // --- Zoom around the cursor, panning, reset ---
    if (hovered && io.MouseWheel != 0.0f) {
        const float wx = v.centerX + (io.MousePos.x - c.x) / v.pxPerCount;
        const float wy = v.centerY + (io.MousePos.y - c.y) / v.pxPerCount;
        v.pxPerCount = std::clamp(v.pxPerCount * std::pow(1.25f, io.MouseWheel), 1e-4f, 400.0f);
        v.centerX = wx - (io.MousePos.x - c.x) / v.pxPerCount;
        v.centerY = wy - (io.MousePos.y - c.y) / v.pxPerCount;
        v.autoFit = false;
    }
    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left, 0.0f)) {
        v.centerX -= io.MouseDelta.x / v.pxPerCount;
        v.centerY -= io.MouseDelta.y / v.pxPerCount;
        v.autoFit = false;
    }
    if (hovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) v.autoFit = true;

    auto toScreen = [&](float x, float y) {
        return ImVec2(c.x + (x - v.centerX) * v.pxPerCount, c.y + (y - v.centerY) * v.pxPerCount);
    };

    // --- Background and grid in counts ---
    dl->AddRectFilled(p0, p1, IM_COL32(14, 15, 18, 255), 8.0f);
    dl->PushClipRect(p0, p1, true);
    const double step = std::max(1.0, NiceStep(size.x / v.pxPerCount, 8));
    const ImU32 grid = IM_COL32(34, 37, 44, 255);
    const double x0 = std::floor((v.centerX - (c.x - p0.x) / v.pxPerCount) / step) * step;
    const double y0 = std::floor((v.centerY - (c.y - p0.y) / v.pxPerCount) / step) * step;
    for (double gx = x0; ; gx += step) {
        const float sx = toScreen(float(gx), 0).x;
        if (sx > p1.x) break;
        dl->AddLine(ImVec2(sx, p0.y), ImVec2(sx, p1.y), grid);
    }
    for (double gy = y0; ; gy += step) {
        const float sy = toScreen(0, float(gy)).y;
        if (sy > p1.y) break;
        dl->AddLine(ImVec2(p0.x, sy), ImVec2(p1.x, sy), grid);
    }

    if (n >= 2) {
        // Line in chunks, to stay below the vertex limit of a single call.
        // Thicker when zoomed in, so every segment is clearly visible.
        const float thick = std::clamp(1.0f + v.pxPerCount * 0.15f, 1.5f, 3.0f);
        constexpr size_t kChunk = 1500;
        s.pathPts.reserve(kChunk + 1);
        const size_t recent = n > kRecentPoints ? n - kRecentPoints : 0;
        for (size_t start = 0; start + 1 < n; start += kChunk) {
            s.pathPts.clear();
            const size_t end = std::min(n, start + kChunk + 1);
            ImVec2 bbMin(FLT_MAX, FLT_MAX), bbMax(-FLT_MAX, -FLT_MAX);
            for (size_t i = start; i < end; ++i) {
                const ImVec2 q = toScreen(path[i].x, path[i].y);
                bbMin = ImVec2(std::min(bbMin.x, q.x), std::min(bbMin.y, q.y));
                bbMax = ImVec2(std::max(bbMax.x, q.x), std::max(bbMax.y, q.y));
                s.pathPts.push_back(q);
            }
            // Chunks entirely outside the view are not drawn (useful when zoomed in).
            if (bbMax.x < p0.x || bbMin.x > p1.x || bbMax.y < p0.y || bbMin.y > p1.y) continue;
            const ImU32 col = start + kChunk >= recent ? ImGui::ColorConvertFloat4ToU32(kAccent) : IM_COL32(60, 110, 140, 255);
            dl->AddPolyline(s.pathPts.data(), (int)s.pathPts.size(), col, 0, thick);
        }

        // Up close, every single packet shows up as a dot.
        if (v.pxPerCount >= 3.0f) {
            const ImU32 dot = IM_COL32(230, 240, 255, 255);
            const float rad = std::min(1.5f + v.pxPerCount * 0.08f, 3.5f);
            for (size_t i = 0; i < n; ++i) {
                const ImVec2 q = toScreen(path[i].x, path[i].y);
                if (q.x >= p0.x && q.x <= p1.x && q.y >= p0.y && q.y <= p1.y) dl->AddCircleFilled(q, rad, dot, 8);
            }
        }
    }

    // Angle snapping: guides through the start of the last stroke and its best-fit line.
    const StrokeFit fit = a.strokeFit();
    if (v.guides && fit.valid) {
        const ImU32 guide = IM_COL32(255, 255, 255, 28);
        const double reach = 1e6; // long enough to cross the whole view at any zoom
        for (int deg = 0; deg < 180; deg += 45) {
            const double r = deg * 3.14159265358979 / 180.0;
            const double gx = std::cos(r) * reach, gy = -std::sin(r) * reach;
            dl->AddLine(toScreen(float(fit.x0 - gx), float(fit.y0 - gy)), toScreen(float(fit.x0 + gx), float(fit.y0 + gy)),
                        guide, 1.0f);
        }
        dl->AddLine(toScreen(float(fit.x0), float(fit.y0)), toScreen(float(fit.x1), float(fit.y1)),
                    IM_COL32(255, 255, 255, 200), 1.5f);
    }

    // Sensor anomalies highlighted on the path.
    const auto& marks = a.marks();
    for (size_t i = 0; i < marks.size(); ++i) {
        const SensorMark& m = marks[i];
        dl->AddCircle(toScreen(m.x, m.y), 6.0f, kMarkColors[int(m.kind)], 12, 2.0f);
    }

    // Grid scale in the bottom right corner.
    char t[48];
    std::snprintf(t, sizeof(t), T(Str::Grid), step);
    const ImVec2 ts = ImGui::CalcTextSize(t);
    dl->AddText(ImVec2(p1.x - ts.x - 8.0f, p1.y - ts.y - 6.0f), ImGui::GetColorU32(ImGuiCol_TextDisabled), t);
    dl->PopClipRect();
}

void DrawSensorTab(Analyzer& a, UiState& s) {
    if (ImGui::BeginTable("sstats", 5, ImGuiTableFlags_SizingStretchSame)) {
        // Color by frequency: anomalies count relative to the checked packets.
        auto color = [&](uint64_t v) {
            const double perMille = a.checkedPackets ? 1000.0 * double(v) / double(a.checkedPackets) : 0.0;
            return v == 0 ? &kGood : perMille < 1.0 ? &kWarn : &kBad;
        };
        char buf[32];
        std::snprintf(buf, sizeof(buf), "%llu", (unsigned long long)a.checkedPackets);
        Stat(T(Str::PacketsChecked), buf, nullptr, T(Str::TipChecked));
        std::snprintf(buf, sizeof(buf), "%llu", (unsigned long long)a.skippedEstimate);
        Stat(T(Str::Missing), buf, color(a.skippedEstimate), MarkTip(0));
        std::snprintf(buf, sizeof(buf), "%llu", (unsigned long long)a.spikes);
        Stat(T(Str::Spikes), buf, color(a.spikes), MarkTip(1));
        std::snprintf(buf, sizeof(buf), "%llu", (unsigned long long)a.dips);
        Stat(T(Str::Dips), buf, color(a.dips), MarkTip(2));
        std::snprintf(buf, sizeof(buf), "%llu", (unsigned long long)a.reversals);
        Stat(T(Str::Reversals), buf, color(a.reversals), MarkTip(3));
        ImGui::EndTable();
    }
    // Last stroke: angle and straightness, to spot angle snapping.
    const StrokeFit fit = a.strokeFit();
    ImGui::Spacing();
    if (fit.valid) {
        const double nearest = std::round(fit.angleDeg / 45.0) * 45.0;
        const double off = fit.angleDeg - nearest;
        const double devPct = 100.0 * fit.rmsDev / fit.length;
        // Very straight and right on a guide: likely snapping. Thresholds are deliberately strict,
        // a hand on a long stroke always wobbles by more than this.
        const bool suspect = fit.length >= 1000 && fit.rmsDev < 1.0 && std::fabs(off) < 0.5;
        ImGui::BeginGroup();
        ImGui::TextDisabled("%s", T(Str::LastStroke));
        ImGui::SameLine();
        ImGui::Text(T(Str::StrokeInfo), fit.length, fit.angleDeg, fit.rmsDev, devPct, nearest == -90.0 ? 90.0 : nearest, off);
        if (suspect) {
            ImGui::SameLine();
            ImGui::TextColored(kWarn, "%s", T(Str::PossibleSnapping));
        }
        ImGui::EndGroup();
        Explain(T(Str::TipStroke));
    } else {
        ImGui::TextDisabled("%s", T(Str::StrokeNone));
    }

    ImGui::Spacing();
    SectionTitle(T(Str::Path));

    // View controls and anomaly legend
    UiState::PathView& v = s.pathView;
    if (ImGui::Button(L(Str::Fit, "fit"))) v.autoFit = true;
    ImGui::SameLine();
    if (ImGui::Checkbox(L(Str::RecentOnly, "recent"), &v.recentOnly)) v.autoFit = true;
    ImGui::SameLine();
    ImGui::Checkbox(L(Str::SnapGuides, "guides"), &v.guides);
    ImGui::SameLine();
    ImGui::TextDisabled(T(Str::Zoom), v.pxPerCount, v.autoFit ? T(Str::ZoomAuto) : "");
    for (int i = 0; i < 4; ++i) {
        ImGui::SameLine(0, i == 0 ? 24.0f * s.scale : -1.0f);
        ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(kMarkColors[i]), "o %s", MarkName(i));
        Explain(MarkTip(i));
    }

    const float h = std::max(200.0f, ImGui::GetContentRegionAvail().y - ImGui::GetTextLineHeightWithSpacing() * 5.5f);
    DrawPath(a, s, ImVec2(ImGui::GetContentRegionAvail().x, h));
    Hint(T(Str::HintPath));
    Hint(T(Str::HintContinuity));
    if (ImGui::Button(L(Str::ResetSensor, "resetsensor"))) a.resetSensor();
}

// ---------------------------------------------------------------- DPI tab
// Colour for a deviation from the DPI set on the mouse.
const ImVec4* DeviationColor(double pct) {
    return std::fabs(pct) < 2 ? &kGood : std::fabs(pct) < 5 ? &kWarn : &kBad;
}

void DrawDpiTab(Analyzer& a, UiState& s) {
    Hint(T(Str::HintDpi));
    ImGui::Spacing();

    // --- Settings ---
    ImGui::SetNextItemWidth(160.0f * s.scale);
    ImGui::InputFloat("##distance", &s.dpiDistance, 1.0f, 5.0f, "%.2f");
    s.dpiDistance = std::max(s.dpiDistance, 0.5f);
    ImGui::SameLine();
    int unit = s.dpiInches ? 1 : 0;
    ImGui::SetNextItemWidth(70.0f * s.scale);
    if (ImGui::Combo(L(Str::Distance, "unit"), &unit, "cm\0in\0")) s.dpiInches = unit == 1;
    ImGui::SameLine(0, 30.0f * s.scale);
    ImGui::SetNextItemWidth(160.0f * s.scale);
    ImGui::InputInt(L(Str::DpiSet, "dpiset"), &s.dpiConfigured, 100, 400);
    s.dpiConfigured = std::max(s.dpiConfigured, 1);
    const double inches = s.dpiInches ? s.dpiDistance : s.dpiDistance / 2.54;

    // --- Measuring pad ---
    ImGui::Spacing();
    const ImVec2 padSize(ImGui::GetContentRegionAvail().x, 110.0f * s.scale);
    const ImVec2 p0 = ImGui::GetCursorScreenPos();
    const ImVec2 p1(p0.x + padSize.x, p0.y + padSize.y);
    ImGui::InvisibleButton("##dpipad", padSize);
    const bool hovered = ImGui::IsItemHovered();
    a.dpiArmed = hovered; // the raw left-button press decides when the pass really starts

    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImU32 accent = ImGui::ColorConvertFloat4ToU32(kAccent);
    const ImU32 border = a.dpiActive ? accent : hovered ? ImGui::GetColorU32(ImGuiCol_Text, 0.5f)
                                                         : ImGui::GetColorU32(ImGuiCol_Border);
    dl->AddRectFilled(p0, p1, a.dpiActive ? IM_COL32(20, 40, 55, 255) : IM_COL32(14, 15, 18, 255), 8.0f);
    dl->AddRect(p0, p1, border, 8.0f, 0, a.dpiActive ? 2.0f : 1.0f);
    char t[256];
    if (a.dpiActive) {
        const double counts = std::hypot(a.dpiSumX, a.dpiSumY);
        std::snprintf(t, sizeof(t), T(Str::Recording), counts, counts / inches);
    } else {
        std::snprintf(t, sizeof(t), "%s", T(hovered ? Str::PadHover : Str::PadIdle));
    }
    const ImVec2 ts = ImGui::CalcTextSize(t);
    dl->AddText(ImVec2((p0.x + p1.x - ts.x) * 0.5f, (p0.y + p1.y - ts.y) * 0.5f),
                a.dpiActive ? accent : ImGui::GetColorU32(ImGuiCol_TextDisabled), t);

    // --- Results ---
    const DpiSummary sum = a.dpiSummary(inches);
    const size_t n = sum.passes;
    const double mean = sum.mean, sd = sum.stddev, lo = sum.min, hi = sum.max;
    const double dev = n ? 100.0 * (mean - s.dpiConfigured) / s.dpiConfigured : 0.0;

    ImGui::Spacing();
    ImGui::PushFont(s.fontBig);
    if (n) ImGui::Text("%.0f DPI", mean);
    else ImGui::TextDisabled("--- DPI");
    ImGui::PopFont();
    ImGui::SameLine();
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + s.fontBig->FontSize * 0.35f);
    if (n) ImGui::TextDisabled(T(Str::AveragePasses), n);
    else ImGui::TextDisabled("%s", T(Str::NoPasses));

    if (ImGui::BeginTable("dstats", 4, ImGuiTableFlags_SizingStretchSame)) {
        StatF(T(Str::DevFromSet), "%+.1f %%", dev, n ? DeviationColor(dev) : nullptr);
        // Spread between passes: tells how repeatable the hand movement was.
        const double sdPct = n > 1 && mean > 0 ? 100.0 * sd / mean : 0.0;
        StatF(T(Str::Spread), "%.1f %%", sdPct, n > 1 ? (sdPct < 1 ? &kGood : sdPct < 3 ? &kWarn : &kBad) : nullptr);
        char buf[64];
        std::snprintf(buf, sizeof(buf), "%.0f / %.0f", lo, hi);
        Stat(T(Str::MinMax), buf);
        StatF(T(Str::Passes), "%.0f", double(n));
        ImGui::EndTable();
    }

    if (n && ImGui::BeginTable("passes", 6, ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchSame |
                                                  ImGuiTableFlags_ScrollY,
                               ImVec2(0, std::clamp(ImGui::GetContentRegionAvail().y - ImGui::GetFrameHeightWithSpacing() * 1.5f,
                                                    ImGui::GetTextLineHeightWithSpacing() * 4.0f,
                                                    ImGui::GetTextLineHeightWithSpacing() * 11.0f)))) {
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("#");
        ImGui::TableSetupColumn(T(Str::ColCountsX));
        ImGui::TableSetupColumn(T(Str::ColCountsY));
        ImGui::TableSetupColumn(T(Str::ColAngle));
        ImGui::TableSetupColumn("DPI");
        ImGui::TableSetupColumn(T(Str::ColDeviation));
        ImGui::TableHeadersRow();
        for (size_t i = n; i-- > 0;) { // newest first
            const auto& p = a.dpiPasses[i];
            const double d = a.passDpi(p, inches);
            const double dv = 100.0 * (d - s.dpiConfigured) / s.dpiConfigured;
            ImGui::TableNextRow();
            ImGui::TableNextColumn(); ImGui::Text("%zu", i + 1);
            ImGui::TableNextColumn(); ImGui::Text("%.0f", p.x);
            ImGui::TableNextColumn(); ImGui::Text("%.0f", p.y);
            // Angle of the stroke: a pass far from horizontal/vertical suggests a crooked movement.
            ImGui::TableNextColumn(); ImGui::Text("%.1f°", std::atan2(-p.y, p.x) * 180.0 / 3.14159265358979);
            ImGui::TableNextColumn(); ImGui::Text("%.0f", d);
            ImGui::TableNextColumn(); ImGui::TextColored(*DeviationColor(dv), "%+.1f %%", dv);
        }
        ImGui::EndTable();
    }

    ImGui::Spacing();
    if (ImGui::Button(L(Str::RemoveLast, "removelast")) && n) a.dpiPasses.pop_back();
    ImGui::SameLine();
    if (ImGui::Button(L(Str::ClearPasses, "clearpasses"))) a.dpiPasses.clear();
}

// ---------------------------------------------------------------- clicks & wheel tab
void DrawClickTab(Analyzer& a, UiState& s) {
    float thr = (float)a.chatterThresholdMs;
    ImGui::SetNextItemWidth(220.0f * s.scale);
    if (ImGui::SliderFloat(L(Str::ChatterThreshold, "chatter"), &thr, 5.0f, 100.0f, "%.0f ms")) a.chatterThresholdMs = thr;
    Hint(T(Str::HintChatter));
    ImGui::Spacing();

    if (ImGui::BeginTable("clicks", 6, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn(T(Str::ColButton));
        ImGui::TableSetupColumn(T(Str::ColState));
        ImGui::TableSetupColumn(T(Str::ColPresses));
        ImGui::TableSetupColumn(T(Str::ColReleasePress));
        ImGui::TableSetupColumn(T(Str::ColHold));
        ImGui::TableSetupColumn(T(Str::ColChatter));
        ImGui::TableHeadersRow();
        for (int i = 0; i < 5; ++i) {
            const ButtonInfo& b = a.buttons[i];
            ImGui::TableNextRow();
            ImGui::TableNextColumn(); ImGui::TextUnformatted(T(Str(int(Str::BtnLeft) + i)));
            ImGui::TableNextColumn();
            if (b.down) ImGui::TextColored(kAccent, "%s", T(Str::Pressed)); else ImGui::TextDisabled("%s", T(Str::Released));
            ImGui::TableNextColumn(); ImGui::Text("%llu", (unsigned long long)b.presses);
            ImGui::TableNextColumn();
            if (b.lastGapMs >= 0) ImGui::Text("%.1f ms", b.lastGapMs); else ImGui::TextDisabled("-");
            ImGui::TableNextColumn();
            if (b.lastHoldMs >= 0) ImGui::Text("%.1f ms", b.lastHoldMs); else ImGui::TextDisabled("-");
            ImGui::TableNextColumn();
            if (b.chatter) ImGui::TextColored(kBad, T(Str::ChatterLast), (unsigned long long)b.chatter, b.lastChatterMs);
            else ImGui::TextColored(kGood, "0");
        }
        ImGui::EndTable();
    }

    ImGui::Spacing();
    SectionTitle(T(Str::Wheel));
    if (ImGui::BeginTable("wheel", 4, ImGuiTableFlags_SizingStretchSame)) {
        StatF(T(Str::NotchesUp), "%.1f", a.wheelUp / 120.0);
        StatF(T(Str::NotchesDown), "%.1f", a.wheelDown / 120.0);
        char buf[32];
        std::snprintf(buf, sizeof(buf), "%llu", (unsigned long long)a.wheelReversals);
        Stat(T(Str::QuickReversals), buf, a.wheelReversals ? &kBad : &kGood);
        std::snprintf(buf, sizeof(buf), "%.0f / %.0f", a.tiltLeft / 120.0, a.tiltRight / 120.0);
        Stat(T(Str::Tilt), buf);
        ImGui::EndTable();
    }
    Hint(T(Str::HintWheel));

    ImGui::Spacing();
    SectionTitle(T(Str::ClickMove));
    if (ImGui::BeginTable("clickmove", 4, ImGuiTableFlags_SizingStretchSame)) {
        char buf[48];
        std::snprintf(buf, sizeof(buf), "%llu", (unsigned long long)a.clickMoveClicks);
        Stat(T(Str::ClickMoveMeasured), buf);
        std::snprintf(buf, sizeof(buf), "%llu", (unsigned long long)a.clickMoveMoved);
        Stat(T(Str::ClickMoveMoved), buf, a.clickMoveClicks ? (a.clickMoveMoved ? &kWarn : &kGood) : nullptr);
        if (a.clickMoveClicks)
            std::snprintf(buf, sizeof(buf), "%.1f / %.0f", a.clickMoveSum / double(a.clickMoveClicks), a.clickMoveMax);
        else
            std::snprintf(buf, sizeof(buf), "-");
        Stat(T(Str::ClickMoveAvgMax), buf);
        if (a.clickMoveLast >= 0) std::snprintf(buf, sizeof(buf), "%.0f", a.clickMoveLast);
        else std::snprintf(buf, sizeof(buf), "-");
        Stat(T(Str::ClickMoveLast), buf);
        ImGui::EndTable();
    }
    Hint(T(Str::HintClickMove));
    if (ImGui::Button(L(Str::ResetButtons, "resetbuttons"))) a.resetButtons();
}

} // namespace

void ApplyTheme(float scale) {
    ImGuiStyle& st = ImGui::GetStyle();
    st.WindowRounding = 0.0f;
    st.ChildRounding = 10.0f;
    st.FrameRounding = 6.0f;
    st.PopupRounding = 6.0f;
    st.GrabRounding = 6.0f;
    st.TabRounding = 6.0f;
    st.WindowPadding = ImVec2(16, 16);
    st.FramePadding = ImVec2(10, 6);
    st.ItemSpacing = ImVec2(10, 8);
    st.CellPadding = ImVec2(8, 6);
    st.WindowBorderSize = 0.0f;
    st.ChildBorderSize = 1.0f;
    st.ScaleAllSizes(scale);

    ImVec4* c = st.Colors;
    const ImVec4 bg(0.065f, 0.070f, 0.080f, 1.0f);
    const ImVec4 panel(0.090f, 0.095f, 0.110f, 1.0f);
    const ImVec4 frame(0.140f, 0.150f, 0.170f, 1.0f);
    const ImVec4 frameHi(0.180f, 0.190f, 0.220f, 1.0f);
    const ImVec4 accentDim(kAccent.x * 0.55f, kAccent.y * 0.55f, kAccent.z * 0.55f, 1.0f);

    c[ImGuiCol_Text] = ImVec4(0.92f, 0.93f, 0.95f, 1.0f);
    c[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.53f, 0.58f, 1.0f);
    c[ImGuiCol_WindowBg] = bg;
    c[ImGuiCol_ChildBg] = panel;
    c[ImGuiCol_PopupBg] = panel;
    c[ImGuiCol_Border] = ImVec4(0.17f, 0.18f, 0.21f, 1.0f);
    c[ImGuiCol_FrameBg] = frame;
    c[ImGuiCol_FrameBgHovered] = frameHi;
    c[ImGuiCol_FrameBgActive] = frameHi;
    c[ImGuiCol_Button] = frame;
    c[ImGuiCol_ButtonHovered] = accentDim;
    c[ImGuiCol_ButtonActive] = kAccent;
    c[ImGuiCol_Header] = frame;
    c[ImGuiCol_HeaderHovered] = frameHi;
    c[ImGuiCol_HeaderActive] = accentDim;
    c[ImGuiCol_Separator] = ImVec4(0.17f, 0.18f, 0.21f, 1.0f);
    c[ImGuiCol_SliderGrab] = kAccent;
    c[ImGuiCol_SliderGrabActive] = kAccent;
    c[ImGuiCol_CheckMark] = kAccent;
    c[ImGuiCol_Tab] = frame;
    c[ImGuiCol_TabHovered] = accentDim;
    c[ImGuiCol_TabSelected] = accentDim;
    c[ImGuiCol_TabSelectedOverline] = kAccent;
    c[ImGuiCol_PlotHistogram] = kAccent;
    c[ImGuiCol_PlotHistogramHovered] = ImVec4(1, 1, 1, 1);
    c[ImGuiCol_PlotLines] = kAccent;
    c[ImGuiCol_TableHeaderBg] = frame;
    c[ImGuiCol_TableRowBgAlt] = ImVec4(1, 1, 1, 0.025f);
    c[ImGuiCol_TableBorderLight] = ImVec4(0.17f, 0.18f, 0.21f, 1.0f);
}

void DrawUi(Analyzer& a, UiState& s, double deltaSeconds, uint64_t droppedPackets) {
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->WorkPos);
    ImGui::SetNextWindowSize(vp->WorkSize);
    ImGui::Begin("##main", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                     ImGuiWindowFlags_NoBringToFrontOnFocus);

    ImGui::BeginChild("left", ImVec2(340.0f * s.scale, 0), ImGuiChildFlags_Borders);
    DrawLeftPanel(a, s, deltaSeconds);
    ImGui::EndChild();

    ImGui::SameLine();
    ImGui::BeginChild("right", ImVec2(0, 0), ImGuiChildFlags_Borders);
    // Frame time in the top right corner: average and worst of the last frames, to spot stutters.
    if (s.frameMs.size() > 0) {
        double sum = 0, worst = 0;
        for (size_t i = 0; i < s.frameMs.size(); ++i) {
            sum += s.frameMs[i];
            worst = std::max(worst, double(s.frameMs[i]));
        }
        const double avg = sum / double(s.frameMs.size());
        char t[256];
        if (s.captureUsPerPacket >= 0)
            std::snprintf(t, sizeof(t), T(Str::CaptureStats), s.captureCpuPct, s.captureUsPerPacket, avg, worst);
        else
            std::snprintf(t, sizeof(t), T(Str::CaptureIdle), avg, worst);
        // Drawn on the tab bar row, which scrolls with the content. The layout cursor is not
        // moved: moving it to a fixed screen position used to break the scrollbar.
        const ImVec2 ts = ImGui::CalcTextSize(t);
        const ImVec2 row = ImGui::GetCursorScreenPos();
        const ImVec2 tp(row.x + ImGui::GetContentRegionAvail().x - ts.x, row.y);
        const ImU32 col = worst > avg * 2.5 + 4.0 ? ImGui::ColorConvertFloat4ToU32(kWarn) : ImGui::GetColorU32(ImGuiCol_TextDisabled);
        ImGui::GetWindowDrawList()->AddText(tp, col, t);
        if (ImGui::IsWindowHovered() && ImGui::IsMouseHoveringRect(tp, ImVec2(tp.x + ts.x, tp.y + ts.y)) &&
            ImGui::BeginTooltip()) {
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 26.0f);
            ImGui::TextUnformatted(T(Str::TipPerf));
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
    }
    a.dpiArmed = false; // re-armed by the DPI tab only while the cursor is over its pad
    if (droppedPackets)
        ImGui::TextColored(kBad, T(Str::Dropped), (unsigned long long)droppedPackets);
    if (ImGui::BeginTabBar("tabs")) {
        if (ImGui::BeginTabItem(L(Str::TabPolling, "tab-polling"))) { DrawPollingTab(a, s); ImGui::EndTabItem(); }
        if (ImGui::BeginTabItem(L(Str::TabSensor, "tab-sensor")))   { DrawSensorTab(a, s);  ImGui::EndTabItem(); }
        if (ImGui::BeginTabItem(L(Str::TabDpi, "tab-dpi")))         { DrawDpiTab(a, s);     ImGui::EndTabItem(); }
        if (ImGui::BeginTabItem(L(Str::TabClicks, "tab-clicks")))   { DrawClickTab(a, s);   ImGui::EndTabItem(); }
        ImGui::EndTabBar();
    }
    ImGui::EndChild();

    ImGui::End();
}
