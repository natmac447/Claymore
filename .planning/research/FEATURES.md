# Feature Research

**Domain:** Mixing-focused distortion plugin (Rat emulation)
**Researched:** 2026-02-23
**Confidence:** MEDIUM — competitive product research via WebSearch; no Context7 applicable (non-library domain); cross-referenced across multiple plugin reviews and manufacturer pages.

---

## Feature Landscape

### Table Stakes (Users Expect These)

Features users assume exist. Missing these = product feels incomplete.

| Feature | Why Expected | Complexity | Notes |
|---------|--------------|------------|-------|
| Input Gain | Gain staging before distortion stage is universal across every mixing tool; users expect to hit the circuit at their level | LOW | Standard dB range (-30 to +30 dB typical; Analog Obsession Distox, Plugin Alliance HG-2 all do this) |
| Output Gain | Distortion changes loudness significantly; users must compensate downstream | LOW | Independent of drive; essential for gain staging |
| Drive / Distortion Amount | The primary parameter — literally the point of the plugin | LOW | Rat's "Distortion" knob is the reference; Decapitator "Drive," bx_blackdist2 "Distortion" |
| Dry/Wet Mix | Parallel processing without external routing is now expected by mixing engineers | LOW | Decapitator, HG-2, Distox, almost every modern saturation plugin includes this; absence is a complaint |
| Tone / Filter | Voiced distortion without tonal shaping is unusable on most sources | LOW | Rat has its characteristic LP sweep filter; Decapitator has a Tone knob; bx_blackdist2 has a Filter |
| Bypass | Hard bypass with zero processing artifacts | LOW | Fundamental DAW plugin behavior; JUCE provides this |
| Parameter Automation | All knobs must be automatable from the DAW | LOW | DAW standard; JUCE handles via AudioProcessorValueTreeState |
| DAW Latency Reporting | Plugin must report added latency from oversampling so DAW can compensate | MEDIUM | Failing to do this causes phase issues in multi-track sessions; JUCE setLatencySamples() |
| Selectable Oversampling | Mixing engineers expect to choose quality/CPU tradeoff; present in all serious saturation tools | MEDIUM | 2x/4x/8x is the standard range; Distox uses 4x fixed, FabFilter offers "Good" (8x) and "Superb" (32x) |
| Noise Gate | High-gain distortion introduces noise; gate prevents noise tail bleed between notes | MEDIUM | Already in PROJECT.md requirements; confirmed as expected in high-gain plugin context (Neural DSP archetypes, Arturia Dist TUBE-CULTURE) |
| Brickwall Output Limiter | Prevents digital overs when distortion increases level unexpectedly; users catching clipping downstream is a pain point | MEDIUM | Already in PROJECT.md; prevents support complaints |
| VST3 + AU Formats | Required for pro DAW support on macOS | LOW | Logic requires AU; Cubase/Studio One/Live all prioritize VST3; already scoped |

### Differentiators (Competitive Advantage)

Features that set the product apart. Not required, but valued.

| Feature | Value Proposition | Complexity | Notes |
|---------|-------------------|------------|-------|
| Descriptive Knob Names | Most Rat emulations use the original pedal labels ("Filter," "Distortion," "Volume") which are confusing in a mixing context. Using "Gain," "Tone," "Presence," "Symmetry," "Tightness," "Sag" communicates intent to mixing engineers immediately | LOW | Already in PROJECT.md; this is the clearest UX differentiator vs bx_blackdist2 and Siberian Hamster which use pedal-verbatim naming |
| Symmetry Control | Bias the clipping stage between symmetrical (even harmonics) and asymmetrical (odd harmonics) — most Rat emulations don't expose this | MEDIUM | Already in PROJECT.md as "Symmetry"; FabFilter Saturn exposes this implicitly through distortion type selection, but single-band Rat emulations don't; adds musicality without complexity |
| Sag / Bias Starve | Models power supply droop under load — the characteristic Rat "bloom" and "sputter" at high gain; no competing Rat plugin exposes this | HIGH | Already in PROJECT.md as "Sag"; requires the LM308 slew rate + bias modeling from Gunk Lord FuzzCore/FuzzStage; genuine competitive differentiation |
| Tightness / Low-End Gate | Controls low-frequency input to distortion stage — lets it work cleanly on bass and full-range material | MEDIUM | Already in PROJECT.md; Distox approximates this with a pre-distortion HPF but no named "Tightness" concept; makes Claymore more useful on non-guitar sources |
| Presence Control | High-frequency shelf separate from main tone filter — useful for restoring air after low-pass Rat tone circuit | MEDIUM | Already in PROJECT.md; no standard Rat emulation exposes this; adds mix utility |
| Mixing-Tool Framing | Pedal emulations are marketed to guitarists; Claymore is framed as a mixing tool — works on any source | LOW | No DSP difference; entirely positioning and UI copy; changes who discovers and buys it |
| Auto Gain / Gain Compensation | Makes A/B evaluation faster — users can hear distortion character without level-bias; Decapitator's "Auto" switch is praised widely | MEDIUM | Not in current PROJECT.md scope but strongly requested in community discussions; option to lock output level while sweeping Drive |

### Anti-Features (Commonly Requested, Often Problematic)

Features that seem good but create problems.

| Feature | Why Requested | Why Problematic | Alternative |
|---------|---------------|-----------------|-------------|
| Multiband Processing | FabFilter Saturn is popular; users ask for per-band distortion | Multiband architecture requires crossover design, band-specific parameter sets, and dramatically increases DSP complexity and UI scope — it's a different product category entirely | Use pre-distortion Tightness (HPF) and post-distortion Tone/Presence to address the core use case of "too much bass distortion" without adding bands |
| Modulation / LFO | Waveform/tremolo effects on distortion parameters look fun | Scope creep into effect processor territory; increases UI complexity; already explicitly out-of-scope in PROJECT.md | Dry/wet + external modulation sources in the DAW cover this without adding to plugin scope |
| Cab Simulation / IR Loader | Some guitar-focused users expect this; Neural DSP archetypes bundle it | Makes the plugin a guitar rig, not a mixing tool; adds significant DSP, UI, and content creation scope; alienates the mixing engineer target | Position Claymore as pre-cab signal processing; describe in docs how to pair with cab sims |
| Preset Browser | Users request presets immediately | Presets without a working DSP are empty; shipping presets well requires time for sound design and organization; adds file management complexity | Ship factory presets in v1.x after DSP is validated and stable; plugin formats provide preset management infrastructure |
| Stereo Width Controls | Stereo mid/side processing is trendy | Distortion on stereo content already widens through harmonic generation; an explicit width control invites phase issues and isn't part of the Rat's character | Let M/S behavior emerge naturally; don't add explicit stereo controls |
| MIDI Control / Scenes | Neural DSP archetypes offer MIDI program change for live use | Claymore is a mixing tool for DAW use; MIDI control is handled by the host's automation lane system | Document how to use host automation for parameter control in all major DAWs |

---

## Feature Dependencies

```
[Input Gain]
    └──feeds──> [Distortion Core (Drive, Symmetry, Sag, Tightness)]
                    └──feeds──> [Tone / Presence EQ]
                                    └──feeds──> [Noise Gate]
                                                    └──feeds──> [Dry/Wet Blender]
                                                                    └──feeds──> [Output Gain]
                                                                                    └──feeds──> [Brickwall Limiter]

[Oversampling Selector] ──wraps──> [Distortion Core]
    └──reports latency to──> [DAW via setLatencySamples()]

[Dry/Wet Blender]
    └──requires latency compensation──> [Dry path delay must match wet path processing delay]
```

### Dependency Notes

- **Input Gain requires careful ordering:** It must precede the distortion stage. Changing input gain changes how the clipping circuit behaves — it's not just a volume fader.
- **Noise Gate placement:** Gating post-distortion catches noise floor from the distortion algorithm itself. Gating pre-distortion lets loud transients pass but kills sustained noise. For mixing use, post-distortion gate is more correct (catches circuit noise); validate in implementation.
- **Oversampling requires DAW latency reporting:** Without calling setLatencySamples(), dry/wet parallel blending produces comb filtering in multi-track sessions.
- **Dry/Wet requires dry path delay:** The dry signal must be delayed to match the wet path's processing latency (from oversampling filter group delay) before blending. This is non-trivial to implement correctly.
- **Brickwall limiter must be last in chain:** Placing it before output gain defeats its purpose.

---

## MVP Definition

### Launch With (v1)

Minimum viable product — what's needed to validate the concept.

- [ ] Distortion core (Drive/Distortion parameter) — reason to exist
- [ ] Symmetry control — first differentiator from other Rat emulations
- [ ] Tightness control — enables non-guitar use cases
- [ ] Sag control — the distinctive character parameter
- [ ] Tone + Presence — tonal shaping without which distortion is unusable
- [ ] Input Gain + Output Gain — proper gain staging workflow
- [ ] Dry/Wet Mix — expected by every mixing engineer
- [ ] Noise Gate with Threshold — high-gain requirement
- [ ] Brickwall output limiter — prevents user-visible failures
- [ ] Selectable oversampling (2x / 4x / 8x) — quality/CPU tradeoff
- [ ] DAW latency reporting — correctness requirement for multi-track sessions
- [ ] VST3 + AU — format coverage for macOS

### Add After Validation (v1.x)

Features to add once core is working and deployed.

- [ ] Factory presets — requires stable DSP and sound design time; add when parameter behavior is locked
- [ ] Auto Gain Compensation option — quality-of-life for A/B evaluation; community widely praises Decapitator's implementation
- [ ] CLAP format — already noted in PROJECT.md as future addition

### Future Consideration (v2+)

Features to defer until product-market fit is established.

- [ ] Windows/Linux builds — requires cross-platform testing infrastructure
- [ ] Resizable GUI — increases UI implementation scope significantly
- [ ] Additional distortion character modes — would require new DSP modeling work beyond the Rat circuit

---

## Feature Prioritization Matrix

| Feature | User Value | Implementation Cost | Priority |
|---------|------------|---------------------|----------|
| Drive / Distortion | HIGH | LOW (port from Gunk Lord) | P1 |
| Input Gain | HIGH | LOW | P1 |
| Output Gain | HIGH | LOW | P1 |
| Dry/Wet Mix (with correct delay) | HIGH | MEDIUM (dry path latency matching) | P1 |
| Tone + Presence | HIGH | LOW (port from Gunk Lord FuzzTone) | P1 |
| Symmetry | HIGH | LOW (expose existing Gunk Lord param) | P1 |
| Sag | HIGH | MEDIUM (expose + tune from Gunk Lord) | P1 |
| Tightness | HIGH | LOW (pre-distortion HPF) | P1 |
| Noise Gate | HIGH | MEDIUM (threshold + attack/release logic) | P1 |
| Oversampling selector + latency report | HIGH | MEDIUM (JUCE dsp::Oversampling + setLatencySamples) | P1 |
| Brickwall limiter | HIGH | LOW (JUCE Limiter DSP or simple clip) | P1 |
| VST3 + AU formats | HIGH | LOW (JUCE handles) | P1 |
| Factory presets | MEDIUM | MEDIUM (sound design time) | P2 |
| Auto Gain Compensation | MEDIUM | LOW | P2 |
| CLAP format | LOW | LOW (JUCE supports) | P3 |
| Resizable GUI | LOW | HIGH | P3 |

**Priority key:**
- P1: Must have for launch
- P2: Should have, add when possible
- P3: Nice to have, future consideration

---

## Competitor Feature Analysis

| Feature | bx_blackdist2 (Rat 2 emu) | Siberian Hamster (Rat emu) | Decapitator | FabFilter Saturn 2 | Claymore (plan) |
|---------|--------------------------|---------------------------|-------------|---------------------|-----------------|
| Drive param | Yes ("Distortion") | Yes | Yes ("Drive") | Yes (per band) | Yes ("Gain") |
| Tone/filter | Yes (LP filter) | Yes (LP filter) | Yes | Yes (multiband) | Yes + Presence |
| Input Gain | No listed | No | Yes | Yes | Yes |
| Output Gain | Yes ("Volume") | Yes ("Volume") | Yes | Yes | Yes |
| Dry/Wet | No | No | Yes | Yes | Yes |
| Noise gate | No | No | No | No | Yes |
| Oversampling | Not listed | Not listed | Not listed | Yes (8x / 32x) | Yes (2x/4x/8x) |
| Sag / bias starve | No | No | No | No | Yes |
| Symmetry | No | No | No | No (implicit via type) | Yes |
| Tightness | No | No | No | Yes (via multiband) | Yes |
| Presets | Yes | No | Yes | Yes | v1.x |
| Mixing-tool framing | No (guitar) | No (guitar) | Mixed | Yes | Yes |
| Parameter naming | Pedal-verbatim | Pedal-verbatim | Creative/obscure | Technical/clear | Descriptive/mixing |

---

## Sources

- Soundtoys Decapitator product page and manual: https://www.soundtoys.com/product/decapitator/ (MEDIUM confidence — official manufacturer page)
- FabFilter Saturn 2 help documentation: https://www.fabfilter.com/help/saturn/using/overview (HIGH confidence — official documentation)
- Plugin Alliance HG-2 product: https://www.plugin-alliance.com/products/hg-2 (MEDIUM confidence — official page)
- Plugin Alliance bx_blackdist2: https://www.plugin-alliance.com/en/products/bx_blackdist2.html (MEDIUM confidence — official page)
- Analog Obsession Distox KVR thread: https://www.kvraudio.com/forum/viewtopic.php?t=505799 (LOW confidence — community forum)
- Witch Pig Siberian Hamster review: https://bedroomproducersblog.com/2023/06/17/witch-pig-siberian-hamster/ (MEDIUM confidence — editorial review)
- KVR Audio auto gain compensation discussion: https://www.kvraudio.com/forum/viewtopic.php?t=448374 (LOW confidence — community forum)
- Sound On Sound: Should I use automatic level compensation: https://www.soundonsound.com/sound-advice/q-should-use-my-plug-ins-automatic-level-compensation (MEDIUM confidence — editorial)
- JUCE oversampling latency documentation: https://daudio.dev/explore/HowToImplementOversamplingInJuce (MEDIUM confidence — community tutorial verified against JUCE docs)
- Guitar Chalk: Distortion pedals with built-in noise gate: https://www.guitarchalk.com/distortion-pedals-noise-gate/ (LOW confidence — editorial list)

---
*Feature research for: Mixing-focused standalone Rat-based distortion plugin (Claymore)*
*Researched: 2026-02-23*
