# Pitfalls Research

**Domain:** Standalone JUCE distortion plugin (Rat circuit port, selectable oversampling)
**Researched:** 2026-02-23
**Confidence:** MEDIUM-HIGH (JUCE forum primary evidence; DSP claims cross-verified with multiple community sources)

---

## Critical Pitfalls

### Pitfall 1: Oversampling Rate Change on the Audio Thread Causes Crashes and Glitches

**What goes wrong:**
Switching oversampling rate (2x/4x/8x) in response to a parameter change directly inside `processBlock` triggers real-time violations. The JUCE `dsp::Oversampling` object must be re-initialized when the rate changes — this involves heap allocation, filter coefficient calculation, and resizing of internal buffers. All of these are forbidden on the audio thread. The result is either an audible pop/glitch at the switch point, or an out-of-memory crash.

**Why it happens:**
Developers wire the oversampling ComboBox directly to an APVTS parameter, poll it each `processBlock`, and call `oversampler.reset()` / `initProcessing()` inline. This feels clean but is fundamentally wrong: those calls allocate memory and are not real-time-safe.

**How to avoid:**
Use a Timer (200ms polling interval) on the message thread to detect when the oversampling parameter has changed. In the timer callback: call `suspendProcessing(true)`, rebuild or re-prepare the oversampler, re-`prepare()` all downstream DSP objects with the new `ProcessSpec` (sample rate * factor, block size * factor), call `setLatencySamples(oversampler.getLatencyInSamples())`, then call `suspendProcessing(false)`. Accept the brief audio dropout — oversampling rate changes are rare user actions. Alternatively, pre-allocate all three oversamplers (2x, 4x, 8x) at `prepareToPlay` time and select between them with an atomic index that `processBlock` reads locklessly.

**Warning signs:**
- Click or dropout audible at the moment of switching
- DAW host logs a real-time deadline miss
- Valgrind/AddressSanitizer reports malloc calls from audio thread
- AudioDeviceIOCallback shows CPU spike correlating with the switch moment

**Phase to address:** DSP Port & Oversampling phase (the phase that wires up oversampling selection)

---

### Pitfall 2: Latency Reporting Triggers a Second prepareToPlay While processBlock Runs

**What goes wrong:**
Calling `setLatencySamples()` inside `prepareToPlay` causes VST3 hosts to send a component restart notification asynchronously. This triggers a second `prepareToPlay` call before the current `processBlock` finishes. On Apple Silicon with Nuendo, this is observed as a bad_access crash deep inside filter classes — `processBlock` is reading filter state while `prepareToPlay` is reinitializing it on a different thread.

**Why it happens:**
The JUCE lifecycle appears sequential but VST3 hosts are not required to serialize `prepareToPlay` and `processBlock`. The VST3 spec allows them to run concurrently. The pattern of "calculate latency from sample rate in prepareToPlay, then report it" is documented in JUCE tutorials and seems correct, but the side-effect of triggering a restart is not obvious.

**How to avoid:**
- Call `setLatencySamples()` only once at constructor time with a worst-case estimate (8x oversampling latency), or
- Use a flag that prevents `processBlock` from running while `prepareToPlay` is re-initializing (output silence/dry passthrough during that window)
- Treat DSP objects as owned by `prepareToPlay` and guard access with a lightweight spinlock only around the swap — not around normal audio processing
- Never do heavyweight allocation inside `prepareToPlay` after the initial setup; use pre-allocated structures

**Warning signs:**
- Intermittent crash only in specific DAWs (Nuendo, some Cubase versions) on Apple Silicon
- Crash inside filter or oversampler internals, not in plugin code
- No crash with AU format but crash with VST3 format of same plugin

**Phase to address:** DSP Port & Oversampling phase; verify during validation phase with both Reaper and Nuendo

---

### Pitfall 3: Dry/Wet Mix Is Phase-Misaligned When Oversampling Is Active

**What goes wrong:**
Oversampling filters introduce latency (for JUCE's IIR half-band filters, approximately 3–4 samples at 2x; proportionally more at 4x/8x). When a dry/wet control blends the processed (oversampled, delayed) signal with the unprocessed (immediate) dry signal, the result is phase-misaligned. At certain mix ratios this produces comb filtering — the wet signal sounds slightly late relative to the dry, creating a hollow, phasey artefact. At unity mix it's inaudible; at mid dry/wet settings it can be obvious.

**Why it happens:**
Developers add a dry/wet knob as an afterthought. The wet path runs through the oversample-upsample-process-downsample chain. The dry path bypasses all of this. No one adds a matching delay to the dry path, so they arrive at the summer at different times.

**How to avoid:**
Maintain a delay line on the dry signal path equal to the current oversampling latency. When oversampling rate changes, update the dry delay line length to match. JUCE's `dsp::DelayLine` is appropriate for this. Alternatively, use `juce::dsp::DryWetMixer` which provides built-in latency compensation for the dry signal.

**Warning signs:**
- Comb filtering or hollow sound at 50% dry/wet with oversampling enabled
- Null test: wet+dry at unity shows residual signal instead of silence when all other processing is bypassed
- Phase plot shows dry and wet arrival at different sample offsets

**Phase to address:** DSP Port & Oversampling phase — must be designed in from the start, not patched later

---

### Pitfall 4: processBlock Called Before prepareToPlay by Some Hosts

**What goes wrong:**
Several hosts (FL Studio Patcher, some Maschine configurations, older Ableton versions) call `processBlock` before ever calling `prepareToPlay`. If the DSP chain is uninitialized — no `ProcessSpec` set, no filter coefficients computed, no oversampling object ready — the plugin crashes or outputs garbage on load.

**Why it happens:**
Developers assume `prepareToPlay` is always called first. The JUCE contract implies this, but hosts are not required to follow it strictly. The ported Rat DSP expects a valid sample rate and oversampled buffer size to be set before any processing.

**How to avoid:**
Add an `isInitialized` atomic bool that `prepareToPlay` sets to `true`. At the top of `processBlock`, check this flag; if not set, output silence (zero the output buffer) and return immediately. This is a single compare of an atomic variable — negligible cost. Do not attempt lazy initialization from `processBlock`.

**Warning signs:**
- Plugin crashes on load in FL Studio Patcher specifically
- Crash happens at plugin insert, not during playback
- Stack trace shows DSP object method called on null or uninitialized state

**Phase to address:** DSP Port phase — defensive initialization guard belongs in the initial port

---

## Moderate Pitfalls

### Pitfall 5: JUCE dsp::Oversampling Stage Count Is Not the Same as the Factor

**What goes wrong:**
`juce::dsp::Oversampling` is built from stages, where each stage doubles the rate. To get 4x oversampling you need 2 stages; for 8x you need 3 stages. Developers assume a `factorOversampling` property directly controls the factor and try to swap it to switch rates — this does not work. The result is the object silently runs at 2x regardless of the intended factor.

**Why it happens:**
The API is not immediately obvious. The class template parameter is the maximum number of channels; the factor is derived from how many stages are added, not from a single settable property. Tutorials rarely show the multi-rate case.

**How to avoid:**
Allocate one `dsp::Oversampling` object per rate (or rebuild with the correct stage count when switching). For the pre-allocated-three-objects approach: create `oversamplers[3]` at construction — `{1 stage, 2 stages, 3 stages}` — and select by index. Each must be separately `reset()` and `initProcessing()`'d in `prepareToPlay`.

**Warning signs:**
- 4x and 8x sound identical to 2x (aliasing is not reduced)
- CPU does not increase proportionally with the selected rate
- Filter rolloff is at the wrong frequency

**Phase to address:** DSP Port & Oversampling phase

---

### Pitfall 6: DC Offset Accumulation Through the Distortion Chain

**What goes wrong:**
The Rat circuit's bias-starve (Sag parameter) and hard/soft clip blending can introduce asymmetric waveforms, which produce DC offset. DC offset passed to the limiter stage or output causes the limiter to waste headroom clamping a constant offset, not transients. DC in the output also causes problems in DAW metering and downstream clipping.

**Why it happens:**
The original circuit's op-amp (LM308 model) produces inherently asymmetric clipping. The soft-clip blending in FuzzCore may not perfectly cancel the DC component introduced by the bias offset parameter. In a hardware pedal, DC is blocked by capacitors; the plugin equivalent (a highpass filter) is easy to forget.

**How to avoid:**
Insert a 1-pole highpass DC-blocking filter (cutoff ~5–10 Hz) at the output of the FuzzStage, before the output gain stage and limiter. JUCE's `dsp::IIR::Coefficients<float>::makeHighPass` at 10 Hz works. Verify with a DC offset meter on the output during steady-state processing with Sag at maximum.

**Warning signs:**
- Oscilloscope view in DAW shows waveform not centered at zero
- Output limiter engages without any audio input when Sag/Gain is high
- Noise gate has difficulty finding silence threshold

**Phase to address:** DSP Port phase — verify with DC measurement immediately after porting FuzzStage

---

### Pitfall 7: AU Validation Failures From Listener Cleanup and Editor Management

**What goes wrong:**
`auval` and `pluginval` both exercise plugin destruction and re-creation aggressively. Two common failure modes: (1) `AudioProcessorValueTreeState` listeners are not removed in the Component destructor, causing use-after-free crashes. (2) AUv2 hits the assertion "There's already an active editor!" when the host tries to create a second editor instance before the first is fully destroyed.

**Why it happens:**
JUCE's APVTS uses `ValueTree::Listener` objects. When a Component that added itself as a listener is destroyed, the listener must be explicitly removed. JUCE does not do this automatically. For the duplicate editor issue, some hosts call `createEditor()` before confirming the old editor is gone.

**How to avoid:**
In every Component destructor that calls `addListener`, add the matching `removeListener` call. Use JUCE's `juce::ChangeListener` RAII helper or `juce::Component::SafePointer` to guard editor access. Run `pluginval` at strictness level 5+ as a CI gate before every release. Run `auval -v aufx NMcM [bundle_id]` on every AU build.

**Warning signs:**
- `pluginval` crashes but the plugin works in DAWs
- Intermittent crashes when rapidly opening/closing the plugin editor
- `auval` output mentions "editor" or "listener" in the failure message

**Phase to address:** Validation phase — must pass pluginval and auval before shipping

---

### Pitfall 8: Noise Gate Placed After Distortion Suppresses Sustain, Not Noise

**What goes wrong:**
The noise gate is placed at the end of the signal chain (after distortion). High-gain distortion creates a sustained noise floor that the gate must work against. The gate's threshold must be set so high that it also cuts musical sustain tails, producing an unmusical "chop." Users complain the gate is unusable.

**Why it happens:**
The intuitive serial order is: input → distortion → gate → output. This feels correct — gate what's noisy. But distortion raises the noise floor along with the signal; the gate sees noise + signal as one blob and cannot distinguish them.

**How to avoid:**
Place the noise gate before the distortion stage (on the pre-distortion signal). The gate sees the clean input signal and can accurately detect silence vs. playing. When the player stops, the gate closes before the distortion stage ever sees the signal — the distortion's noise floor is never produced. Expose a single Threshold control; the gate operates on the pre-distortion signal internally.

**Warning signs:**
- Gate threshold needs to be very high to cut noise, but doing so chops sustain
- Natural decay of notes is cut abruptly
- Users report gate is either "always on" or "always off"

**Phase to address:** DSP Port phase — signal chain order must be defined at architecture time

---

### Pitfall 9: Parameter State Not Restored Correctly on Plugin Reload

**What goes wrong:**
Reloading a session with the plugin in Logic or Ableton shows parameters at their default values despite the session having saved non-default values. The dry/wet, noise gate threshold, or oversampling rate resets.

**Why it happens:**
Two common root causes: (1) `getStateInformation` serializes state but `setStateInformation` uses `apvts.replaceState()` with a ValueTree built before the APVTS is fully constructed — the timing of `setStateInformation` relative to `prepareToPlay` varies by host. (2) Non-APVTS state (like the oversampling rate ComboBox selection stored separately) is not included in `getStateInformation` at all.

**How to avoid:**
Store all persistent state in the APVTS, including the oversampling rate (as a choice parameter). Use `apvts.copyState()` in `getStateInformation` to get a thread-safe snapshot. In `setStateInformation`, use `apvts.replaceState(ValueTree::fromXml(...))`. Test state roundtrip explicitly: set non-default values, call `getStateInformation`, call `setStateInformation`, verify all values.

**Warning signs:**
- Plugin opens with default settings in Logic but remembers settings in Reaper
- Oversampling rate resets to 2x on every session load
- A specific parameter always shows its default value after reload

**Phase to address:** DSP Port phase (state management) — test in at least two DAWs before shipping

---

### Pitfall 10: DSP Port Creates Hidden Coupling to Gunk Lord's Infrastructure

**What goes wrong:**
Copying FuzzCore.h, FuzzTone.h, FuzzStage.h into Claymore brings along `#include` chains that reference Gunk Lord-specific headers — shared types, configuration enums, or plugin-wide constants from the parent project. The files compile only because the include paths happen to resolve against Gunk Lord's source tree on the developer's machine. On a clean Claymore checkout, builds fail with missing headers.

**Why it happens:**
DSP files in a larger plugin often grow implicit dependencies on the surrounding project. Headers include other headers that include plugin-wide config files. Nobody audits this because everything builds fine in the original project.

**How to avoid:**
Before copying any file, run `gcc -M` (or equivalent) on each to list all transitive dependencies. Copy only files with no Gunk Lord-specific dependencies, or inline/replace any shared types. After copying, do a clean build of Claymore without Gunk Lord's include paths on the compiler line. A CI build from a fresh directory (not the developer's machine) will catch this immediately.

**Warning signs:**
- Build succeeds locally but fails on another machine or in CI
- `#include "../../gunklord/source/..."` paths exist anywhere in ported files
- FuzzConfig.h or FuzzType.h includes anything not also copied over

**Phase to address:** DSP Port phase — must be the first thing verified after copying files

---

## Technical Debt Patterns

| Shortcut | Immediate Benefit | Long-term Cost | When Acceptable |
|----------|-------------------|----------------|-----------------|
| Hardcode 2x oversampling only initially | Ship faster, simpler code | Must refactor DSP chain, APVTS, and latency reporting when adding 4x/8x; high risk of regression | Never — selectable oversampling is a stated requirement, design for it from the start |
| Use `std::mutex` in processBlock for oversampling switch | Appears to prevent race conditions | Causes priority inversion, audio dropouts under load; will fail real-time safety audits | Never in processBlock — use atomic swap or suspension pattern instead |
| Skip DC offset filter until user reports it | Saves one filter | DC offset manifests as mysterious limiter engagement; hard to diagnose late | Never — add DC block at port time, costs one IIR coefficient |
| Not running pluginval/auval until pre-release | Faster iteration during development | Listener and editor bugs accumulate; late discovery means late rewrites | Acceptable to skip on every commit, but must run weekly and before any distribution |
| Storing oversampling rate outside APVTS | Simpler to implement as a member variable | Rate silently resets on session reload; difficult to debug | Never — all persistent state belongs in APVTS |

---

## Performance Traps

| Trap | Symptoms | Prevention | When It Breaks |
|------|----------|------------|----------------|
| Allocating a new Oversampling object in processBlock during rate switch | CPU spike, audio glitch, possible crash | Use pre-allocated objects or suspension pattern | Every time user changes oversampling rate |
| All three Oversampling objects (2x/4x/8x) running simultaneously "just in case" | 14x CPU usage compared to active rate only | Only prepare/run the selected oversampler | At 8x with complex DSP: may cause dropout on slower machines |
| IIR half-band filter transient on reset | Click/pop after oversampling switch | Zero filter state on reset, output silence for one buffer after switch | Every oversampling rate change if not handled |
| ProcessorChain with oversampling stages: forgetting to update ProcessSpec for inner DSP | Distortion DSP running at wrong sample rate | Ensure all inner processors get `ProcessSpec{sampleRate * factor, blockSize * factor, channels}` | At 4x/8x — Rat DSP sees wrong sample rate, tone circuit tuning is wrong |
| Non-integer oversampling latency from IIR filters | Dry/wet phase error, auval latency test failure | Round reported latency to nearest integer sample; compensate fractional difference in dry delay | Always with IIR half-band — must be handled by design |

---

## UX Pitfalls

| Pitfall | User Impact | Better Approach |
|---------|-------------|-----------------|
| No visual indication of active oversampling rate | User doesn't know if high-quality mode is on; no feedback after ComboBox change | Highlight the active rate button; show CPU indicator or label |
| Oversampling switch causes audible dropout | Jarring experience during live monitoring | Document expected behaviour; show "switching..." state or disable audio while switching |
| Noise gate threshold labeled in dBFS from output perspective | User tunes gate based on output level but gate operates pre-distortion; threshold seems wrong | Label in terms of input signal level; document that threshold is pre-distortion |
| Knob drag sensitivity mismatched from hardware pedal expectation | Guitar players used to large rotary throws; JUCE default linear drag feels coarse | Use velocity-sensitive drag or high pixel-per-unit ratio for fine control |
| Plugin window non-resizable at a fixed small size | Unreadable on high-DPI displays or in small DAW windows | Support at least 2x scaling; test on Retina/HiDPI displays |

---

## "Looks Done But Isn't" Checklist

- [ ] **Oversampling rate switch:** Sounds clean — verify no memory allocation happens on audio thread during switch (use a memory profiler or JUCE `JUCE_ENABLE_AUDIO_GUARD`)
- [ ] **Dry/wet mix:** Sounds correct — run null test: invert wet, sum with dry at 50/50; should be near silence if aligned; comb filtering indicates latency mismatch
- [ ] **DC offset:** Output meter reads clean — measure DC component with all parameters at maximum Sag/Gain; should be < -90 dBFS
- [ ] **State restore:** Parameters save and load — test in Logic AND Reaper; oversampling rate must restore correctly
- [ ] **AU validation:** Passes pluginval — must also pass `auval -v aufx NMcM [bundle_id]` at strictness 5
- [ ] **Universal Binary:** Both architectures ship — verify with `lipo -info` on every output binary (AU component and VST3 bundle)
- [ ] **Noise gate ordering:** Gate closes on silence — with maximum Gain and Sag, silence input; gate should close without cutting sustain tails at moderate threshold
- [ ] **Latency reporting:** Host compensates correctly — insert plugin between two identical tracks; null them; verify host PDC aligns them

---

## Recovery Strategies

| Pitfall | Recovery Cost | Recovery Steps |
|---------|---------------|----------------|
| Audio thread allocation on oversampling switch | MEDIUM | Audit processBlock for all non-RT-safe calls; refactor to timer-based suspension pattern; 1–2 days |
| Latency reporting crash | HIGH | Reproduce in specific DAW; add flag-based guard around DSP init; verify no concurrent access; 2–4 days including testing |
| Dry/wet phase misalignment | MEDIUM | Add DelayLine on dry path matching oversampling latency; update on rate change; 1 day |
| DC offset accumulation | LOW | Insert IIR highpass at 10 Hz post-FuzzStage; 2 hours |
| Hidden Gunk Lord include dependencies | MEDIUM | Audit all transitive includes; inline or copy needed types; clean build verify; 1 day |
| State not restoring | LOW | Add non-APVTS state to getStateInformation/setStateInformation; 2–4 hours |
| AU validation failures | MEDIUM | Fix removeListener calls in destructors; run auval to get specific failure messages; 1–3 days |
| processBlock before prepareToPlay crash | LOW | Add isInitialized atomic guard; output silence when false; 1–2 hours |

---

## Pitfall-to-Phase Mapping

| Pitfall | Prevention Phase | Verification |
|---------|------------------|--------------|
| Oversampling rate change on audio thread | DSP Port & Oversampling (Phase 2) | Timer-based switch with audio profiler; no malloc in processBlock during switch |
| Latency reporting triggers double prepareToPlay | DSP Port & Oversampling (Phase 2) | Test in VST3 format in Nuendo/Cubase; no crash after 50 open/close cycles |
| Dry/wet phase misalignment | DSP Port & Oversampling (Phase 2) | Null test at 50% dry/wet with each oversampling rate |
| processBlock before prepareToPlay crash | DSP Port (Phase 1) | Load plugin in FL Studio; no crash; silence output before initialization |
| dsp::Oversampling stage count vs. factor | DSP Port & Oversampling (Phase 2) | Spectrum analyzer confirms aliasing reduction at each rate |
| DC offset accumulation | DSP Port (Phase 1) | DC measurement immediately post-FuzzStage port |
| AU validation failures | Validation (Phase 4) | pluginval at level 5; auval passing; 50 editor open/close cycles |
| Noise gate placement | DSP Port (Phase 1) | Gate closes on silence without cutting sustain at moderate threshold |
| Parameter state not restoring | DSP Port (Phase 1) | Roundtrip test in Logic and Reaper |
| Hidden Gunk Lord include coupling | DSP Port (Phase 1) | Clean CI build with no Gunk Lord source on include path |

---

## Sources

- JUCE Forum: [Advice on dynamically changing oversampling factor](https://forum.juce.com/t/advice-needed-on-when-and-where-to-dynamically-change-the-oversampling-factor-is-this-code-ok/58162) — timer-based suspension pattern, latency update requirement
- JUCE Forum: [Switchable dsp::Oversampling Factor](https://forum.juce.com/t/switchable-dsp-oversampling-factor/50734) — stage count vs. factor misunderstanding
- JUCE Forum: [prepareToPlay and processBlock thread-safety](https://forum.juce.com/t/preparetoplay-and-processblock-thread-safety/32193) — concurrent call race conditions, latency reporting re-entrancy
- JUCE Forum: [processBlock called before prepareToPlay](https://forum.juce.com/t/processblock-called-before-preparetoplay/53562) — FL Studio Patcher issue, defensive guard pattern
- JUCE Forum: [Dry/Wet mix in plugin with latency](https://forum.juce.com/t/dry-wet-mix-in-plugin-with-latency/4967) — delay line on dry path for phase alignment
- JUCE Forum: [How to report Plugin latency](https://forum.juce.com/t/how-to-report-plugin-latency/55869) — setLatencySamples and integer rounding requirement
- JUCE Forum: [Bug AUv2 hitting assert "There's already an active editor!" with pluginval](https://forum.juce.com/t/bug-1628-auv2-hitting-assert-theres-already-an-active-editor-with-pluginval/68224) — editor lifecycle AU failure
- Melatonin Blog: [Pluginval is a plugin dev's best friend](https://melatonin.dev/blog/pluginval-is-a-plugin-devs-best-friend/) — removeListener cleanup, auval at strictness 5+
- Moonbase: [Code signing audio plugins in 2025](https://moonbase.sh/articles/code-signing-audio-plugins-in-2025-a-round-up/) — notarization requirements for macOS distribution
- KVR Audio: [Antialiasing filters — IIR or FIR?](https://www.kvraudio.com/forum/viewtopic.php?t=439271) — IIR latency and phase tradeoffs for oversampling filters
- KVR Audio: [Dealing with feedback loops](https://www.kvraudio.com/forum/viewtopic.php?t=456191) — DC offset highpass filtering to prevent instability
- Sweetwater InSync: [Where should I put my Noise Gate?](https://www.sweetwater.com/insync/where-should-i-put-my-noise-gate-in-my-guitar-signal-chain/) — gate pre-distortion signal chain ordering

---
*Pitfalls research for: Claymore — standalone Rat distortion plugin (JUCE 8, macOS)*
*Researched: 2026-02-23*
