#include "export.hpp"

#include <commdlg.h>

#include <cmath>
#include <cstdio>
#include <ctime>

namespace {

// UTF-8 file opened from a wide path (the path may contain non-ASCII characters).
FILE* OpenUtf8(const std::wstring& path) {
    FILE* f = nullptr;
    if (_wfopen_s(&f, path.c_str(), L"wb") != 0) return nullptr;
    return f;
}

} // namespace

std::wstring AskSavePath(const wchar_t* defaultName, const wchar_t* filter, const wchar_t* defaultExt) {
    wchar_t buf[MAX_PATH] = {};
    wcsncpy_s(buf, defaultName, _TRUNCATE);
    OPENFILENAMEW ofn{sizeof(ofn)};
    ofn.hwndOwner = static_cast<HWND>(ImGui::GetMainViewport()->PlatformHandleRaw);
    ofn.lpstrFilter = filter;
    ofn.lpstrFile = buf;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrDefExt = defaultExt;
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    return GetSaveFileNameW(&ofn) ? std::wstring(buf) : std::wstring();
}

bool ExportReport(const Analyzer& a, const UiState& s, const std::wstring& path) {
    FILE* f = OpenUtf8(path);
    if (!f) return false;

    char date[64];
    const std::time_t now = std::time(nullptr);
    std::tm tm{};
    localtime_s(&tm, &now);
    std::strftime(date, sizeof(date), "%Y-%m-%d %H:%M", &tm);

    std::string device = "All devices";
    if (a.deviceFilter) {
        const auto it = s.deviceNames.find(a.deviceFilter);
        if (it != s.deviceNames.end()) device = it->second;
    }

    std::fprintf(f, "Mouse Tester report\n");
    std::fprintf(f, "Date:   %s\n", date);
    std::fprintf(f, "Device: %s\n\n", device.c_str());

    // --- Polling ---
    const IntervalStats& st = a.stats;
    const double hz = st.meanUs > 0 ? 1e6 / st.meanUs : 0.0;
    std::fprintf(f, "[Polling rate]\n");
    if (st.count > 100) {
        const int nominal = NearestStandardRate(hz);
        std::fprintf(f, "Rate:                  %.0f Hz", hz);
        if (nominal) std::fprintf(f, " (nominal %d Hz)", nominal);
        std::fprintf(f, "\nIntervals measured:    %zu\n", st.count);
        std::fprintf(f, "Mean / median:         %.1f / %.1f µs\n", st.meanUs, st.medianUs);
        std::fprintf(f, "Jitter (std. dev.):    %.1f µs (%.1f %%)\n", st.stddevUs, 100.0 * st.stddevUs / st.meanUs);
        std::fprintf(f, "Min / max:             %.0f / %.0f µs\n", st.minUs, st.maxUs);
        std::fprintf(f, "1st / 99th percentile: %.0f / %.0f µs\n", st.p01Us, st.p99Us);
        std::fprintf(f, "Outliers (>1.5x):      %.2f %%\n", st.outlierPct);
        std::fprintf(f, "Batched (<0.25x):      %.2f %%\n\n", st.batchedPct);
    } else {
        std::fprintf(f, "Not measured (move the mouse quickly in circles on the Polling rate tab).\n\n");
    }

    // --- Sensor ---
    std::fprintf(f, "[Sensor]\n");
    std::fprintf(f, "Packets checked: %llu\n", (unsigned long long)a.checkedPackets);
    std::fprintf(f, "Missing:         %llu\n", (unsigned long long)a.skippedEstimate);
    std::fprintf(f, "Spikes:          %llu\n", (unsigned long long)a.spikes);
    std::fprintf(f, "Dips:            %llu\n", (unsigned long long)a.dips);
    std::fprintf(f, "Reversals:       %llu\n\n", (unsigned long long)a.reversals);

    // --- DPI ---
    const double inches = s.dpiInches ? s.dpiDistance : s.dpiDistance / 2.54;
    const DpiSummary d = a.dpiSummary(inches);
    std::fprintf(f, "[DPI]\n");
    std::fprintf(f, "Set on the mouse: %d\n", s.dpiConfigured);
    std::fprintf(f, "Distance:         %.2f %s\n", s.dpiDistance, s.dpiInches ? "in" : "cm");
    if (d.passes) {
        std::fprintf(f, "Average:          %.0f DPI (%+.1f %% from the set value)\n", d.mean,
                     100.0 * (d.mean - s.dpiConfigured) / s.dpiConfigured);
        std::fprintf(f, "Spread:           %.1f %%\n", d.mean > 0 ? 100.0 * d.stddev / d.mean : 0.0);
        std::fprintf(f, "Min / max:        %.0f / %.0f DPI\n", d.min, d.max);
        std::fprintf(f, "Passes:           %zu\n", d.passes);
        for (size_t i = 0; i < a.dpiPasses.size(); ++i) {
            const auto& p = a.dpiPasses[i];
            std::fprintf(f, "  %2zu. X %7.0f  Y %7.0f  %6.0f DPI\n", i + 1, p.x, p.y, a.passDpi(p, inches));
        }
    } else {
        std::fprintf(f, "No passes recorded.\n");
    }
    std::fprintf(f, "\n");

    // --- Buttons and wheel ---
    std::fprintf(f, "[Buttons] (chatter threshold %.0f ms)\n", a.chatterThresholdMs);
    std::fprintf(f, "%-8s %8s %8s\n", "Button", "Presses", "Chatter");
    for (int i = 0; i < 5; ++i)
        std::fprintf(f, "%-8s %8llu %8llu\n", kButtonNames[i], (unsigned long long)a.buttons[i].presses,
                     (unsigned long long)a.buttons[i].chatter);
    std::fprintf(f, "\n[Wheel]\n");
    std::fprintf(f, "Notches up / down:        %.1f / %.1f\n", a.wheelUp / 120.0, a.wheelDown / 120.0);
    std::fprintf(f, "Quick reversals (<40 ms): %llu\n", (unsigned long long)a.wheelReversals);
    std::fprintf(f, "Tilt left / right:        %.0f / %.0f\n", a.tiltLeft / 120.0, a.tiltRight / 120.0);

    const bool ok = std::ferror(f) == 0;
    std::fclose(f);
    return ok;
}

bool ExportIntervalsCsv(const Analyzer& a, const std::wstring& path) {
    FILE* f = OpenUtf8(path);
    if (!f) return false;
    std::fprintf(f, "index,interval_us\n");
    const auto& iv = a.intervals();
    for (size_t i = 0; i < iv.size(); ++i) std::fprintf(f, "%zu,%.2f\n", i, iv[i]);
    const bool ok = std::ferror(f) == 0;
    std::fclose(f);
    return ok;
}
