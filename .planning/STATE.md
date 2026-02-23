# Project State

## Project Reference

See: .planning/PROJECT.md (updated 2026-02-23)

**Core value:** A Rat-inspired distortion with enough range and tonal control to be useful on any source material — from subtle saturation to aggressive destruction — in a focused, pedal-style interface.
**Current focus:** Phase 3 — GUI (Phase 2 complete)

## Current Position

Phase: 3 of 4 (GUI)
Plan: 1 of 2 in current phase — Phase 3 Plan 1 COMPLETE
Status: Phase 3 Plan 1 complete, ready for Phase 3 Plan 2
Last activity: 2026-02-23 — Plan 03-01 complete: ClaymoreTheme LookAndFeel + OversamplingSelector + BinaryData asset pipeline

Progress: [█████░░░░░] 50%

## Performance Metrics

**Velocity:**
- Total plans completed: 3
- Average duration: 4 min
- Total execution time: 0.20 hours

**By Phase:**

| Phase | Plans | Total | Avg/Plan |
|-------|-------|-------|----------|
| 01-dsp-foundation | 3 | 12 min | 4 min |
| 02-selectable-oversampling | 1 | 4 min | 4 min |
| 03-gui | 1 | 3 min | 3 min |

**Recent Trend:**
- Last 5 plans: 7 min, 2 min, 3 min, 4 min, 3 min
- Trend: Fast (3 min avg — targeted additions, no processor changes)

*Updated after each plan completion*

## Accumulated Context

### Decisions

Decisions are logged in PROJECT.md Key Decisions table.
Recent decisions affecting current work:

- Pre-project: Port Rat algorithm from Gunk Lord (proven DSP, no reimplementation)
- Pre-project: Selectable oversampling 2x/4x/8x (quality vs. CPU tradeoff for mixing use)
- Pre-project: No standalone build, plugin-only (DAW workflows)
- 01-01: Custom noise gate state machine (dual-threshold hysteresis) instead of juce::dsp::NoiseGate — JUCE gate has no hysteresis; ~40-line envelope follower gives full control
- 01-01: Sag gain-makeup linear approximation (clipped *= 1 + sag * 0.5) — tunable by ear; CONTEXT.md allows chaos at extreme settings
- 01-01: setLatencySamples reports engine.getLatencyInSamples() directly (not *2 as GunkLord does for double-processing)
- 01-01: Gate defaults — attack 1ms, release 80ms, hysteresis 4 dB, range -60 dB (per RESEARCH.md Pitfall 3)
- 01-02: Header-only ClaymoreEngine (plan specified .h/.cpp split) — all methods inline, template-heavy JUCE DSP; no functional difference
- 01-02: Sidechain HPF at 150 Hz in gate level-detection — prevents bass from holding gate open; setGateSidechainHPF() allows adjustment
- 01-02: Gate ratio as range scaler (actualRangeDB = gateRangeDB * gateRatio) — Phase 3 can expose for soft-knee behavior
- [Phase 01-03]: processBlock pre-complete from Plan 01-01 (fca99eb): no changes needed in Plan 01-03 — verified correct against all must_have requirements
- [Phase 01-03]: CMakeLists.txt unchanged: ClaymoreEngine is header-only (01-02 decision), no ClaymoreEngine.cpp needed in target_sources
- [Phase 02-01]: Pre-allocate all 3 Oversampling objects in prepare() — no allocation in setOversamplingFactor() or process(); zero-allocation hot path guaranteed
- [Phase 02-01]: useIntegerLatency = false (consistent with Phase 1); std::round used for setLatencySamples() and DryWetMixer::setWetLatency()
- [Phase 02-01]: Reset OLD oversampling object state before switching index to prevent stale IIR filter ringing
- [Phase 02-01]: Re-prepare all smoothers and FuzzCoreState at new oversampled rate on every switch
- [Phase 02-01]: dryWetMixer capacity 256 (was 64) — 8x IIR latency can reach ~60 samples at high sample rates
- [Phase 02-01]: setLatencySamples() called directly from processBlock on rate change (Crucible pattern); VST3 re-entrancy remains empirical verification item
- [Phase 03-gui]: BinaryData symbol names strip hyphens: CormorantGaramond-Light.ttf -> CormorantGaramondLight_ttf (not CormorantGaramond_Light_ttf)
- [Phase 03-gui]: getLabelFont renamed to getKnobLabelFont to avoid collision with LookAndFeel_V2::getLabelFont(Label&) virtual function
- [Phase 03-gui]: BinaryData.h included as plain header — JUCE adds the JuceLibraryCode include path automatically

### Pending Todos

None.

### Blockers/Concerns

- Phase 2: Latency reporting VST3 re-entrancy — exact guard pattern for preventing concurrent prepareToPlay/processBlock during oversampling rate change needs empirical verification in Nuendo.
- Phase 2: Non-integer IIR oversampling latency — rounding direction for setLatencySamples() and dry delay compensation for fractional sample must be verified empirically at all three rates.
- Phase 1 (ongoing): setLatencySamples() value should be verified empirically in Nuendo before Phase 2 adds selectable oversampling.

## Session Continuity

Last session: 2026-02-23
Stopped at: Completed 03-01-PLAN.md — ClaymoreTheme LookAndFeel + OversamplingSelector + BinaryData asset pipeline
Resume file: None
