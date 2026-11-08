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
};

void ApplyTheme(float scale);
void DrawUi(Analyzer& a, UiState& s, double deltaSeconds, uint64_t droppedPackets);
