# Project Research Summary

**Project:** Claymore
**Domain:** JUCE 8 distortion audio plugin — Rat circuit port (VST3 + AU, macOS)
**Researched:** 2026-02-23
**Confidence:** HIGH

## Executive Summary

Claymore is a focused distortion plugin for mixing engineers, built by porting the Rat-circuit DSP from Gunk Lord into a standalone JUCE 8 plugin with a pedal-style GUI and selectable oversampling. The recommended approach is direct: the four core DSP files (FuzzCore.h, FuzzTone.h, FuzzConfig.h, FuzzType.h) are verbatim-portable from Gunk Lord with no algorithm changes required. The principal structural work is replacing Gunk Lord's hardcoded 2x oversampling with a pre-allocated array of three `juce::dsp::Oversampling<float>` objects (2x/4x/8x), wrapped in a new `ClaymoreEngine` class. The JUCE 8 CMake pattern, FetchContent setup, and LookAndFeel conventions all have direct precedents in the developer's existing projects (Bathysphere, Crucible).

The competitive opportunity is clear: existing Rat emulations (bx_blackdist2, Siberian Hamster) are positioned as guitar pedal emulations and expose only pedal-verbatim controls. Claymore's differentiators — mixing-tool framing, descriptive parameter names, Symmetry/Sag/Tightness controls, built-in noise gate, and selectable oversampling — are all achievable within the ported DSP and require no new circuit modeling. Every differentiating feature is either directly exposed from existing Gunk Lord parameters or is a thin wrapper (pre-distortion HPF for Tightness, IIR highpass for DC block, `juce::dsp::Limiter` for brickwall output).

The critical risks are concentrated in the oversampling implementation. Three of the four critical pitfalls map directly to oversampling: audio-thread allocation on rate switch, latency reporting re-entrancy in VST3 hosts, and dry/wet phase misalignment. All three are known, documented JUCE issues with established solutions. The pre-allocated-oversampler array pattern (borrowed from Crucible's `ClipperSection`) resolves the first; calling `setLatencySamples()` only at block boundaries (not inside `prepareToPlay()`) addresses the second; `juce::dsp::DryWetMixer` with `setWetLatency()` resolves the third. Addressing all three requires deliberate design at DSP port time — they cannot be patched in later without regression risk.

---

## Key Findings

### Recommended Stack

The stack is fully specified and consistent with the developer's existing projects. JUCE 8.0.12 (bumped from 8.0.4 used in Bathysphere/Crucible) is the correct version — no breaking changes affect the ported DSP, and 8.0.12 includes VST3 parameter migration support and 8 months of post-8.0.4 fixes. CMake 3.25 + FetchContent + C++17 matches Gunk Lord's CMake floor and keeps the port frictionless. Four JUCE modules are required; all others should be omitted to minimize compile time.

The oversampling implementation pattern is established: `std::array<juce::dsp::Oversampling<float>, 3>` pre-allocated at `prepareToPlay()`, selected by atomic index at block boundaries. This is the exact pattern in JUCE's official `DSPModulePluginDemo.h` and is already used in Crucible. The filter type (`filterHalfBandPolyphaseIIR`) must match Gunk Lord — it provides ~4 sample latency at 2x vs. ~60-120 samples for FIR, which is mandatory for real-time monitoring usability.

**Core technologies:**
- JUCE 8.0.12: Audio plugin framework (DSP + GUI + plugin formats) — current stable, no breaking changes vs. ported DSP
- CMake 3.25+: Build system — matches Gunk Lord's CMake floor; FetchContent for reproducible builds
- C++17: Language standard — matches Gunk Lord; ported DSP uses no C++20 features
- `filterHalfBandPolyphaseIIR`: Oversampling filter type — ~4 sample latency vs. 60-120 for FIR; essential for real-time use
- Custom `LookAndFeel_V4` subclass: GUI — procedural rotary drawing, no image assets, consistent with Bathysphere/Crucible

### Expected Features

All features in the v1 feature set are either direct ports from Gunk Lord (Drive, Symmetry, Sag, Tone), thin wrappers on JUCE DSP primitives (Tightness = pre-distortion HPF, Brickwall Limiter = `juce::dsp::Limiter`, Noise Gate = `juce::dsp::NoiseGate`), or standard JUCE infrastructure (APVTS parameter automation, state persistence, DAW latency reporting). No v1 feature requires new circuit modeling. The competitor analysis confirms that Dry/Wet, Noise Gate, Symmetry, Sag, Tightness, and mixing-tool framing are absent from all direct Rat emulation competitors — the differentiation is real and achievable.

**Must have (table stakes):**
- Drive / Distortion + Input Gain + Output Gain — core gain staging workflow; expected by every mixing engineer
- Tone + Presence — post-distortion tonal shaping; distortion is unusable without it
- Dry/Wet Mix — expected by mixing engineers; absent in bx_blackdist2 and Siberian Hamster (competitive gap)
- Noise Gate with Threshold — required for high-gain distortion; no competing Rat plugin has one
- Selectable Oversampling (2x/4x/8x) + DAW Latency Reporting — quality/CPU tradeoff; latency reporting is a correctness requirement
- Brickwall Output Limiter — prevents user-visible digital overs; required for professional use
- VST3 + AU formats — Logic requires AU; all major macOS DAWs require VST3

**Should have (competitive differentiators):**
- Symmetry control — exposes clipping asymmetry; no competing Rat plugin does this
- Sag / bias-starve — models power supply droop; genuinely distinctive character parameter; no competition
- Tightness — pre-distortion HPF enabling non-guitar use; expands addressable market
- Descriptive mixing-oriented parameter names — clearest UX differentiator vs. pedal-verbatim competitors
- Presence shelf — post-distortion air recovery; no competing Rat plugin exposes this

**Defer (v1.x / v2+):**
- Factory presets — requires stable DSP and sound design time; add post-launch
- Auto Gain Compensation — quality-of-life for A/B evaluation; well-regarded but not blocking
- CLAP format — future addition; JUCE 9 upcoming
- Resizable GUI, Windows/Linux builds — v2+ scope

### Architecture Approach

The architecture is a standard JUCE plugin pattern with one significant structural addition: `ClaymoreEngine` replaces Gunk Lord's `FuzzStage` as the main DSP wrapper, adding selectable oversampling and exposing the oversampling index as an atomic for safe cross-thread switching. The four ported fuzz files are unchanged in a `dsp/fuzz/` subdirectory. `PluginProcessor` owns `ClaymoreEngine`, a `juce::dsp::DryWetMixer`, and a `juce::dsp::Limiter`. `PluginEditor` communicates exclusively through APVTS — it never touches the engine directly. All persistent state, including the oversampling rate, lives in the APVTS ValueTree.

The build order respects hard dependencies: `Parameters.h` (APVTS layout) must exist before either processor or editor; the fuzz port must compile before `ClaymoreEngine`; all DSP must be functional before GUI work begins. This is the correct phase structure for the roadmap.

**Major components:**
1. `PluginProcessor` — JUCE host interface; APVTS ownership; processBlock signal routing; latency reporting
2. `ClaymoreEngine` — complete distortion signal chain; owns three pre-allocated oversamplers; exposes atomic index for thread-safe rate switching
3. `dsp/fuzz/` (FuzzCore, FuzzTone, FuzzConfig, FuzzType) — verbatim port from Gunk Lord; no logic changes; isolated in subdirectory
4. `OutputLimiter` — thin wrapper around `juce::dsp::Limiter<float>`; last in signal chain
5. `PluginEditor` + `ClaymoreKnob` + `ClaymoreColours` — pedal-style GUI; APVTS attachments only; custom `LookAndFeel_V4`
6. `Parameters.h` — central APVTS parameter ID constants; prevents string drift between processor and editor

### Critical Pitfalls

1. **Oversampling rate change on audio thread** — Pre-allocate all three oversamplers at `prepareToPlay()` time; switch by atomic index at block boundary only; never call `initProcessing()` or reconstruct inside `processBlock()`. Recovery if discovered late: 1-2 days.

2. **Latency reporting triggers second prepareToPlay in VST3 hosts** — Call `setLatencySamples()` only at block boundaries when the oversampling index actually changes, not on every `prepareToPlay()` call. Add an `isInitialized` guard that outputs silence until `prepareToPlay()` completes. Verify in both Reaper and Nuendo. Recovery if discovered late: 2-4 days.

3. **Dry/wet phase misalignment at mid mix values** — Use `juce::dsp::DryWetMixer` with `setWetLatency(engine.getLatencyInSamples())` called in `prepareToPlay()` and on every oversampling rate change. Verify with null test at 50% mix. Must be designed in from the start. Recovery if discovered late: 1 day but requires regression testing.

4. **Noise gate post-distortion suppresses sustain** — Gate must be placed pre-distortion (before the oversampling upsample block). The gate sees the clean input signal and accurately detects silence. Signal chain order: Gate → Upsample → FuzzCore → Downsample → FuzzTone. Verify: maximum Gain + Sag, silence input, gate closes without cutting musical sustain.

5. **Hidden Gunk Lord include dependencies in ported files** — Before copying any file, audit all transitive `#include` chains. Perform a clean build of Claymore without Gunk Lord on the include path immediately after the port. A CI build from a clean directory catches this; local builds will silently succeed due to shared include paths.

---

## Implications for Roadmap

Based on research, the architecture's build-order dependency chain maps directly to a 4-phase roadmap. The primary constraint is: no GUI work before DSP is validated; no distribution work before plugin validation passes.

### Phase 1: Project Foundation and DSP Port

**Rationale:** All subsequent work depends on a compiling CMake project and correctly ported DSP files. This phase has the highest density of pitfalls that cannot be patched later (include coupling, DC offset, noise gate placement, signal chain order, state persistence). The fuzz files are verbatim-portable but must be verified with a clean build before assuming correctness.

**Delivers:** Compiling JUCE 8 CMake project with APVTS parameter layout, ported fuzz DSP (FuzzCore/FuzzTone/FuzzConfig/FuzzType), `ClaymoreEngine` with fixed 2x oversampling (upgrade to selectable in Phase 2), `PluginProcessor` with processBlock signal chain, output limiter, state persistence, and an `isInitialized` guard. Plugin loads in at least one DAW and produces audio.

**Addresses:** Drive, Input Gain, Output Gain, Tone, Presence, Symmetry, Sag, Tightness, Dry/Wet Mix, Noise Gate (pre-distortion), Brickwall Limiter

**Avoids:** Hidden Gunk Lord include coupling (clean build verification); DC offset (IIR highpass post-FuzzStage); noise gate placement (pre-distortion); processBlock-before-prepareToPlay crash (isInitialized guard); state restore failure (all state in APVTS)

### Phase 2: Selectable Oversampling and Latency Reporting

**Rationale:** Oversampling is the highest-risk technical component — three of the four critical pitfalls concentrate here. It is architecturally isolated to `ClaymoreEngine`, so it can be added cleanly after Phase 1 proves the signal chain works at fixed 2x. Separating this from the initial port reduces the surface area of concurrent unknowns.

**Delivers:** `ClaymoreEngine` upgraded to `std::array<unique_ptr<dsp::Oversampling<float>>, 3>` pre-allocated at `prepareToPlay()`; atomic index for thread-safe rate switching at block boundaries; `setLatencySamples()` called only on rate change; `juce::dsp::DryWetMixer` with `setWetLatency()` for phase-correct dry/wet; oversampling selector wired to APVTS parameter. All switching verified with null test and audio profiler.

**Uses:** `juce::dsp::Oversampling<float>` (3-object pre-allocated array pattern from Crucible/DSPModulePluginDemo); `juce::dsp::DryWetMixer`; `filterHalfBandPolyphaseIIR`

**Avoids:** Audio-thread allocation on rate switch; latency reporting VST3 re-entrancy; dry/wet phase misalignment; dsp::Oversampling stage count vs. factor confusion

### Phase 3: GUI and Plugin Polish

**Rationale:** GUI work requires stable DSP — knobs that don't connect to working DSP provide no feedback for iteration. With Phase 1 and Phase 2 complete, the APVTS parameter layout is locked and APVTS attachments in the editor will bind to real, tested parameters. This phase is lower-risk: procedural `LookAndFeel_V4` pattern is well-established in Bathysphere and Crucible.

**Delivers:** Pedal-style `PluginEditor` with custom `LookAndFeel_V4`, `ClaymoreKnob` components for all 9 parameters, oversampling selector (ComboBox or button group), bypass button, HiDPI-ready software rendering. Plugin is fully usable in a DAW session with correct visual feedback.

**Addresses:** All UX pitfalls (visual indication of active oversampling rate, knob drag sensitivity, HiDPI scaling); mixing-tool parameter naming

**Avoids:** OpenGL overhead (procedural drawing only); image-strip knob fragility; fixed small window on HiDPI displays (test on Retina)

### Phase 4: Validation and Distribution

**Rationale:** Plugin validation (`pluginval`, `auval`) and code signing/notarization are final gates, not ongoing tasks. Running them continuously during development creates noise; running them once before shipping means late discovery of AU listener cleanup bugs. The correct cadence is: run `pluginval` at strictness 5 at the end of each earlier phase, gate distribution on full pass.

**Delivers:** AU/VST3 plugin passing `auval -v aufx NMcM Clyr` and `pluginval` at strictness 5; Universal Binary verified with `lipo -info`; code-signed and notarized for macOS distribution; latency PDC verified in multi-track session; all items on "Looks Done But Isn't" checklist confirmed.

**Avoids:** AU validation failures from listener cleanup bugs (APVTS listener removeListener in all Component destructors); duplicate editor assertion (SafePointer guard); session state restore failures (test in Logic AND Reaper)

### Phase Ordering Rationale

- Foundation before oversampling: the three-oversampler pattern requires a working `ClaymoreEngine` to upgrade; building Phase 2 atop a broken Phase 1 multiplies debugging surface area.
- Oversampling before GUI: the oversampling selector must bind to a working APVTS int parameter; the null test that validates Phase 2 requires the dry/wet mix to work correctly — both must be verified in code, not just wired in the editor.
- GUI before distribution: `pluginval` and `auval` exercise the editor lifecycle; the editor must be complete before validation is meaningful.
- The build order from ARCHITECTURE.md (Parameters.h → fuzz/ → ClaymoreEngine → PluginProcessor → OutputLimiter → validate → GUI → CMake delivery) maps directly onto this 4-phase structure.

### Research Flags

Phases likely needing deeper research during planning:
- **Phase 2 (Oversampling):** The latency reporting re-entrancy issue (Pitfall 2) has documented workarounds but the exact guard implementation for VST3 hosts needs verification in Nuendo specifically. JUCE forum evidence is MEDIUM confidence — test-driven discovery may require adjustment.
- **Phase 4 (Distribution):** Code signing and notarization requirements change on Apple's timeline. The Moonbase 2025 article is the most current source but should be re-verified against Apple's current notarytool documentation at signing time.

Phases with standard patterns (skip additional research):
- **Phase 1 (DSP Port):** All patterns are directly verified against existing source code (Gunk Lord, Crucible, Bathysphere). High confidence. No research-phase needed.
- **Phase 3 (GUI):** Procedural `LookAndFeel_V4` with rotary knobs is the developer's established pattern across two plugins. No research-phase needed.

---

## Confidence Assessment

| Area | Confidence | Notes |
|------|------------|-------|
| Stack | HIGH | JUCE 8.0.12 version confirmed via GitHub releases; CMake/C++ choices verified against 3 existing developer projects; oversampling filter type cross-referenced against JUCE official example |
| Features | MEDIUM | Competitor feature analysis via product pages and editorial reviews (no direct access to competitor DSP); feature gap analysis is accurate but competitive landscape may shift |
| Architecture | HIGH | Based on direct source inspection of GunkLord FuzzStage.h, FuzzCore.h, FuzzTone.h, PluginProcessor.h, and Crucible ClipperSection.h; JUCE API cross-referenced against official docs |
| Pitfalls | MEDIUM-HIGH | Critical pitfalls corroborated by multiple JUCE forum threads with concrete reproduction cases; DC offset and noise gate ordering confirmed via secondary sources; latency re-entrancy evidence is JUCE forum only (no official JUCE documentation) |

**Overall confidence:** HIGH

### Gaps to Address

- **Latency reporting VST3 re-entrancy:** The specific guard pattern for preventing concurrent `prepareToPlay`/`processBlock` during latency change needs empirical validation in Nuendo. The forum evidence describes the problem clearly but the recommended fix (worst-case latency at constructor time) trades correctness for safety. Verify during Phase 2 with a Nuendo test session.
- **Noise gate threshold UX:** PITFALLS.md flags that threshold labeled from the output perspective creates user confusion (gate operates pre-distortion on the input signal). The exact threshold labeling and documentation approach should be decided during Phase 1 implementation and reflected in the `Parameters.h` range definition.
- **Non-integer IIR oversampling latency:** PITFALLS.md notes IIR half-band filters produce non-integer latency. JUCE's `getLatencyInSamples()` returns a float; `setLatencySamples()` takes an int. Rounding direction and dry delay compensation for the fractional sample must be verified empirically at all three oversampling rates and sample rates.

---

## Sources

### Primary (HIGH confidence)
- GunkLord source — FuzzStage.h, FuzzCore.h, FuzzTone.h, PluginProcessor.h: `/Users/nathanmcmillan/Projects/gunklord/source/`
- Crucible source — ClipperSection.h/.cpp, PluginProcessor.h: `/Users/nathanmcmillan/Projects/Crucible/Source/`
- Bathysphere source — CMakeLists.txt, BathysphereLookAndFeel: `/Users/nathanmcmillan/Projects/Bathysphere/`
- JUCE DSPModulePluginDemo.h — selectable oversampling array pattern: https://github.com/juce-framework/JUCE/blob/master/examples/Plugins/DSPModulePluginDemo.h
- JUCE CMake API docs — `juce_add_plugin` parameters: https://github.com/juce-framework/JUCE/blob/master/docs/CMake%20API.md
- JUCE BREAKING_CHANGES.md — all 8.x breaking changes: https://github.com/juce-framework/JUCE/blob/master/BREAKING_CHANGES.md
- JUCE dsp::Oversampling class reference: https://docs.juce.com/master/classjuce_1_1dsp_1_1Oversampling.html
- JUCE dsp::DryWetMixer class reference: https://docs.juce.com/master/classdsp_1_1DryWetMixer.html

### Secondary (MEDIUM confidence)
- JUCE Forum: Advice on dynamically changing oversampling factor — timer-based suspension pattern: https://forum.juce.com/t/advice-needed-on-when-and-where-to-dynamically-change-the-oversampling-factor-is-this-code-ok/58162
- JUCE Forum: prepareToPlay and processBlock thread-safety — concurrent call race conditions: https://forum.juce.com/t/preparetoplay-and-processblock-thread-safety/32193
- JUCE Forum: Dry/Wet mix in plugin with latency — delay line on dry path: https://forum.juce.com/t/dry-wet-mix-in-plugin-with-latency/4967
- Melatonin JUCE CMake guide — FetchContent vs. submodule tradeoffs: https://melatonin.dev/blog/how-to-use-cmake-with-juce/
- Melatonin Blog: Pluginval is a plugin dev's best friend — listener cleanup, auval at strictness 5: https://melatonin.dev/blog/pluginval-is-a-plugin-devs-best-friend/
- FabFilter Saturn 2 help documentation: https://www.fabfilter.com/help/saturn/using/overview
- Plugin Alliance bx_blackdist2 product page: https://www.plugin-alliance.com/en/products/bx_blackdist2.html
- Witch Pig Siberian Hamster review: https://bedroomproducersblog.com/2023/06/17/witch-pig-siberian-hamster/

### Tertiary (LOW confidence)
- Analog Obsession Distox KVR thread — feature comparison: https://www.kvraudio.com/forum/viewtopic.php?t=505799
- KVR Audio auto gain compensation discussion: https://www.kvraudio.com/forum/viewtopic.php?t=448374
- Moonbase: Code signing audio plugins in 2025 — notarization requirements (verify against current Apple docs at signing time): https://moonbase.sh/articles/code-signing-audio-plugins-in-2025-a-round-up/

---
*Research completed: 2026-02-23*
*Ready for roadmap: yes*
