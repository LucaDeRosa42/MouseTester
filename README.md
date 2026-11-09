# Mouse Tester

High-performance mouse tester (up to 8000 Hz and beyond) for Windows, written in C++20 with Dear ImGui and DirectX 11.

## Requirements
- Windows 10 or 11 (64-bit)
- **Visual Studio 2022 or later** (Community is free) with the **"Desktop development with C++"** workload. CMake is included.
- Internet connection on the first build: CMake downloads Dear ImGui from GitHub.

## Building
**From Visual Studio:** File → Open → Folder... and pick this folder. Visual Studio detects `CMakeLists.txt`. Choose the `x64-Release` configuration, select `MouseTester.exe` as the startup item and press F5.

**From the command line** (Visual Studio "Developer PowerShell"):
```
cmake -B build -A x64
cmake --build build --config Release
build\Release\MouseTester.exe
```

## Layout
| File | Contents |
|---|---|
| `src/raw_input.*` | Capture thread: Raw Input, QueryPerformanceCounter timestamps, HID device name |
| `src/spsc_queue.hpp` | Lock-free queue between the capture thread and the UI thread |
| `src/analyzer.*` | Statistics: polling, jitter, sensor continuity, DPI, chatter, wheel |
| `src/mouse_view.*` | Vector drawing of the mouse with buttons that light up |
| `src/ui.*` | UI: theme and tabs |
| `src/main.cpp` | Win32 window, DirectX 11, main loop |
| `res/`, `tools/make_icon.ps1` | App icon and the script that generates it |

## Measurement notes
- Keep the window **in the foreground** during tests: some Windows 11 versions throttle input for background apps.
- Plug the mouse or receiver **directly into the PC**, not into a hub or the monitor.
- The polling rate is measured only while the mouse moves fast enough to fill every poll (at least 1.5 counts per packet). At 8000 Hz, use a high DPI (1600 or more).
- The timestamp is taken when Windows delivers the packet, not when the mouse sends it. Measured jitter therefore includes USB, driver and scheduler.
- Click latency, sensor latency and lift-off distance cannot be measured in software.
