# Claymore

## What This Is

A standalone distortion plugin built around a customized Rat circuit algorithm, ported from the Gunk Lord project's fuzz stage. Claymore is a versatile mixing tool that ranges from pleasant saturation to face-melting distortion, leveraging the Rat pedal's thick chunky midrange push to creatively distort individual tracks, busses, or guitar DI signals before amp simulators.

## Core Value

A Rat-inspired distortion with enough range and tonal control to be useful on any source material — from subtle saturation to aggressive destruction — in a focused, pedal-style interface.

## Requirements

### Validated

(None yet — ship to validate)

### Active

- [ ] Rat distortion algorithm ported from Gunk Lord (FuzzCore, FuzzTone, FuzzStage)
- [ ] Descriptive parameter names: Gain, Symmetry, Tightness, Sag, Tone, Presence
- [ ] Noise gate with threshold control
- [ ] Input gain, output gain, dry/wet mix controls
- [ ] Brickwall output limiter
- [ ] User-selectable oversampling (2x, 4x, 8x)
- [ ] Minimal pedal-style GUI with knobs
- [ ] VST3 and AU plugin formats (macOS Universal Binary)
- [ ] JUCE 8 / CMake build system (same toolchain as Gunk Lord)

### Out of Scope

- Chain/position control — Gunk Lord-specific, no effects chain to route around
- LFO/modulation system — not needed for a focused distortion plugin
- Standalone application — plugin-only for DAW use
- CLAP format — potential future addition, not v1
- Cross-platform (Windows/Linux) — macOS only for now
- Presets — can add later, ship the DSP first

## Context

- The Rat algorithm is fully implemented and working in Gunk Lord at `~/projects/gunklord/source/dsp/fuzz/`
- Core DSP files to port: FuzzCore.h, FuzzTone.h, FuzzStage.h, FuzzConfig.h (FuzzType.h)
- Gunk Lord uses 2x fixed oversampling with IIR half-band filters — Claymore needs selectable rates (2x/4x/8x)
- The algorithm models the LM308 op-amp's slew rate limiting, hard/soft clip blending, bias-starve sputter, and the Rat's tone circuit (800–8000 Hz LP sweep + 4 kHz presence shelf)
- Gunk Lord's parameter smoothing approach (10ms drive, 5ms controls) should carry over
- Manufacturer code: NMcM (same developer identity as Gunk Lord)

## Constraints

- **Tech stack**: JUCE 8, C++17, CMake — matching Gunk Lord for code portability
- **Platform**: macOS Universal Binary (Intel + Apple Silicon)
- **Formats**: VST3 and AU only (no Standalone, no CLAP for v1)
- **Latency**: Must be low enough for real-time guitar monitoring (oversampling adds latency proportional to rate)
- **CPU**: Selectable oversampling lets users trade quality vs. CPU — important for mixing sessions with many instances

## Key Decisions

| Decision | Rationale | Outcome |
|----------|-----------|---------|
| Port Rat algorithm from Gunk Lord | Proven DSP, avoids reimplementation | — Pending |
| Descriptive parameter names | Mixing-tool audience expects clarity over personality | — Pending |
| Selectable oversampling (2x/4x/8x) | Mixing use case demands quality; guitar use case demands low latency | — Pending |
| No standalone build | Plugin-only for DAW workflows | — Pending |
| Pedal-style minimal GUI | Focused tool, not a complex interface | — Pending |

---
*Last updated: 2026-02-23 after initialization*
