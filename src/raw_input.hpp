#pragma once
#include <windows.h>

#include <atomic>
#include <cstdint>
#include <string>
#include <thread>

#include "spsc_queue.hpp"

// Un pacchetto ricevuto dal mouse, con il timestamp preso appena arrivato.
struct MouseEvent {
    int64_t  qpc = 0;          // QueryPerformanceCounter all'arrivo
    HANDLE   device = nullptr; // dispositivo che l'ha generato
    int32_t  dx = 0, dy = 0;   // spostamento relativo in count del sensore
    uint16_t buttonFlags = 0;  // RI_MOUSE_*
    int16_t  wheel = 0;        // rotella verticale (120 = uno scatto)
    int16_t  hwheel = 0;       // rotella orizzontale / tilt
};

// Cattura Raw Input su un thread dedicato ad alta priorità.
class RawInputCapture {
public:
    using Queue = SpscQueue<MouseEvent, (1u << 18)>; // ~30 s di margine a 8 kHz

    ~RawInputCapture() { stop(); }

    bool start();
    void stop();

    Queue&   queue() { return queue_; }
    uint64_t dropped() const { return dropped_.load(std::memory_order_relaxed); }

    // Nome leggibile del dispositivo (nome prodotto HID, oppure VID/PID).
    static std::string deviceName(HANDLE device);

private:
    void run();
    void handleRawInput(HRAWINPUT handle, int64_t qpc);

    Queue                 queue_;
    std::thread           thread_;
    std::atomic<bool>     running_{false};
    std::atomic<bool>     ready_{false};
    std::atomic<bool>     ok_{false};
    std::atomic<uint64_t> dropped_{0};
    DWORD                 threadId_ = 0;
};
