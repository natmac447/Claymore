---
phase: 02-selectable-oversampling
plan: "01"
subsystem: dsp
tags: [juce, oversampling, dsp, audio-plugin, latency, vst3, au]

# Dependency graph
requires:
  - phase: 01-dsp-foundation
    provides: ClaymoreEngine with fixed 2x IIR oversampling, APVTS parameter layout, DryWetMixer latency compensation
provides:
  - AudioParameterChoice for oversampling rate selection (2x/4x/8x) in APVTS
  - ClaymoreEngine pre-allocated array of 3 juce::dsp::Oversampling objects (zero allocation in process())
  - setOversamplingFactor() method for audio-thread-safe rate switching
  - Per-block rate-change detection in PluginProcessor with DryWetMixer + DAW latency re-sync
  - Correct DAW latency reporting at all three oversampling rates using std::round
affects: [03-ui, phase-3-ui, any phase touching ClaymoreEngine or PluginProcessor parameters]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "Pre-allocate N DSP objects at prepare() time; select active object by index in process() — zero allocation on hot path"
    - "Per-block rate-change detection via atomic compare: read param, compare to lastIndex, call engine method on mismatch"
    - "std::round for fractional IIR oversampling latency before setLatencySamples() and DryWetMixer::setWetLatency()"
    - "Reset stale filter state on the old oversampling object before switching to new index"

key-files:
  created: []
  modified:
    - Source/Parameters.h
    - Source/dsp/ClaymoreEngine.h
    - Source/PluginProcessor.h
    - Source/PluginProcessor.cpp

key-decisions:
  - "Pre-allocate all three Oversampling objects in prepare() — no allocation or destruction in setOversamplingFactor() or process()"
  - "useIntegerLatency = false (consistent with Phase 1; non-integer IIR latency handled with std::round)"
  - "Reset OLD oversampling object state before switching index to prevent stale IIR filter artifacts"
  - "Re-prepare smoothers (driveSmoother, etc.) and FuzzCoreState at new oversampled rate on every switch"
  - "DryWetMixer capacity 256 samples (was 64) — covers 8x IIR latency ~60 samples at high sample rates"
  - "setLatencySamples() called directly from processBlock on rate change (Crucible pattern — no timer or message-thread deferral)"

patterns-established:
  - "Pre-allocated DSP object array: objects created in prepare(), selected by index in process() — apply to any future selectable DSP variant"
  - "Rate-change detection block: atomic load, compare to lastIndex, call engine method, re-sync downstream DSP — template for future per-block param change handling"

requirements-completed: [QUAL-01, QUAL-02]

# Metrics
duration: 4min
completed: 2026-02-23
---

# Phase 2 Plan 01: Selectable Oversampling Summary

**ClaymoreEngine upgraded from hardcoded 2x to selectable 2x/4x/8x IIR oversampling with APVTS AudioParameterChoice, pre-allocated object array, and per-block DAW latency re-sync**

## Performance

- **Duration:** 4 min
- **Started:** 2026-02-23T18:16:24Z
- **Completed:** 2026-02-23T18:20:02Z
- **Tasks:** 2
- **Files modified:** 4

## Accomplishments

- Added `AudioParameterChoice` for oversampling (2x/4x/8x, default 2x) as a DAW-automatable parameter
- Replaced single `juce::dsp::Oversampling` member with pre-allocated `std::array` of 3 `unique_ptr` objects, created in `prepare()` with zero allocation in `process()` or `setOversamplingFactor()`
- Wired per-block rate-change detection in `processBlock()` that calls `engine.setOversamplingFactor()`, re-syncs `DryWetMixer::setWetLatency()`, and reports new latency to DAW with `std::round`
- Release VST3 and AU Universal Binaries built successfully

## Task Commits

Each task was committed atomically:

1. **Task 1: Add oversampling parameter and upgrade ClaymoreEngine** - `64451a6` (feat)
2. **Task 2: Wire per-block rate-change detection in PluginProcessor and verify Release build** - `857560f` (feat)

**Plan metadata:** (docs commit follows)

## Files Created/Modified

- `Source/Parameters.h` - Added `ParamIDs::oversampling`, `AudioParameterChoice` with StringArray `{"2x", "4x", "8x"}`, default index 0
- `Source/dsp/ClaymoreEngine.h` - Replaced single `oversampling` member with `oversamplingObjects[3]` array; added `setOversamplingFactor()`, updated `prepare()`, `process()`, `reset()`, `getLatencyInSamples()`
- `Source/PluginProcessor.h` - Added `oversamplingParam` pointer, `lastOversamplingIndex` tracker, bumped `dryWetMixer` capacity to 256
- `Source/PluginProcessor.cpp` - Constructor caches `oversamplingParam`; `prepareToPlay` applies initial index and uses `std::round`; `processBlock` per-block rate-change detection block

## Decisions Made

- Pre-allocate all three Oversampling objects in `prepare()`: no allocation in `setOversamplingFactor()` or `process()`. Chosen over lazy allocation to guarantee zero allocation on the audio thread.
- `useIntegerLatency = false` (consistent with Phase 1 decision): non-integer IIR latency handled with `std::round` as documented in RESEARCH.md.
- Reset the OLD oversampling object's state (`oversamplingObjects[oldIndex]->reset()`) before switching index — prevents stale IIR filter ringing on rate change.
- Re-prepare all four smoothers and `FuzzCoreState` at the new oversampled rate on every switch — ensures correct time constants after rate change.
- `dryWetMixer { 256 }` (was 64): 8x IIR latency can reach ~60 samples at high sample rates; 256 gives comfortable headroom.
- `setLatencySamples()` called directly from `processBlock` on rate change (Crucible pattern per RESEARCH.md recommendation). VST3 re-entrancy concern remains a flagged empirical verification item, not a design constraint.

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Selectable oversampling fully functional: QUAL-01 (user selects rate) and QUAL-02 (correct DAW latency) addressed
- Ready for Phase 3 UI work to expose the oversampling parameter in the plugin interface
- Empirical verification in Nuendo remains a documented concern in STATE.md (VST3 re-entrancy on setLatencySamples, non-integer latency rounding direction)

## Self-Check: PASSED

- 02-01-SUMMARY.md: FOUND
- Source/Parameters.h: FOUND
- Source/dsp/ClaymoreEngine.h: FOUND
- Source/PluginProcessor.h: FOUND
- Source/PluginProcessor.cpp: FOUND
- Commit 64451a6 (Task 1): FOUND
- Commit 857560f (Task 2): FOUND

---
*Phase: 02-selectable-oversampling*
*Completed: 2026-02-23*
