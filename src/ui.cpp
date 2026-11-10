#include "ui.hpp"

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstdio>

namespace {

const ImVec4 kAccent(0.30f, 0.76f, 1.00f, 1.00f);
const ImVec4 kGood(0.35f, 0.85f, 0.55f, 1.00f);
const ImVec4 kWarn(1.00f, 0.72f, 0.30f, 1.00f);
const ImVec4 kBad(1.00f, 0.40f, 0.40f, 1.00f);

const char* kButtonNames[5] = {"Left", "Right", "Middle", "Side 4", "Side 5"};

// Nearest standard polling rate (within 15%), 0 if none.
int NearestStandardRate(double hz) {
    static const int rates[] = {125, 250, 500, 1000, 2000, 4000, 8000};
    for (int r : rates)
        if (std::fabs(hz - r) / r < 0.15) return r;
    return 0;
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

// "Small label + value" cell in a statistics table.
void Stat(const char* label, const char* value, const ImVec4* color = nullptr) {
    ImGui::TableNextColumn();
    ImGui::TextDisabled("%s", label);
    if (color) ImGui::TextColored(*color, "%s", value);
    else ImGui::TextUnformatted(value);
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
    ImGui::Spacing();

    // Device selection
    for (auto& [h, count] : a.devicePackets)
        if (!s.deviceNames.count(h)) s.deviceNames[h] = RawInputCapture::deviceName(h);

    const char* current = a.deviceFilter ? s.deviceNames[a.deviceFilter].c_str() : "All devices";
    ImGui::SetNextItemWidth(-FLT_MIN);
    if (ImGui::BeginCombo("##device", current)) {
        if (ImGui::Selectable("All devices", a.deviceFilter == nullptr)) {
            a.deviceFilter = nullptr;
            a.resetAll();
        }
        for (auto& [h, count] : a.devicePackets) {
            char label[320];
            std::snprintf(label, sizeof(label), "%s  -  %llu packets##%p", s.deviceNames[h].c_str(), (unsigned long long)count, h);
            if (ImGui::Selectable(label, a.deviceFilter == h)) {
                a.deviceFilter = h;
                a.resetAll();
            }
        }
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

    // Mouse drawing
    const ImVec2 area(ImGui::GetContentRegionAvail().x, 330.0f * s.scale);
    const ImVec2 pos = ImGui::GetCursorScreenPos();
    DrawMouse(ImGui::GetWindowDrawList(), pos, area, s.visual, ImGui::ColorConvertFloat4ToU32(kAccent));
    ImGui::Dummy(area);

    // Button summary
    if (ImGui::BeginTable("buttons", 3, ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn("Button");
        ImGui::TableSetupColumn("Presses");
        ImGui::TableSetupColumn("Chatter");
        ImGui::TableHeadersRow();
        for (int i = 0; i < 5; ++i) {
            const ButtonInfo& b = a.buttons[i];
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            if (b.down) ImGui::TextColored(kAccent, "%s", kButtonNames[i]);
            else ImGui::TextUnformatted(kButtonNames[i]);
            ImGui::TableNextColumn();
            ImGui::Text("%llu", (unsigned long long)b.presses);
            ImGui::TableNextColumn();
            if (b.chatter) ImGui::TextColored(kBad, "%llu", (unsigned long long)b.chatter);
            else ImGui::TextDisabled("0");
        }
        ImGui::EndTable();
    }

    ImGui::Spacing();
    ImGui::Text("Wheel:  up %.0f   down %.0f notches", a.wheelUp / 120.0, a.wheelDown / 120.0);

    // Button at the bottom of the panel
    const float btnH = ImGui::GetFrameHeight();
    ImGui::SetCursorPosY(std::max(ImGui::GetCursorPosY(), ImGui::GetWindowHeight() - btnH - ImGui::GetStyle().WindowPadding.y));
    if (ImGui::Button("Reset all", ImVec2(-FLT_MIN, 0))) a.resetAll();
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
    if (nominal && st.count > 100) ImGui::TextDisabled("nominal %d Hz", nominal);
    else ImGui::TextDisabled("move the mouse quickly in circles");

    // Measuring state: current speed compared with the required minimum.
    const double v = a.speedPerPacket();
    if (live)
        ImGui::TextColored(kGood, "Measuring · speed %.1f counts per packet", v);
    else if (a.moving())
        ImGui::TextColored(kWarn, "Paused: too slow (%.1f counts per packet, at least %.1f needed). "
                                  "Move faster or raise the DPI.", v, Analyzer::kFastSpeed);
    else
        ImGui::TextDisabled("Paused: mouse not moving");

    ImGui::Spacing();
    if (ImGui::BeginTable("pstats", 4, ImGuiTableFlags_SizingStretchSame)) {
        StatF("Mean", "%.1f µs", st.meanUs);
        StatF("Median", "%.1f µs", st.medianUs);
        const double jitterPct = st.meanUs > 0 ? 100.0 * st.stddevUs / st.meanUs : 0.0;
        const ImVec4* jc = jitterPct < 10 ? &kGood : jitterPct < 25 ? &kWarn : &kBad;
        StatF("Jitter (std. dev.)", "%.1f µs", st.stddevUs, st.count > 100 ? jc : nullptr);
        StatF("Packets / s", "%.0f", a.packetsLastSecond);

        char buf[64];
        std::snprintf(buf, sizeof(buf), "%.0f / %.0f µs", st.minUs, st.maxUs);
        Stat("Min / Max", buf);
        std::snprintf(buf, sizeof(buf), "%.0f / %.0f µs", st.p01Us, st.p99Us);
        Stat("1st / 99th percentile", buf);
        const ImVec4* oc = st.outlierPct < 1 ? &kGood : st.outlierPct < 5 ? &kWarn : &kBad;
        StatF("Outliers (>1.5x)", "%.2f %%", st.outlierPct, st.count > 100 ? oc : nullptr);
        StatF("Batched (<0.25x)", "%.2f %%", st.batchedPct);
        ImGui::EndTable();
    }

    // Interval histogram
    ImGui::Spacing();
    SectionTitle("Interval distribution");
    const double maxUs = std::max(st.meanUs * 3.0, 400.0);
    // Reference: the interval of the detected standard rate, otherwise the mean.
    const double refUs = st.count > 100 ? (nominal ? 1e6 / nominal : st.meanUs) : 0.0;
    a.histogram(120, maxUs, s.histogram);
    const float plotH = 170.0f * s.scale;
    ImGui::PlotHistogram("##hist", s.histogram.data(), (int)s.histogram.size(), 0, nullptr, 0.0f, FLT_MAX, ImVec2(-FLT_MIN, plotH));
    XAxisMicros(maxUs, refUs);

    // Over time
    ImGui::Spacing();
    SectionTitle("Last 2000 intervals");
    const auto& iv = a.intervals();
    const size_t n = std::min<size_t>(iv.size(), 2000);
    s.timeline.resize(n);
    for (size_t i = 0; i < n; ++i) s.timeline[i] = iv[iv.size() - n + i];
    ImGui::PlotLines("##timeline", s.timeline.data(), (int)n, 0, nullptr, 0.0f, (float)maxUs, ImVec2(-FLT_MIN, plotH));
    YAxisMicros(maxUs, refUs);

    ImGui::Spacing();
    Hint("Only movements fast enough to fill every poll are measured: at least 1.5 counts per packet. "
         "At 8000 Hz and 800 DPI that means about 38 cm/s; at 3200 DPI about 10 cm/s is enough.");
    Hint("\"Batched\" intervals are packets delivered together by Windows: they point to system or USB "
         "driver load rather than a mouse defect. Plug the cable/receiver directly into the PC, not into a hub.");
    if (ImGui::Button("Reset polling")) a.resetPolling();
}

// ---------------------------------------------------------------- sensor tab
constexpr size_t kRecentPoints = 3000; // "recent movements": highlighted and used by "Recent only"

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

    // Sensor anomalies highlighted on the path.
    const auto& marks = a.marks();
    for (size_t i = 0; i < marks.size(); ++i) {
        const SensorMark& m = marks[i];
        dl->AddCircle(toScreen(m.x, m.y), 6.0f, kMarkColors[int(m.kind)], 12, 2.0f);
    }

    // Grid scale in the bottom right corner.
    char t[48];
    std::snprintf(t, sizeof(t), "grid %.0f counts", step);
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
        Stat("Packets checked", buf);
        std::snprintf(buf, sizeof(buf), "%llu", (unsigned long long)a.skippedEstimate);
        Stat("Missing", buf, color(a.skippedEstimate));
        std::snprintf(buf, sizeof(buf), "%llu", (unsigned long long)a.spikes);
        Stat("Spikes", buf, color(a.spikes));
        std::snprintf(buf, sizeof(buf), "%llu", (unsigned long long)a.dips);
        Stat("Dips", buf, color(a.dips));
        std::snprintf(buf, sizeof(buf), "%llu", (unsigned long long)a.reversals);
        Stat("Reversals", buf, color(a.reversals));
        ImGui::EndTable();
    }
    ImGui::Spacing();
    SectionTitle("Path");

    // View controls and anomaly legend
    UiState::PathView& v = s.pathView;
    if (ImGui::Button("Fit")) v.autoFit = true;
    ImGui::SameLine();
    if (ImGui::Checkbox("Recent movements only", &v.recentOnly)) v.autoFit = true;
    ImGui::SameLine();
    ImGui::TextDisabled("  zoom %.2f px/count%s", v.pxPerCount, v.autoFit ? " (auto)" : "");
    const char* markNames[4] = {"missing", "spike", "dip", "reversal"};
    for (int i = 0; i < 4; ++i) {
        ImGui::SameLine(0, i == 0 ? 24.0f * s.scale : -1.0f);
        ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(kMarkColors[i]), "o %s", markNames[i]);
    }

    const float h = std::max(200.0f, ImGui::GetContentRegionAvail().y - ImGui::GetTextLineHeightWithSpacing() * 5.5f);
    DrawPath(a, s, ImVec2(ImGui::GetContentRegionAvail().x, h));
    Hint("Wheel over the path: zoom. Drag: pan. Double click: fit. Up close, every dot is one packet.");
    Hint("Continuity: move the mouse fast and steadily. Every packet is compared with its 4 neighbours: if it "
         "carries 2-4 times their movement, packets are missing; if it carries much more or much less, it is a "
         "spike or a dip. With a healthy sensor the counters stay at zero. Angle snapping: draw diagonals "
         "freehand; if they come out perfectly straight, snapping is on. The path uses raw sensor counts, "
         "without Windows acceleration.");
    if (ImGui::Button("Reset sensor")) a.resetSensor();
}

// ---------------------------------------------------------------- DPI tab
// Colour for a deviation from the DPI set on the mouse.
const ImVec4* DeviationColor(double pct) {
    return std::fabs(pct) < 2 ? &kGood : std::fabs(pct) < 5 ? &kWarn : &kBad;
}

void DrawDpiTab(Analyzer& a, UiState& s) {
    Hint("1. Lay a ruler (or the edge of the mousepad) and enter its length.  2. Put the mouse at the start "
         "mark and the cursor over the pad below.  3. Hold the left button, slide the mouse in a straight line "
         "to the end mark, release.  Repeat 5-10 times: every release adds a pass.");
    ImGui::Spacing();

    // --- Settings ---
    ImGui::SetNextItemWidth(160.0f * s.scale);
    ImGui::InputFloat("##distance", &s.dpiDistance, 1.0f, 5.0f, "%.2f");
    s.dpiDistance = std::max(s.dpiDistance, 0.5f);
    ImGui::SameLine();
    int unit = s.dpiInches ? 1 : 0;
    ImGui::SetNextItemWidth(70.0f * s.scale);
    if (ImGui::Combo("Distance##unit", &unit, "cm\0in\0")) s.dpiInches = unit == 1;
    ImGui::SameLine(0, 30.0f * s.scale);
    ImGui::SetNextItemWidth(160.0f * s.scale);
    ImGui::InputInt("DPI set on the mouse", &s.dpiConfigured, 100, 400);
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
    char t[96];
    if (a.dpiActive) {
        const double counts = std::hypot(a.dpiSumX, a.dpiSumY);
        std::snprintf(t, sizeof(t), "Recording...  %.0f counts  (%.0f DPI so far)", counts, counts / inches);
    } else {
        std::snprintf(t, sizeof(t), hovered ? "Hold the left button and slide along the ruler"
                                            : "Move the cursor here to measure");
    }
    const ImVec2 ts = ImGui::CalcTextSize(t);
    dl->AddText(ImVec2((p0.x + p1.x - ts.x) * 0.5f, (p0.y + p1.y - ts.y) * 0.5f),
                a.dpiActive ? accent : ImGui::GetColorU32(ImGuiCol_TextDisabled), t);

    // --- Results ---
    const size_t n = a.dpiPasses.size();
    double mean = 0, sd = 0, lo = 0, hi = 0;
    if (n > 0) {
        lo = DBL_MAX; hi = -DBL_MAX;
        for (const auto& p : a.dpiPasses) {
            const double d = std::hypot(p.x, p.y) / inches;
            mean += d; lo = std::min(lo, d); hi = std::max(hi, d);
        }
        mean /= double(n);
        for (const auto& p : a.dpiPasses) {
            const double d = std::hypot(p.x, p.y) / inches - mean;
            sd += d * d;
        }
        sd = n > 1 ? std::sqrt(sd / double(n - 1)) : 0.0;
    }
    const double dev = n ? 100.0 * (mean - s.dpiConfigured) / s.dpiConfigured : 0.0;

    ImGui::Spacing();
    ImGui::PushFont(s.fontBig);
    if (n) ImGui::Text("%.0f DPI", mean);
    else ImGui::TextDisabled("--- DPI");
    ImGui::PopFont();
    ImGui::SameLine();
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + s.fontBig->FontSize * 0.35f);
    if (n) ImGui::TextDisabled("average of %zu pass%s", n, n == 1 ? "" : "es");
    else ImGui::TextDisabled("no passes yet");

    if (ImGui::BeginTable("dstats", 4, ImGuiTableFlags_SizingStretchSame)) {
        StatF("Deviation from set DPI", "%+.1f %%", dev, n ? DeviationColor(dev) : nullptr);
        // Spread between passes: tells how repeatable the hand movement was.
        const double sdPct = n > 1 && mean > 0 ? 100.0 * sd / mean : 0.0;
        StatF("Spread (std. dev.)", "%.1f %%", sdPct, n > 1 ? (sdPct < 1 ? &kGood : sdPct < 3 ? &kWarn : &kBad) : nullptr);
        char buf[64];
        std::snprintf(buf, sizeof(buf), "%.0f / %.0f", lo, hi);
        Stat("Min / Max", buf);
        StatF("Passes", "%.0f", double(n));
        ImGui::EndTable();
    }

    if (n && ImGui::BeginTable("passes", 6, ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchSame |
                                                  ImGuiTableFlags_ScrollY,
                               ImVec2(0, std::clamp(ImGui::GetContentRegionAvail().y - ImGui::GetFrameHeightWithSpacing() * 1.5f,
                                                    ImGui::GetTextLineHeightWithSpacing() * 4.0f,
                                                    ImGui::GetTextLineHeightWithSpacing() * 11.0f)))) {
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("#");
        ImGui::TableSetupColumn("Counts X");
        ImGui::TableSetupColumn("Counts Y");
        ImGui::TableSetupColumn("Angle");
        ImGui::TableSetupColumn("DPI");
        ImGui::TableSetupColumn("Deviation");
        ImGui::TableHeadersRow();
        for (size_t i = n; i-- > 0;) { // newest first
            const auto& p = a.dpiPasses[i];
            const double d = std::hypot(p.x, p.y) / inches;
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
    if (ImGui::Button("Remove last pass") && n) a.dpiPasses.pop_back();
    ImGui::SameLine();
    if (ImGui::Button("Clear passes")) a.dpiPasses.clear();
}

// ---------------------------------------------------------------- clicks & wheel tab
void DrawClickTab(Analyzer& a, UiState& s) {
    float thr = (float)a.chatterThresholdMs;
    ImGui::SetNextItemWidth(220.0f * s.scale);
    if (ImGui::SliderFloat("Chatter threshold", &thr, 5.0f, 100.0f, "%.0f ms")) a.chatterThresholdMs = thr;
    Hint("A new press arriving within the threshold after the previous release is counted as an "
         "unintended double click (worn switch or debounce set too low).");
    ImGui::Spacing();

    if (ImGui::BeginTable("clicks", 6, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn("Button");
        ImGui::TableSetupColumn("State");
        ImGui::TableSetupColumn("Presses");
        ImGui::TableSetupColumn("Release → press");
        ImGui::TableSetupColumn("Hold time");
        ImGui::TableSetupColumn("Chatter");
        ImGui::TableHeadersRow();
        for (int i = 0; i < 5; ++i) {
            const ButtonInfo& b = a.buttons[i];
            ImGui::TableNextRow();
            ImGui::TableNextColumn(); ImGui::TextUnformatted(kButtonNames[i]);
            ImGui::TableNextColumn();
            if (b.down) ImGui::TextColored(kAccent, "PRESSED"); else ImGui::TextDisabled("released");
            ImGui::TableNextColumn(); ImGui::Text("%llu", (unsigned long long)b.presses);
            ImGui::TableNextColumn();
            if (b.lastGapMs >= 0) ImGui::Text("%.1f ms", b.lastGapMs); else ImGui::TextDisabled("-");
            ImGui::TableNextColumn();
            if (b.lastHoldMs >= 0) ImGui::Text("%.1f ms", b.lastHoldMs); else ImGui::TextDisabled("-");
            ImGui::TableNextColumn();
            if (b.chatter) ImGui::TextColored(kBad, "%llu (last %.1f ms)", (unsigned long long)b.chatter, b.lastChatterMs);
            else ImGui::TextColored(kGood, "0");
        }
        ImGui::EndTable();
    }

    ImGui::Spacing();
    SectionTitle("Wheel");
    if (ImGui::BeginTable("wheel", 4, ImGuiTableFlags_SizingStretchSame)) {
        StatF("Notches up", "%.1f", a.wheelUp / 120.0);
        StatF("Notches down", "%.1f", a.wheelDown / 120.0);
        char buf[32];
        std::snprintf(buf, sizeof(buf), "%llu", (unsigned long long)a.wheelReversals);
        Stat("Quick reversals (<40 ms)", buf, a.wheelReversals ? &kBad : &kGood);
        std::snprintf(buf, sizeof(buf), "%.0f / %.0f", a.tiltLeft / 120.0, a.tiltRight / 120.0);
        Stat("Tilt left / right", buf);
        ImGui::EndTable();
    }
    Hint("Scroll the wheel slowly in one direction: every notch should count as 1 and reversals should "
         "stay at zero. Decimal values mean a high-resolution wheel.");
    if (ImGui::Button("Reset buttons and wheel")) a.resetButtons();
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
        char t[64];
        std::snprintf(t, sizeof(t), "frame %.1f ms · worst %.1f ms", avg, worst);
        const ImVec2 ts = ImGui::CalcTextSize(t);
        const ImVec2 wp = ImGui::GetWindowPos();
        const ImVec2 pad = ImGui::GetStyle().WindowPadding;
        const ImU32 col = worst > avg * 2.5 + 4.0 ? ImGui::ColorConvertFloat4ToU32(kWarn) : ImGui::GetColorU32(ImGuiCol_TextDisabled);
        ImGui::GetWindowDrawList()->AddText(ImVec2(wp.x + ImGui::GetWindowWidth() - ts.x - pad.x, wp.y + pad.y), col, t);
    }
    a.dpiArmed = false; // re-armed by the DPI tab only while the cursor is over its pad
    if (droppedPackets)
        ImGui::TextColored(kBad, "Warning: %llu packets lost by the internal queue.", (unsigned long long)droppedPackets);
    if (ImGui::BeginTabBar("tabs")) {
        if (ImGui::BeginTabItem("Polling rate"))    { DrawPollingTab(a, s); ImGui::EndTabItem(); }
        if (ImGui::BeginTabItem("Sensor"))          { DrawSensorTab(a, s);  ImGui::EndTabItem(); }
        if (ImGui::BeginTabItem("DPI"))             { DrawDpiTab(a, s);     ImGui::EndTabItem(); }
        if (ImGui::BeginTabItem("Clicks & wheel"))  { DrawClickTab(a, s);   ImGui::EndTabItem(); }
        ImGui::EndTabBar();
    }
    ImGui::EndChild();

    ImGui::End();
}
