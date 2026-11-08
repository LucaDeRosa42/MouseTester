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

const char* kButtonNames[5] = {"Sinistro", "Destro", "Centrale", "Laterale 4", "Laterale 5"};

// Polling rate standard più vicino (entro il 15%), 0 se nessuno.
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

// Cella "etichetta piccola + valore" in una tabella di statistiche.
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

// ---------------------------------------------------------------- pannello sinistro
void DrawLeftPanel(Analyzer& a, UiState& s, double dt) {
    ImGui::PushFont(s.fontBig);
    ImGui::TextColored(kAccent, "MOUSE TESTER");
    ImGui::PopFont();
    ImGui::Spacing();

    // Selezione del dispositivo
    for (auto& [h, count] : a.devicePackets)
        if (!s.deviceNames.count(h)) s.deviceNames[h] = RawInputCapture::deviceName(h);

    const char* current = a.deviceFilter ? s.deviceNames[a.deviceFilter].c_str() : "Tutti i dispositivi";
    ImGui::SetNextItemWidth(-FLT_MIN);
    if (ImGui::BeginCombo("##device", current)) {
        if (ImGui::Selectable("Tutti i dispositivi", a.deviceFilter == nullptr)) {
            a.deviceFilter = nullptr;
            a.resetAll();
        }
        for (auto& [h, count] : a.devicePackets) {
            char label[320];
            std::snprintf(label, sizeof(label), "%s  -  %llu pacchetti##%p", s.deviceNames[h].c_str(), (unsigned long long)count, h);
            if (ImGui::Selectable(label, a.deviceFilter == h)) {
                a.deviceFilter = h;
                a.resetAll();
            }
        }
        ImGui::EndCombo();
    }

    // Illuminazione: piena mentre è premuto, poi sfuma in ~200 ms
    const float decay = float(dt) * 5.0f;
    for (int i = 0; i < 5; ++i)
        s.visual.button[i] = a.buttons[i].down ? 1.0f : std::max(0.0f, s.visual.button[i] - decay);
    s.visual.wheelUp = std::max(0.0f, s.visual.wheelUp - decay);
    s.visual.wheelDown = std::max(0.0f, s.visual.wheelDown - decay);
    if (a.wheelLastQpc != s.seenWheelQpc) {
        s.seenWheelQpc = a.wheelLastQpc;
        (a.wheelLastDir > 0 ? s.visual.wheelUp : s.visual.wheelDown) = 1.0f;
    }

    // Disegno del mouse
    const ImVec2 area(ImGui::GetContentRegionAvail().x, 330.0f * s.scale);
    const ImVec2 pos = ImGui::GetCursorScreenPos();
    DrawMouse(ImGui::GetWindowDrawList(), pos, area, s.visual, ImGui::ColorConvertFloat4ToU32(kAccent));
    ImGui::Dummy(area);

    // Riepilogo tasti
    if (ImGui::BeginTable("buttons", 3, ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn("Tasto");
        ImGui::TableSetupColumn("Pressioni");
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
    ImGui::Text("Rotella:  su %.0f   giù %.0f scatti", a.wheelUp / 120.0, a.wheelDown / 120.0);

    // Pulsante in fondo al pannello
    const float btnH = ImGui::GetFrameHeight();
    ImGui::SetCursorPosY(std::max(ImGui::GetCursorPosY(), ImGui::GetWindowHeight() - btnH - ImGui::GetStyle().WindowPadding.y));
    if (ImGui::Button("Azzera tutto", ImVec2(-FLT_MIN, 0))) a.resetAll();
}

// ---------------------------------------------------------------- scheda polling
void DrawPollingTab(Analyzer& a, UiState& s) {
    const IntervalStats& st = a.stats;
    const double hz = st.medianUs > 0 ? 1e6 / st.medianUs : 0.0;
    const int nominal = NearestStandardRate(hz);

    ImGui::PushFont(s.fontBig);
    if (st.count > 100) ImGui::Text("%.0f Hz", hz);
    else ImGui::TextDisabled("--- Hz");
    ImGui::PopFont();
    ImGui::SameLine();
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + s.fontBig->FontSize * 0.35f);
    if (nominal) ImGui::TextDisabled("nominale %d Hz", nominal);
    else ImGui::TextDisabled("muovi il mouse velocemente in cerchio");

    ImGui::Spacing();
    if (ImGui::BeginTable("pstats", 4, ImGuiTableFlags_SizingStretchSame)) {
        StatF("Media", "%.1f µs", st.meanUs);
        StatF("Mediana", "%.1f µs", st.medianUs);
        const double jitterPct = st.meanUs > 0 ? 100.0 * st.stddevUs / st.meanUs : 0.0;
        const ImVec4* jc = jitterPct < 10 ? &kGood : jitterPct < 25 ? &kWarn : &kBad;
        StatF("Jitter (dev. std.)", "%.1f µs", st.stddevUs, st.count > 100 ? jc : nullptr);
        StatF("Pacchetti / s", "%.0f", a.packetsLastSecond);

        char buf[64];
        std::snprintf(buf, sizeof(buf), "%.0f / %.0f µs", st.minUs, st.maxUs);
        Stat("Min / Max", buf);
        std::snprintf(buf, sizeof(buf), "%.0f / %.0f µs", st.p01Us, st.p99Us);
        Stat("1° / 99° percentile", buf);
        const ImVec4* oc = st.outlierPct < 1 ? &kGood : st.outlierPct < 5 ? &kWarn : &kBad;
        StatF("Fuori norma (>1.5x)", "%.2f %%", st.outlierPct, st.count > 100 ? oc : nullptr);
        StatF("Raggruppati (<0.25x)", "%.2f %%", st.batchedPct);
        ImGui::EndTable();
    }

    // Istogramma degli intervalli
    ImGui::Spacing();
    SectionTitle("Distribuzione degli intervalli");
    const double maxUs = std::max(st.medianUs * 3.0, 400.0);
    a.histogram(120, maxUs, s.histogram);
    const float plotH = 170.0f * s.scale;
    ImGui::PlotHistogram("##hist", s.histogram.data(), (int)s.histogram.size(), 0, nullptr, 0.0f, FLT_MAX, ImVec2(-FLT_MIN, plotH));
    ImGui::TextDisabled("0 µs");
    char right[32];
    std::snprintf(right, sizeof(right), "%.0f µs", maxUs);
    ImGui::SameLine(ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(right).x);
    ImGui::TextDisabled("%s", right);

    // Andamento nel tempo
    ImGui::Spacing();
    SectionTitle("Ultimi 2000 intervalli");
    const auto& iv = a.intervals();
    const size_t n = std::min<size_t>(iv.size(), 2000);
    s.timeline.resize(n);
    for (size_t i = 0; i < n; ++i) s.timeline[i] = iv[iv.size() - n + i];
    ImGui::PlotLines("##timeline", s.timeline.data(), (int)n, 0, nullptr, 0.0f, (float)maxUs, ImVec2(-FLT_MIN, plotH));

    ImGui::Spacing();
    Hint("Gli intervalli \"raggruppati\" sono pacchetti consegnati insieme da Windows: indicano carico "
         "di sistema o del driver USB più che un difetto del mouse. Usa un cavo/ricevitore collegato "
         "direttamente al PC, non a un hub.");
    if (ImGui::Button("Azzera polling")) a.resetPolling();
}

// ---------------------------------------------------------------- scheda sensore
void DrawPath(const Analyzer& a, ImVec2 size) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 p0 = ImGui::GetCursorScreenPos();
    const ImVec2 p1(p0.x + size.x, p0.y + size.y);
    dl->AddRectFilled(p0, p1, IM_COL32(14, 15, 18, 255), 8.0f);
    const ImVec2 c((p0.x + p1.x) * 0.5f, (p0.y + p1.y) * 0.5f);
    dl->AddLine(ImVec2(p0.x, c.y), ImVec2(p1.x, c.y), IM_COL32(40, 43, 50, 255));
    dl->AddLine(ImVec2(c.x, p0.y), ImVec2(c.x, p1.y), IM_COL32(40, 43, 50, 255));
    ImGui::Dummy(size);

    const auto& path = a.path();
    const size_t n = path.size();
    if (n < 2) return;

    float minX = FLT_MAX, minY = FLT_MAX, maxX = -FLT_MAX, maxY = -FLT_MAX;
    for (size_t i = 0; i < n; ++i) {
        minX = std::min(minX, path[i].x); maxX = std::max(maxX, path[i].x);
        minY = std::min(minY, path[i].y); maxY = std::max(maxY, path[i].y);
    }
    const float margin = 16.0f;
    const float sc = std::min((size.x - 2 * margin) / std::max(maxX - minX, 1.0f),
                              (size.y - 2 * margin) / std::max(maxY - minY, 1.0f));
    const float ox = c.x - (minX + maxX) * 0.5f * sc;
    const float oy = c.y - (minY + maxY) * 0.5f * sc;

    dl->PushClipRect(p0, p1, true);
    // A blocchi, per non superare il limite di vertici per singola chiamata
    constexpr size_t kChunk = 1500;
    std::vector<ImVec2> pts;
    pts.reserve(kChunk + 1);
    const size_t recent = n > 3000 ? n - 3000 : 0;
    for (size_t start = 0; start + 1 < n; start += kChunk) {
        pts.clear();
        const size_t end = std::min(n, start + kChunk + 1);
        for (size_t i = start; i < end; ++i) pts.push_back(ImVec2(ox + path[i].x * sc, oy + path[i].y * sc));
        const ImU32 col = start >= recent ? ImGui::ColorConvertFloat4ToU32(kAccent) : IM_COL32(60, 110, 140, 255);
        dl->AddPolyline(pts.data(), (int)pts.size(), col, 0, 1.5f);
    }
    dl->PopClipRect();
}

void DrawSensorTab(Analyzer& a, UiState& s) {
    if (ImGui::BeginTable("sstats", 3, ImGuiTableFlags_SizingStretchSame)) {
        auto color = [](uint64_t v) { return v == 0 ? &kGood : v < 10 ? &kWarn : &kBad; };
        char buf[32];
        std::snprintf(buf, sizeof(buf), "%llu", (unsigned long long)a.skippedEstimate);
        Stat("Pacchetti mancanti (stima)", buf, color(a.skippedEstimate));
        std::snprintf(buf, sizeof(buf), "%llu", (unsigned long long)a.spikes);
        Stat("Picchi anomali", buf, color(a.spikes));
        std::snprintf(buf, sizeof(buf), "%llu", (unsigned long long)a.reversals);
        Stat("Inversioni improvvise", buf, color(a.reversals));
        ImGui::EndTable();
    }
    ImGui::Spacing();
    SectionTitle("Tracciato");
    const float h = std::max(200.0f, ImGui::GetContentRegionAvail().y - ImGui::GetTextLineHeightWithSpacing() * 4.5f);
    DrawPath(a, ImVec2(ImGui::GetContentRegionAvail().x, h));
    Hint("Continuità: muovi il mouse veloce e in modo regolare; con un sensore sano i contatori restano a zero. "
         "Angle snapping: traccia linee diagonali a mano libera, se diventano perfettamente dritte "
         "lo snapping è attivo. Il tracciato usa i count grezzi del sensore, senza accelerazione di Windows.");
    if (ImGui::Button("Azzera sensore")) a.resetSensor();
}

// ---------------------------------------------------------------- scheda DPI
void DrawDpiTab(Analyzer& a, UiState& s) {
    Hint("1. Appoggia un righello sul tappetino.  2. Premi Inizia.  3. Sposta il mouse in linea retta "
         "della distanza indicata.  4. Premi Ferma. Ripeti più volte e fai la media.");
    ImGui::Spacing();

    ImGui::SetNextItemWidth(160.0f * s.scale);
    ImGui::InputFloat("Distanza percorsa (cm)", &s.dpiDistanceCm, 1.0f, 5.0f, "%.1f");
    s.dpiDistanceCm = std::max(s.dpiDistanceCm, 0.5f);
    ImGui::SetNextItemWidth(160.0f * s.scale);
    ImGui::InputInt("DPI impostato nel mouse", &s.dpiConfigured, 100, 400);
    s.dpiConfigured = std::max(s.dpiConfigured, 1);

    ImGui::Spacing();
    if (!a.dpiActive) {
        if (ImGui::Button("Inizia misura", ImVec2(180.0f * s.scale, 0))) a.dpiStart();
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.55f, 0.20f, 0.20f, 1.0f));
        if (ImGui::Button("Ferma misura", ImVec2(180.0f * s.scale, 0))) a.dpiStop();
        ImGui::PopStyleColor();
        ImGui::SameLine();
        ImGui::TextColored(kAccent, "registrazione in corso...");
    }

    const double counts = std::hypot(a.dpiSumX, a.dpiSumY);
    const double dpi = counts / (s.dpiDistanceCm / 2.54);
    ImGui::Spacing();
    ImGui::PushFont(s.fontBig);
    if (counts > 0) ImGui::Text("%.0f DPI", dpi);
    else ImGui::TextDisabled("--- DPI");
    ImGui::PopFont();

    if (ImGui::BeginTable("dstats", 3, ImGuiTableFlags_SizingStretchSame)) {
        StatF("Count X", "%.0f", a.dpiSumX);
        StatF("Count Y", "%.0f", a.dpiSumY);
        const double dev = counts > 0 ? 100.0 * (dpi - s.dpiConfigured) / s.dpiConfigured : 0.0;
        const ImVec4* c = std::fabs(dev) < 2 ? &kGood : std::fabs(dev) < 5 ? &kWarn : &kBad;
        StatF("Scostamento dal DPI impostato", "%+.1f %%", dev, counts > 0 ? c : nullptr);
        ImGui::EndTable();
    }
}

// ---------------------------------------------------------------- scheda click e rotella
void DrawClickTab(Analyzer& a, UiState& s) {
    float thr = (float)a.chatterThresholdMs;
    ImGui::SetNextItemWidth(220.0f * s.scale);
    if (ImGui::SliderFloat("Soglia chatter", &thr, 5.0f, 100.0f, "%.0f ms")) a.chatterThresholdMs = thr;
    Hint("Una nuova pressione che arriva entro la soglia dal rilascio precedente viene contata come "
         "doppio click involontario (switch usurato o debounce troppo basso).");
    ImGui::Spacing();

    if (ImGui::BeginTable("clicks", 6, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn("Tasto");
        ImGui::TableSetupColumn("Stato");
        ImGui::TableSetupColumn("Pressioni");
        ImGui::TableSetupColumn("Rilascio → pressione");
        ImGui::TableSetupColumn("Durata pressione");
        ImGui::TableSetupColumn("Chatter");
        ImGui::TableHeadersRow();
        for (int i = 0; i < 5; ++i) {
            const ButtonInfo& b = a.buttons[i];
            ImGui::TableNextRow();
            ImGui::TableNextColumn(); ImGui::TextUnformatted(kButtonNames[i]);
            ImGui::TableNextColumn();
            if (b.down) ImGui::TextColored(kAccent, "PREMUTO"); else ImGui::TextDisabled("rilasciato");
            ImGui::TableNextColumn(); ImGui::Text("%llu", (unsigned long long)b.presses);
            ImGui::TableNextColumn();
            if (b.lastGapMs >= 0) ImGui::Text("%.1f ms", b.lastGapMs); else ImGui::TextDisabled("-");
            ImGui::TableNextColumn();
            if (b.lastHoldMs >= 0) ImGui::Text("%.1f ms", b.lastHoldMs); else ImGui::TextDisabled("-");
            ImGui::TableNextColumn();
            if (b.chatter) ImGui::TextColored(kBad, "%llu (ultimo %.1f ms)", (unsigned long long)b.chatter, b.lastChatterMs);
            else ImGui::TextColored(kGood, "0");
        }
        ImGui::EndTable();
    }

    ImGui::Spacing();
    SectionTitle("Rotella");
    if (ImGui::BeginTable("wheel", 4, ImGuiTableFlags_SizingStretchSame)) {
        StatF("Scatti su", "%.1f", a.wheelUp / 120.0);
        StatF("Scatti giù", "%.1f", a.wheelDown / 120.0);
        char buf[32];
        std::snprintf(buf, sizeof(buf), "%llu", (unsigned long long)a.wheelReversals);
        Stat("Inversioni rapide (<40 ms)", buf, a.wheelReversals ? &kBad : &kGood);
        std::snprintf(buf, sizeof(buf), "%.0f / %.0f", a.tiltLeft / 120.0, a.tiltRight / 120.0);
        Stat("Tilt sx / dx", buf);
        ImGui::EndTable();
    }
    Hint("Gira la rotella lentamente in una sola direzione: ogni scatto deve contare 1 e le inversioni "
         "devono restare a zero. Valori decimali indicano una rotella ad alta risoluzione.");
    if (ImGui::Button("Azzera tasti e rotella")) a.resetButtons();
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
    if (droppedPackets)
        ImGui::TextColored(kBad, "Attenzione: %llu pacchetti persi dalla coda interna.", (unsigned long long)droppedPackets);
    if (ImGui::BeginTabBar("tabs")) {
        if (ImGui::BeginTabItem("Polling rate")) { DrawPollingTab(a, s); ImGui::EndTabItem(); }
        if (ImGui::BeginTabItem("Sensore"))      { DrawSensorTab(a, s);  ImGui::EndTabItem(); }
        if (ImGui::BeginTabItem("DPI"))          { DrawDpiTab(a, s);     ImGui::EndTabItem(); }
        if (ImGui::BeginTabItem("Click e rotella")) { DrawClickTab(a, s); ImGui::EndTabItem(); }
        ImGui::EndTabBar();
    }
    ImGui::EndChild();

    ImGui::End();
}
