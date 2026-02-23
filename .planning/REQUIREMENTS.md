# Requirements: Claymore

**Defined:** 2026-02-23
**Core Value:** A Rat-inspired distortion with enough range and tonal control to be useful on any source material — from subtle saturation to aggressive destruction — in a focused, pedal-style interface.

## v1 Requirements

Requirements for initial release. Each maps to roadmap phases.

### Distortion

- [x] **DIST-01**: User can control distortion amount via Drive knob (1x–40x gain range)
- [x] **DIST-02**: User can blend between hard clip and soft clip via Symmetry knob
- [x] **DIST-03**: User can control low-frequency input to distortion via Tightness (HPF 20–300 Hz)
- [x] **DIST-04**: User can add bias-starve sputter character via Sag knob
- [x] **DIST-05**: User can shape post-distortion tone via Tone (LP sweep 800–8000 Hz)
- [x] **DIST-06**: User can boost/cut high-frequency presence via Presence shelf (+/-6 dB at 4 kHz)

### Signal Chain

- [x] **SIG-01**: User can adjust input level before distortion via Input Gain (-24 to +24 dB)
- [x] **SIG-02**: User can adjust output level after distortion via Output Gain (-48 to +12 dB)
- [x] **SIG-03**: User can blend dry and wet signal via Mix knob (latency-compensated dry path)
- [x] **SIG-04**: Output is limited by brickwall limiter to prevent digital overs
- [x] **SIG-05**: User can enable noise gate with adjustable threshold (-60 to -10 dB)

### Quality

- [x] **QUAL-01**: User can select oversampling rate (2x, 4x, 8x) to trade CPU for quality
- [x] **QUAL-02**: Plugin reports correct latency to DAW for all oversampling rates

### Format

- [ ] **FMT-01**: Plugin builds as VST3 on macOS (Universal Binary)
- [ ] **FMT-02**: Plugin builds as AU on macOS (Universal Binary)

### GUI

- [x] **GUI-01**: Pedal-style minimal interface with knobs for all distortion and signal chain parameters
- [x] **GUI-02**: Oversampling rate selector accessible from GUI
- [x] **GUI-03**: All parameters automatable from DAW

## v2 Requirements

Deferred to future release. Tracked but not in current roadmap.

### Presets

- **PRES-01**: Factory presets for common use cases (guitar, bass, drums, vocals, bus)
- **PRES-02**: User can save and recall custom presets

### Quality of Life

- **QOL-01**: Auto gain compensation option (lock output level while sweeping Drive)
- **QOL-02**: CLAP plugin format support

## Out of Scope

Explicitly excluded. Documented to prevent scope creep.

| Feature | Reason |
|---------|--------|
| Multiband processing | Different product category entirely; use Tightness + Tone/Presence instead |
| LFO / modulation system | Scope creep into effect processor territory; DAW automation covers this |
| Cab simulation / IR loader | Makes it a guitar rig, not a mixing tool |
| Standalone application | Plugin-only for DAW workflows |
| MIDI control / scenes | DAW automation handles parameter control |
| Stereo width controls | Distortion naturally widens through harmonics; explicit controls invite phase issues |
| Windows / Linux builds | macOS only for v1; cross-platform later |
| Resizable GUI | Increases UI implementation scope significantly |

## Traceability

Which phases cover which requirements. Updated during roadmap creation.

| Requirement | Phase | Status |
|-------------|-------|--------|
| DIST-01 | Phase 1 | Complete |
| DIST-02 | Phase 1 | Complete |
| DIST-03 | Phase 1 | Complete |
| DIST-04 | Phase 1 | Complete |
| DIST-05 | Phase 1 | Complete |
| DIST-06 | Phase 1 | Complete |
| SIG-01 | Phase 1 | Complete |
| SIG-02 | Phase 1 | Complete |
| SIG-03 | Phase 1 | Complete |
| SIG-04 | Phase 1 | Complete |
| SIG-05 | Phase 1 | Complete |
| QUAL-01 | Phase 2 | Complete (02-01) |
| QUAL-02 | Phase 2 | Complete (02-01) |
| FMT-01 | Phase 4 | Pending |
| FMT-02 | Phase 4 | Pending |
| GUI-01 | Phase 3 | Complete |
| GUI-02 | Phase 3 | Complete |
| GUI-03 | Phase 3 | Complete |

**Coverage:**
- v1 requirements: 18 total
- Mapped to phases: 18
- Unmapped: 0

---
*Requirements defined: 2026-02-23*
*Last updated: 2026-02-23 after roadmap creation*
