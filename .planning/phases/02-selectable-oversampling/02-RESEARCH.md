# Phase 2: Selectable Oversampling - Research

**Researched:** 2026-02-23
**Domain:** JUCE DSP oversampling, PDC (Plugin Delay Compensation), APVTS choice parameters, thread-safe rate switching
**Confidence:** HIGH

---

## Summary

Phase 1 hardcoded a single `juce::dsp::Oversampling<float>` instance at 2x IIR inside `ClaymoreEngine`. Phase 2 replaces that single object with an array of three pre-allocated oversampling objects (2x, 4x, 8x), selects among them by index, and reports the correct latency to the DAW and to the internal `DryWetMixer` on every change.

The authoritative pattern for this exact problem exists in the same codebase: `Crucible/Source/dsp/ClipperSection.h/.cpp` implements selectable oversampling at Off/2x/4x/8x using a `std::array<std::unique_ptr<juce::dsp::Oversampling<float>>, 4>`. Claymore's version is simpler because (a) there is no "Off" state (minimum is 2x), (b) there is no lookahead, and (c) there is no stereo linking concern. The Crucible pattern can be adopted almost verbatim.

The three technically hard problems for this phase are: (1) pre-allocating all oversampling objects in `prepare()` so `process()` never allocates; (2) keeping the DryWetMixer latency compensation and `setLatencySamples()` in sync whenever the user switches the oversampling rate; and (3) managing the DryWetMixer's maximum latency capacity — the current hardcoded `{ 64 }` is sized for 2x IIR (~4 samples) but must be grown to accommodate 8x IIR (which can reach ~40–60 samples depending on filter type and whether `useIntegerLatency` is enabled). None of these require re-entrancy guards or DAW callbacks; the simplest correct approach is to call `prepareToPlay` semantics (re-prepare the DryWetMixer and call `setLatencySamples`) when the oversampling selection changes, which is already safe because both happen on the message thread in any host.

**Primary recommendation:** Adopt the Crucible ClipperSection array-of-objects pattern. Pre-allocate 2x/4x/8x IIR objects in `ClaymoreEngine::prepare()`, store a `currentOversamplingIndex` (0=2x, 1=4x, 2=8x), select the active object in `process()`, and expose `setOversamplingFactor(int)` + `getLatencyInSamples()`. In `PluginProcessor`, detect when the APVTS oversampling parameter changes (per-block atomic read), call `engine.setOversamplingFactor()`, then re-synchronize `dryWetMixer.setWetLatency()` and `setLatencySamples()`. Grow `dryWetMixer` capacity to 256 samples to safely cover all three rates.

---

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|-----------------|
| QUAL-01 | User can select oversampling rate (2x, 4x, 8x) to trade CPU for quality | APVTS `AudioParameterChoice` with `StringArray{"2x", "4x", "8x"}` exposes the selector; array-of-Oversampling-objects pattern (Crucible ClipperSection) handles runtime selection with zero audio allocation. |
| QUAL-02 | Plugin reports correct latency to DAW for all oversampling rates | `oversampling[index]->getLatencyInSamples()` returns the exact float latency; `setLatencySamples(static_cast<int>(...))` reports to the host; `dryWetMixer.setWetLatency(latency)` keeps the dry path aligned. Both must be updated whenever the rate changes. |
</phase_requirements>

---

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| `juce::dsp::Oversampling<float>` | JUCE 8 (in-repo) | Up/downsample with anti-aliasing filters | Built into juce_dsp; already used in Phase 1; no external dependency |
| `juce::dsp::DryWetMixer<float>` | JUCE 8 (in-repo) | Latency-compensated dry path for Mix knob | Already used in Phase 1; must have capacity for 8x IIR latency (~60 samples max) |
| `juce::AudioParameterChoice` | JUCE 8 (in-repo) | Discrete selector (2x/4x/8x) exposed as automatable DAW parameter | Correct type for a fixed discrete choice; maps to APVTS; stores as float index, readable with `std::atomic<float>*` load |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| `juce::SmoothedValue<float>` | JUCE 8 (in-repo) | Parameter smoothing for smoothers that reset on rate change | Already used; smoothers must be reset with the new oversampled rate after a factor switch |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| `filterHalfBandPolyphaseIIR` | `filterHalfBandFIREquiripple` | FIR has linear phase and higher latency (~3x more samples per stage); IIR has non-linear phase but lower latency and same alias rejection. Phase 1 already chose IIR; keep it for consistency. |
| Pre-allocated array of objects | Destroy/recreate Oversampling on switch | Recreation during playback causes allocation on the audio thread — forbidden. Pre-allocation is the only correct approach. |

**Installation:** No new packages. All libraries are already in juce_dsp, which is already a dependency.

---

## Architecture Patterns

### Recommended Project Structure

No new files required. All changes are inside:
```
Source/
├── dsp/
│   └── ClaymoreEngine.h          # Add oversampling array + setOversamplingFactor()
├── Parameters.h                   # Add "oversampling" AudioParameterChoice
└── PluginProcessor.cpp            # Detect rate change + re-sync latency
```

### Pattern 1: Pre-Allocated Array of Oversampling Objects

**What:** Allocate all three oversampling objects once in `prepare()`. In `process()`, select the active object by index — no allocation, no construction.

**When to use:** Any time oversampling factor must change at runtime without audio glitch or memory allocation on the audio thread.

**Example (from Crucible ClipperSection.cpp, adapted for Claymore):**
```cpp
// In ClaymoreEngine.h
// Remove:
//   juce::dsp::Oversampling<float> oversampling;
//
// Add:
static constexpr int numOversamplingFactors = 3; // 2x, 4x, 8x
std::array<std::unique_ptr<juce::dsp::Oversampling<float>>, numOversamplingFactors> oversamplingObjects;
int currentOversamplingIndex = 0; // 0=2x, 1=4x, 2=8x

// In prepare():
for (int i = 0; i < numOversamplingFactors; ++i)
{
    // factor = i+1 means 2^(i+1) = 2x, 4x, 8x
    oversamplingObjects[i] = std::make_unique<juce::dsp::Oversampling<float>>(
        static_cast<size_t>(numChannels),
        static_cast<size_t>(i + 1),               // 1=2x, 2=4x, 3=8x
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
        true,   // isMaxQuality
        false   // useIntegerLatency (see Pitfall 2)
    );
    oversamplingObjects[i]->initProcessing(static_cast<size_t>(spec.maximumBlockSize));
}

// In process():
auto& os = *oversamplingObjects[currentOversamplingIndex];
auto oversampledBlock = os.processSamplesUp(block);
// ... waveshaping ...
os.processSamplesDown(block);

// Latency query:
float getLatencyInSamples() const
{
    return oversamplingObjects[currentOversamplingIndex]->getLatencyInSamples();
}
```

### Pattern 2: Per-Block Rate Change Detection in PluginProcessor

**What:** Read the `oversampling` APVTS parameter atomically each block. If it changed from the previous value, update the engine's factor, then re-synchronize `dryWetMixer.setWetLatency()` and `setLatencySamples()`.

**When to use:** Whenever a JUCE plugin needs to change a latency-affecting setting at runtime without calling `prepareToPlay`.

**Example:**
```cpp
// In PluginProcessor.h: add
std::atomic<float>* oversamplingParam = nullptr;
int lastOversamplingIndex = 0;

// In processBlock(), before engine.process():
const int newOversamplingIndex = static_cast<int>(oversamplingParam->load(std::memory_order_relaxed));
if (newOversamplingIndex != lastOversamplingIndex)
{
    lastOversamplingIndex = newOversamplingIndex;
    engine.setOversamplingFactor(newOversamplingIndex);

    const float newLatency = engine.getLatencyInSamples();
    dryWetMixer.setWetLatency(newLatency);
    setLatencySamples(static_cast<int>(newLatency));
}
```

### Pattern 3: Smoother Reset After Rate Change

**What:** When the oversampling factor changes, the oversampled sample rate changes. Smoothers that were reset against the old oversampled rate must be re-reset.

**When to use:** Whenever a ClaymoreEngine::setOversamplingFactor() call changes the active factor.

**Example (inside ClaymoreEngine::setOversamplingFactor):**
```cpp
void setOversamplingFactor(int index)
{
    currentOversamplingIndex = juce::jlimit(0, numOversamplingFactors - 1, index);

    // Recalculate oversampled rate for smoothers
    const double newOsRate = sampleRate * std::pow(2.0, currentOversamplingIndex + 1);

    // Re-reset smoothers at new oversampled rate (preserves current target values)
    driveSmoother.reset(newOsRate, 0.010);
    symmetrySmoother.reset(newOsRate, 0.005);
    tightnessSmoother.reset(newOsRate, 0.005);
    sagSmoother.reset(newOsRate, 0.005);
}
```

### Pattern 4: APVTS AudioParameterChoice for Oversampling Selector

**What:** Use `AudioParameterChoice` instead of `AudioParameterFloat` for the oversampling selector. This maps to DAW automation as a discrete step parameter.

**Example (in Parameters.h):**
```cpp
// In ParamIDs namespace:
inline constexpr const char* oversampling = "oversampling";

// In createParameterLayout():
layout.add(std::make_unique<juce::AudioParameterChoice>(
    juce::ParameterID { ParamIDs::oversampling, 1 },
    "Oversampling",
    juce::StringArray { "2x", "4x", "8x" },
    0  // default: 2x (index 0)
));
```

### Anti-Patterns to Avoid

- **Creating/destroying Oversampling objects in processBlock:** Causes allocation on the audio thread. All objects must be created in `prepare()` or the constructor.
- **Calling `initProcessing()` in `process()`:** `initProcessing` allocates memory. It must only be called in `prepare()`.
- **Not resetting oversamplingObjects after factor switch:** The inactive objects accumulate filter state from stale audio. Call `oversamplingObjects[oldIndex]->reset()` when switching away from a factor, so if the user switches back it starts clean.
- **Not updating `dryWetMixer.setWetLatency()` after a rate switch:** The dry path will be offset from the wet path by the difference in latency between old and new rates, causing comb filtering at Mix < 1.0.
- **`DryWetMixer { 64 }` capacity with 8x oversampling:** 8x IIR (3 stages, max quality) produces approximately 24–60 samples of latency depending on sample rate and `useIntegerLatency`. The current 64-sample capacity may work but is tight. Use 256 to guarantee safety across all rates and sample rates.

---

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Anti-aliasing filters for up/downsampling | Custom polyphase FIR/IIR half-band filters | `juce::dsp::Oversampling<float>` | JUCE's implementation handles filter design, state management, latency tracking, and the multi-stage cascade for 4x/8x correctly. Manual implementations get the normalization, group delay, and stopping-band depth wrong. |
| Latency compensation for dry path | Custom circular buffer with matching delay | `juce::dsp::DryWetMixer` | Already in use for Phase 1; handles sample-accurate delay of the dry signal relative to the wet. Rolling a ring buffer gets off-by-one errors under buffer size changes. |
| Discrete parameter for oversampling | `AudioParameterFloat` with integer snapping | `AudioParameterChoice` | Choice parameters display as named options in host automation lanes. Float with snapping displays as a number from 0–1. Choice is correct for a small fixed set. |

**Key insight:** The oversampling problem is fully solved by JUCE's built-in classes. The implementation complexity is entirely in the glue: pre-allocation, factor switching, and latency re-reporting. Do not attempt to write custom filter stages.

---

## Common Pitfalls

### Pitfall 1: DryWetMixer Capacity Too Small for 8x
**What goes wrong:** The `DryWetMixer` is constructed with a maximum latency capacity (in samples). If `setWetLatency()` is called with a value exceeding the construction-time capacity, the excess latency is silently clamped, causing the dry/wet null test to fail at high oversampling rates.

**Why it happens:** `PluginProcessor.h` currently constructs `dryWetMixer { 64 }`. At 8x IIR oversampling (max quality, `useIntegerLatency=false`), latency can exceed 64 samples at sample rates above 44.1 kHz.

**How to avoid:** Increase construction capacity to 256. This is a one-character change: `juce::dsp::DryWetMixer<float> dryWetMixer { 256 };`

**Warning signs:** Null test at Mix=0.5 passes for 2x but fails for 4x or 8x. Audible comb filtering or pre-echo on the processed track at higher oversampling rates.

### Pitfall 2: Fractional Latency and `useIntegerLatency`
**What goes wrong:** `juce::dsp::Oversampling` with `useIntegerLatency = false` (current Phase 1 setting) can return a non-integer from `getLatencyInSamples()`. `setLatencySamples()` requires an integer. Truncation vs. rounding gives different results. The dry delay compensation in `DryWetMixer` uses the float value directly, but the DAW receives the truncated integer — this creates a fractional-sample offset between PDC and the actual delay, causing a very small but audible pre-echo or timing error at Mix < 1.0.

**Why it happens:** IIR half-band polyphase filters have non-integer group delay. At 2x the latency is small (usually integer or near-integer). At 4x and 8x the accumulated non-integer components grow.

**How to avoid:** Two options:
1. Set `useIntegerLatency = true` in all three Oversampling constructors. JUCE adds a Thiran allpass fractional delay to bring total latency to the next integer. This slightly increases latency but guarantees integer values.
2. Keep `useIntegerLatency = false` and round (not truncate) in `setLatencySamples`: `static_cast<int>(std::round(engine.getLatencyInSamples()))`.

**Recommendation:** Use `useIntegerLatency = false` + round, consistent with Phase 1 behavior. Validate empirically at all three rates. The STATE.md blocker entry about "non-integer IIR oversampling latency" confirms this is a known risk.

**Warning signs:** DAW reports N samples PDC but DryWetMixer compensates N+0.5 samples; audible at Mix=0.5 with a pure tone input.

### Pitfall 3: Stale Filter State When Switching Back to Previous Rate
**What goes wrong:** User switches from 2x to 8x and back to 2x. The 2x oversampling object's IIR filter state accumulated residue from the previous 2x period and was not reset when switching away. When switching back, the filters contain pre-transition state, causing a click or settling artifact.

**Why it happens:** Only the active oversampling object is used in `process()`, but inactive ones retain state from their last use.

**How to avoid:** When switching away from factor N, call `oversamplingObjects[N]->reset()`. When switching to factor M, the fresh reset state starts from silence, which is correct.

**Warning signs:** Audible click or brief tonal artifact immediately after switching from 8x back to 2x.

### Pitfall 4: Smoothers Running at Wrong Oversampled Rate After Switch
**What goes wrong:** `driveSmoother` and other smoothers are initialized with a ramp time at `sampleRate * 2` (2x). After switching to 4x, the smoother still steps at the 2x rate, making parameter transitions 2x faster/slower than intended.

**Why it happens:** `SmoothedValue::reset(sampleRate, rampLengthSeconds)` pre-computes the per-sample step from the given rate. If the oversampled rate doubles, the step must be recomputed.

**How to avoid:** In `setOversamplingFactor()`, re-call `reset()` on all smoothers with the new oversampled rate. Use `setCurrentAndTargetValue(getCurrentValue())` if you want to preserve current position, or just `reset()` which is acceptable at a user-initiated rate switch.

**Warning signs:** Drive transitions feel abruptly fast or unexpectedly sluggish after switching oversampling rates.

### Pitfall 5: setLatencySamples Re-entrancy in Hosts (Known Blocker from STATE.md)
**What goes wrong:** In some hosts (specifically noted: Nuendo), calling `setLatencySamples()` from inside `processBlock()` may trigger a `prepareToPlay()` callback before `processBlock()` has returned, creating a re-entrant call into `prepare()` while `process()` is still on the stack.

**Why it happens:** The VST3 spec allows hosts to call `setProcessing(false)` and `setupProcessing()` synchronously in response to a latency change notification. Some hosts (Nuendo) appear to do this immediately.

**How to avoid:** The safest pattern is to use a flag (`latencyChanged` atomic bool) set during `processBlock()` and consumed on the message thread via a timer or parameter change callback, then calling `setLatencySamples()` and triggering host notification from the message thread. However, the Crucible project calls `setLatencySamples()` directly from `processBlock()` (line 940 of PluginProcessor.cpp) and has not reported re-entrancy issues. This suggests the risk is host-specific.

**Recommendation:** Call `setLatencySamples()` from `processBlock()` as in the Crucible pattern. Document the empirical Nuendo verification step as a required pre-merge check. The STATE.md blocker ("exact guard pattern... needs empirical verification in Nuendo") remains valid as a verification obligation, not a blocking design constraint.

**Warning signs:** DAW crash or audio dropout immediately after switching oversampling rate. Debugger showing re-entrant call stack with `prepareToPlay` → `processBlock` overlap.

---

## Code Examples

Verified patterns from the Claymore codebase and Crucible reference:

### APVTS AudioParameterChoice (Crucible PluginProcessor.cpp, line 280–284)
```cpp
// Source: Crucible/Source/PluginProcessor.cpp lines 279-284
params.push_back(std::make_unique<juce::AudioParameterChoice>(
    juce::ParameterID { ParamIDs::clipOversampling, 1 },
    "Clip Oversampling",
    juce::StringArray { "Off", "2x", "4x", "8x" },
    ParamDefaults::clipOversampling));
```
For Claymore, omit "Off" and default to index 0 ("2x").

### Pre-Allocating Oversampling Array (Crucible ClipperSection.cpp, lines 43–56)
```cpp
// Source: Crucible/Source/dsp/ClipperSection.cpp lines 43-56
for (size_t i = 0; i <= 3; ++i)
{
    oversampling[i] = std::make_unique<juce::dsp::Oversampling<float>>(
        2,                    // numChannels (stereo)
        static_cast<int>(i), // oversampling order (2^i: 1x, 2x, 4x, 8x)
        juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple,
        true,  // isMaxQuality
        true   // useIntegerLatency (important for DAW PDC)
    );
    oversampling[i]->initProcessing(static_cast<size_t>(samplesPerBlock));
}
```
For Claymore: use `filterHalfBandPolyphaseIIR` (lower latency, consistent with Phase 1), and index range `i in {1, 2, 3}` for 2x/4x/8x (skipping the 1x dummy).

### Selecting Active Stage in process() (Crucible ClipperSection.cpp, lines 211–228)
```cpp
// Source: Crucible/Source/dsp/ClipperSection.cpp lines 211-228
if (currentOversamplingFactor == OversamplingFactor::Off)
{
    processClipping(block);
}
else
{
    auto index = static_cast<size_t>(currentOversamplingFactor);
    auto& os = *oversampling[index];
    auto oversampledBlock = os.processSamplesUp(block);
    processClipping(oversampledBlock);
    os.processSamplesDown(block);
}
```

### Latency Reporting on Rate Change (Crucible PluginProcessor.cpp, line 940)
```cpp
// Source: Crucible/Source/PluginProcessor.cpp line 940
setLatencySamples(clipperSection.getLatencySamples() + saturationSection.getLatencySamples());
```
For Claymore (single latency source):
```cpp
setLatencySamples(static_cast<int>(std::round(engine.getLatencyInSamples())));
dryWetMixer.setWetLatency(engine.getLatencyInSamples());
```

### FuzzCore State Prepare at New Oversampled Rate
```cpp
// When oversampling factor changes, re-prepare FuzzCoreState at new oversampled rate
// (already done in prepare(); must repeat in setOversamplingFactor for rate changes)
const double newOsRate = sampleRate * std::pow(2.0, currentOversamplingIndex + 1);
for (int ch = 0; ch < maxChannels; ++ch)
    coreState[ch].prepare(newOsRate);
```

---

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Hardcoded 2x IIR (Phase 1) | Selectable 2x/4x/8x IIR (Phase 2) | This phase | Latency increases from ~4 samples to ~12–40+ samples at higher rates; DryWetMixer capacity must grow |
| Single `Oversampling` member | Array of pre-allocated objects | Crucible reference pattern (2024) | Zero allocation during rate switch; allows instantaneous switching without audio dropout |

**Deprecated/outdated:**
- `juce::dsp::Oversampling` with a single fixed-factor constructor (Phase 1 pattern): adequate for fixed-rate but must be replaced for runtime selection.

---

## Open Questions

1. **Exact integer latency values for IIR at each rate/sample-rate combination**
   - What we know: `getLatencyInSamples()` returns the authoritative value; the DryWetMixer uses it directly; `setLatencySamples()` uses rounded integer.
   - What's unclear: The exact sample counts at 44.1 kHz, 48 kHz, 88.2 kHz, 96 kHz for 2x/4x/8x IIR with `useIntegerLatency=false`. These need empirical measurement to confirm the 256-sample DryWetMixer capacity is sufficient.
   - Recommendation: In the implementation task, add a jassert or debug log that prints `getLatencyInSamples()` for each factor after `initProcessing()` to confirm values are below 256.

2. **setLatencySamples() re-entrancy in Nuendo**
   - What we know: STATE.md identifies this as a blocker requiring empirical verification. Crucible calls `setLatencySamples()` from `processBlock()` without a guard.
   - What's unclear: Whether calling it from `processBlock()` (Crucible pattern) is sufficient for Nuendo, or whether a deferred-on-message-thread pattern is required.
   - Recommendation: Implement the Crucible pattern (direct call from processBlock). Include an explicit verification step in the plan: "Test in Nuendo — switch oversampling mid-playback 10 times, confirm no crash or dropout." If empirical test fails, implement the atomic-flag + message-thread pattern.

3. **FuzzCoreState re-prepare on rate change**
   - What we know: `FuzzCoreState::prepare(oversampledRate)` configures the tightness HPF at the oversampled rate. If the rate doubles, the HPF coefficient is stale.
   - What's unclear: Whether failing to re-prepare causes an audible artifact or just a very small coefficient error (since TPT filter coefficients are recalculated per-sample from cutoff frequency, not from stored rate-dependent coefficients).
   - Recommendation: Call `coreState[ch].prepare(newOsRate)` in `setOversamplingFactor()` to be safe. Inspect FuzzCore.h to confirm whether `prepare()` stores rate-dependent state.

---

## Sources

### Primary (HIGH confidence)
- **Local source: Crucible/Source/dsp/ClipperSection.h + .cpp** — Direct reference implementation of selectable Off/2x/4x/8x oversampling with PDC in the same project ecosystem; patterns verified by reading source.
- **Local source: Mycelium/JUCE/modules/juce_dsp/processors/juce_Oversampling.h** — Authoritative JUCE API: constructor signature, `filterHalfBandPolyphaseIIR` / `filterHalfBandFIREquiripple` enum values, `initProcessing()`, `processSamplesUp()`, `processSamplesDown()`, `getLatencyInSamples()`, `useIntegerLatency` parameter, factor range (0–4 via `jassert`).
- **Local source: Mycelium/JUCE/modules/juce_dsp/processors/juce_Oversampling.cpp** — Implementation confirms: factor=0 creates dummy stage; each added stage multiplies `factorOversampling` by 2; latency is sum of stage latencies divided by their order; `shouldUseIntegerLatency` adds Thiran fractional delay.
- **Local source: Mycelium/JUCE/modules/juce_dsp/processors/juce_DryWetMixer.h** — Confirms `DryWetMixer(int maximumWetLatencyInSamples)` constructor; `setWetLatency()` signature.
- **Local source: Claymore/Source/PluginProcessor.h + .cpp** — Current state of `DryWetMixer { 64 }` capacity, `prepareToPlay` latency reporting pattern, and `isInitialized` guard.
- **Local source: Claymore/Source/dsp/ClaymoreEngine.h** — Current fixed-2x IIR oversampling structure to be replaced.

### Secondary (MEDIUM confidence)
- **Local source: Crucible/Source/PluginProcessor.cpp line 940** — Pattern for calling `setLatencySamples()` per block from `processBlock()` on every clipper section process. Suggests no re-entrancy guard is required in practice (at least in tested hosts), but STATE.md's Nuendo concern is specific to Nuendo's VST3 implementation.

### Tertiary (LOW confidence)
- **STATE.md blocker note**: "Phase 2: Latency reporting VST3 re-entrancy — exact guard pattern for preventing concurrent prepareToPlay/processBlock during oversampling rate change needs empirical verification in Nuendo." — This is a first-person note from a prior session, not verified against official VST3 spec or JUCE forum. Treat as a flag to verify empirically, not a known confirmed bug.

---

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — All libraries are already in the project; JUCE API verified from in-repo source.
- Architecture: HIGH — Pattern copied from working Crucible implementation; key differences (no "Off" state, IIR not FIR, no lookahead) are minor and well-understood.
- Pitfalls: HIGH for 1–4 (verified from source code analysis); MEDIUM for Pitfall 5 (empirical, host-specific, not definitively confirmed).

**Research date:** 2026-02-23
**Valid until:** 2026-03-25 (JUCE stable; patterns unlikely to change)
