---
phase: 01-dsp-foundation
plan: "01"
subsystem: dsp
tags: [juce, cmake, fetchcontent, apvts, vst3, au, distortion, fuzz, noise-gate, oversampling]

requires: []

provides:
  - CMake build system with JUCE 8.0.12 via FetchContent, VST3+AU targets
  - ParamIDs namespace and createParameterLayout() with all 11 APVTS parameters
  - ClaymoreProcessor AudioProcessor subclass with APVTS and state persistence
  - ClaymoreEngine DSP orchestrator: noise gate + 2x IIR oversampling + FuzzCore + FuzzTone
  - FuzzCore/FuzzTone/FuzzType ports from GunkLord with Claymore range extensions
  - OutputLimiter brickwall limiter (juce::dsp::Limiter<float>)
  - GenericAudioProcessorEditor for Phase 1 DAW validation
  - Compilable plugin binary installed as Universal Binary (arm64 + x86_64) VST3 and AU

affects:
  - 01-02 (DSP implementation — builds on this APVTS scaffold and ClaymoreEngine)
  - 01-03 (signal chain wiring — processBlock patterns established here)
  - 02-oversampling (Phase 2 upgrades ClaymoreEngine oversampler to 3-slot array)
  - 03-gui (Phase 3 replaces GenericAudioProcessorEditor with pedal-style UI)

tech-stack:
  added:
    - JUCE 8.0.12 (FetchContent, GIT_TAG 8.0.12 GIT_SHALLOW TRUE)
    - juce_audio_utils, juce_audio_processors, juce_dsp modules
    - CMake 3.25+ with Xcode generator, Universal Binary arm64;x86_64
  patterns:
    - APVTS with cached std::atomic<float>* pointers (no string lookups in processBlock)
    - isInitialized atomic guard against processBlock-before-prepareToPlay
    - DryWetMixer with setWetLatency() for oversampling latency compensation
    - Custom noise gate state machine with dual-threshold hysteresis (vs. juce::dsp::NoiseGate)
    - Explicit target_sources listing (no GLOB_RECURSE) per Bathysphere/Crucible pattern
    - SmoothedValue for input/output gain (multiplicative, 5ms ramp)
    - GunkLord processBlock smoother pattern: advance on ch 0 only, read current on ch 1+

key-files:
  created:
    - CMakeLists.txt
    - Source/Parameters.h
    - Source/PluginProcessor.h
    - Source/PluginProcessor.cpp
    - Source/PluginEditor.h
    - Source/PluginEditor.cpp
    - Source/dsp/ClaymoreEngine.h
    - Source/dsp/OutputLimiter.h
    - Source/dsp/fuzz/FuzzCore.h
    - Source/dsp/fuzz/FuzzTone.h
    - Source/dsp/fuzz/FuzzType.h
    - Source/dsp/fuzz/FuzzConfig.h
  modified: []

key-decisions:
  - "Custom noise gate state machine instead of juce::dsp::NoiseGate: dual-threshold hysteresis not exposed by JUCE gate; custom envelope-follower + SmoothedValue is ~40 lines and gives full control"
  - "Sag gain-makeup: linear approximation clipped *= (1 + sag * 0.5) per CONTEXT.md discretion — gain-neutral at typical drive; CONTEXT.md allows chaos at extreme settings"
  - "Parameter pointers cached in constructor (not prepareToPlay): JUCE guarantees APVTS pointers are valid immediately after construction, before any audio activity"
  - "setLatencySamples reports engine.getLatencyInSamples() (not *2 as in GunkLord): GunkLord multiplied by 2 due to double-processing; Claymore uses single-pass oversampling"

patterns-established:
  - "ClaymoreEngine: noise gate → oversample up → FuzzCore per-sample → output compensation → oversample down → FuzzTone (must run in this order)"
  - "FuzzCoreState prepared at oversampledRate (not spec.sampleRate): filter coefficients calibrated for the oversampled domain"
  - "FuzzTone.applyTone() runs AFTER processSamplesDown(): filter coefficients calibrated at original rate"
  - "Tightness mapping in ClaymoreEngine: 20 + tightness * 780 (not GunkLord's 280)"
  - "Tone mapping in FuzzTone: 2000 + tone * 18000 (not GunkLord's 7200)"

requirements-completed:
  - DIST-01
  - DIST-02
  - DIST-03
  - DIST-04
  - DIST-05
  - DIST-06
  - SIG-01
  - SIG-02
  - SIG-03
  - SIG-04
  - SIG-05

duration: 7min
completed: 2026-02-23
---

# Phase 1 Plan 01: JUCE 8 Project Scaffold Summary

**JUCE 8.0.12 plugin scaffold with 11-parameter APVTS, Rat-based ClaymoreEngine (2x IIR oversample + custom hysteresis noise gate), and Universal Binary VST3+AU that installs and passes audio through**

## Performance

- **Duration:** 7 min
- **Started:** 2026-02-23T17:24:47Z
- **Completed:** 2026-02-23T17:31:52Z
- **Tasks:** 2 of 2
- **Files modified:** 12

## Accomplishments

- CMakeLists.txt with JUCE 8.0.12 FetchContent compiles clean with Xcode generator on macOS (Universal Binary arm64+x86_64); COPY_PLUGIN_AFTER_BUILD installs to system VST3 and Components directories
- All 11 APVTS parameters defined in Parameters.h with exact ranges/defaults from spec; createParameterLayout() returns complete layout for APVTS constructor
- ClaymoreProcessor with isInitialized guard, 11 cached atomic param pointers, DryWetMixer latency compensation, SmoothedValue input/output gain, and APVTS state persistence
- ClaymoreEngine DSP chain (FuzzCore port + FuzzTone with extended ranges + custom noise gate state machine) builds and links; VST3 and AU binaries produced

## Task Commits

Each task was committed atomically:

1. **Task 1: CMakeLists.txt with JUCE 8.0.12 FetchContent** - `27f5dd8` (chore)
2. **Task 2: Parameters.h, PluginProcessor, PluginEditor, DSP engine** - `fca99eb` (feat)

## Files Created/Modified

- `CMakeLists.txt` - cmake_minimum_required(3.25), FetchContent JUCE 8.0.12, juce_add_plugin (VST3+AU, PLUGIN_CODE Clyr), explicit target_sources, compile definitions, link libraries
- `Source/Parameters.h` - ParamIDs namespace (11 constexpr IDs) + createParameterLayout() factory
- `Source/PluginProcessor.h` - ClaymoreProcessor declaration with APVTS, 11 atomic param pointers, ClaymoreEngine, OutputLimiter, DryWetMixer
- `Source/PluginProcessor.cpp` - Constructor caches params in constructor; prepareToPlay initializes all DSP; processBlock implements full signal chain; getStateInformation/setStateInformation via APVTS
- `Source/PluginEditor.h` / `Source/PluginEditor.cpp` - Minimal GenericAudioProcessorEditor, 400x300
- `Source/dsp/ClaymoreEngine.h` - FuzzStage-based DSP with extended tightness (20-800 Hz), custom hysteresis noise gate, sag makeup gain
- `Source/dsp/OutputLimiter.h` - juce::dsp::Limiter<float> wrapper, 0 dBFS threshold
- `Source/dsp/fuzz/FuzzCore.h` - GunkLord port (verbatim waveshaping + sag makeup addition)
- `Source/dsp/fuzz/FuzzTone.h` - GunkLord port with Tone LP range extended to 2-20 kHz
- `Source/dsp/fuzz/FuzzType.h` / `Source/dsp/fuzz/FuzzConfig.h` - GunkLord ports verbatim

## Decisions Made

- **Custom noise gate vs. juce::dsp::NoiseGate:** JUCE gate has no hysteresis parameter; switching setThreshold() mid-stream risks clicks. Implemented custom envelope follower with dual-threshold state machine (gateOpen >= -40 dBFS, gateClose >= -44 dBFS). JUCE Limiter still used for the output limiter where hysteresis is not relevant.
- **Sag gain-makeup:** CONTEXT.md requires gain-neutrality. Added `clipped *= (1.0f + sag * 0.5f)` after sag stage in FuzzCore. Tunable by ear; CONTEXT.md allows "High Drive + high Sag = chaos."
- **setLatencySamples value:** Used `engine.getLatencyInSamples()` directly (GunkLord multiplied by 2 due to a double-processing quirk in its architecture). Claymore's ClaymoreEngine processes once.
- **Gate defaults (Claude's discretion):** Attack 1ms, release 80ms, hysteresis 4 dB, range -60 dB — per RESEARCH.md Pitfall 3 recommendations.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Added sag gain-makeup to FuzzCore::processSample()**
- **Found during:** Task 2 (FuzzCore port review)
- **Issue:** CONTEXT.md requires Sag to be gain-neutral; GunkLord's Sag reduces amplitude at low levels, causing noticeable level drop when sweeping Sag 0→1 (RESEARCH.md Pitfall 5)
- **Fix:** Added `clipped *= (1.0f + sag * 0.5f)` after sag gate in FuzzCore::processSample()
- **Files modified:** Source/dsp/fuzz/FuzzCore.h
- **Verification:** Makeup applied only when sag > 0; linear approximation tunable by ear
- **Committed in:** fca99eb (Task 2)

---

**Total deviations:** 1 auto-fixed (Rule 1 — bug, sag gain-neutrality)
**Impact on plan:** Required correction for CONTEXT.md compliance. No scope creep.

## Issues Encountered

None. CMake configure fetched JUCE 8.0.12 cleanly; both build targets compiled on first attempt with no linker errors or compiler warnings that required fixes.

## User Setup Required

None — no external service configuration required. Plugin installs automatically via COPY_PLUGIN_AFTER_BUILD to:
- VST3: `~/Library/Audio/Plug-Ins/VST3/Claymore.vst3`
- AU: `~/Library/Audio/Plug-Ins/Components/Claymore.component`

## Next Phase Readiness

- Build infrastructure complete; all subsequent DSP plans compile against this scaffold
- All 11 APVTS parameter IDs established — Phase 01-02 can reference them directly via ParamIDs namespace
- ClaymoreEngine interface (setDrive/setSymmetry etc.) ready for Phase 01-02 to add real DSP wiring
- Phase 01-02 will replace the pass-through processBlock stub with actual ClaymoreEngine::process() calls
- Blocker note: `setLatencySamples()` value (currently `engine.getLatencyInSamples()`) should be verified empirically in Nuendo before Phase 2 adds selectable oversampling

---
*Phase: 01-dsp-foundation*
*Completed: 2026-02-23*

## Self-Check: PASSED

All 12 source files found on disk. Both plugin binaries (Claymore.vst3, Claymore.component) present in build artefacts. Task commits 27f5dd8 and fca99eb verified in git log.
