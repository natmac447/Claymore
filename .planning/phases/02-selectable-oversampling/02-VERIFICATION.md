---
phase: 02-selectable-oversampling
verified: 2026-02-23T19:00:00Z
status: passed
score: 5/5 must-haves verified
re_verification: false
human_verification:
  - test: "Switch oversampling from 2x to 4x to 8x mid-playback in Nuendo/Logic/Reaper"
    expected: "No audio dropout, glitch, or DAW crash; playback continues uninterrupted"
    why_human: "Audio-thread behavior and host re-entrancy on setLatencySamples cannot be verified by static analysis"
  - test: "In a multi-track session, process one track at 2x then switch to 8x while playing"
    expected: "PDC-compensated dry track stays time-aligned with processed wet track — no pre-echo or timing offset"
    why_human: "DAW PDC re-calculation on latency change is host-specific runtime behavior"
  - test: "Set Mix to 50%, null-test dry against wet at each oversampling rate (2x, 4x, 8x)"
    expected: "Summing dry + inverted wet produces silence (null test passes) at all three rates — DryWetMixer latency compensation is correct"
    why_human: "Phase alignment of DryWetMixer internal delay line requires real audio path execution; latency value correctness verified by code but timing must be confirmed empirically"
---

# Phase 2: Selectable Oversampling Verification Report

**Phase Goal:** User can select 2x, 4x, or 8x oversampling with correct latency reported to the DAW at all times
**Verified:** 2026-02-23T19:00:00Z
**Status:** passed
**Re-verification:** No — initial verification

---

## Goal Achievement

### Observable Truths

| # | Truth | Status | Evidence |
|---|-------|--------|----------|
| 1 | User can select oversampling rate (2x, 4x, 8x) as a DAW-automatable parameter | VERIFIED | `Parameters.h:143` — `AudioParameterChoice` with `StringArray { "2x", "4x", "8x" }`, default index 0; `ParamIDs::oversampling` defined at line 34 |
| 2 | Switching oversampling rate mid-playback does not cause audio dropout, glitch, or DAW crash | VERIFIED (code) | `PluginProcessor.cpp:139-150` — per-block detection; `setOversamplingFactor` performs no allocation (`make_unique` only in `prepare()` at `ClaymoreEngine.h:43`); stale state cleared via `oversamplingObjects[old]->reset()` at `ClaymoreEngine.h:219` |
| 3 | Plugin reports correct latency to DAW at all three oversampling rates | VERIFIED | `PluginProcessor.cpp:78` — `setLatencySamples(static_cast<int>(std::round(engine.getLatencyInSamples())))` in `prepareToPlay`; `PluginProcessor.cpp:148` — same pattern on per-block rate change; `getLatencyInSamples()` at `ClaymoreEngine.h:192` delegates to active `oversamplingObjects[currentOversamplingIndex]` |
| 4 | Dry/wet mix remains phase-aligned after switching oversampling rates | VERIFIED (code) | `PluginProcessor.cpp:147` — `dryWetMixer.setWetLatency(newLatency)` called immediately after rate change; `PluginProcessor.h:97` — `dryWetMixer { 256 }` capacity confirmed sufficient for 8x IIR (~60 samples max) |
| 5 | Parameter smoothers operate at the correct oversampled rate after a rate switch | VERIFIED | `ClaymoreEngine.h:224-233` — `setOversamplingFactor` recalculates `newOsRate = sampleRate * pow(2.0, currentOversamplingIndex + 1)` and resets all four smoothers (`driveSmoother`, `symmetrySmoother`, `tightnessSmoother`, `sagSmoother`) plus re-prepares `coreState[ch]` at the new rate |

**Score:** 5/5 truths verified

---

### Required Artifacts

| Artifact | Expected | Status | Details |
|----------|----------|--------|---------|
| `Source/Parameters.h` | `ParamIDs::oversampling` ID + `AudioParameterChoice` (2x/4x/8x, default 0) | VERIFIED | Line 34: `oversampling = "oversampling"`; Lines 143-148: `AudioParameterChoice` with correct `StringArray` and default index 0 |
| `Source/dsp/ClaymoreEngine.h` | Pre-allocated array of 3 `Oversampling` objects, `setOversamplingFactor()`, updated `getLatencyInSamples()` | VERIFIED | Line 374: `std::array<unique_ptr<Oversampling<float>>, 3> oversamplingObjects`; Lines 212-234: `setOversamplingFactor()`; Lines 190-193: `getLatencyInSamples()` returning from active index; allocation only in `prepare()` loop lines 41-51 |
| `Source/PluginProcessor.h` | `oversamplingParam` pointer, `lastOversamplingIndex` tracker, `dryWetMixer { 256 }` | VERIFIED | Line 89: `std::atomic<float>* oversamplingParam = nullptr`; Line 72: `int lastOversamplingIndex = 0`; Line 97: `dryWetMixer { 256 }` with comment documenting 8x headroom |
| `Source/PluginProcessor.cpp` | Per-block rate-change detection, `setOversamplingFactor` call, latency re-sync | VERIFIED | Line 31: constructor caches `oversamplingParam`; Lines 64-67: `prepareToPlay` applies initial index; Lines 139-150: rate-change detection block calling `engine.setOversamplingFactor`, `dryWetMixer.setWetLatency`, and `setLatencySamples` with `std::round` |

---

### Key Link Verification

| From | To | Via | Status | Details |
|------|----|-----|--------|---------|
| `PluginProcessor.cpp` | `ClaymoreEngine::setOversamplingFactor()` | `engine.setOversamplingFactor(newOversamplingIndex)` in per-block detection | WIRED | `PluginProcessor.cpp:144` — called when `newOversamplingIndex != lastOversamplingIndex`; also `line 66` in `prepareToPlay` for saved state |
| `PluginProcessor.cpp` | `dryWetMixer.setWetLatency` | Re-sync DryWetMixer latency after rate change using `engine.getLatencyInSamples()` | WIRED | `PluginProcessor.cpp:147` — `dryWetMixer.setWetLatency(newLatency)` where `newLatency = engine.getLatencyInSamples()`; also line 75 in `prepareToPlay` |
| `PluginProcessor.cpp` | `setLatencySamples` | Report new latency to DAW host with `std::round` | WIRED | `PluginProcessor.cpp:148` — `setLatencySamples(static_cast<int>(std::round(newLatency)))`; also line 78 in `prepareToPlay` |
| `ClaymoreEngine::process()` | `oversamplingObjects[currentOversamplingIndex]` | `auto& os = *oversamplingObjects[currentOversamplingIndex]` selects active stage | WIRED | `ClaymoreEngine.h:110-111` — reference `os` used for both `processSamplesUp` (line 111) and `processSamplesDown` (line 160) |

---

### Requirements Coverage

| Requirement | Source Plan | Description | Status | Evidence |
|-------------|-------------|-------------|--------|----------|
| QUAL-01 | 02-01-PLAN.md | User can select oversampling rate (2x, 4x, 8x) to trade CPU for quality | SATISFIED | `AudioParameterChoice` registered in APVTS (`Parameters.h:143`); parameter is DAW-automatable via APVTS; per-block detection wires selection to engine |
| QUAL-02 | 02-01-PLAN.md | Plugin reports correct latency to DAW for all oversampling rates | SATISFIED | `setLatencySamples` called with `std::round(engine.getLatencyInSamples())` both at init (`prepareToPlay:78`) and on every rate switch (`processBlock:148`); `getLatencyInSamples()` returns from the currently active `oversamplingObjects[currentOversamplingIndex]` |

No orphaned requirements found. REQUIREMENTS.md traceability table correctly marks both QUAL-01 and QUAL-02 as `Complete (02-01)`.

---

### Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
|------|------|---------|----------|--------|
| `PluginProcessor.h` | 53 | `return {}` in `getProgramName` | Info | Standard JUCE boilerplate for single-preset plugins; not related to phase changes |

No TODO, FIXME, placeholder comments, or stub implementations found in any of the four modified files. The `return {}` on line 53 is standard AudioProcessor boilerplate for a plugin with no program management — no impact on phase goal.

---

### Human Verification Required

The following items require empirical testing in a real DAW. Automated checks confirm correct code structure and call ordering; runtime behavior must be validated by a human.

#### 1. Mid-Playback Oversampling Switch (No Dropout or Crash)

**Test:** Load Claymore on an audio track in Nuendo, Logic, or Reaper. Start playback with looping audio. Switch oversampling from 2x to 4x to 8x using the parameter (automate or manual). Repeat in the opposite direction.
**Expected:** Playback continues with no audio dropout, glitch, pop, or DAW crash at any transition.
**Why human:** `setLatencySamples()` is called directly from `processBlock` on rate change (Crucible pattern). VST3 re-entrancy behavior of this call is host-specific and cannot be verified by static analysis. SUMMARY.md documents this as a flagged empirical concern.

#### 2. PDC Remains Correct After Rate Switch (No Pre-Echo or Timing Offset)

**Test:** In a multi-track session, route the same audio to two tracks. Place Claymore on one. Bounce or monitor both. Switch oversampling mid-session. Align and compare.
**Expected:** After the switch, the Claymore-processed track remains time-aligned with the direct track. No pre-echo (PDC under-compensation) or timing offset (PDC over-compensation).
**Why human:** DAW PDC recalculation in response to `setLatencySamples()` during playback is host-specific. The rounding direction (`std::round` vs. floor/ceil) matters for non-integer IIR latencies; only a null-test in a running session can confirm correctness.

#### 3. Dry/Wet Phase Alignment Null Test at All Three Rates (Success Criterion 3)

**Test:** Set Mix to 50%. Record the plugin output. Invert the wet signal and sum with dry. Perform a null test at 2x, 4x, and 8x.
**Expected:** Null test passes (sum produces silence or very near silence) at all three oversampling rates — confirming `DryWetMixer::setWetLatency` correctly compensates the dry path delay at each rate.
**Why human:** The DryWetMixer internal delay line is only correctly sized if the latency value returned by `getLatencyInSamples()` is accurate for the IIR filter at each oversampling factor. The code path is verified correct, but the numerical accuracy of JUCE's IIR oversampling latency reporting at 4x and 8x requires runtime confirmation.

---

### Build Verification

Both Release targets confirmed built successfully during this verification session:

- `Claymore_VST3`: `** BUILD SUCCEEDED **` — installed to `/Users/nathanmcmillan/Library/Audio/Plug-Ins/VST3/Claymore.vst3`
- `Claymore_AU`: `** BUILD SUCCEEDED **` — installed to `/Users/nathanmcmillan/Library/Audio/Plug-Ins/Components/Claymore.component`

---

### Commits Verified

Both task commits documented in SUMMARY.md exist in git history:

- `64451a6` — Task 1: Parameters.h + ClaymoreEngine.h (81 insertions, 20 deletions — substantive)
- `857560f` — Task 2: PluginProcessor.h + PluginProcessor.cpp (34 insertions, 4 deletions — substantive)

---

### Summary

All five observable truths are verified against the actual codebase. No stubs, no orphaned artifacts, and no broken key links were found. The four required artifacts are substantive, correctly wired to each other, and the complete call chain from APVTS parameter through engine switching through DAW latency reporting is end-to-end connected.

The three human verification items correspond directly to the three phase success criteria stated in the roadmap. They cannot be resolved by static analysis but the code structure supporting each is fully in place.

---

_Verified: 2026-02-23T19:00:00Z_
_Verifier: Claude (gsd-verifier)_
