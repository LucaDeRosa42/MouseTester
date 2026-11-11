// Mouse Tester - Copyright (C) 2026 lcrd42
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
// Interface translations. The exported report stays in English on purpose,
// so it can be read by anyone it is sent to.

struct ImFontGlyphRangesBuilder;

enum class Lang { En, It, Ru, Es, Fr, De, Pt, Zh, Count };

enum class Str {
    // Left panel
    AllDevices, DevicePackets, Language, ColButton, ColPresses, ColChatter,
    BtnLeft, BtnRight, BtnMiddle, BtnSide4, BtnSide5,
    WheelSummary, ExportReport, ResetAll, ReportSaved, ReportFailed, IntervalsSaved, IntervalsFailed,
    // Top bar and tabs
    CaptureStats, CaptureIdle, TipPerf, Dropped, TabPolling, TabSensor, TabDpi, TabClicks,
    // Polling
    Nominal, MoveCircles, Measuring, TooSlow, NotMoving,
    Mean, Median, Jitter, PacketsPerSec, MinMax, Percentiles, Outliers, Batched,
    IntervalDistribution, LastIntervals, HintPollSpeed, HintBatched, ResetPolling, ExportIntervals,
    // Sensor
    PacketsChecked, TipChecked, Missing, Spikes, Dips, Reversals,
    TipMissing, TipSpike, TipDip, TipReversal, MarkMissing, MarkSpike, MarkDip, MarkReversal,
    LastStroke, StrokeInfo, PossibleSnapping, TipStroke, StrokeNone,
    Path, Fit, RecentOnly, SnapGuides, Zoom, ZoomAuto, Grid, HintPath, HintContinuity, ResetSensor,
    // DPI
    HintDpi, Distance, DpiSet, Recording, PadHover, PadIdle, AveragePasses, NoPasses,
    DevFromSet, Spread, Passes, ColCountsX, ColCountsY, ColAngle, ColDeviation, RemoveLast, ClearPasses,
    // Clicks & wheel
    ChatterThreshold, HintChatter, ColState, ColReleasePress, ColHold, Pressed, Released, ChatterLast,
    Wheel, NotchesUp, NotchesDown, QuickReversals, Tilt, HintWheel, ResetButtons,
    Count
};

// Text of `id` in the current language.
const char* T(Str id);

Lang CurrentLanguage();
void SetLanguage(Lang lang);           // also saves it for the next start
void LoadLanguage();                   // saved choice, or the Windows language on first start
const char* LanguageName(Lang lang);   // native name, e.g. "Italiano"

// Adds every character used by every translation, to build the font atlas.
void AddTranslationGlyphs(ImFontGlyphRangesBuilder& builder);
