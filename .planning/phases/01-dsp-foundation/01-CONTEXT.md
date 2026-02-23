# Phase 1: DSP Foundation - Context

**Gathered:** 2026-02-23
**Status:** Ready for planning

<domain>
## Phase Boundary

JUCE 8 plugin that loads in a DAW and produces correctly distorted audio. Rat-inspired DSP with full signal chain controls (Drive, Symmetry, Tightness, Sag, Tone, Presence, Input Gain, Output Gain, Mix, Limiter, Noise Gate). No GUI beyond DAW generic editor. No oversampling. No distribution packaging.

</domain>

<decisions>
## Implementation Decisions

### Distortion Voicing
- Reference circuit: Original Rat LM308 (1978) — faithful to the thick, chunky midrange character
- Not a strict circuit model — "Cairn brand" flavor: more usable and flexible than the original Rat
- Key quality: forgiving on many sources due to mid-heavy voicing (less fizzy than typical distortion)
- Tone and Presence stay within the Rat's natural range with slight extension — the added Presence knob already gives more brightness than the original pedal
- Extended filter ranges vs requirements: Tightness HPF 20–800 Hz (was 20–300 Hz), Tone LPF 2 kHz–20 kHz (was 800–8 kHz)
- Pre/post toggle on tone-shaping stages (Tightness, Tone, Presence) — DSP supports both routing paths; default is post-distortion. Toggle UI deferred to Phase 3

### Sag Behavior
- Full range: minimum Sag = perfect power supply (clean LM308 behavior), maximum Sag = dying-battery sputter with note choking
- Gain-neutral — Sag changes character without affecting overall output level
- High Drive + high Sag = chaos is acceptable; users cranking both expect extreme behavior

### Noise Gate
- Full-featured gate DSP: threshold, attack, release, sidechain HPF (adjustable cutoff), range (attenuation amount when closed), hysteresis
- UI approach: only threshold knob visible; attack, release, sidechain HPF, and range exposed via right-click context menu (Phase 3)
- Good defaults so gate sounds right with just the threshold knob — power users access hidden controls
- Internal sidechain only (no external sidechain input)
- Hysteresis: yes — different open/close thresholds to prevent chatter on borderline signals

### Signal Chain Order
- Default order: Input Gain → Noise Gate → Tightness HPF → Drive + Clipping → Tone LPF → Presence Shelf → Sag → Output Gain → Brickwall Limiter → Dry/Wet Mix
- Tone stages support pre/post toggle (DSP routing flexibility; default is post-distortion per chain above)
- Dry/Wet mix: latency-compensated dry path (null test passes at 50%)

### Claude's Discretion
- Low-gain distortion character (below 25% Drive) — follow LM308 circuit behavior
- High-gain distortion character (above 75% Drive) — within 1x–40x gain range
- Symmetry knob feel (gradual blend vs distinct zones)
- Sag dynamics (input-reactive vs static) — pick what's most authentic and useful
- Noise gate style (hard vs smooth) and default on/off state
- Limiter character (transparent vs subtle warmth)
- Channel configuration (mono/stereo support) — user likes the idea of true stereo L/R processing for width but defers to best practice
- Pre/post toggle availability for Sag stage
- Attack/release/sidechain HPF/range default values for the noise gate

</decisions>

<specifics>
## Specific Ideas

- "I have the JHS PackRat pedal which has a large array of different rat pedals in one and it's great" — multiple circuit variants is a future goal
- "The Rat's thick chunky midrange can be very forgiving for things that distortion would normally not work on due to the high fizz caused by a lot of other distortion types"
- "I often reach for an EQ to really shape the gain into a specific zone in the upper mids" — extended Tightness/Tone ranges let users bandpass the distortion without external EQ
- Noise gate right-click pattern: visible threshold knob, hidden power-user controls — applies to gate and potentially other parameters

</specifics>

<deferred>
## Deferred Ideas

- Multiple Rat circuit variants selectable (LM308, OP07, Turbo Rat, etc.) — like the JHS PackRat. Could be its own phase or v2 feature
- Right-click context menu UI for gate parameters — Phase 3 (GUI)
- Pre/post toggle UI for tone stages — Phase 3 (GUI)

</deferred>

---

*Phase: 01-dsp-foundation*
*Context gathered: 2026-02-23*
