# Phase 3: GUI - Context

**Gathered:** 2026-02-23
**Status:** Ready for planning

<domain>
## Phase Boundary

Pedal-style editor with knobs for all 9 signal parameters (Drive, Symmetry, Tightness, Sag, Tone, Presence, Input Gain, Output Gain, Mix), Noise Gate threshold, and oversampling rate selector. All parameters automatable from the DAW. Fixed-size window, no resizing. Single Rat algorithm — no model switching.

</domain>

<decisions>
## Implementation Decisions

### Visual aesthetic
- Dark theme with subtle skeuomorphism — Neural DSP dark themes as reference, touch of physicality
- Warm Cairn base palette (Basalt/Hearth range) with a cool tint to match the steel/blade identity
- Powder-coated grain texture on the pedal body surface — subtle, felt more than seen
- Cold blue steel accent color for active states, LED indicators, and highlights
- White matte knobs with silver metallic indicator line — inverted EHX Memory Man style
- LED bulb-style indicator light with glow bloom for bypass/active state (not flat button)
- Product name "CLAYMORE" in Cormorant Garamond with subtle metallic/silver steel-etched treatment in header

### Knob layout
- Drive-centric hero layout: large Drive knob centered, satellite knobs arranged around it
- Pre-distortion controls (Tightness, Symmetry, Input Gain, Sag) flanking left/below Drive
- Post-distortion controls (Tone, Presence, Noise Gate) flanking right/below Drive
- Output Gain and Mix in the Utility Zone per Cairn design system
- Parameter labels: name only below knob, value displayed on hover/tooltip (not persistent readout)

### Oversampling selector
- Three options: 2x, 4x, 8x
- Label shows rate values only — no CPU hints or quality descriptors
- Placement and control type (segmented toggle, flip switch, etc.) at Claude's discretion

### Window size
- Medium standard window: approximately 700x500px
- Spacious density — generous padding around the Drive hero knob, clear separation between controls
- HiDPI / Retina aware (2x pixel density rendering)
- Fixed size, not resizable (per v1 scope)
- Exact aspect ratio at Claude's discretion

### Claude's Discretion
- Knob size hierarchy (two sizes vs three sizes for Drive/satellite/utility)
- Toggle switch usage (which controls get flip toggles vs other control types)
- Oversampling selector control type and exact placement
- Exact window aspect ratio and pixel dimensions
- Knob arrangement symmetry vs signal-flow ordering
- Loading skeleton, error states, parameter smoothing UI feedback

</decisions>

<specifics>
## Specific Ideas

- "Neural DSP dark themes with a subtle amount of skeuomorphism" — the aesthetic north star
- "White matte knob with a silver metallic indicator line — like EHX Memory Man knobs but inverted colors. This gives the controls a tactile weight"
- "Silver metallic flip toggle switches" — physical hardware switches somewhere in the UI
- "Blue LED bulbs as the indicator" — real pedal-style LEDs, not flat digital buttons
- "Steel-etched look" for the CLAYMORE title — like an inscription on a blade
- Cairn brand guide applies: Cormorant Garamond for title, DM Sans for labels, Spectral for descriptions
- Cairn 4-zone layout system: Header (title + bypass), Primary (knobs), Utility (output/mix), Footer (maker's mark)
- Cairn maker's mark in footer at 20-30% opacity
- Brand reference files at ~/projects/brand/ — includes SVG logos, full brand identity docs, plugin design system

</specifics>

<deferred>
## Deferred Ideas

- Multiple Rat algorithm models with color-coded LED indicators (blue for base, other colors per model) — future phase
- Resizable GUI — explicitly out of scope for v1

</deferred>

---

*Phase: 03-gui*
*Context gathered: 2026-02-23*
