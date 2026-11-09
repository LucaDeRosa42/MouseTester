#pragma once
#include <array>
#include <cstdint>
#include <deque>
#include <unordered_map>
#include <vector>

#include "raw_input.hpp"

// Buffer circolare a dimensione fissa. Indice 0 = elemento più vecchio.
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
    double outlierPct = 0; // intervalli > 1.5x la mediana
    double batchedPct = 0; // intervalli < 0.25x la mediana (pacchetti arrivati insieme)
};

struct ButtonInfo {
    bool     down = false;
    uint64_t presses = 0;
    uint64_t chatter = 0;          // doppi click involontari
    double   lastGapMs = -1;       // tempo dal rilascio precedente alla nuova pressione
    double   lastHoldMs = -1;      // durata dell'ultima pressione
    double   lastChatterMs = -1;
    int64_t  lastDownQpc = 0, lastUpQpc = 0;
};

struct PathPoint { float x, y; };

// Punto anomalo del tracciato, per evidenziarlo nel disegno.
enum class MarkKind : uint8_t { Skipped, Spike, Dip, Reversal };
struct SensorMark { float x, y; MarkKind kind; };

class Analyzer {
public:
    static constexpr size_t kIntervals = 16384;
    static constexpr size_t kPath = 20000;

    explicit Analyzer(int64_t qpcFrequency) : freq_(double(qpcFrequency)) {}

    void process(const MouseEvent& e);
    void refresh(int64_t nowQpc); // da chiamare una volta per frame

    void resetPolling();
    void resetSensor();
    void resetButtons();
    void resetAll() { resetPolling(); resetSensor(); resetButtons(); }

    void dpiStart() { dpiActive = true; dpiSumX = dpiSumY = 0; }
    void dpiStop()  { dpiActive = false; }

    void histogram(int bins, double maxUs, std::vector<float>& out) const;
    const Ring<float, kIntervals>& intervals() const { return intervals_; }
    const Ring<PathPoint, kPath>& path() const { return path_; }
    const Ring<SensorMark, 1024>& marks() const { return marks_; }

    // --- Impostazioni ---
    HANDLE deviceFilter = nullptr; // nullptr = tutti i dispositivi
    double chatterThresholdMs = 30.0;

    // --- Risultati (lettura) ---
    std::unordered_map<HANDLE, uint64_t> devicePackets;
    IntervalStats stats;
    double   packetsLastSecond = 0;
    uint64_t movePackets = 0;

    // Continuità del sensore, valutata sui count di ogni pacchetto rispetto ai vicini.
    uint64_t checkedPackets = 0;  // pacchetti abbastanza veloci e regolari da essere valutati
    uint64_t skippedEstimate = 0; // pacchetti mancanti: un pacchetto porta 2-4 volte il movimento dei vicini
    uint64_t spikes = 0;          // pacchetto con molto più movimento dei vicini (non multiplo intero)
    uint64_t dips = 0;            // pacchetto con molto meno movimento dei vicini
    uint64_t reversals = 0;       // inversioni di direzione in un solo pacchetto

    std::array<ButtonInfo, 5> buttons{};
    int64_t  wheelUp = 0, wheelDown = 0, tiltLeft = 0, tiltRight = 0;
    uint64_t wheelReversals = 0;
    int64_t  wheelLastQpc = 0;
    int      wheelLastDir = 0;

    bool   dpiActive = false;
    double dpiSumX = 0, dpiSumY = 0;

private:
    void processMotion(const MouseEvent& e);
    void processButtons(const MouseEvent& e);
    double toUs(int64_t ticks) const { return double(ticks) * 1e6 / freq_; }
    double toMs(int64_t ticks) const { return double(ticks) * 1e3 / freq_; }

    double freq_;
    int64_t lastMoveQpc_ = 0;
    double speedEma_ = 0;  // velocità media recente in count/pacchetto
    int32_t lastDx_ = 0, lastDy_ = 0;
    float posX_ = 0, posY_ = 0;

    Ring<float, kIntervals> intervals_;
    Ring<PathPoint, kPath> path_;
    Ring<SensorMark, 1024> marks_;

    // Finestra degli ultimi 5 pacchetti di movimento continuo: il pacchetto centrale
    // viene confrontato con i 2 precedenti e i 2 successivi.
    struct Sample { float mag, x, y; };
    std::array<Sample, 5> window_{};
    size_t windowCount_ = 0;
    void checkContinuity();
    std::deque<int64_t> lastSecond_;
    std::vector<float> scratch_;
};
