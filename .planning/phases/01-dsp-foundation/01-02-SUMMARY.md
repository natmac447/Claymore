---
phase: 01-dsp-foundation
plan: "02"
subsystem: dsp
tags: [juce, dsp, fuzz, noise-gate, oversampling, waveshaping, tone-filtering, hysteresis]

requires:
  - phase: 01-01
    provides: "ClaymoreEngine DSP orchestrator, FuzzCore/FuzzTone/FuzzConfig/FuzzType ports, noise gate state machine, all 11 APVTS parameters, compilable VST3+AU scaffold"

provides:
  - FuzzCore.h: Rat-based per-sample waveshaping with drive (1-40x), symmetry, tightness HPF (20-800 Hz), sag bias-starve
  - FuzzTone.h: Tone LPF (2-20 kHz) and Presence high-shelf (+/-6 dB at 4 kHz) with DC blocker
  - FuzzConfig.h: mapDrive() and outputCompensation (0.30f) constants
  - FuzzType.h: FuzzConfig namespace (verbatim GunkLord port)
  - ClaymoreEngine.h: Complete DSP chain (gate -> oversample -> fuzz -> downsample -> tone) with all hidden gate setters
  - OutputLimiter.h: Brickwall 0 dBFS limiter wrapping juce::dsp::Limiter<float>
  - Noise gate: dual-threshold hysteresis with sidechain HPF, configurable attack/release/range/ratio/hysteresis

affects:
  - 01-03 (processBlock wiring — ClaymoreEngine interface now complete)
  - 02-oversampling (Phase 2 upgrades ClaymoreEngine oversampler)
  - 03-gui (Phase 3 will surface hidden gate params via right-click context menu)

tech-stack:
  added: []
  patterns:
    - "Hidden gate parameters as mutable member variables (not static constexpr) to enable setGateAttack/Release/Ratio/SidechainHPF/Range/Hysteresis setters"
    - "Sidechain HPF per-channel array (FirstOrderTPTFilter[maxChannels]) in gate: filters level-detection path only, audio path unchanged"
    - "recalculateGateCoefficients() helper: recomputes attack/release IIR coeffs when time constants change at runtime"
    - "Gate ratio parameter scales effective range: actualRangeDB = gateRangeDB * gateRatio"

key-files:
  created: []
  modified:
    - Source/dsp/ClaymoreEngine.h

key-decisions:
  - "Header-only ClaymoreEngine instead of .h/.cpp split: plan specified ClaymoreEngine.cpp but all methods are inline and the class fits cleanly in one header; avoids ODR issues with template-heavy JUCE DSP; no functional difference"
  - "Sidechain HPF in gate detection path: 150 Hz default filters bass from level detection — prevents kick drum low-end from keeping gate open when no guitar signal is present"
  - "Gate ratio as range scalar (0=no attenuation, 1=full gateRangeDB): allows soft-knee-like behavior at intermediate values without changing the state machine logic"
  - "setGateHysteresis recalculates closeThreshold immediately: gateCloseThreshold = gateOpenThreshold - gateHysteresisDB keeps both thresholds consistent after either hysteresis or threshold changes"

patterns-established:
  - "Sidechain HPF processes copy of audio for level detection; original audio samples in buffer are never touched by gate filters"
  - "All hidden gate parameters exposed as setters now; Phase 3 context menu just calls these setters via APVTS or non-automatable parameter"
  - "recalculateGateCoefficients() pattern: reuse whenever sampleRate or time constants change"

requirements-completed:
  - DIST-01
  - DIST-02
  - DIST-03
  - DIST-04
  - DIST-05
  - DIST-06
  - SIG-05

duration: 2min
completed: 2026-02-23
---

# Phase 1 Plan 02: Fuzz DSP Port and ClaymoreEngine Signal Chain Summary

**Rat-based waveshaping engine with 2x IIR oversampling, dual-threshold hysteresis noise gate (sidechain HPF, attack/release/range/ratio/hysteresis all configurable), and Claymore-extended tone sweep (2-20 kHz)**

## Performance

- **Duration:** 2 min
- **Started:** 2026-02-23T17:36:17Z
- **Completed:** 2026-02-23T17:38:36Z
- **Tasks:** 2 of 2 (verified)
- **Files modified:** 1

## Accomplishments

- All four fuzz DSP headers (FuzzCore.h, FuzzTone.h, FuzzConfig.h, FuzzType.h) confirmed correct: FuzzCore has drive/symmetry/tightness/sag waveshaping; FuzzTone has Claymore-extended tone range (2000-20000 Hz)
- ClaymoreEngine signal chain verified: gate -> processSamplesUp -> FuzzCore per-sample -> outputCompensation -> processSamplesDown -> tone.applyTone — matches CONTEXT.md locked order
- Added six hidden gate setter methods (setGateAttack, setGateRelease, setGateRatio, setGateSidechainHPF, setGateRange, setGateHysteresis) to support Phase 3 right-click context menu
- Added per-channel sidechain HPF array (FirstOrderTPTFilter, 150 Hz default) to gate level-detection path — prevents bass from triggering the gate
- Project compiles to VST3 binary after all changes with no errors

## Task Commits

Each task was committed atomically:

1. **Task 1: Fuzz DSP headers verified** — no commit required (files correct from 01-01)
2. **Task 2: ClaymoreEngine with hidden gate setters and sidechain HPF** - `8d6952d` (feat)

**Plan metadata:** (docs commit — to follow)

## Files Created/Modified

- `Source/dsp/ClaymoreEngine.h` - Added setGateAttack/Release/Ratio/SidechainHPF/Range/Hysteresis setters; converted static constexpr gate params to mutable members; added sidechainHPF[maxChannels] FirstOrderTPTFilter array; added recalculateGateCoefficients() helper; sidechain HPF applied in applyNoiseGate() level-detection path
- `Source/dsp/fuzz/FuzzCore.h` - No changes (already correct from 01-01): processSample with drive, symmetry, tightness, sag
- `Source/dsp/fuzz/FuzzTone.h` - No changes (already correct from 01-01): applyTone with 2000-20000 Hz Tone LP sweep
- `Source/dsp/fuzz/FuzzConfig.h` - No changes (already correct from 01-01): mapDrive, outputCompensation
- `Source/dsp/fuzz/FuzzType.h` - No changes (already correct from 01-01): FuzzConfig namespace
- `Source/dsp/OutputLimiter.h` - No changes (already correct from 01-01): juce::dsp::Limiter<float> wrapper

## Decisions Made

- **Header-only ClaymoreEngine vs. .h/.cpp split:** Plan specified `ClaymoreEngine.cpp` but the implementation delivered by 01-01 is cleanly header-only. All methods are inline, the JUCE DSP chain is template-heavy making a .cpp split counterproductive (would require explicit instantiation), and the project compiles cleanly. No functional difference.
- **Sidechain HPF at 150 Hz:** Filters bass from gate level-detection path. Prevents kick drum low-end (60-100 Hz) from holding the gate open when only a thud registers, not guitar signal. Applied to detection copy only — audio path is unaffected.
- **Gate ratio as range scaler:** `actualRangeDB = gateRangeDB * gateRatio` gives intermediate-ratio soft behavior. At ratio=1.0 (default), behavior is identical to the original implementation.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 2 - Missing Critical] Added hidden gate setter methods and sidechain HPF**
- **Found during:** Task 2 (ClaymoreEngine verification)
- **Issue:** Plan success_criteria requires "Noise gate state machine has configurable threshold, attack, release, sidechain HPF, range, and hysteresis" — but 01-01 implementation had these as static constexpr constants with no setter methods and no sidechain HPF filter
- **Fix:** Added `setGateAttack`, `setGateRelease`, `setGateRatio`, `setGateSidechainHPF`, `setGateRange`, `setGateHysteresis` setter methods; converted static constexpr gate params to mutable member variables; added `sidechainHPF[maxChannels]` FirstOrderTPTFilter array initialized in `prepare()`; updated `applyNoiseGate()` to run sidechain signal through HPF before level detection; added `recalculateGateCoefficients()` helper
- **Files modified:** Source/dsp/ClaymoreEngine.h
- **Verification:** Project compiles cleanly; all 6 setter names verified with grep
- **Committed in:** 8d6952d (Task 2 commit)

---

**Total deviations:** 1 auto-fixed (Rule 2 — missing critical gate configurability)
**Impact on plan:** Required for plan success_criteria compliance. No scope creep. All hidden gate parameters now accessible via setter methods as the plan's public interface requires.

## Issues Encountered

None — the fuzz DSP files were already correct from plan 01-01; only the gate setter methods and sidechain HPF were missing.

## User Setup Required

None — no external service configuration required. Plugin installs automatically via COPY_PLUGIN_AFTER_BUILD.

## Next Phase Readiness

- All DSP classes complete and compile: ClaymoreEngine is the sole interface PluginProcessor needs to call
- Hidden gate setters ready for Phase 3 context menu wiring (no further DSP changes needed)
- Blocker (from 01-01, still pending): setLatencySamples() value should be verified empirically in Nuendo before Phase 2 adds selectable oversampling

---
*Phase: 01-dsp-foundation*
*Completed: 2026-02-23*
