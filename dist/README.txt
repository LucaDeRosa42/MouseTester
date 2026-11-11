MOUSE TESTER 0.2.0
==================

A tester for high-end gaming mice on Windows: polling rate up to 8000 Hz,
sensor continuity, angle snapping, DPI, button chatter and wheel.


GETTING STARTED
---------------
1. Extract MouseTester.exe anywhere and double-click it. Nothing to install.
2. The first time, Windows may show "Windows protected your PC" because the
   program is not digitally signed. Click "More info", then "Run anyway".

Requirements: Windows 10 or 11 (64-bit), any DirectX 11 graphics card.
No administrator rights needed.


TIPS FOR ACCURATE RESULTS
-------------------------
- Keep the Mouse Tester window in the foreground while testing.
- Plug the mouse or its receiver directly into the PC, not into a hub or monitor.
- At 4000-8000 Hz use a high DPI (1600 or more): the polling rate is only
  measured while the mouse moves fast enough to report in every poll.
- If more than one mouse is connected, pick yours in the menu at the top left.
- The interface language can be changed in the menu below it (English, Italiano,
  Русский, Español, Français, Deutsch, Português, 中文). It is remembered.


THE TABS
--------
Polling rate
  Move the mouse quickly in circles. The big number is the real rate; below
  it you see jitter, outliers and the distribution of the intervals. The
  status line tells you if you are moving fast enough ("Measuring") or not.

Sensor
  Move the mouse fast and steadily. A healthy sensor keeps Missing, Spikes,
  Dips and Reversals at zero; hover over each counter for an explanation.
  For angle snapping, draw slow freehand lines near 0, 45 and 90 degrees and
  read "Last stroke": a long, almost perfectly straight line exactly on a
  guide means snapping is on. Wheel zooms the path, drag pans, double click fits.

DPI
  Measure a known distance (a ruler or the edge of the mousepad) and enter it.
  Put the cursor over the measuring pad, hold the left button, slide the mouse
  along the distance and release. Repeat 5-10 times and read the average.

Clicks & wheel
  Click each button: a second press arriving too soon after a release is
  counted as chatter (unwanted double click). Scroll the wheel slowly: every
  notch should count as 1 and reversals should stay at zero.


EXPORT
------
"Export report..." (bottom left) saves a text file with all the results.
"Export intervals (CSV)..." on the Polling rate tab saves the raw intervals.


LIMITS
------
Timestamps are taken when Windows delivers each packet, so the measured jitter
includes USB, driver and system load. Click latency, sensor latency and
lift-off distance cannot be measured in software.
