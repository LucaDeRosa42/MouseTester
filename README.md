# Mouse Tester

A free, open-source tester for high-end gaming mice on Windows: **polling rate up to 8000 Hz**, sensor continuity, angle snapping, DPI, button chatter and wheel. Written in C++20 with Dear ImGui and DirectX 11.

![Mouse Tester, Polling rate tab with a mouse at 8000 Hz](docs/polling.png)

| Sensor | Clicks & wheel |
|---|---|
| ![Sensor tab: path, anomaly counters and angle snapping guides](docs/sensor.png) | ![Clicks & wheel tab: chatter, hold time and wheel](docs/clicks.png) |

## Download
Get the latest `MouseTester-x.y.z.zip` from the [Releases](../../releases) page, extract it and run `MouseTester.exe`. Nothing to install.

- Windows 10 or 11 (64-bit), any DirectX 11 graphics card, no administrator rights.
- The program is not digitally signed, so Windows SmartScreen may say "Windows protected your PC": click **More info → Run anyway**. The full source is here if you want to check what it does, or build it yourself.

## Features
- **Polling rate:** real rate, jitter, percentiles, interval histogram and timeline, with a "measuring / paused" indicator that tells you when you are moving fast enough.
- **Sensor:** missing packets, spikes, dips and direction reversals, shown on a zoomable path where every packet is a dot; best-fit line of the last stroke to detect **angle snapping**.
- **DPI:** guided measurement in passes over a known distance, with average, spread and per-pass angle.
- **Clicks & wheel:** chatter (unwanted double clicks) with adjustable threshold, hold time, wheel notches, quick reversals and tilt.
- **Export:** text report of all results and CSV of the raw polling intervals.
- **Languages:** English, Italiano, Русский, Español, Français, Deutsch, Português, 中文 (简体).

## How it measures
- A dedicated `TIME_CRITICAL` thread receives Raw Input and timestamps every packet with `QueryPerformanceCounter` as soon as it leaves the message queue. A lock-free queue hands packets to the UI thread; the capture thread never blocks or allocates.
- At 4000-8000 Hz Windows delivers packets in batches. The rate is therefore computed from the **mean** interval (total time / packets), not the median, and only while the mouse moves fast enough to report in every poll (at least 1.5 counts per packet).
- Sensor continuity is judged on the **counts carried by each packet** compared with its 4 neighbours, not on arrival times: a packet carrying 2-4x the movement means skipped polls, other large deviations are spikes or dips.
- Timestamps are taken when Windows delivers each packet, so measured jitter includes USB, driver and system load. Click latency, sensor latency and lift-off distance cannot be measured in software.

Tips: keep the window in the foreground, plug the mouse or receiver directly into the PC (not a hub or monitor), and use a high DPI (1600+) at 8000 Hz.

## Building
Requirements: **Visual Studio 2022 or later** (Community is free) with the **"Desktop development with C++"** workload, which includes CMake. The first configure downloads Dear ImGui from GitHub.

**From Visual Studio:** File → Open → Folder..., choose `x64-Release`, select `MouseTester.exe` as the startup item and press F5.

**From the command line** (Visual Studio "Developer PowerShell"):
```
cmake -B build -A x64
cmake --build build --config Release
build\Release\MouseTester.exe
```
The result is a single self-contained `.exe` (static C runtime, only Windows system DLLs). The version is set in `res/resource.h`.

## Layout
| File | Contents |
|---|---|
| `src/raw_input.*` | Capture thread: Raw Input, QueryPerformanceCounter timestamps, HID device name |
| `src/spsc_queue.hpp` | Lock-free queue between the capture thread and the UI thread |
| `src/analyzer.*` | Statistics: polling, jitter, sensor continuity, angle snapping, DPI, chatter, wheel |
| `src/mouse_view.*` | Vector drawing of the mouse with buttons that light up |
| `src/ui.*` | UI: theme and tabs |
| `src/i18n.*` | Interface translations and saved language |
| `src/export.*` | Text report and CSV export |
| `src/main.cpp` | Win32 window, DirectX 11, main loop |
| `res/`, `tools/make_icon.ps1` | App icon and the script that generates it |

## Contributing
Bug reports are welcome, especially with an exported report (**Export report...**) from your mouse: real data is what tunes the sensor thresholds.

Translations live in `src/i18n.cpp`; corrections from native speakers are very welcome. A new language needs one entry per row, with the same printf placeholders as the English text. Languages that need complex text shaping or right-to-left layout (Arabic, Hindi, Bengali…) are not supported by Dear ImGui.

## License
Copyright (C) 2026 lcrd42.

Mouse Tester is free software: you can redistribute it and/or modify it under the terms of the **GNU General Public License, version 3 or later**. See [LICENSE](LICENSE). It is distributed in the hope that it will be useful, but without any warranty.

Third-party software: Dear ImGui (MIT License), see [THIRD_PARTY.txt](THIRD_PARTY.txt).
