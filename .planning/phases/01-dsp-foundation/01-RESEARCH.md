# Phase 1: DSP Foundation - Research

**Researched:** 2026-02-23
**Domain:** JUCE 8 audio plugin — DSP port, CMake project setup, signal chain architecture
**Confidence:** HIGH

---

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions

#### Distortion Voicing
- Reference circuit: Original Rat LM308 (1978) — faithful to the thick, chunky midrange character
- Not a strict circuit model — "Cairn brand" flavor: more usable and flexible than the original Rat
- Key quality: forgiving on many sources due to mid-heavy voicing (less fizzy than typical distortion)
- Tone and Presence stay within the Rat's natural range with slight extension — the added Presence knob already gives more brightness than the original pedal
- Extended filter ranges vs requirements: Tightness HPF 20–800 Hz (was 20–300 Hz), Tone LPF 2 kHz–20 kHz (was 800–8 kHz)
- Pre/post toggle on tone-shaping stages (Tightness, Tone, Presence) — DSP supports both routing paths; default is post-distortion. Toggle UI deferred to Phase 3

#### Sag Behavior
- Full range: minimum Sag = perfect power supply (clean LM308 behavior), maximum Sag = dying-battery sputter with note choking
- Gain-neutral — Sag changes character without affecting overall output level
- High Drive + high Sag = chaos is acceptable; users cranking both expect extreme behavior

#### Noise Gate
- Full-featured gate DSP: threshold, attack, release, sidechain HPF (adjustable cutoff), range (attenuation amount when closed), hysteresis
- UI approach: only threshold knob visible; attack, release, sidechain HPF, and range exposed via right-click context menu (Phase 3)
- Good defaults so gate sounds right with just the threshold knob — power users access hidden controls
- Internal sidechain only (no external sidechain input)
- Hysteresis: yes — different open/close thresholds to prevent chatter on borderline signals

#### Signal Chain Order
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

### Deferred Ideas (OUT OF SCOPE)
- Multiple Rat circuit variants selectable (LM308, OP07, Turbo Rat, etc.) — like the JHS PackRat. Could be its own phase or v2 feature
- Right-click context menu UI for gate parameters — Phase 3 (GUI)
- Pre/post toggle UI for tone stages — Phase 3 (GUI)
</user_constraints>

---

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|-----------------|
| DIST-01 | User can control distortion amount via Drive knob (1x–40x gain range) | FuzzConfig::mapDrive() and FuzzCore::processSample() implement this directly. Port verbatim. Parameter: NormalisableRange<float>(0.0f, 1.0f) mapped via mapDrive(). |
| DIST-02 | User can blend between hard clip and soft clip via Symmetry knob | FuzzCore::processSample() `symmetry` parameter (0=hard/Rat, 1=soft/envelope-biased). Port verbatim. Knob feel (gradual vs zones) is at Claude's discretion. |
| DIST-03 | User can control low-frequency input to distortion via Tightness (HPF 20–300 Hz; extended to 800 Hz per CONTEXT.md) | FuzzCore tightnessFilter (FirstOrderTPTFilter highpass). Currently maps 0–1 to 20–300 Hz. Must extend to 800 Hz: `tightCutoff = 20.0f + tightness * 780.0f`. |
| DIST-04 | User can add bias-starve sputter character via Sag knob | FuzzCore::processSample() `sag` parameter. Port verbatim. CONTEXT.md adds gain-neutrality requirement — verify output level stays consistent across Sag range. |
| DIST-05 | User can shape post-distortion tone via Tone (LP sweep; extended to 2 kHz–20 kHz per CONTEXT.md) | FuzzTone::applyTone() — currently 800–8000 Hz. Must change: `cutoff = 2000.0f + tone * 18000.0f`. FuzzTone is owned by ClaymoreEngine, not ported verbatim. |
| DIST-06 | User can boost/cut high-frequency presence via Presence shelf (+/-6 dB at 4 kHz) | FuzzTone::updatePresenceCoefficients() — currently +/-6 dB at 4 kHz (gainDB = (presence - 0.5f) * 12.0f maps ±6 dB). Matches requirement exactly. Port verbatim. |
| SIG-01 | User can adjust input level before distortion via Input Gain (-24 to +24 dB) | Implemented in PluginProcessor as a multiplicative SmoothedValue before ClaymoreEngine. Not in GunkLord fuzz files — write fresh. |
| SIG-02 | User can adjust output level after distortion via Output Gain (-48 to +12 dB) | Same pattern as Input Gain but after DryWetMixer. Multiplicative SmoothedValue. Write fresh. |
| SIG-03 | User can blend dry and wet signal via Mix knob (latency-compensated dry path) | Use juce::dsp::DryWetMixer<float>. Phase 1 uses fixed 2x oversampling so latency is constant (~4 samples IIR). setWetLatency() called in prepareToPlay(). |
| SIG-04 | Output is limited by brickwall limiter to prevent digital overs | Use juce::dsp::Limiter<float> as thin OutputLimiter wrapper. Last in signal chain before output. |
| SIG-05 | User can enable noise gate with adjustable threshold (-60 to -10 dB); gate operates pre-distortion | juce::dsp::NoiseGate<float> — already in FuzzStage. Extract and place in ClaymoreEngine BEFORE the oversampling block. Full-featured DSP with hysteresis per CONTEXT.md. JUCE's NoiseGate supports threshold/attack/release/ratio. Hysteresis needs research — see Open Questions. |
</phase_requirements>

---

## Summary

Phase 1 is a DSP port with a well-understood, directly accessible source. The four core GunkLord fuzz files (`FuzzCore.h`, `FuzzTone.h`, `FuzzConfig.h`, `FuzzType.h`) are header-only and have minimal external dependencies — `FuzzCore.h` and `FuzzTone.h` depend only on `<juce_dsp/juce_dsp.h>`, and `FuzzConfig.h` and `FuzzType.h` depend only on `<juce_core/juce_core.h>`. After direct inspection of all four files, these are self-contained and portable with no GunkLord-specific imports beyond standard JUCE module headers. `FuzzStage.h` wraps them and also depends only on JUCE — but `FuzzStage` itself is NOT ported verbatim; it is the model for `ClaymoreEngine`, which replaces it.

Two parameters require range adjustments versus the GunkLord source: Tightness HPF must extend from 20–300 Hz to 20–800 Hz, and Tone LPF must extend from 800–8000 Hz to 2000–20000 Hz. These are single-line constant changes in the processing loop. All other DSP parameters (Drive 1x–40x, Symmetry 0–1, Sag 0–1, Presence ±6 dB at 4 kHz) match the requirements exactly as implemented.

The most structurally significant decision for Phase 1 is that the noise gate in CONTEXT.md requires hysteresis (different open/close thresholds). JUCE's built-in `juce::dsp::NoiseGate<float>` does not expose a hysteresis parameter — it exposes threshold, attack, release, and ratio. Hysteresis requires either wrapping the JUCE gate with a dual-threshold check on the sidechain signal, or using a different gate implementation. This is the only substantive new DSP work in Phase 1 beyond the port.

**Primary recommendation:** Port the four fuzz headers verbatim to `Source/dsp/fuzz/`, write a fresh `ClaymoreEngine` based on `FuzzStage` as the template, add a dual-threshold noise gate wrapper around `juce::dsp::NoiseGate<float>` or implement a simple gate state machine directly, and build the `PluginProcessor` APVTS scaffold using GunkLord's `Parameters.h` pattern with Claymore-specific IDs. The CMake project should use FetchContent for JUCE 8.0.12 (not the vendored copy in GunkLord).

---

## Standard Stack

### Core

| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| JUCE | 8.0.12 | Audio plugin framework — AudioProcessor, APVTS, DSP primitives, plugin formats | Current stable. All DSP primitives used in this phase (FirstOrderTPTFilter, DryWetMixer, Limiter, NoiseGate, Oversampling, SmoothedValue) are in juce_dsp. GunkLord uses JUCE vendored at a slightly older version; Claymore uses FetchContent for 8.0.12. No breaking changes affect ported code. |
| CMake | 3.25+ | Build system | Matches GunkLord `cmake_minimum_required(VERSION 3.25)`. Use FetchContent (not GunkLord's `add_subdirectory(JUCE)` vendored pattern). |
| C++17 | — | Language standard | Matches GunkLord. All ported fuzz files use C++17 features only. No C++20 required. |
| Xcode CLI 15+ | — | macOS compiler (Clang) | Required for Universal Binary and AU builds. Already installed on developer machine. |

### JUCE Modules Required (Phase 1)

| Module | Purpose | Notes |
|--------|---------|-------|
| `juce::juce_audio_utils` | AudioProcessor base class, APVTS | Always required for plugin processors |
| `juce::juce_audio_processors` | Plugin format support (VST3, AU) | Split from audio_utils for clarity |
| `juce::juce_dsp` | FirstOrderTPTFilter, Oversampling, SmoothedValue, DryWetMixer, NoiseGate, Limiter, IIR::Coefficients | All DSP primitives used in this phase |
| `juce::juce_gui_basics` | Component — needed even for generic DAW editor | Base GUI infrastructure |

Do NOT add: `juce_audio_formats`, `juce_animation`, `juce_gui_extra`, `juce_opengl`. Phase 1 has no GUI (generic DAW editor only).

### Supporting Tools

| Tool | Purpose | Notes |
|------|---------|-------|
| JUCE AudioPluginHost | Validate VST3 loads and produces audio without launching a full DAW | Build from `juce-src/extras/AudioPluginHost` once JUCE is fetched |
| `auval` (macOS built-in) | Validate AU format compliance | Run `auval -v aufx Clyr NMcM` after first install; requires valid code signing |

### Alternatives Considered

| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| `juce::dsp::NoiseGate` with dual-threshold wrapper | Custom per-sample gate state machine | Custom is more flexible for hysteresis; JUCE gate is simpler but limited to single threshold. Given hysteresis is a locked requirement, custom gate state machine is actually simpler to implement correctly than wrapping JUCE's gate. |
| `juce::dsp::Limiter<float>` | Custom brickwall clip | JUCE Limiter uses lookahead and knee — appropriate. Custom clip is simpler but harsher. |
| FetchContent for JUCE 8.0.12 | Vendored JUCE (GunkLord pattern) | FetchContent is more portable; vendored is simpler for offline. Use FetchContent for Claymore — matches Bathysphere/Crucible pattern. |

**Installation (CMake FetchContent — no npm/package install):**
```cmake
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.12
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)
```

---

## Architecture Patterns

### Recommended Project Structure (Phase 1 scope)

```
Source/
├── PluginProcessor.h/.cpp      # AudioProcessor subclass; APVTS; processBlock(); state I/O
├── PluginEditor.h/.cpp         # Minimal editor — createEditor() returns generic JUCE editor for Phase 1
├── Parameters.h                # APVTS layout factory + ParamIDs namespace with constexpr string constants
│
└── dsp/
    ├── ClaymoreEngine.h/.cpp   # Signal chain: gate → upsample → FuzzCore → downsample → FuzzTone
    ├── OutputLimiter.h         # Thin wrapper: juce::dsp::Limiter<float>
    │
    └── fuzz/                   # Ported verbatim from GunkLord — do not modify logic
        ├── FuzzCore.h          # processSample() + FuzzCoreState — NO changes needed
        ├── FuzzTone.h          # applyTone() — one range change: Tone LP 2–20 kHz
        ├── FuzzConfig.h        # FuzzConfig namespace: mapDrive(), outputCompensation
        └── FuzzType.h          # FuzzConfig alias — same content, kept for include compatibility
```

**Note on Phase 1 editor:** The requirement is "no GUI beyond DAW generic editor." Implement `createEditor()` returning `new juce::GenericAudioProcessorEditor (*this)`. This gives a functional, ugly parameter list in the DAW — sufficient for Phase 1 validation. No `PluginEditor` class is needed until Phase 3.

### Verified Signal Chain Implementation Order (processBlock)

Based on locked CONTEXT.md decision and GunkLord `FuzzStage` as reference:

```
1. [isInitialized guard] — return silence if prepareToPlay not yet called
2. [Input Gain SmoothedValue] — multiplicative, before everything
3. [DryWetMixer::pushDrySamples()] — capture dry signal with latency buffer
4. [ClaymoreEngine::process()]
   a. [NoiseGate] — pre-distortion (CONTEXT.md locked decision)
   b. [Tightness HPF] — inside FuzzCoreState, called per-sample in loop
   c. [Oversampling::processSamplesUp()] — 2x IIR fixed for Phase 1
   d. [per-sample FuzzCore::processSample()] — drive, symmetry, sag, slew filter
   e. [× FuzzConfig::outputCompensation (0.30f)]
   f. [Oversampling::processSamplesDown()]
   g. [FuzzTone::applyTone()] — Tone LP + Presence shelf + DC block at original rate
5. [DryWetMixer::mixWetSamples()] — blend with latency-compensated dry
6. [Output Gain SmoothedValue] — multiplicative, after mix
7. [OutputLimiter::process()] — brickwall, last in chain
```

**Critical ordering notes:**
- Noise gate is BEFORE oversampling (CONTEXT.md locked). Gate sees clean input signal.
- Tightness HPF is inside `FuzzCore::processSample()` via `FuzzCoreState.tightnessFilter` — it is applied at the oversampled rate, which is correct (HPF before clipping).
- `FuzzTone::applyTone()` runs AFTER `processSamplesDown()` — at original rate. This is how GunkLord does it and how the filter coefficients are designed.
- DC blocker is inside `FuzzTone::applyTone()` already. Do not add a second DC block.
- Dry/Wet mix is AFTER output gain in the locked CONTEXT.md chain.

### Pattern 1: ClaymoreEngine Based on FuzzStage

**What:** `ClaymoreEngine` replaces `FuzzStage` as the main DSP wrapper. For Phase 1 it uses fixed 2x oversampling (one `dsp::Oversampling` object), identical to GunkLord. Phase 2 upgrades this to three pre-allocated oversamplers.

**Key difference from FuzzStage:** `ClaymoreEngine` exposes the noise gate as a full-featured unit with threshold, attack, release, ratio, and hysteresis. `FuzzStage` exposes only threshold and a boolean enable.

```cpp
// Source: GunkLord FuzzStage.h (reference) + Phase 1 Claymore adaptation
class ClaymoreEngine
{
public:
    ClaymoreEngine()
        : oversampling (2, 1,
            juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true)
    {}

    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        sampleRate   = spec.sampleRate;
        numChannels  = static_cast<int> (spec.numChannels);

        oversampling.initProcessing (static_cast<size_t> (spec.maximumBlockSize));
        oversampling.reset();

        const double oversampledRate = sampleRate * 2.0;
        for (int ch = 0; ch < maxChannels; ++ch)
            coreState[ch].prepare (oversampledRate);

        // FuzzTone runs at original rate
        tone.prepare (spec);

        // Noise gate at original rate (pre-distortion)
        noiseGate.prepare (spec);
        noiseGate.setThreshold (gateOpenThreshold);
        noiseGate.setAttack (gateAttackMs);
        noiseGate.setRelease (gateReleaseMs);
        noiseGate.setRatio (gateRatio);

        // Smoothers at oversampled rate
        driveSmoother.reset    (oversampledRate, 0.010);
        symmetrySmoother.reset (oversampledRate, 0.005);
        tightnessSmoother.reset(oversampledRate, 0.005);
        sagSmoother.reset      (oversampledRate, 0.005);
    }

    float getLatencyInSamples() const
    {
        return oversampling.getLatencyInSamples();
    }

    // ... setDrive(), setSymmetry(), setTightness(), setSag(), setTone(), setPresence()
    // ... setGateEnabled(), setGateThreshold()

private:
    static constexpr int maxChannels = 8;
    juce::dsp::Oversampling<float>   oversampling;
    FuzzCoreState                    coreState[maxChannels];
    FuzzTone                         tone;
    juce::dsp::NoiseGate<float>      noiseGate;
    bool                             gateEnabled = false;

    // Gate dual-threshold for hysteresis
    float gateOpenThreshold  = -40.0f;  // dBFS — gate opens at this level
    float gateCloseThreshold = -44.0f;  // dBFS — gate closes 4 dB lower (hysteresis)

    // Parameter targets + smoothers (same as FuzzStage)
    float targetDrive = 0.5f, targetSymmetry = 0.0f,
          targetTightness = 0.0f, targetSag = 0.0f;
    juce::SmoothedValue<float> driveSmoother, symmetrySmoother,
                               tightnessSmoother, sagSmoother;

    double sampleRate = 44100.0;
    int    numChannels = 2;

    // Gate defaults
    static constexpr float gateAttackMs  = 1.0f;
    static constexpr float gateReleaseMs = 50.0f;
    static constexpr float gateRatio     = 10.0f;
};
```

### Pattern 2: Cached Atomic Parameter Pointers (from GunkLord PluginProcessor.h)

**What:** Cache `std::atomic<float>*` pointers in `prepareToPlay()` via `apvts.getRawParameterValue()`. Read in `processBlock()` without string lookups.

```cpp
// Source: GunkLord source/PluginProcessor.h — exact pattern
// In PluginProcessor.h (private):
std::atomic<float>* driveParam         = nullptr;
std::atomic<float>* symmetryParam      = nullptr;
std::atomic<float>* tightnessParam     = nullptr;
std::atomic<float>* sagParam           = nullptr;
std::atomic<float>* toneParam          = nullptr;
std::atomic<float>* presenceParam      = nullptr;
std::atomic<float>* gateOnParam        = nullptr;
std::atomic<float>* gateThresholdParam = nullptr;
std::atomic<float>* inputGainParam     = nullptr;
std::atomic<float>* outputGainParam    = nullptr;
std::atomic<float>* mixParam           = nullptr;

// In prepareToPlay():
driveParam         = apvts.getRawParameterValue (ParamIDs::drive);
symmetryParam      = apvts.getRawParameterValue (ParamIDs::symmetry);
// ... etc

// In processBlock():
engine.setDrive (driveParam->load (std::memory_order_relaxed));
```

### Pattern 3: APVTS Layout with Claymore-Specific Parameter IDs

**What:** Claymore uses descriptive mixing-tool names (not GunkLord's creative names). All parameters in one group for a focused plugin.

```cpp
// Source: GunkLord Parameters.h (structure reference) + Claymore parameter spec
namespace ParamIDs
{
    // Distortion
    inline constexpr const char* drive     = "drive";
    inline constexpr const char* symmetry  = "symmetry";
    inline constexpr const char* tightness = "tightness";
    inline constexpr const char* sag       = "sag";
    inline constexpr const char* tone      = "tone";
    inline constexpr const char* presence  = "presence";

    // Signal Chain
    inline constexpr const char* inputGain       = "inputGain";
    inline constexpr const char* outputGain      = "outputGain";
    inline constexpr const char* mix             = "mix";
    inline constexpr const char* gateEnabled     = "gateEnabled";
    inline constexpr const char* gateThreshold   = "gateThreshold";
}

// In createParameterLayout():
// Drive: NormalisableRange<float>(0.0f, 1.0f, 0.01f), default 0.5f
//   — normalized; ClaymoreEngine calls FuzzConfig::mapDrive() internally
// Symmetry: NormalisableRange<float>(0.0f, 1.0f, 0.01f), default 0.0f
// Tightness: NormalisableRange<float>(0.0f, 1.0f, 0.01f), default 0.0f
//   — maps to 20–800 Hz (extended from GunkLord's 20–300 Hz)
// Sag: NormalisableRange<float>(0.0f, 1.0f, 0.01f), default 0.0f
// Tone: NormalisableRange<float>(0.0f, 1.0f, 0.01f), default 0.5f
//   — maps to 2000–20000 Hz (extended from GunkLord's 800–8000 Hz)
// Presence: NormalisableRange<float>(0.0f, 1.0f, 0.01f), default 0.5f
//   — maps to ±6 dB (unchanged)
// Input Gain: NormalisableRange<float>(-24.0f, 24.0f, 0.1f), default 0.0f, label "dB"
// Output Gain: NormalisableRange<float>(-48.0f, 12.0f, 0.1f), default 0.0f, label "dB"
// Mix: NormalisableRange<float>(0.0f, 1.0f, 0.01f), default 1.0f
// Gate Enabled: AudioParameterBool, default false
// Gate Threshold: NormalisableRange<float>(-60.0f, -10.0f, 0.1f), default -40.0f, label "dB"
```

### Pattern 4: DryWetMixer with Wet Latency (SIG-03)

```cpp
// Source: JUCE dsp::DryWetMixer class reference
// In PluginProcessor.h:
juce::dsp::DryWetMixer<float> dryWetMixer { 64 }; // 64 sample dry buffer capacity

// In prepareToPlay():
juce::dsp::ProcessSpec spec { sampleRate,
    static_cast<uint32>(samplesPerBlock),
    static_cast<uint32>(numChannels) };
dryWetMixer.prepare (spec);
dryWetMixer.setWetLatency (engine.getLatencyInSamples());

// In processBlock():
dryWetMixer.setWetMixProportion (mixParam->load (std::memory_order_relaxed));
juce::dsp::AudioBlock<float> inputBlock (buffer);
dryWetMixer.pushDrySamples (inputBlock);
// ... engine.process(buffer) ...
juce::dsp::AudioBlock<float> wetBlock (buffer);
dryWetMixer.mixWetSamples (wetBlock);
```

### Pattern 5: State Persistence via APVTS

```cpp
// Source: GunkLord PluginProcessor pattern
void ClaymoreProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void ClaymoreProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState != nullptr)
        if (xmlState->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}
```

### Anti-Patterns to Avoid

- **Modifying FuzzCore.h logic:** The per-sample waveshaping is verified-working in GunkLord. Do not alter it. Parameter range changes happen at the call site in ClaymoreEngine (tightness cutoff mapping), not inside FuzzCore.
- **Running FuzzTone at oversampled rate:** `FuzzTone::applyTone()` must run AFTER `processSamplesDown()`. Its filter coefficients (4 kHz presence shelf, DC block at 20 Hz) are calibrated for the original sample rate. Running at 2x shifts all frequencies.
- **Placing noise gate after distortion:** CONTEXT.md explicitly locks the gate pre-distortion. Gate after distortion fights the distortion noise floor and cuts musical sustain.
- **Allocating inside processBlock:** No `new`, no vector resize, no string operations. Pre-allocate all buffers and objects in `prepareToPlay()`.
- **String lookups in processBlock:** Do not call `apvts.getParameter("id")->getValue()`. Use cached `std::atomic<float>*` pointers.
- **Skipping `isInitialized` guard:** Some DAW hosts call `processBlock()` before `prepareToPlay()`. Output silence until initialized.

---

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| LP/HP filter for Tone and Tightness | Custom biquad | `juce::dsp::FirstOrderTPTFilter<float>` | Already used in FuzzCore/FuzzTone with correct TPT topology. Changing to a different filter type changes the character. |
| Presence shelf filter | Custom shelf | `juce::dsp::IIR::Coefficients<float>::makeHighShelf()` via `ProcessorDuplicator` | Already in FuzzTone. Handles multi-channel correctly via ProcessorDuplicator. |
| DC blocking filter | Custom highpass | `juce::dsp::IIR::Coefficients<float>::makeFirstOrderHighPass()` at 20 Hz | Already in FuzzTone's `dcBlocker`. Do not add a second one. |
| Brickwall limiter | Custom soft-clip at output | `juce::dsp::Limiter<float>` | Lookahead + knee; handles transients correctly; one `process()` call. |
| Dry/wet mixing with latency compensation | Manual delay line + mix | `juce::dsp::DryWetMixer<float>` with `setWetLatency()` | Built-in dry path delay line; handles the oversampling latency offset automatically. |
| Per-sample parameter smoothing | Linear interpolation | `juce::SmoothedValue<float>` | Already used in FuzzStage. GunkLord uses 10ms for Drive, 5ms for everything else. Carry this over exactly. |
| Basic noise gate | Custom level detector + VCA | `juce::dsp::NoiseGate<float>` with dual-threshold hysteresis wrapper | JUCE gate handles attack/release/ratio correctly. Only hysteresis needs custom logic on top. |

**Key insight:** The DSP in this phase is almost entirely a port of verified, running code. The only new work is: (1) the APVTS scaffold with Claymore-specific parameter IDs and ranges, (2) the ClaymoreEngine wrapper class, (3) hysteresis logic for the noise gate, (4) the two range extensions (Tightness to 800 Hz, Tone to 20 kHz). Do not rewrite what already works.

---

## Common Pitfalls

### Pitfall 1: FuzzType.h Is an Include-Chain Dependency of FuzzStage.h

**What goes wrong:** `FuzzStage.h` includes `"fuzz/FuzzType.h"`. When porting `FuzzCore.h`, the include `#include "FuzzType.h"` must resolve. The actual `FuzzType.h` file in GunkLord contains the same content as `FuzzConfig.h` (both are `FuzzConfig` namespace files — the original FuzzType.h was the per-fuzz-type enum, now replaced). Confirm what is actually in `FuzzType.h` before assuming it is unused.

**After inspection:** `FuzzType.h` at `/Users/nathanmcmillan/Projects/gunklord/source/dsp/fuzz/FuzzType.h` contains only the `FuzzConfig` namespace (same as `FuzzConfig.h`). Copy it to `Source/dsp/fuzz/FuzzType.h` in Claymore unchanged.

**How to avoid:** Copy all four files (`FuzzCore.h`, `FuzzTone.h`, `FuzzConfig.h`, `FuzzType.h`) to `Source/dsp/fuzz/`. Do a clean build without GunkLord on the include path. The files depend only on `<juce_dsp/juce_dsp.h>` and `<juce_core/juce_core.h>` — both provided by JUCE 8.0.12.

### Pitfall 2: Tightness and Tone Range Extensions Require Two Targeted Changes

**What goes wrong:** The developer copies FuzzCore.h and FuzzTone.h verbatim and assumes they are correct. They are correct for GunkLord's ranges but not for CONTEXT.md's extended ranges.

**Specific changes required:**

In `ClaymoreEngine::process()` (the per-sample loop), change:
```cpp
// GunkLord: maps 0–1 to 20–300 Hz
const float tightCutoff = 20.0f + tightness * 280.0f;

// Claymore (CONTEXT.md extended range): maps 0–1 to 20–800 Hz
const float tightCutoff = 20.0f + tightness * 780.0f;
```

In `FuzzTone::applyTone()` (the per-sample tone loop), change:
```cpp
// GunkLord: maps 0–1 to 800–8000 Hz
const float cutoff = 800.0f + tone * 7200.0f;

// Claymore (CONTEXT.md extended range): maps 0–1 to 2000–20000 Hz
const float cutoff = 2000.0f + tone * 18000.0f;
```

Since `FuzzTone.h` is modified, it is NOT a verbatim port — it is a derivative with this one line changed.

**Warning signs:** Tone knob at maximum sounds muffled (Tone LPF at 8 kHz instead of 20 kHz). Tightness at maximum still passes significant low-end (HPF at 300 Hz instead of 800 Hz).

### Pitfall 3: Noise Gate Hysteresis Is Not Exposed by juce::dsp::NoiseGate

**What goes wrong:** Developer uses `juce::dsp::NoiseGate<float>` with a single `setThreshold()` call and assumes hysteresis is handled internally. It is not. JUCE's NoiseGate has a single threshold; the gate opens AND closes at the same level. On borderline signals this causes chatter.

**How to avoid:** Implement hysteresis as a state machine layer on top of the JUCE gate. Track a `gateIsOpen` bool in `ClaymoreEngine`. On each block, measure the signal level (use a simple RMS or peak follower). If gate is closed and level exceeds `gateOpenThreshold`, open it. If gate is open and level falls below `gateCloseThreshold` (= openThreshold - hysteresis dB), close it. Separately, call `noiseGate.setThreshold()` with the appropriate value based on the current open/close state.

Alternative: Skip `juce::dsp::NoiseGate` entirely for Phase 1 and implement a simple envelope-follower gate state machine. For a first-order gate (the Threshold knob only), this is ~20 lines of code and gives full control over hysteresis, attack, release, and range.

**Default values for hidden gate parameters (Claude's discretion):**
- Attack: 1 ms (fast, avoids click artifacts on gate open)
- Release: 80 ms (natural tail without cutting sustain at moderate Drive)
- Sidechain HPF cutoff: 150 Hz (reduces low-frequency level contribution to gate decision, prevents kick drum bleed from keeping gate open)
- Range (attenuation when closed): -60 dB (effective silence, not hard mute)
- Hysteresis: 4 dB (open threshold minus close threshold)

### Pitfall 4: processBlock Called Before prepareToPlay

**What goes wrong:** FL Studio Patcher and some other hosts call `processBlock` before `prepareToPlay`. DSP objects are uninitialized.

**How to avoid:**
```cpp
// In PluginProcessor.h (private):
std::atomic<bool> isInitialized { false };

// In prepareToPlay() — last line after all initialization:
isInitialized.store (true, std::memory_order_release);

// In processBlock() — first thing:
if (! isInitialized.load (std::memory_order_acquire))
{
    buffer.clear();
    return;
}
```

### Pitfall 5: Sag Gain-Neutrality

**What goes wrong:** CONTEXT.md requires Sag to be gain-neutral (changes character without changing output level). GunkLord's FuzzCore applies Sag as an amplitude suppressor: when `absClipped < sagThreshold`, the sample is reduced exponentially. At high Sag values this significantly reduces output level.

**How to avoid:** After the Sag stage in `FuzzCore::processSample()`, apply a makeup gain. The makeup amount depends on the Sag value. One approach: measure the average attenuation introduced by the Sag circuit at different sag values and build a lookup or formula. Simpler approach: after the clipped output, apply `clipped *= (1.0f + sag * sagMakeupGain)` where `sagMakeupGain` is tuned empirically (likely around 1.5–2.0 at sag=1.0). This is at Claude's discretion per CONTEXT.md.

**Warning signs:** Output level drops noticeably when sweeping Sag from 0 to 1 with Drive at a fixed value.

### Pitfall 6: Parameter State Not Restored on Session Reload

**What goes wrong:** Non-APVTS state (if any) is not included in `getStateInformation`. In Claymore, ALL state (including gateEnabled) must be in the APVTS.

**How to avoid:** Every persistent parameter lives in the APVTS parameter layout. Use `apvts.copyState()` / `apvts.replaceState()`. Test roundtrip in Logic AND Reaper after implementing.

---

## Code Examples

Verified patterns from source inspection:

### FuzzCore::processSample() — Verified Signature (from GunkLord)
```cpp
// Source: /Users/nathanmcmillan/Projects/gunklord/source/dsp/fuzz/FuzzCore.h
namespace FuzzCore
{
    inline float processSample (float x, float drive, float symmetry, float sag,
                                FuzzCoreState& state)
    {
        x = state.tightnessFilter.processSample (0, x);  // HPF before gain
        const float gained = x * drive;
        // ... slew filter, clip blend, sag ...
        return clipped;
    }
}
// Usage in ClaymoreEngine process loop:
const float mappedDrive = FuzzConfig::mapDrive (drive);        // maps 0–1 to 1–40
data[s] = FuzzCore::processSample (data[s], mappedDrive, symmetry, sag, coreState[ch]);
data[s] *= FuzzConfig::outputCompensation;                     // 0.30f fixed attenuation
```

### FuzzCoreState Prepare at Oversampled Rate (from GunkLord FuzzStage)
```cpp
// Source: /Users/nathanmcmillan/Projects/gunklord/source/dsp/FuzzStage.h
// Critical: prepare at oversampledRate, NOT spec.sampleRate
const double oversampledRate = sampleRate * 2.0;  // Phase 1: fixed 2x
for (int ch = 0; ch < maxChannels; ++ch)
    coreState[ch].prepare (oversampledRate);       // FuzzCoreState::prepare() takes sampleRate

// Also: SmoothedValues must reset at oversampledRate
driveSmoother.reset (oversampledRate, 0.010);      // 10ms at oversampled rate
symmetrySmoother.reset (oversampledRate, 0.005);   // 5ms
```

### FuzzTone::applyTone() — Range Change Location
```cpp
// Source: /Users/nathanmcmillan/Projects/gunklord/source/dsp/fuzz/FuzzTone.h
// The ONLY change needed: line 75
// GunkLord original:
const float cutoff = 800.0f + tone * 7200.0f;     // 800–8000 Hz
// Claymore (CONTEXT.md extended):
const float cutoff = 2000.0f + tone * 18000.0f;   // 2000–20000 Hz
// Everything else in FuzzTone.h is unchanged.
```

### GunkLord CMake Target Pattern (reference for Claymore CMake)
```cmake
# Source: /Users/nathanmcmillan/Projects/gunklord/CMakeLists.txt (reference)
# GunkLord uses add_subdirectory(JUCE) — Claymore uses FetchContent instead.
# GunkLord modules: juce_audio_utils + juce_dsp
# Claymore adds juce_audio_processors for format clarity

target_compile_definitions(Claymore
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(Claymore
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
```

### Full juce_add_plugin for Claymore (CMake)
```cmake
# Source: JUCE CMake API docs + GunkLord CMakeLists.txt reference
juce_add_plugin(Claymore
    COMPANY_NAME "Nathan McMillan"
    PLUGIN_MANUFACTURER_CODE NMcM
    PLUGIN_CODE Clyr
    FORMATS VST3 AU
    PRODUCT_NAME "Claymore"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    VST3_CATEGORIES "Fx" "Distortion"
    AU_MAIN_TYPE kAudioUnitType_Effect
    BUNDLE_ID com.nathanmcmillan.claymore
)
# Note: NO Standalone format. NO CLAP. VST3 + AU only.
# PLUGIN_CODE Clyr: unique 4-char code, different from Gunk/Cruc/Bath/NMcM.
```

### Tightness Cutoff Mapping in ClaymoreEngine process() Loop
```cpp
// Source: GunkLord FuzzStage.h line 121 (reference) + extended range
// GunkLord: 20.0f + tightness * 280.0f  (20–300 Hz)
// Claymore:
const float tightCutoff = 20.0f + tightness * 780.0f;  // 20–800 Hz (CONTEXT.md)
coreState[ch].tightnessFilter.setCutoffFrequency (tightCutoff);
```

---

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| `add_subdirectory(JUCE)` vendored | `FetchContent` with GIT_TAG | GunkLord → Bathysphere/Crucible | Reproducible builds; no 200MB+ JUCE in repo |
| Separate FuzzType enum (5 fuzz types) | Unified FuzzCore with Symmetry/Tightness/Sag | GunkLord refactor (current) | Single processSample function; no per-type dispatch |
| Fixed 2x oversampling | Selectable 2x/4x/8x pre-allocated array | Phase 1 → Phase 2 | Phase 1 uses fixed 2x; Phase 2 adds selector |
| GunkLord parameter names (Grime/Skew/Grip/Starve/Murk/Sheen) | Descriptive mixing names (Drive/Symmetry/Tightness/Sag/Tone/Presence) | Claymore design decision | Clearer for mixing engineers; different ParamIDs namespace |

**Deprecated/outdated:**
- `file(GLOB_RECURSE SourceFiles ...)` in CMake: GunkLord uses this; Claymore should explicitly list source files (Bathysphere/Crucible pattern). Safer for IDE integration.
- Projucer-generated CMakeLists: Developer has moved to hand-authored CMake. Do not use Projucer for Claymore.

---

## Open Questions

1. **Noise gate hysteresis — juce::dsp::NoiseGate vs. custom state machine**
   - What we know: `juce::dsp::NoiseGate<float>` exposes threshold, ratio, attack, release. No hysteresis parameter.
   - What's unclear: Whether using the JUCE gate with external dual-threshold switching of `setThreshold()` will produce clean audio (switching threshold mid-stream may cause clicks), or whether a custom simple gate state machine is cleaner for Phase 1.
   - Recommendation: For Phase 1, implement a minimal custom gate: an envelope follower (first-order IIR level detector) with dual-threshold logic and a `juce::SmoothedValue<float>` for attack/release gain ramping. This is ~30 lines and is unambiguous. The `juce::dsp::NoiseGate` is available as a fallback if Phase 3 needs it for pluginval compliance.

2. **Sag gain-neutrality makeup gain formula**
   - What we know: GunkLord's Sag reduces amplitude by up to `ratio^2` when `absClipped < sagThreshold`. At sag=1.0, sagThreshold=0.3, meaning signals below 0.3 are exponentially reduced. At high Drive settings most signal is above 0.3, so Sag primarily affects quiet sections (sputter character).
   - What's unclear: The exact relationship between sag value and average level reduction across different Drive settings. Gain-neutrality that sounds right may need empirical tuning.
   - Recommendation: Start with a simple linear makeup: `const float sagMakeup = 1.0f + sag * 0.5f` and tune by ear. The CONTEXT.md allows "High Drive + high Sag = chaos," so perfect gain-neutrality at extreme settings is not required.

3. **Stereo channel configuration**
   - What we know: CONTEXT.md defers to best practice. GunkLord's FuzzStage supports up to `maxChannels = 8` via `FuzzCoreState coreState[maxChannels]`, with per-channel waveshaping state.
   - What's unclear: Whether Claymore should support mono, stereo, or both. Processing stereo L/R independently through the same FuzzCore per-channel loop is naturally what GunkLord does.
   - Recommendation: Support mono and stereo (standard for a mixing plugin). `isBusesLayoutSupported()` should accept mono-in/mono-out and stereo-in/stereo-out. Use `juce::jmin(numChannels, buffer.getNumChannels())` in the processing loop (already the GunkLord pattern). No explicit stereo width controls (out of scope).

---

## Sources

### Primary (HIGH confidence)

- `/Users/nathanmcmillan/Projects/gunklord/source/dsp/fuzz/FuzzCore.h` — Direct inspection; complete per-sample waveshaping implementation; verified include dependencies
- `/Users/nathanmcmillan/Projects/gunklord/source/dsp/fuzz/FuzzTone.h` — Direct inspection; complete tone/presence/DC-block implementation; verified filter types and ranges
- `/Users/nathanmcmillan/Projects/gunklord/source/dsp/fuzz/FuzzType.h` — Direct inspection; confirmed it contains FuzzConfig namespace only (same as FuzzConfig.h)
- `/Users/nathanmcmillan/Projects/gunklord/source/dsp/FuzzStage.h` — Direct inspection; complete ClaymoreEngine template; verified signal chain ordering, smoother rates, gate setup
- `/Users/nathanmcmillan/Projects/gunklord/source/PluginProcessor.h` — Direct inspection; cached atomic pointer pattern; DryWetMixer capacity (64 samples); bypassSmoother pattern
- `/Users/nathanmcmillan/Projects/gunklord/source/Parameters.h` — Direct inspection; APVTS layout factory pattern; GunkLord fuzz parameter ID names and ranges
- `/Users/nathanmcmillan/Projects/gunklord/CMakeLists.txt` — Direct inspection; confirmed C++17, CMake 3.25, `add_subdirectory(JUCE)`, `juce_audio_utils` + `juce_dsp` modules, `PLUGIN_MANUFACTURER_CODE NMcM`
- JUCE dsp::DryWetMixer class reference: https://docs.juce.com/master/classdsp_1_1DryWetMixer.html
- JUCE dsp::Oversampling class reference: https://docs.juce.com/master/classjuce_1_1dsp_1_1Oversampling.html
- JUCE CMake API docs: https://github.com/juce-framework/JUCE/blob/master/docs/CMake%20API.md

### Secondary (MEDIUM confidence)

- `.planning/research/STACK.md` — JUCE 8.0.12 as current stable; FetchContent pattern; IIR vs FIR oversampling filter comparison; JUCE module selection rationale
- `.planning/research/ARCHITECTURE.md` — ClaymoreEngine architecture design; build order; signal flow diagram; five architectural patterns with code examples
- `.planning/research/PITFALLS.md` — processBlock before prepareToPlay; DC offset; noise gate placement; parameter state restore; hidden include coupling pitfalls with recovery costs

### Tertiary (LOW confidence)

- None — all critical claims are grounded in direct source inspection or official JUCE documentation.

---

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — JUCE 8.0.12 version confirmed; GunkLord CMakeLists inspected; all modules verified against actual source includes
- Architecture: HIGH — based on direct inspection of GunkLord FuzzStage.h and PluginProcessor.h; all patterns copied from running code
- Pitfalls: HIGH — Pitfalls 1/2 discovered by direct source inspection; Pitfalls 3/4/5/6 verified by previous PITFALLS.md research with JUCE forum corroboration
- Phase requirements mapping: HIGH — all 11 requirements directly traceable to specific functions/lines in inspected source files

**Research date:** 2026-02-23
**Valid until:** 2026-04-23 (stable JUCE ecosystem; JUCE 8.x API not expected to change)
