---
phase: 01-dsp-foundation
plan: "03"
subsystem: dsp
tags: [juce, cmake, vst3, au, universal-binary, processblock, signal-chain, release-build]

requires:
  - phase: 01-01
    provides: "Complete processBlock wiring with all 11 param reads, SmoothedValue gains, DryWetMixer, ClaymoreEngine, OutputLimiter — all committed in fca99eb"
  - phase: 01-02
    provides: "ClaymoreEngine with hidden gate setters and sidechain HPF, all DSP interfaces complete — committed in 8d6952d"

provides:
  - "Release VST3 binary: build/Claymore_artefacts/Release/VST3/Claymore.vst3 (Universal arm64+x86_64)"
  - "Release AU binary: build/Claymore_artefacts/Release/AU/Claymore.component (Universal arm64+x86_64)"
  - "Verified signal chain: Input Gain -> Dry Capture -> ClaymoreEngine (gate->fuzz->tone) -> Wet Mix -> Output Gain -> OutputLimiter"
  - "Both binaries installed to ~/Library/Audio/Plug-Ins/ via COPY_PLUGIN_AFTER_BUILD"

affects:
  - 02-oversampling (Phase 2 upgrades ClaymoreEngine oversampler — builds on this Release scaffold)
  - 03-gui (Phase 3 replaces GenericAudioProcessorEditor)
  - 04-validation (Phase 4 does auval + DAW load testing on these binaries)

tech-stack:
  added: []
  patterns:
    - "Release build uses juce_recommended_lto_flags for LTO-enabled universal binary"
    - "COPY_PLUGIN_AFTER_BUILD installs automatically to system plugin directories on build"
    - "Xcode CMake generator produces valid VST3 bundle structure (Contents/MacOS/Claymore, Info.plist, PkgInfo)"

key-files:
  created: []
  modified:
    - Source/PluginProcessor.h
    - Source/PluginProcessor.cpp

key-decisions:
  - "processBlock code was pre-complete from Plan 01-01 (fca99eb): no changes needed in Plan 01-03 — verified as correct against all plan must_haves"
  - "CMakeLists.txt needed no changes: ClaymoreEngine is header-only (confirmed in 01-02), no ClaymoreEngine.cpp to add to target_sources"
  - "Release build uses same CMakeLists.txt as Debug: juce_recommended_config_flags enables Release optimizations automatically"

patterns-established:
  - "Task 1 verification: check plan must_haves.key_links patterns against source before rebuilding — avoids unnecessary rebuilds"
  - "Release build path: cmake --build build --config Release --target [Target] (Xcode multi-config generator)"
  - "Universal Binary verification: lipo -info on Contents/MacOS/[binary] confirms arm64+x86_64"

requirements-completed:
  - SIG-01
  - SIG-02
  - SIG-03
  - SIG-04
  - SIG-05

duration: 3min
completed: 2026-02-23
---

# Phase 1 Plan 03: processBlock Wiring and Release Build Summary

**Complete signal chain wired and Release-verified: Input Gain -> Dry Capture -> ClaymoreEngine (gate+fuzz+tone) -> Wet Mix -> Output Gain -> OutputLimiter; Universal Binary VST3 and AU built in Release with LTO**

## Performance

- **Duration:** 3 min
- **Started:** 2026-02-23T17:44:15Z
- **Completed:** 2026-02-23T17:47:42Z
- **Tasks:** 2 of 2
- **Files modified:** 0 (all code pre-complete from Plan 01-01; Release build produces non-tracked artifacts)

## Accomplishments

- Verified processBlock against all plan must_haves: engine.process(), limiter.process(), driveParam->load pattern, all 11 atomic param reads, SmoothedValue on input/output gain, DryWetMixer latency compensation — all confirmed correct
- Release VST3 and AU built successfully: both are Universal Binary (x86_64 + arm64), signed locally, installed to ~/Library/Audio/Plug-Ins/
- Confirmed CMakeLists.txt requires no changes: ClaymoreEngine is header-only (Plan 01-02 decision), already linked via juce_dsp, no extra source file needed

## Task Commits

Each task was committed atomically:

1. **Task 1: Wire processBlock with complete signal chain** - `fca99eb` (feat, Plan 01-01) — pre-committed; verified correct, no changes needed
2. **Task 2: Build Release, install plugin, verify binaries** — Release build produces non-tracked artifacts; no new source files

**Plan metadata:** (docs commit — to follow)

## Files Created/Modified

- `Source/PluginProcessor.h` - ClaymoreEngine, OutputLimiter, DryWetMixer<float>{64}, SmoothedValue input/output gain smoothers as private members; isInitialized guard; 11 cached atomic param pointers
- `Source/PluginProcessor.cpp` - processBlock: guard -> all param reads -> input gain (SmoothedValue) -> dryWetMixer.pushDry -> engine.setDrive/Symmetry/Tightness/Sag/Tone/Presence/GateEnabled/GateThreshold -> engine.process -> dryWetMixer.mixWet -> output gain (SmoothedValue) -> outputLimiter.process

## Decisions Made

- **processBlock pre-complete from Plan 01-01:** The 01-01 execution wrote the complete signal chain wiring (not just a stub). Plan 01-03 verified the existing code meets all must_have requirements without modification. No scope creep.
- **CMakeLists.txt unchanged:** ClaymoreEngine.cpp does not exist (header-only, per 01-02 decision). target_sources already correct with only PluginProcessor.cpp and PluginEditor.cpp.

## Deviations from Plan

None — plan executed exactly as written. processBlock and CMakeLists.txt were already correct from prior plans. Release builds for both VST3 and AU succeeded on first attempt with no errors. Universal Binary confirmed for both targets.

## Issues Encountered

None. Both Release targets compiled clean on first attempt. lipo confirms Universal Binary (x86_64 arm64) for both VST3 and AU bundles.

## User Setup Required

None — binaries install automatically via COPY_PLUGIN_AFTER_BUILD to:
- VST3: `~/Library/Audio/Plug-Ins/VST3/Claymore.vst3`
- AU: `~/Library/Audio/Plug-Ins/Components/Claymore.component`

## Next Phase Readiness

- Phase 1 (DSP Foundation) complete: all three plans executed, Release binaries produced and installed
- Phase 4 (Validation) can run `auval -v aufx Clyr NMcM` against the installed AU binary
- Blocker (carried from 01-01): setLatencySamples() value should be verified empirically in Nuendo before Phase 2 adds selectable oversampling
- Phase 2 (Oversampling) begins with ClaymoreEngine already having complete DSP interface — only needs oversampler array upgrade

---
*Phase: 01-dsp-foundation*
*Completed: 2026-02-23*

## Self-Check: PASSED

All source files (PluginProcessor.h, PluginProcessor.cpp) found on disk. Task commits fca99eb and 8d6952d verified in git log. Release VST3 bundle and AU component exist in build artefacts. Both are Universal Binary (x86_64 arm64) per lipo. SUMMARY.md created.
