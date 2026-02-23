# Architecture Research

**Domain:** JUCE 8 standalone distortion audio plugin (VST3 + AU)
**Researched:** 2026-02-23
**Confidence:** HIGH — Based on direct inspection of source DSP files (FuzzCore.h, FuzzTone.h, FuzzStage.h), working reference implementations (Crucible's ClipperSection with selectable oversampling), and GunkLord's PluginProcessor. JUCE API cross-referenced against official docs.

---

## Standard Architecture

### System Overview

```
┌────────────────────────────────────────────────────────────────┐
│                     PluginEditor (UI Thread)                    │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────────┐   │
│  │  Knobs   │  │ OS Menu  │  │  Bypass  │  │  LookAndFeel │   │
│  │ (APVTS   │  │ (2x/4x/ │  │  Button  │  │  (pedal skin)│   │
│  │ attach.) │  │   8x)   │  │          │  │              │   │
│  └────┬─────┘  └────┬─────┘  └────┬─────┘  └──────────────┘   │
└───────┼─────────────┼─────────────┼────────────────────────────┘
        │ APVTS (parameter tree, thread-safe)
┌───────┼─────────────┼─────────────┼────────────────────────────┐
│       ▼             ▼             ▼                             │
│                  PluginProcessor (Audio Thread)                  │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │  processBlock()                                          │   │
│  │  1. Read atomic param pointers                          │   │
│  │  2. Apply input gain (smoothed)                         │   │
│  │  3. Push dry to DryWetMixer                             │   │
│  │  4. ClaymoreEngine.process()                            │   │
│  │  5. DryWetMixer.mixWetSamples() → apply mix param       │   │
│  │  6. Apply output gain (smoothed)                        │   │
│  │  7. OutputLimiter.process()                             │   │
│  └─────────────────────────────────────────────────────────┘   │
├─────────────────────────────────────────────────────────────────┤
│                     ClaymoreEngine                               │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │  NoiseGate → Upsample → FuzzCore (per-sample) →          │  │
│  │  Downsample → FuzzTone (LP sweep + presence shelf + DC)  │  │
│  └──────────────────────────────────────────────────────────┘  │
│  ┌───────────────────────────────────────────────────────────┐  │
│  │  OversamplingManager                                       │  │
│  │  std::array<unique_ptr<dsp::Oversampling>, 3>             │  │
│  │  [0]=2x  [1]=4x  [2]=8x   (all pre-allocated in prepare) │  │
│  └───────────────────────────────────────────────────────────┘  │
├─────────────────────────────────────────────────────────────────┤
│                     Parameters (APVTS)                           │
│  Gain | Symmetry | Tightness | Sag | Tone | Presence |          │
│  NoiseGate | InputGain | OutputGain | Mix | Oversampling         │
└─────────────────────────────────────────────────────────────────┘
```

### Component Responsibilities

| Component | Responsibility | Typical Implementation |
|-----------|----------------|------------------------|
| `PluginProcessor` | JUCE host interface; reads APVTS params; routes audio; reports latency | Subclass of `juce::AudioProcessor`; owns ClaymoreEngine, DryWetMixer, OutputLimiter, SmoothedValues for input/output gain |
| `ClaymoreEngine` | Complete distortion signal chain; owns oversampling selection | New class wrapping ported FuzzStage logic with selectable oversampling; replaces fixed-2x design |
| `FuzzCore` (ported) | Per-sample Rat waveshaping: slew filter, clipping blend, sag | Pure function `processSample()` + `FuzzCoreState` struct per channel — copied verbatim from GunkLord |
| `FuzzTone` (ported) | Post-distortion tone circuit: LP sweep + presence shelf + DC block | Class with `applyTone(buffer)` — copied verbatim from GunkLord |
| `FuzzConfig` (ported) | Drive mapping constants | Namespace with `mapDrive()` — copied verbatim from GunkLord |
| `OutputLimiter` | Brickwall soft-knee limiter at final output | `juce::dsp::Limiter<float>` or a thin wrapper; no oversampling needed here |
| `PluginEditor` | Pedal-style GUI; knob layout; oversampling selector | Subclass of `juce::AudioProcessorEditor`; uses APVTS attachments for all controls |
| `Parameters` (header) | Central parameter ID constants and APVTS layout factory | Static `createParameterLayout()` function; keeps processor and editor in sync on IDs |

---

## Recommended Project Structure

```
Source/
├── PluginProcessor.h/.cpp      # JUCE host interface, processBlock(), latency reporting
├── PluginEditor.h/.cpp         # Pedal GUI, knobs, oversampling selector
├── Parameters.h                # APVTS layout + parameter ID string constants
│
├── dsp/
│   ├── ClaymoreEngine.h/.cpp   # Full signal chain: gate → oversample → fuzz → tone
│   ├── OutputLimiter.h         # Thin wrapper around juce::dsp::Limiter
│   │
│   └── fuzz/                   # Ported verbatim from GunkLord (minimal changes)
│       ├── FuzzCore.h          # processSample() + FuzzCoreState — NO changes needed
│       ├── FuzzTone.h          # applyTone() — NO changes needed
│       ├── FuzzConfig.h        # Constants + mapDrive() — NO changes needed
│       └── FuzzType.h          # Enum (retained for reference) — NO changes needed
│
└── ui/
    ├── ClaymoreKnob.h/.cpp     # Custom rotary knob component (pedal style)
    ├── ClaymoreColours.h        # Colour palette constants
    └── ClaymoreIDs.h            # Component IDs for look and feel
```

### Structure Rationale

- **`dsp/fuzz/` as verbatim port:** The four GunkLord files (FuzzCore, FuzzTone, FuzzConfig, FuzzType) require zero logical changes. Keeping them in a sub-folder makes the provenance clear and simplifies future sync if GunkLord's algorithm evolves. Only their `#include` paths may need updating.
- **`ClaymoreEngine` replaces FuzzStage:** FuzzStage hardcodes 2x oversampling and owns a noise gate. ClaymoreEngine reimplements the wrapper with a `std::array` of three pre-allocated `dsp::Oversampling` objects (2x/4x/8x) and exposes an `int currentOversamplingIndex` that can be swapped safely between blocks. This is the only structural departure from GunkLord.
- **`Parameters.h` centralizes IDs:** Prevents string drift between processor (where parameters are registered) and editor (where APVTS attachments use parameter IDs). A single header with `constexpr juce::StringRef` constants eliminates a whole class of typo bugs.
- **`ui/` separation:** Keeps GUI components decoupled from DSP. The editor only reads from APVTS — it never calls into ClaymoreEngine directly.

---

## Architectural Patterns

### Pattern 1: Pre-Allocate All Oversampling Objects in prepare()

**What:** Create one `juce::dsp::Oversampling<float>` for each supported factor (2x, 4x, 8x) during `prepareToPlay()`. Store in `std::array<std::unique_ptr<juce::dsp::Oversampling<float>>, 3>`. At process time, switch between them by index — no allocation occurs.

**When to use:** Any time oversampling factor is user-selectable and must change without a prepare() call.

**Trade-offs:** Uses ~3x the memory of a single oversampler. Acceptable for a focused plugin — the total overhead is a few kilobytes of filter state. Alternative (re-initializing a single oversampler) causes audio glitches on switch.

**Example:**
```cpp
// In ClaymoreEngine.h
std::array<std::unique_ptr<juce::dsp::Oversampling<float>>, 3> oversamplers;
int currentOversamplerIndex = 0; // 0=2x, 1=4x, 2=8x
std::atomic<int> pendingOversamplerIndex { 0 }; // written by UI thread

// In prepare():
// orders: 1=2x, 2=4x, 3=8x
for (int i = 0; i < 3; ++i)
{
    oversamplers[i] = std::make_unique<juce::dsp::Oversampling<float>>(
        numChannels,
        i + 1,  // oversampling order (2^(i+1))
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
        true,   // isMaxQuality
        true    // useIntegerLatency (required for DAW PDC)
    );
    oversamplers[i]->initProcessing(static_cast<size_t>(maxBlockSize));
}

// In process():
// Check for pending switch at block boundary (safe: atomic read)
int pending = pendingOversamplerIndex.load(std::memory_order_relaxed);
if (pending != currentOversamplerIndex)
{
    oversamplers[currentOversamplerIndex]->reset();
    currentOversamplerIndex = pending;
    // Notify processor to update reported latency on next block
}
auto& os = *oversamplers[currentOversamplerIndex];
auto oversampledBlock = os.processSamplesUp(block);
// ... process ...
os.processSamplesDown(block);
```

### Pattern 2: Cached Atomic Parameter Pointers

**What:** In `prepareToPlay()`, use `apvts.getRawParameterValue("paramId")` once to get `std::atomic<float>*` pointers. Cache them as private member variables. In `processBlock()`, read these pointers directly — no string lookups on the audio thread.

**When to use:** Every JUCE plugin using APVTS. This is the established pattern in both GunkLord and Crucible.

**Trade-offs:** Requires careful management (pointers become invalid if APVTS is destroyed, but lifetime is tied to the processor). Minor boilerplate vs. major performance benefit on audio thread.

**Example:**
```cpp
// In PluginProcessor.h (private):
std::atomic<float>* gainParam      = nullptr;
std::atomic<float>* symmetryParam  = nullptr;
std::atomic<float>* oversamplingParam = nullptr;

// In prepareToPlay():
gainParam         = apvts.getRawParameterValue ("gain");
symmetryParam     = apvts.getRawParameterValue ("symmetry");
oversamplingParam = apvts.getRawParameterValue ("oversampling");

// In processBlock():
engine.setGain (gainParam->load (std::memory_order_relaxed));
engine.setSymmetry (symmetryParam->load (std::memory_order_relaxed));
int osIndex = static_cast<int> (oversamplingParam->load (std::memory_order_relaxed));
engine.setPendingOversamplingIndex (osIndex);
```

### Pattern 3: DryWetMixer for Mix Parameter with Latency Compensation

**What:** Use `juce::dsp::DryWetMixer<float>` for the dry/wet mix control. The mixer internally delay-lines the dry signal to match the wet path's oversampling latency, so the dry signal stays time-aligned regardless of the oversampling factor.

**When to use:** Any plugin with a mix/blend parameter where the wet path introduces latency (oversampling always does).

**Trade-offs:** The `maximumWetLatencyInSamples` must be set at construction time (it cannot be changed after the fact without reconstruction). Set it to the maximum possible latency — 8x FIR oversampling at 192kHz can be ~60-80 samples; 64 samples is a safe floor. IIR oversampling (used in GunkLord) has much shorter latency (~4 samples at 2x).

**Example:**
```cpp
// In PluginProcessor.h:
// Capacity for up to 8x FIR oversampling latency at high sample rates
juce::dsp::DryWetMixer<float> dryWetMixer { 128 };

// In prepareToPlay():
juce::dsp::ProcessSpec spec { sampleRate, (uint32) samplesPerBlock, (uint32) numChannels };
dryWetMixer.prepare (spec);
dryWetMixer.setWetLatency (engine.getLatencyInSamples());

// In processBlock():
dryWetMixer.setWetMixProportion (mixParam->load (std::memory_order_relaxed));
dryWetMixer.pushDrySamples (inputBlock);
engine.process (buffer);
dryWetMixer.mixWetSamples (wetBlock);
```

### Pattern 4: Latency Reporting via setLatencySamples()

**What:** Call `setLatencySamples()` in `processBlock()` (or `prepareToPlay()`) whenever the oversampling factor changes. The host uses this for Plugin Delay Compensation (PDC) to keep the plugin time-aligned with other tracks.

**When to use:** Any plugin with variable latency (selectable oversampling, lookahead limiters).

**Trade-offs:** Only call when latency actually changes — calling every block is wasteful but not incorrect. The oversampling factor change only happens at block boundaries anyway.

**Example:**
```cpp
// In processBlock(), after checking for pending oversampling switch:
if (oversamplingChanged)
{
    setLatencySamples (engine.getLatencyInSamples());
    dryWetMixer.setWetLatency (engine.getLatencyInSamples());
}
```

### Pattern 5: SmoothedValue for Audio-Rate Parameter Updates

**What:** Carry GunkLord's existing smoothing strategy into ClaymoreEngine. Drive: 10ms linear ramp. All other controls (Symmetry, Tightness, Sag, Tone, Presence): 5ms linear ramp. Input/output gain: multiplicative smoothing to avoid additive gain steps.

**When to use:** All continuously-variable parameters that feed per-sample processing. Prevents zipper noise on knob automation.

**Trade-offs:** Smoothers must be reset to current value (not target) on `reset()` to prevent artifacts when DAW transport restarts.

---

## Data Flow

### Audio Signal Flow (processBlock)

```
DAW Audio Input
      ↓
[Input Gain Smoother] (multiplicative SmoothedValue)
      ↓
[DryWetMixer::pushDrySamples()] ← dry branch stored here
      ↓
[ClaymoreEngine::process()]
      │
      ├── [NoiseGate] (juce::dsp::NoiseGate, at original rate)
      │
      ├── [Oversamplers[currentIndex]::processSamplesUp()]
      │
      ├── [Per-sample loop, oversampled rate]:
      │     FuzzCore::processSample()  (drive, symmetry, sag + slew filter)
      │     × outputCompensation
      │
      ├── [Oversamplers[currentIndex]::processSamplesDown()]
      │
      └── [FuzzTone::applyTone()] (LP sweep + presence shelf + DC block, original rate)
      ↓
[DryWetMixer::mixWetSamples()] ← blends delayed dry + wet by mix param
      ↓
[Output Gain Smoother] (multiplicative SmoothedValue)
      ↓
[OutputLimiter::process()] (juce::dsp::Limiter, brickwall)
      ↓
DAW Audio Output
```

### Parameter Flow (GUI → Audio Thread)

```
[User turns knob in PluginEditor]
      ↓
[APVTS parameter update] (JUCE handles thread safety)
      ↓
[std::atomic<float> value updated] (written by message thread)
      ↓
[processBlock() reads atomic<float>* pointer]
      ↓
[SmoothedValue::setTargetValue()] (starts ramp toward new value)
      ↓
[Per-sample: SmoothedValue::getNextValue()] → FuzzCore / FuzzTone
```

### Oversampling Factor Change Flow

```
[User selects 2x/4x/8x in PluginEditor]
      ↓
[APVTS int parameter updated] (message thread)
      ↓
[processBlock() reads pendingOversamplerIndex (atomic)]
      ↓
[Block boundary: currentIndex != pendingIndex]
      ↓
[oldOversampler->reset()] + [currentIndex = pendingIndex]
      ↓
[setLatencySamples(engine.getLatencyInSamples())]
      ↓
[dryWetMixer.setWetLatency(newLatency)]
      ↓
DAW PDC adjusts automatically
```

### State Persistence Flow (DAW Save/Load)

```
[getStateInformation()]
      ↓
[apvts.copyState()] → serialize APVTS ValueTree to MemoryBlock

[setStateInformation()]
      ↓
[apvts.replaceState()] → deserialize ValueTree
      ↓
APVTS notifies all parameter listeners automatically
```

---

## Suggested Build Order

This order respects dependencies — each layer only requires what is already built.

| Order | Component | Depends On | Milestone |
|-------|-----------|------------|-----------|
| 1 | `Parameters.h` — define all parameter IDs and APVTS layout | Nothing | Foundation |
| 2 | Port `fuzz/` files verbatim — FuzzConfig, FuzzType, FuzzCore, FuzzTone | `juce_dsp` | Foundation |
| 3 | `ClaymoreEngine` — wraps ported fuzz files, adds selectable oversampling | fuzz/ | Core DSP |
| 4 | `PluginProcessor` — APVTS, processBlock(), latency reporting, state I/O | ClaymoreEngine, Parameters | Core DSP |
| 5 | `OutputLimiter` — thin wrapper, add to processBlock() signal chain | PluginProcessor | Core DSP |
| 6 | Build validation — host plugin in DAW, verify audio, check PDC | All DSP | Core DSP |
| 7 | `ClaymoreKnob` / `ClaymoreColours` — custom look and feel | juce_gui_basics | GUI |
| 8 | `PluginEditor` — pedal layout, APVTS attachments, oversampling selector | All DSP, LookAndFeel | GUI |
| 9 | CMake build scripts — VST3 + AU targets, Universal Binary flags | All | Delivery |
| 10 | Notarization / distribution packaging | CMake done | Delivery |

---

## Anti-Patterns

### Anti-Pattern 1: Allocating Inside processBlock()

**What people do:** Constructing `juce::AudioBuffer` or oversampling objects inside `processBlock()` as a quick fix.

**Why it's wrong:** Memory allocation on the audio thread causes priority inversion and dropouts. The DAW's audio callback has a hard real-time deadline. Any system call (malloc, new) can block for an arbitrary duration.

**Do this instead:** Pre-allocate everything in `prepareToPlay()` and `reset()`. Use `setSize()` with `keepExistingContent=false, clearExtraSpace=false, avoidReallocating=true` when resizing pre-allocated buffers.

### Anti-Pattern 2: String Lookups in processBlock()

**What people do:** Calling `apvts.getParameter("paramId")->getValue()` inside `processBlock()` to read current parameter values.

**Why it's wrong:** `getParameter()` does a linear search through the parameter list. Called at 44100+ Hz for every parameter, this degrades CPU measurably. Confirmed issue in GunkLord's codebase comment ("Cached atomic parameter pointers for audio thread access").

**Do this instead:** Cache `std::atomic<float>*` from `apvts.getRawParameterValue()` in `prepareToPlay()`. Read with `load(std::memory_order_relaxed)` in `processBlock()`.

### Anti-Pattern 3: Re-Initializing Oversampler on Factor Change

**What people do:** Calling `oversampler.initProcessing()` or reconstructing the `dsp::Oversampling` object when the user switches oversampling factor at runtime.

**Why it's wrong:** `initProcessing()` allocates memory. Reconstruction deallocates and reallocates. Both are forbidden on the audio thread. Both also reset filter state, causing a pop or silence.

**Do this instead:** Pre-allocate separate `dsp::Oversampling` objects for each supported factor in `prepareToPlay()`. Switch by changing which one you call in `processBlock()`. Reset the old one before switching (to avoid stale filter state on next use). This is the approach Crucible's ClipperSection uses — see `std::array<std::unique_ptr<dsp::Oversampling<float>>, 4> oversampling`.

### Anti-Pattern 4: Not Reporting Latency to Host

**What people do:** Add oversampling and forget to call `setLatencySamples()`. Plugin works in isolation but is misaligned with other tracks.

**Why it's wrong:** The DAW trusts `getLatencySamples()` to compensate for the plugin's delay. Without correct reporting, the distorted signal arrives late relative to the dry DAW bus — audible as pre-ringing or comb filtering in parallel processing.

**Do this instead:** Call `setLatencySamples(engine.getLatencyInSamples())` in `prepareToPlay()` and again whenever the oversampling factor changes. Match this value to `dryWetMixer.setWetLatency()`.

### Anti-Pattern 5: Applying Tone Filtering at the Oversampled Rate

**What people do:** Moving FuzzTone's LP sweep and presence shelf inside the oversampling loop to process them at the upsampled rate.

**Why it's wrong:** FuzzTone is designed to run at the original sample rate. Its coefficients (especially the 4 kHz presence shelf and the DC blocker at 20 Hz) are calibrated for that rate. Running it at 2x/4x/8x shifts all filter frequencies and changes the character of the tone circuit.

**Do this instead:** Preserve GunkLord's architecture: waveshaping happens inside the oversampled block, FuzzTone runs on the downsampled buffer after `processSamplesDown()`. This is already correct in FuzzStage.h — maintain this separation in ClaymoreEngine.

---

## Integration Points

### Internal Boundaries

| Boundary | Communication | Notes |
|----------|---------------|-------|
| PluginProcessor ↔ ClaymoreEngine | Direct call: `engine.process(buffer)`, `engine.setXxx()` setters, `engine.getLatencyInSamples()` | ClaymoreEngine is fully owned by PluginProcessor. No shared state. |
| PluginProcessor ↔ PluginEditor | APVTS ValueTree only | Editor reads/writes parameters through APVTS; never touches engine directly. |
| ClaymoreEngine ↔ FuzzCore/FuzzTone | Direct include; per-sample function call | FuzzCore is a pure function namespace. FuzzTone is a member of ClaymoreEngine. No virtual dispatch. |
| Audio thread ↔ Message thread | `std::atomic<float>*` via APVTS | APVTS guarantees thread safety. Additional atomics for oversampling index and latency-change flag. |

### External Services (Host/DAW Integration)

| Integration | Pattern | Notes |
|-------------|---------|-------|
| VST3 format | JUCE CMake target `juce_add_plugin` with `FORMATS VST3` | No extra code needed; JUCE wraps `AudioProcessor` |
| AU format | Same CMake target, `FORMATS AU` | Requires valid Apple Developer ID + code signing for DAW pickup |
| PDC (Plugin Delay Compensation) | `setLatencySamples()` in PluginProcessor | Must be called in prepareToPlay AND on oversampling changes |
| State recall | `getStateInformation()` / `setStateInformation()` via APVTS serialization | APVTS `copyState()` / `replaceState()` handles all parameter persistence |
| Bypass | APVTS bypass parameter or `AudioProcessor::getBypassParameter()` | Use a SmoothedValue for soft bypass crossfade to avoid clicks |

---

## Scaling Considerations

This is an audio plugin, not a web service. "Scaling" means CPU load, latency, and instance count — not users.

| Concern | Low Instance Count | Many Instances |
|---------|-------------------|----------------|
| CPU (oversampling) | 8x at 96kHz is ~3% per instance on M1 | Users can drop to 2x when running many instances — the UI control exists for this |
| Latency reporting | Always correct via `setLatencySamples()` | PDC handles it per-instance automatically |
| Memory | Pre-allocated in prepareToPlay(); ~3 oversamplers × filter state | Predictable; no per-block allocation |
| Thread safety | APVTS + atomics covers all shared state | Scales to any number of instances — no shared globals |

---

## Sources

- GunkLord source: `/Users/nathanmcmillan/projects/GunkLord/source/dsp/FuzzStage.h` — direct inspection (HIGH confidence)
- GunkLord source: `/Users/nathanmcmillan/projects/GunkLord/source/dsp/fuzz/FuzzCore.h`, `FuzzTone.h`, `FuzzType.h` — direct inspection (HIGH confidence)
- GunkLord source: `/Users/nathanmcmillan/projects/GunkLord/source/PluginProcessor.h` — direct inspection (HIGH confidence)
- Crucible source: `/Users/nathanmcmillan/Projects/Crucible/Source/dsp/ClipperSection.h/.cpp` — direct inspection of selectable oversampling pattern (HIGH confidence)
- Crucible source: `/Users/nathanmcmillan/Projects/Crucible/Source/PluginProcessor.h` — direct inspection of APVTS + cached param pointer pattern (HIGH confidence)
- [JUCE dsp::Oversampling Class Reference](https://docs.juce.com/master/classdsp_1_1Oversampling.html) — official documentation on OwnedArray pattern and dummy stages (HIGH confidence)
- [JUCE dsp::DryWetMixer Class Reference](https://docs.juce.com/master/classdsp_1_1DryWetMixer.html) — latency compensation integration (HIGH confidence)
- [JUCE Forum: dsp::Oversampling class performance](https://forum.juce.com/t/dsp-oversampling-class-performance/40000) — runtime switching patterns (MEDIUM confidence)
- [JUCE Forum: How to use JUCE oversampling?](https://forum.juce.com/t/how-to-use-juce-oversampling/36750) — selectable factor implementation (MEDIUM confidence)
- [JUCE DSPModulePluginDemo](https://github.com/juce-framework/JUCE/blob/master/examples/Plugins/DSPModulePluginDemo.h) — official example with selectable oversampling (HIGH confidence)

---
*Architecture research for: Claymore — standalone Rat-based distortion plugin (JUCE 8, VST3 + AU)*
*Researched: 2026-02-23*
