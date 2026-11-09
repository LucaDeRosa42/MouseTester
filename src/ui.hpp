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

    // Vista del tracciato del sensore, in count del sensore.
    struct PathView {
        bool  autoFit = true;      // adatta la vista a ogni frame
        bool  recentOnly = false;  // adatta solo agli ultimi movimenti
        float centerX = 0, centerY = 0;
        float pxPerCount = 1.0f;
    } pathView;
    std::vector<ImVec2> pathPts;
};

void ApplyTheme(float scale);
void DrawUi(Analyzer& a, UiState& s, double deltaSeconds, uint64_t droppedPackets);
