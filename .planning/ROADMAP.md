# Roadmap: Claymore

## Overview

Claymore ships as a focused Rat-inspired distortion plugin for DAW use. The work follows the hard dependency chain baked into the architecture: port and validate the DSP before upgrading oversampling, build the GUI once the APVTS parameter layout is locked, then gate distribution on formal validation passing. Four phases, each delivering one coherent and verifiable capability.

## Phases

**Phase Numbering:**
- Integer phases (1, 2, 3): Planned milestone work
- Decimal phases (2.1, 2.2): Urgent insertions (marked with INSERTED)

Decimal phases appear between their surrounding integers in numeric order.

- [ ] **Phase 1: DSP Foundation** - JUCE 8 project scaffolding, ported Rat DSP, complete signal chain, plugin loads and produces audio
- [ ] **Phase 2: Selectable Oversampling** - ClaymoreEngine upgraded to pre-allocated 2x/4x/8x oversampling, thread-safe rate switching, correct DAW latency reporting
- [ ] **Phase 3: GUI** - Pedal-style editor with knobs for all parameters, oversampling selector, full DAW automation
- [ ] **Phase 4: Validation and Distribution** - auval + pluginval pass, code-signed and notarized Universal Binary, ready to ship

## Phase Details

### Phase 1: DSP Foundation
**Goal**: Plugin loads in a DAW and produces correctly distorted audio with all signal chain controls working
**Depends on**: Nothing (first phase)
**Requirements**: DIST-01, DIST-02, DIST-03, DIST-04, DIST-05, DIST-06, SIG-01, SIG-02, SIG-03, SIG-04, SIG-05
**Success Criteria** (what must be TRUE):
  1. Plugin loads in a DAW host without crashing and passes audio through the signal chain
  2. Drive knob audibly increases distortion across its range; Symmetry knob audibly shifts between symmetric and asymmetric clipping
  3. Tightness, Sag, Tone, and Presence controls each produce audible and distinct tonal changes
  4. Input Gain, Output Gain, and Dry/Wet Mix controls scale the signal as expected; brickwall limiter prevents digital overs on extreme settings
  5. Noise gate closes on silence and reopens on signal above threshold; gate operates pre-distortion (sustain is not cut at high Drive)
**Plans:** 1/3 plans executed
Plans:
- [ ] 01-01-PLAN.md — Project scaffold: CMake + JUCE 8.0.12 FetchContent, APVTS parameter layout, PluginProcessor/Editor shell
- [ ] 01-02-PLAN.md — DSP engine: port fuzz files from GunkLord, ClaymoreEngine with noise gate hysteresis and 2x oversampling, OutputLimiter
- [ ] 01-03-PLAN.md — Signal chain wiring: processBlock connects all parameters to engine, DryWetMixer, gains, limiter; Release build

### Phase 2: Selectable Oversampling
**Goal**: User can select 2x, 4x, or 8x oversampling with correct latency reported to the DAW at all times
**Depends on**: Phase 1
**Requirements**: QUAL-01, QUAL-02
**Success Criteria** (what must be TRUE):
  1. Oversampling rate can be switched between 2x, 4x, and 8x without audio dropout, glitch, or DAW crash
  2. PDC compensation in a multi-track session remains correct after switching oversampling rates (no pre-echo or timing offset on the processed track)
  3. Dry/wet mix at 50% produces no phase cancellation artifact (null test passes) at all three oversampling rates
**Plans**: TBD

### Phase 3: GUI
**Goal**: Plugin is fully usable from the GUI with a pedal-style interface and all parameters automatable from the DAW
**Depends on**: Phase 2
**Requirements**: GUI-01, GUI-02, GUI-03
**Success Criteria** (what must be TRUE):
  1. All 9 signal parameters (Drive, Symmetry, Tightness, Sag, Tone, Presence, Input Gain, Output Gain, Mix) are visible as labeled knobs; Noise Gate threshold is accessible; all controls respond to mouse interaction
  2. Oversampling rate selector is visible and functional in the GUI; current rate is clearly indicated
  3. All parameters accept DAW automation write and read correctly in a session (knobs visually track automation playback)
**Plans**: TBD

### Phase 4: Validation and Distribution
**Goal**: Plugin passes formal AU and VST3 validation and is code-signed and notarized for macOS distribution
**Depends on**: Phase 3
**Requirements**: FMT-01, FMT-02
**Success Criteria** (what must be TRUE):
  1. `auval -v aufx NMcM Clyr` passes with no errors or warnings
  2. `pluginval` passes at strictness 5 for both VST3 and AU formats
  3. `lipo -info` confirms Universal Binary (x86_64 + arm64) for both plugin formats
  4. Plugin is code-signed and notarized; macOS Gatekeeper does not block loading in Logic and Reaper
**Plans**: TBD

## Progress

**Execution Order:**
Phases execute in numeric order: 1 → 2 → 3 → 4

| Phase | Plans Complete | Status | Completed |
|-------|----------------|--------|-----------|
| 1. DSP Foundation | 1/3 | In Progress|  |
| 2. Selectable Oversampling | 0/TBD | Not started | - |
| 3. GUI | 0/TBD | Not started | - |
| 4. Validation and Distribution | 0/TBD | Not started | - |
