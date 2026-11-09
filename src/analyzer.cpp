#include "analyzer.hpp"

#include <algorithm>
#include <cmath>

namespace {
constexpr double kMotionGapUs = 50'000.0; // oltre 50 ms consideriamo il mouse fermo
constexpr double kFastSpeed = 2.0;        // count/pacchetto oltre cui il sensore deve riportare a ogni poll
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

    // --- Polling: intervallo dal pacchetto di movimento precedente ---
    // Contiamo solo gli intervalli a mouse veloce: se il mouse è lento il sensore
    // salta dei poll (nessun movimento da riportare) e gli intervalli si allungano.
    if (continuous && speedEma_ >= kFastSpeed) intervals_.push(float(toUs(e.qpc - lastMoveQpc_)));
    lastMoveQpc_ = e.qpc;

    // --- Tracciato e DPI ---
    posX_ += float(e.dx);
    posY_ += float(e.dy);
    path_.push({posX_, posY_});
    if (dpiActive) {
        dpiSumX += e.dx;
        dpiSumY += e.dy;
    }

    // --- Continuità del sensore ---
    // Non usiamo i tempi di arrivo (Windows consegna i pacchetti a gruppi) ma i count:
    // ogni pacchetto è un poll del mouse, quindi a velocità costante porta lo stesso
    // movimento dei vicini, indipendentemente da quando Windows lo consegna.
    const double mag = std::hypot(double(e.dx), double(e.dy));
    if (!continuous) {
        windowCount_ = 0;
        lastDx_ = lastDy_ = 0;
    }

    // Inversione: due pacchetti consecutivi veloci in direzioni opposte. A 1000 Hz e oltre
    // il mouse non può fermarsi e ripartire all'indietro tra un poll e l'altro.
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
    constexpr double kMinCounts = 6.0;   // sotto, l'arrotondamento dei count falsa i rapporti
    constexpr double kSteady = 1.6;      // i vicini devono essere regolari tra loro
    constexpr double kSpike = 1.6;       // rapporto oltre cui il pacchetto è anomalo
    constexpr double kDip = 0.45;        // rapporto sotto cui il pacchetto è anomalo
    constexpr double kMultipleTol = 0.25;

    const Sample& c = window_[2];
    const float nb[4] = {window_[0].mag, window_[1].mag, window_[3].mag, window_[4].mag};
    const float lo = *std::min_element(nb, nb + 4);
    const float hi = *std::max_element(nb, nb + 4);
    const double avg = (nb[0] + nb[1] + nb[2] + nb[3]) / 4.0;
    if (avg < kMinCounts || lo <= 0 || hi / lo > kSteady) return; // movimento troppo lento o irregolare

    ++checkedPackets;
    const double r = c.mag / avg;
    const double k = std::round(r);
    if (k >= 2 && k <= 4 && std::fabs(r - k) <= kMultipleTol) {
        // Il pacchetto porta il movimento di k poll: i k-1 precedenti non sono arrivati.
        skippedEstimate += uint64_t(k) - 1;
        marks_.push({c.x, c.y, MarkKind::Skipped});
    } else if (r >= kSpike) {
        ++spikes;
        marks_.push({c.x, c.y, MarkKind::Spike});
    } else if (r <= kDip) {
        ++dips;
        marks_.push({c.x, c.y, MarkKind::Dip});
    }
}

void Analyzer::processButtons(const MouseEvent& e) {
    const uint16_t f = e.buttonFlags;

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
        // Scatto nella direzione opposta entro 40 ms: tipico encoder sporco.
        if (wheelLastDir && dir != wheelLastDir && toMs(e.qpc - wheelLastQpc) < 40.0) ++wheelReversals;
        wheelLastDir = dir;
        wheelLastQpc = e.qpc;
    }
    if (f & RI_MOUSE_HWHEEL) {
        if (e.hwheel > 0) tiltRight += e.hwheel; else tiltLeft -= e.hwheel;
    }
}

void Analyzer::refresh(int64_t nowQpc) {
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

    // Riferimento = media, non mediana: a 8000 Hz Windows consegna i pacchetti a gruppi,
    // la distribuzione è asimmetrica e la mediana cade sotto l'intervallo reale.
    // La somma degli intervalli è il tempo trascorso, quindi 1/media è il rate vero.
    size_t outliers = 0, batched = 0;
    for (float v : scratch_) {
        if (v > 1.5 * stats.meanUs) ++outliers;
        if (v < 0.25 * stats.meanUs) ++batched;
    }
    stats.outlierPct = 100.0 * double(outliers) / double(n);
    stats.batchedPct = 100.0 * double(batched) / double(n);
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
    lastMoveQpc_ = 0;
    movePackets = 0;
    stats = IntervalStats{};
}

void Analyzer::resetSensor() {
    path_.clear();
    marks_.clear();
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
