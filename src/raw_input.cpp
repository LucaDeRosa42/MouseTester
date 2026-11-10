#include "raw_input.hpp"

#include <vector>

extern "C" {
#include <hidsdi.h>
}

namespace {

constexpr wchar_t kSinkClass[] = L"MouseTesterRawInputSink";

std::string toUtf8(const std::wstring& w) {
    if (w.empty()) return {};
    const int len = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), nullptr, 0, nullptr, nullptr);
    std::string out(len, '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), out.data(), len, nullptr, nullptr);
    return out;
}

// Extracts "VID_xxxx" and "PID_xxxx" from the device path.
std::string vidPidFromPath(const std::wstring& path) {
    auto grab = [&](const wchar_t* key) -> std::wstring {
        const size_t pos = path.find(key);
        return pos == std::wstring::npos ? L"????" : path.substr(pos + 4, 4);
    };
    return "VID " + toUtf8(grab(L"VID_")) + " PID " + toUtf8(grab(L"PID_"));
}

} // namespace

bool RawInputCapture::start() {
    if (running_) return true;
    running_ = true;
    ready_ = false;
    thread_ = std::thread(&RawInputCapture::run, this);
    while (!ready_.load()) std::this_thread::yield();
    if (!ok_) {
        thread_.join();
        running_ = false;
        return false;
    }
    return true;
}

void RawInputCapture::stop() {
    if (!running_.exchange(false)) return;
    PostThreadMessageW(threadId_, WM_QUIT, 0, 0);
    if (thread_.joinable()) thread_.join();
}

void RawInputCapture::run() {
    threadId_ = GetCurrentThreadId();
    // The capture thread must respond immediately: highest priority.
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

    WNDCLASSEXW wc{sizeof(wc)};
    wc.lpfnWndProc = DefWindowProcW;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = kSinkClass;
    RegisterClassExW(&wc);

    // "Message-only" window: invisible, it only exists to receive WM_INPUT.
    HWND hwnd = CreateWindowExW(0, kSinkClass, L"", 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, wc.hInstance, nullptr);

    RAWINPUTDEVICE rid{};
    rid.usUsagePage = 0x01; // Generic Desktop
    rid.usUsage = 0x02;     // Mouse
    rid.dwFlags = RIDEV_INPUTSINK;
    rid.hwndTarget = hwnd;

    ok_ = hwnd && RegisterRawInputDevices(&rid, 1, sizeof(rid));
    ready_ = true;
    if (!ok_) {
        if (hwnd) DestroyWindow(hwnd);
        return;
    }

    bool quit = false;
    while (!quit) {
        MsgWaitForMultipleObjectsEx(0, nullptr, INFINITE, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
        MSG msg;
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_INPUT) {
                // Timestamp taken as soon as possible after the message arrives.
                LARGE_INTEGER now;
                QueryPerformanceCounter(&now);
                handleRawInput(reinterpret_cast<HRAWINPUT>(msg.lParam), now.QuadPart);
                handled_.fetch_add(1, std::memory_order_relaxed);
                DefWindowProcW(msg.hwnd, msg.message, msg.wParam, msg.lParam); // cleanup required by Windows
            } else if (msg.message == WM_QUIT) {
                quit = true;
                break;
            } else {
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }
        }
        // Once per wake-up, after the queue is empty: costs well under a microsecond.
        ULONG64 cycles = 0;
        QueryThreadCycleTime(GetCurrentThread(), &cycles);
        cycles_.store(cycles, std::memory_order_relaxed);
    }

    rid.dwFlags = RIDEV_REMOVE;
    rid.hwndTarget = nullptr;
    RegisterRawInputDevices(&rid, 1, sizeof(rid));
    DestroyWindow(hwnd);
    UnregisterClassW(kSinkClass, wc.hInstance);
}

void RawInputCapture::handleRawInput(HRAWINPUT handle, int64_t qpc) {
    RAWINPUT raw;
    UINT size = sizeof(raw);
    if (GetRawInputData(handle, RID_INPUT, &raw, &size, sizeof(RAWINPUTHEADER)) == (UINT)-1) return;
    if (raw.header.dwType != RIM_TYPEMOUSE) return;

    const RAWMOUSE& m = raw.data.mouse;
    MouseEvent e;
    e.qpc = qpc;
    e.device = raw.header.hDevice;
    if (!(m.usFlags & MOUSE_MOVE_ABSOLUTE)) { // ignore tablets and remote desktop
        e.dx = m.lLastX;
        e.dy = m.lLastY;
    }
    e.buttonFlags = m.usButtonFlags;
    if (m.usButtonFlags & RI_MOUSE_WHEEL)  e.wheel  = static_cast<int16_t>(m.usButtonData);
    if (m.usButtonFlags & RI_MOUSE_HWHEEL) e.hwheel = static_cast<int16_t>(m.usButtonData);

    if (!queue_.push(e)) dropped_.fetch_add(1, std::memory_order_relaxed);
}

std::string RawInputCapture::deviceName(HANDLE device) {
    if (!device) return "Unknown device";

    UINT len = 0;
    GetRawInputDeviceInfoW(device, RIDI_DEVICENAME, nullptr, &len);
    if (len == 0) return "Unknown device";
    std::wstring path(len, L'\0');
    if (GetRawInputDeviceInfoW(device, RIDI_DEVICENAME, path.data(), &len) == (UINT)-1) return "Unknown device";
    path.resize(wcslen(path.c_str()));

    const std::string ids = vidPidFromPath(path);

    // Try to read the product name from the HID descriptor.
    HANDLE h = CreateFileW(path.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
    if (h != INVALID_HANDLE_VALUE) {
        wchar_t product[128] = {};
        const BOOLEAN got = HidD_GetProductString(h, product, sizeof(product));
        CloseHandle(h);
        if (got && product[0]) return toUtf8(product) + "  (" + ids + ")";
    }
    return ids;
}
