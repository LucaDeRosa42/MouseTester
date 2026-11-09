#pragma once
#include <windows.h>

#include <atomic>
#include <cstdint>
#include <string>
#include <thread>

#include "spsc_queue.hpp"

// A packet received from the mouse, with the timestamp taken as soon as it arrived.
struct MouseEvent {
    int64_t  qpc = 0;          // QueryPerformanceCounter on arrival
    HANDLE   device = nullptr; // device that generated it
    int32_t  dx = 0, dy = 0;   // relative movement in sensor counts
    uint16_t buttonFlags = 0;  // RI_MOUSE_*
    int16_t  wheel = 0;        // vertical wheel (120 = one notch)
    int16_t  hwheel = 0;       // horizontal wheel / tilt
};

// Raw Input capture on a dedicated high-priority thread.
class RawInputCapture {
public:
    using Queue = SpscQueue<MouseEvent, (1u << 18)>; // ~30 s of headroom at 8 kHz

    ~RawInputCapture() { stop(); }

    bool start();
    void stop();

    Queue&   queue() { return queue_; }
    uint64_t dropped() const { return dropped_.load(std::memory_order_relaxed); }

    // Readable device name (HID product name, or VID/PID).
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
