// Mouse Tester - Copyright (C) 2026 lcrd42
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <array>
#include <cstdint>
#include <deque>
#include <unordered_map>
#include <vector>

#include "raw_input.hpp"

// Fixed-size circular buffer. Index 0 = oldest element.
template <typename T, size_t N>
class Ring {
public:
    void push(const T& v) {
        data_[head_] = v;
        head_ = (head_ + 1) % N;
        if (size_ < N) ++size_;
    }
    size_t size() const { return size_; }
    const T& operator[](size_t i) const { return data_[(head_ + N - size_ + i) % N]; }
    void clear() { head_ = size_ = 0; }

private:
    std::array<T, N> data_{};
    size_t head_ = 0, size_ = 0;
};

struct IntervalStats {
    size_t count = 0;
    double meanUs = 0, medianUs = 0, stddevUs = 0;
    double minUs = 0, maxUs = 0, p01Us = 0, p99Us = 0;
    double outlierPct = 0; // intervals > 1.5x the mean
    double batchedPct = 0; // intervals < 0.25x the mean (packets that arrived together)
};

struct ButtonInfo {
    bool     down = false;
    uint64_t presses = 0;
    uint64_t chatter = 0;          // unintended double clicks
    double   lastGapMs = -1;       // time from the previous release to the new press
    double   lastHoldMs = -1;      // duration of the last press
    double   lastChatterMs = -1;
    int64_t  lastDownQpc = 0, lastUpQpc = 0;
};

inline constexpr const char* kButtonNames[5] = {"Left", "Right", "Middle", "Side 4", "Side 5"};

// Nearest standard polling rate (within 15%), 0 if none.
inline int NearestStandardRate(double hz) {
    static const int rates[] = {125, 250, 500, 1000, 2000, 4000, 8000};
    for (int r : rates)
        if ((hz > r ? hz - r : r - hz) / r < 0.15) return r;
    return 0;
}

// Summary of the DPI passes for a given distance.
struct DpiSummary {
    size_t passes = 0;
    double mean = 0, stddev = 0, min = 0, max = 0; // DPI
};

// Best-fit line of the most recent stroke (continuous movement between two pauses),
// used to spot angle snapping: freehand strokes always wobble by a few counts.
struct StrokeFit {
    bool   valid = false;
    size_t points = 0;
    double angleDeg = 0;  // direction of the line, in (-90, 90], 0 = horizontal, positive = up-right
    double length = 0;    // extent along the line, in counts
    double rmsDev = 0;    // RMS distance of the points from the line, in counts
    double cx = 0, cy = 0;            // centre of the line (path coordinates)
    double x0 = 0, y0 = 0, x1 = 0, y1 = 0; // ends of the line (path coordinates)
};

struct PathPoint { float x, y; };

// Anomalous point of the path, to highlight it in the drawing.
enum class MarkKind : uint8_t { Skipped, Spike, Dip, Reversal };
struct SensorMark { float x, y; MarkKind kind; };

class Analyzer {
public:
    static constexpr size_t kIntervals = 16384;
    static constexpr size_t kPath = 20000;

    explicit Analyzer(int64_t qpcFrequency) : freq_(double(qpcFrequency)) {}

    void process(const MouseEvent& e);
    void refresh(int64_t nowQpc); // call once per frame

    void resetPolling();
    void resetSensor();
    void resetButtons();
    void resetAll() { resetPolling(); resetSensor(); resetButtons(); }


    void histogram(int bins, double maxUs, std::vector<float>& out) const;
    const Ring<float, kIntervals>& intervals() const { return intervals_; }
    const Ring<PathPoint, kPath>& path() const { return path_; }
    const Ring<SensorMark, 1024>& marks() const { return marks_; }
    StrokeFit strokeFit() const; // best-fit line of the most recent stroke

    // Minimum speed for every poll to contain movement: below it the sensor skips polls
    // (it has nothing to report) and the measured intervals are no longer the polling rate.
    static constexpr double kFastSpeed = 1.5; // counts per packet

    // Polling measurement state, for the UI.
    bool   moving() const { return lastMoveQpc_ && nowQpc_ - lastMoveQpc_ < int64_t(freq_ * 0.25); }
    bool   collecting() const { return lastFastQpc_ && nowQpc_ - lastFastQpc_ < int64_t(freq_ * 0.25); }
    double speedPerPacket() const { return speedEma_; } // counts per packet, recent average

    // --- Settings ---
    HANDLE deviceFilter = nullptr; // nullptr = all devices
    double chatterThresholdMs = 30.0;

    // --- Results (read only) ---
    std::unordered_map<HANDLE, uint64_t> devicePackets;
    IntervalStats stats;
    double   packetsLastSecond = 0;
    uint64_t movePackets = 0;

    // Sensor continuity, judged on the counts of each packet compared with its neighbours.
    uint64_t checkedPackets = 0;  // packets fast and steady enough to be checked
    uint64_t skippedEstimate = 0; // missing packets: a packet carries 2-4 times its neighbours' movement
    uint64_t spikes = 0;          // packet with much more movement than its neighbours (not an integer multiple)
    uint64_t dips = 0;            // packet with much less movement than its neighbours
    uint64_t reversals = 0;       // direction reversals within a single packet

    std::array<ButtonInfo, 5> buttons{};
    int64_t  wheelUp = 0, wheelDown = 0, tiltLeft = 0, tiltRight = 0;
    uint64_t wheelReversals = 0;
    int64_t  wheelLastQpc = 0;
    int      wheelLastDir = 0;

    // DPI measurement in passes. The UI arms it while the cursor is over the measuring pad;
    // a pass starts on the raw left-button press and ends on the raw release, so its
    // boundaries are exact to the packet.
    struct DpiPass { double x, y; };
    bool   dpiArmed = false;  // set by the UI every frame
    bool   dpiActive = false; // a pass is being recorded
    double dpiSumX = 0, dpiSumY = 0;
    std::vector<DpiPass> dpiPasses;
    DpiSummary dpiSummary(double inches) const;
    double passDpi(const DpiPass& p, double inches) const;

private:
    void processMotion(const MouseEvent& e);
    void processButtons(const MouseEvent& e);
    double toUs(int64_t ticks) const { return double(ticks) * 1e6 / freq_; }
    double toMs(int64_t ticks) const { return double(ticks) * 1e3 / freq_; }

    double freq_;
    int64_t lastMoveQpc_ = 0;
    int64_t lastFastQpc_ = 0; // last recorded interval (mouse fast enough)
    int64_t nowQpc_ = 0;      // time of the last refresh()
    double speedEma_ = 0;  // recent average speed in counts/packet
    int32_t lastDx_ = 0, lastDy_ = 0;
    float posX_ = 0, posY_ = 0;

    Ring<float, kIntervals> intervals_;
    Ring<PathPoint, kPath> path_;
    Ring<SensorMark, 1024> marks_;

    // Window of the last 5 packets of continuous movement: the middle packet
    // is compared with the 2 before and the 2 after it.
    struct Sample { float mag, x, y; };
    std::array<Sample, 5> window_{};
    size_t windowCount_ = 0;
    void checkContinuity();
    // Running sums of the current stroke, relative to its first point (keeps precision).
    struct StrokeSums {
        double n = 0, sx = 0, sy = 0, sxx = 0, syy = 0, sxy = 0;
        double ox = 0, oy = 0;         // first point
        double ex = 0, ey = 0;         // last point, relative to the first
    } stroke_;
    void strokeAdd(double x, double y);

    std::deque<int64_t> lastSecond_;
    std::vector<float> scratch_;
};
