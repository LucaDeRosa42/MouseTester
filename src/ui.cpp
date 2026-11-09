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

// ---------------------------------------------------------------- scale dei grafici
// Passo "tondo" (1, 2, 5 × 10^n) che divide `range` in circa `ticks` parti.
double NiceStep(double range, int ticks) {
    const double raw = range / ticks;
    const double p = std::pow(10.0, std::floor(std::log10(raw)));
    const double f = raw / p;
    return (f < 1.5 ? 1.0 : f < 3.5 ? 2.0 : f < 7.5 ? 5.0 : 10.0) * p;
}

// Area interna dell'ultimo grafico ImGui (senza il FramePadding).
struct PlotArea { ImVec2 min, max; };
PlotArea LastPlotArea() {
    const ImVec2 pad = ImGui::GetStyle().FramePadding;
    const ImVec2 a = ImGui::GetItemRectMin(), b = ImGui::GetItemRectMax();
    return {ImVec2(a.x + pad.x, a.y + pad.y), ImVec2(b.x - pad.x, b.y - pad.y)};
}

// Etichetta del valore di riferimento: "125 µs · 8000 Hz".
void RefLabel(char* buf, size_t size, double refUs) {
    std::snprintf(buf, size, "%.0f µs · %.0f Hz", refUs, 1e6 / refUs);
}

// Asse orizzontale in µs sotto l'ultimo grafico (istogramma), con griglia verticale
// e una linea evidenziata sull'intervallo di riferimento.
void XAxisMicros(double maxUs, double refUs) {
    const PlotArea r = LastPlotArea();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImU32 grid = ImGui::GetColorU32(ImGuiCol_TextDisabled, 0.30f);
    const ImU32 text = ImGui::GetColorU32(ImGuiCol_TextDisabled);
    const float w = r.max.x - r.min.x;

    ImGui::Dummy(ImVec2(w, ImGui::GetTextLineHeight())); // riga riservata ai numeri
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

// Asse verticale in µs dentro l'ultimo grafico (andamento nel tempo), con griglia
// orizzontale e una linea evidenziata sull'intervallo di riferimento.
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
    // Dalla media e non dalla mediana: vedi Analyzer::refresh().
    const double hz = st.meanUs > 0 ? 1e6 / st.meanUs : 0.0;
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
    const double maxUs = std::max(st.meanUs * 3.0, 400.0);
    // Riferimento: l'intervallo del rate standard riconosciuto, altrimenti la media.
    const double refUs = st.count > 100 ? (nominal ? 1e6 / nominal : st.meanUs) : 0.0;
    a.histogram(120, maxUs, s.histogram);
    const float plotH = 170.0f * s.scale;
    ImGui::PlotHistogram("##hist", s.histogram.data(), (int)s.histogram.size(), 0, nullptr, 0.0f, FLT_MAX, ImVec2(-FLT_MIN, plotH));
    XAxisMicros(maxUs, refUs);

    // Andamento nel tempo
    ImGui::Spacing();
    SectionTitle("Ultimi 2000 intervalli");
    const auto& iv = a.intervals();
    const size_t n = std::min<size_t>(iv.size(), 2000);
    s.timeline.resize(n);
    for (size_t i = 0; i < n; ++i) s.timeline[i] = iv[iv.size() - n + i];
    ImGui::PlotLines("##timeline", s.timeline.data(), (int)n, 0, nullptr, 0.0f, (float)maxUs, ImVec2(-FLT_MIN, plotH));
    YAxisMicros(maxUs, refUs);

    ImGui::Spacing();
    Hint("Gli intervalli \"raggruppati\" sono pacchetti consegnati insieme da Windows: indicano carico "
         "di sistema o del driver USB più che un difetto del mouse. Usa un cavo/ricevitore collegato "
         "direttamente al PC, non a un hub.");
    if (ImGui::Button("Azzera polling")) a.resetPolling();
}

// ---------------------------------------------------------------- scheda sensore
constexpr size_t kRecentPoints = 3000; // "ultimi movimenti": evidenziati e usati da "Solo ultimi"

const ImU32 kMarkColors[4] = {
    IM_COL32(255, 90, 90, 255),   // pacchetto mancante
    IM_COL32(255, 184, 77, 255),  // picco
    IM_COL32(200, 120, 255, 255), // calo
    IM_COL32(255, 90, 200, 255),  // inversione
};

// Tracciato del sensore con zoom (rotella), spostamento (trascina) e adattamento
// automatico (doppio click). Le coordinate sono in count del sensore.
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
    ImGui::SetItemKeyOwner(ImGuiKey_MouseWheelY); // la rotella fa zoom, non scorre la finestra
    const ImGuiIO& io = ImGui::GetIO();

    // --- Adattamento automatico ---
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

    // --- Zoom attorno al cursore, spostamento, reset ---
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

    // --- Sfondo e griglia in count ---
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
        // Linea a blocchi, per non superare il limite di vertici per singola chiamata.
        // Più spessa quando si zooma, per vedere bene ogni segmento.
        const float thick = std::clamp(1.0f + v.pxPerCount * 0.15f, 1.5f, 3.0f);
        constexpr size_t kChunk = 1500;
        s.pathPts.reserve(kChunk + 1);
        const size_t recent = n > kRecentPoints ? n - kRecentPoints : 0;
        for (size_t start = 0; start + 1 < n; start += kChunk) {
            s.pathPts.clear();
            const size_t end = std::min(n, start + kChunk + 1);
            for (size_t i = start; i < end; ++i) s.pathPts.push_back(toScreen(path[i].x, path[i].y));
            const ImU32 col = start + kChunk >= recent ? ImGui::ColorConvertFloat4ToU32(kAccent) : IM_COL32(60, 110, 140, 255);
            dl->AddPolyline(s.pathPts.data(), (int)s.pathPts.size(), col, 0, thick);
        }

        // Da vicino si vede ogni singolo pacchetto come un punto.
        if (v.pxPerCount >= 3.0f) {
            const ImU32 dot = IM_COL32(230, 240, 255, 255);
            const float rad = std::min(1.5f + v.pxPerCount * 0.08f, 3.5f);
            for (size_t i = 0; i < n; ++i) {
                const ImVec2 q = toScreen(path[i].x, path[i].y);
                if (q.x >= p0.x && q.x <= p1.x && q.y >= p0.y && q.y <= p1.y) dl->AddCircleFilled(q, rad, dot, 8);
            }
        }
    }

    // Anomalie del sensore evidenziate sul tracciato.
    const auto& marks = a.marks();
    for (size_t i = 0; i < marks.size(); ++i) {
        const SensorMark& m = marks[i];
        dl->AddCircle(toScreen(m.x, m.y), 6.0f, kMarkColors[int(m.kind)], 12, 2.0f);
    }

    // Scala della griglia in basso a destra.
    char t[48];
    std::snprintf(t, sizeof(t), "griglia %.0f count", step);
    const ImVec2 ts = ImGui::CalcTextSize(t);
    dl->AddText(ImVec2(p1.x - ts.x - 8.0f, p1.y - ts.y - 6.0f), ImGui::GetColorU32(ImGuiCol_TextDisabled), t);
    dl->PopClipRect();
}

void DrawSensorTab(Analyzer& a, UiState& s) {
    if (ImGui::BeginTable("sstats", 5, ImGuiTableFlags_SizingStretchSame)) {
        // Colore in base alla frequenza: le anomalie contano rispetto ai pacchetti valutati.
        auto color = [&](uint64_t v) {
            const double perMille = a.checkedPackets ? 1000.0 * double(v) / double(a.checkedPackets) : 0.0;
            return v == 0 ? &kGood : perMille < 1.0 ? &kWarn : &kBad;
        };
        char buf[32];
        std::snprintf(buf, sizeof(buf), "%llu", (unsigned long long)a.checkedPackets);
        Stat("Pacchetti valutati", buf);
        std::snprintf(buf, sizeof(buf), "%llu", (unsigned long long)a.skippedEstimate);
        Stat("Mancanti", buf, color(a.skippedEstimate));
        std::snprintf(buf, sizeof(buf), "%llu", (unsigned long long)a.spikes);
        Stat("Picchi", buf, color(a.spikes));
        std::snprintf(buf, sizeof(buf), "%llu", (unsigned long long)a.dips);
        Stat("Cali", buf, color(a.dips));
        std::snprintf(buf, sizeof(buf), "%llu", (unsigned long long)a.reversals);
        Stat("Inversioni", buf, color(a.reversals));
        ImGui::EndTable();
    }
    ImGui::Spacing();
    SectionTitle("Tracciato");

    // Comandi della vista e legenda delle anomalie
    UiState::PathView& v = s.pathView;
    if (ImGui::Button("Adatta")) v.autoFit = true;
    ImGui::SameLine();
    if (ImGui::Checkbox("Solo ultimi movimenti", &v.recentOnly)) v.autoFit = true;
    ImGui::SameLine();
    ImGui::TextDisabled("  zoom %.2f px/count%s", v.pxPerCount, v.autoFit ? " (auto)" : "");
    const char* markNames[4] = {"mancante", "picco", "calo", "inversione"};
    for (int i = 0; i < 4; ++i) {
        ImGui::SameLine(0, i == 0 ? 24.0f * s.scale : -1.0f);
        ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(kMarkColors[i]), "o %s", markNames[i]);
    }

    const float h = std::max(200.0f, ImGui::GetContentRegionAvail().y - ImGui::GetTextLineHeightWithSpacing() * 5.5f);
    DrawPath(a, s, ImVec2(ImGui::GetContentRegionAvail().x, h));
    Hint("Rotella sul tracciato: zoom. Trascina: sposta. Doppio click: adatta. Da vicino ogni punto è un pacchetto.");
    Hint("Continuità: muovi il mouse veloce e in modo regolare. Ogni pacchetto viene confrontato con i 4 vicini: "
         "se porta 2-4 volte il loro movimento mancano dei pacchetti, se ne porta molto di più o molto di meno è "
         "un picco o un calo. Con un sensore sano i contatori restano a zero. Angle snapping: traccia diagonali a "
         "mano libera, se diventano perfettamente dritte lo snapping è attivo. Il tracciato usa i count grezzi del "
         "sensore, senza accelerazione di Windows.");
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
