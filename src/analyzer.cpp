// Mouse Tester - Copyright (C) 2026 lcrd42
// SPDX-License-Identifier: GPL-3.0-or-later
#include "analyzer.hpp"

#include <algorithm>
#include <cmath>

namespace {
constexpr double kMotionGapUs = 50'000.0; // beyond 50 ms we consider the mouse stopped
}

void Analyzer::process(const MouseEvent& e) {
    ++devicePackets[e.device];
    if (deviceFilter && e.device != deviceFilter) return;

    if (e.dx || e.dy) processMotion(e);
    if (e.buttonFlags) processButtons(e);
}

void Analyzer::processMotion(const MouseEvent& e) {
    ++movePackets;
    lastSecond_.push_back(e.qpc);

    const bool continuous = lastMoveQpc_ && toUs(e.qpc - lastMoveQpc_) < kMotionGapUs;

    // --- Polling: interval since the previous motion packet ---
    // Only intervals with a fast mouse count: when the mouse is slow the sensor
    // skips polls (no movement to report) and the intervals get longer.
    if (continuous && speedEma_ >= kFastSpeed) {
        intervals_.push(float(toUs(e.qpc - lastMoveQpc_)));
        lastFastQpc_ = e.qpc;
    }
    lastMoveQpc_ = e.qpc;

    // --- Path and DPI ---
    if (!continuous) { // a pause starts a new stroke, beginning where the mouse was
        stroke_ = StrokeSums{};
        strokeAdd(posX_, posY_);
    }
    posX_ += float(e.dx);
    posY_ += float(e.dy);
    path_.push({posX_, posY_});
    strokeAdd(posX_, posY_);
    if (dpiActive) {
        dpiSumX += e.dx;
        dpiSumY += e.dy;
    }

    // --- Sensor continuity ---
    // We don't use arrival times (Windows delivers packets in batches) but counts:
    // every packet is one poll of the mouse, so at constant speed it carries the same
    // movement as its neighbours, regardless of when Windows delivers it.
    const double mag = std::hypot(double(e.dx), double(e.dy));
    if (!continuous) {
        windowCount_ = 0;
        lastDx_ = lastDy_ = 0;
    }

    // Reversal: two consecutive fast packets in opposite directions. At 1000 Hz and above
    // the mouse cannot stop and move backwards between one poll and the next.
    const double lastMag = std::hypot(double(lastDx_), double(lastDy_));
    const double dot = double(e.dx) * lastDx_ + double(e.dy) * lastDy_;
    const double minMag = std::max(3.0, 0.5 * speedEma_);
    if (dot < 0 && mag >= minMag && lastMag >= minMag) {
        ++reversals;
        marks_.push({posX_, posY_, MarkKind::Reversal});
    }

    speedEma_ = speedEma_ * 0.95 + mag * 0.05;
    lastDx_ = e.dx;
    lastDy_ = e.dy;

    if (windowCount_ == window_.size()) std::move(window_.begin() + 1, window_.end(), window_.begin());
    else ++windowCount_;
    window_[windowCount_ - 1] = {float(mag), posX_, posY_};
    if (windowCount_ == window_.size()) checkContinuity();
}

void Analyzer::checkContinuity() {
    constexpr double kMinCounts = 3.0;   // at 8000 Hz a fast hand gives only ~4-5 counts per packet
    constexpr double kMinDiff = 2.0;     // and the anomaly must differ by 2+ counts, so ±1 rounding is not flagged
    constexpr double kSteady = 1.6;      // the neighbours must be consistent with each other
    constexpr double kSpike = 1.6;       // ratio above which the packet is anomalous
    constexpr double kDip = 0.45;        // ratio below which the packet is anomalous
    constexpr double kMultipleTol = 0.25;

    const Sample& c = window_[2];
    const float nb[4] = {window_[0].mag, window_[1].mag, window_[3].mag, window_[4].mag};
    const float lo = *std::min_element(nb, nb + 4);
    const float hi = *std::max_element(nb, nb + 4);
    const double avg = (nb[0] + nb[1] + nb[2] + nb[3]) / 4.0;
    if (avg < kMinCounts || lo <= 0 || hi / lo > kSteady) return; // movement too slow or irregular

    ++checkedPackets;
    const double r = c.mag / avg;
    const double k = std::round(r);
    if (k >= 2 && k <= 4 && std::fabs(r - k) <= kMultipleTol) {
        // The packet carries the movement of k polls: the previous k-1 never arrived.
        skippedEstimate += uint64_t(k) - 1;
        marks_.push({c.x, c.y, MarkKind::Skipped});
    } else if (r >= kSpike && c.mag - avg >= kMinDiff) {
        ++spikes;
        marks_.push({c.x, c.y, MarkKind::Spike});
    } else if (r <= kDip && avg - c.mag >= kMinDiff) {
        ++dips;
        marks_.push({c.x, c.y, MarkKind::Dip});
    }
}

void Analyzer::processButtons(const MouseEvent& e) {
    const uint16_t f = e.buttonFlags;

    // DPI pass: left button down starts it (only over the pad), left button up ends it.
    if ((f & RI_MOUSE_LEFT_BUTTON_DOWN) && dpiArmed && !dpiActive) {
        dpiActive = true;
        dpiSumX = dpiSumY = 0;
    }
    if ((f & RI_MOUSE_LEFT_BUTTON_UP) && dpiActive) {
        dpiActive = false;
        if (dpiSumX != 0 || dpiSumY != 0) dpiPasses.push_back({dpiSumX, dpiSumY});
    }

    // RI_MOUSE_BUTTON_n_DOWN = 1 << (2n), RI_MOUSE_BUTTON_n_UP = 1 << (2n+1)
    for (int i = 0; i < 5; ++i) {
        ButtonInfo& b = buttons[i];
        if (f & (1u << (2 * i))) {
            if (b.lastUpQpc) {
                b.lastGapMs = toMs(e.qpc - b.lastUpQpc);
                if (b.lastGapMs < chatterThresholdMs) {
                    ++b.chatter;
                    b.lastChatterMs = b.lastGapMs;
                }
            }
            b.down = true;
            ++b.presses;
            b.lastDownQpc = e.qpc;
        }
        if (f & (1u << (2 * i + 1))) {
            b.down = false;
            b.lastUpQpc = e.qpc;
            if (b.lastDownQpc) b.lastHoldMs = toMs(e.qpc - b.lastDownQpc);
        }
    }

    if (f & RI_MOUSE_WHEEL) {
        const int dir = e.wheel > 0 ? 1 : -1;
        if (e.wheel > 0) wheelUp += e.wheel; else wheelDown -= e.wheel;
        // Notch in the opposite direction within 40 ms: typical of a dirty encoder.
        if (wheelLastDir && dir != wheelLastDir && toMs(e.qpc - wheelLastQpc) < 40.0) ++wheelReversals;
        wheelLastDir = dir;
        wheelLastQpc = e.qpc;
    }
    if (f & RI_MOUSE_HWHEEL) {
        if (e.hwheel > 0) tiltRight += e.hwheel; else tiltLeft -= e.hwheel;
    }
}

void Analyzer::refresh(int64_t nowQpc) {
    nowQpc_ = nowQpc;
    const int64_t oneSecond = int64_t(freq_);
    while (!lastSecond_.empty() && nowQpc - lastSecond_.front() > oneSecond) lastSecond_.pop_front();
    packetsLastSecond = double(lastSecond_.size());

    const size_t n = intervals_.size();
    stats = IntervalStats{};
    stats.count = n;
    if (n < 2) return;

    scratch_.resize(n);
    double sum = 0;
    for (size_t i = 0; i < n; ++i) {
        scratch_[i] = intervals_[i];
        sum += scratch_[i];
    }
    stats.meanUs = sum / double(n);

    double var = 0;
    for (float v : scratch_) var += (v - stats.meanUs) * (v - stats.meanUs);
    stats.stddevUs = std::sqrt(var / double(n));

    std::sort(scratch_.begin(), scratch_.end());
    auto pct = [&](double p) { return double(scratch_[size_t(p * double(n - 1))]); };
    stats.minUs = scratch_.front();
    stats.maxUs = scratch_.back();
    stats.medianUs = pct(0.5);
    stats.p01Us = pct(0.01);
    stats.p99Us = pct(0.99);

    // Reference = mean, not median: at 8000 Hz Windows delivers packets in batches,
    // the distribution is skewed and the median falls below the real interval.
    // The sum of the intervals is the elapsed time, so 1/mean is the true rate.
    size_t outliers = 0, batched = 0;
    for (float v : scratch_) {
        if (v > 1.5 * stats.meanUs) ++outliers;
        if (v < 0.25 * stats.meanUs) ++batched;
    }
    stats.outlierPct = 100.0 * double(outliers) / double(n);
    stats.batchedPct = 100.0 * double(batched) / double(n);
}

void Analyzer::strokeAdd(double x, double y) {
    StrokeSums& s = stroke_;
    if (s.n == 0) { s.ox = x; s.oy = y; }
    const double rx = x - s.ox, ry = y - s.oy;
    s.n += 1; s.sx += rx; s.sy += ry;
    s.sxx += rx * rx; s.syy += ry * ry; s.sxy += rx * ry;
    s.ex = rx; s.ey = ry;
}

StrokeFit Analyzer::strokeFit() const {
    constexpr double kMinLength = 100.0; // counts: shorter strokes say nothing about snapping
    const StrokeSums& s = stroke_;
    StrokeFit f;
    f.points = size_t(s.n);
    if (s.n < 3) return f;

    // Principal axis of the points (orthogonal regression, works for vertical lines too).
    const double mx = s.sx / s.n, my = s.sy / s.n;
    const double cxx = s.sxx / s.n - mx * mx, cyy = s.syy / s.n - my * my, cxy = s.sxy / s.n - mx * my;
    const double theta = 0.5 * std::atan2(2.0 * cxy, cxx - cyy);
    const double ux = std::cos(theta), uy = std::sin(theta);
    const double half = 0.5 * (cxx - cyy);
    const double minorVar = 0.5 * (cxx + cyy) - std::sqrt(half * half + cxy * cxy);
    f.rmsDev = std::sqrt(std::max(0.0, minorVar));

    // Ends: first and last point projected on the line.
    const double t0 = (0 - mx) * ux + (0 - my) * uy;
    const double t1 = (s.ex - mx) * ux + (s.ey - my) * uy;
    f.length = std::fabs(t1 - t0);
    f.cx = s.ox + mx;                f.cy = s.oy + my;
    f.x0 = f.cx + t0 * ux;           f.y0 = f.cy + t0 * uy;
    f.x1 = f.cx + t1 * ux;           f.y1 = f.cy + t1 * uy;

    // Angle with y pointing up, folded into (-90, 90].
    double deg = std::atan2(-uy, ux) * 180.0 / 3.14159265358979;
    if (deg > 90.0) deg -= 180.0;
    if (deg <= -90.0) deg += 180.0;
    f.angleDeg = deg;
    f.valid = f.length >= kMinLength;
    return f;
}

double Analyzer::passDpi(const DpiPass& p, double inches) const {
    return std::hypot(p.x, p.y) / inches;
}

DpiSummary Analyzer::dpiSummary(double inches) const {
    DpiSummary s;
    s.passes = dpiPasses.size();
    if (!s.passes) return s;
    s.min = 1e300; s.max = -1e300;
    for (const auto& p : dpiPasses) {
        const double d = passDpi(p, inches);
        s.mean += d; s.min = std::min(s.min, d); s.max = std::max(s.max, d);
    }
    s.mean /= double(s.passes);
    double var = 0;
    for (const auto& p : dpiPasses) var += (passDpi(p, inches) - s.mean) * (passDpi(p, inches) - s.mean);
    s.stddev = s.passes > 1 ? std::sqrt(var / double(s.passes - 1)) : 0.0;
    return s;
}

void Analyzer::histogram(int bins, double maxUs, std::vector<float>& out) const {
    out.assign(bins, 0.0f);
    if (maxUs <= 0) return;
    for (size_t i = 0; i < intervals_.size(); ++i) {
        int b = int(intervals_[i] / maxUs * bins);
        out[std::clamp(b, 0, bins - 1)] += 1.0f;
    }
}

void Analyzer::resetPolling() {
    intervals_.clear();
    lastSecond_.clear();
    lastMoveQpc_ = lastFastQpc_ = 0;
    movePackets = 0;
    stats = IntervalStats{};
}

void Analyzer::resetSensor() {
    path_.clear();
    marks_.clear();
    stroke_ = StrokeSums{};
    windowCount_ = 0;
    posX_ = posY_ = 0;
    checkedPackets = skippedEstimate = spikes = dips = reversals = 0;
    speedEma_ = 0;
    lastDx_ = lastDy_ = 0;
}

void Analyzer::resetButtons() {
    for (auto& b : buttons) {
        const bool down = b.down;
        b = ButtonInfo{};
        b.down = down;
    }
    wheelUp = wheelDown = tiltLeft = tiltRight = 0;
    wheelReversals = 0;
    wheelLastDir = 0;
}
