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

    float dpiDistanceCm = 10.0f;
    int   dpiConfigured = 1600;

    std::vector<float> histogram;
    std::vector<float> timeline;

    // Sensor path view, in sensor counts.
    struct PathView {
        bool  autoFit = true;      // fit the view every frame
        bool  recentOnly = false;  // fit only the recent movements
        float centerX = 0, centerY = 0;
        float pxPerCount = 1.0f;
    } pathView;
    std::vector<ImVec2> pathPts;

    Ring<float, 240> frameMs; // duration of the last frames, for the stutter indicator
};

void ApplyTheme(float scale);
void DrawUi(Analyzer& a, UiState& s, double deltaSeconds, uint64_t droppedPackets);
