#pragma once
#include <string>
#include <unordered_map>
#include <vector>

#include "analyzer.hpp"
#include "imgui.h"
#include "mouse_view.hpp"

struct UiState {
    float scale = 1.0f;
    ImFont* fontBig = nullptr;

    MouseVisual visual;
    int64_t seenWheelQpc = 0;
    std::unordered_map<HANDLE, std::string> deviceNames;

    float dpiDistance = 10.0f;  // in cm or inches, depending on dpiInches
    bool  dpiInches = false;
    int   dpiConfigured = 1600;

    std::vector<float> histogram;
    std::vector<float> timeline;

    // Sensor path view, in sensor counts.
    struct PathView {
        bool  autoFit = true;      // fit the view every frame
        bool  recentOnly = false;  // fit only the recent movements
        bool  guides = true;       // 0/45/90/135° guides and best-fit line of the last stroke
        float centerX = 0, centerY = 0;
        float pxPerCount = 1.0f;
    } pathView;
    std::vector<ImVec2> pathPts;

    int         exportStatus = -1; // Str id of the last export outcome, -1 = none (shown in the left panel)
    bool        exportOk = false;

    double captureCpuPct = 0;        // capture thread, % of one core (updated by main every 0.5 s)
    double captureUsPerPacket = -1;  // CPU time per WM_INPUT, -1 when no packets arrived

    Ring<float, 240> frameMs; // duration of the last frames, for the stutter indicator
};

void ApplyTheme(float scale);
void DrawUi(Analyzer& a, UiState& s, double deltaSeconds, uint64_t droppedPackets);
