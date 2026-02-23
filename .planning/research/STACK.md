# Stack Research

**Domain:** Focused JUCE distortion plugin (Rat-style, pedal GUI, selectable oversampling)
**Researched:** 2026-02-23
**Confidence:** HIGH

---

## Recommended Stack

### Core Technologies

| Technology | Version | Purpose | Why Recommended |
|------------|---------|---------|-----------------|
| JUCE | 8.0.12 | Audio plugin framework (DSP + GUI + plugin formats) | Current stable release as of December 2024. Existing developer projects (Bathysphere, Crucible) pin 8.0.4 via FetchContent — bump to 8.0.12 for this new project to get all bug fixes. Breaking changes between 8.0.4 and 8.0.12 do not affect distortion plugin code. |
| CMake | 3.25+ | Build system | Matches Gunk Lord's `cmake_minimum_required(VERSION 3.25)` — the highest floor in this developer's projects. Gunk Lord CMakeLists uses 3.25, Bathysphere/Crucible use 3.22. Use 3.25 for consistency with the source project. |
| C++17 | — | Language standard | Matches Gunk Lord (C++17). Bathysphere and Crucible use C++20, but Gunk Lord is C++17 and the ported DSP code makes no use of C++20 features. Use C++17 to keep the port frictionless. If any JUCE 8.0.12 features require C++20, they are not used by this plugin. |
| Xcode (Command Line Tools) | 15+ | macOS compiler (Clang) | Required for AU builds and Universal Binary. No separate installation beyond existing dev setup. |

### JUCE Modules Required

| Module | Purpose | Notes |
|--------|---------|-------|
| `juce::juce_audio_utils` | AudioProcessor base class, AudioProcessorValueTreeState | Always required for plugin processors |
| `juce::juce_audio_processors` | Plugin format support (VST3, AU) | Can be split from audio_utils; include explicitly for clarity |
| `juce::juce_dsp` | `juce::dsp::Oversampling`, `juce::dsp::NoiseGate`, `juce::dsp::FirstOrderTPTFilter`, `juce::dsp::SmoothedValue`, `juce::dsp::DryWetMixer` | All DSP primitives used in FuzzStage come from this module |
| `juce::juce_gui_basics` | Component, Slider, LookAndFeel | Base GUI infrastructure |

No need for `juce_audio_formats`, `juce_animation`, `juce_gui_extra`, or `juce_opengl`. Claymore has no presets (v1), no animation, no web browser, and no OpenGL rendering. Keeping modules minimal reduces compile time.

### Oversampling Implementation

**Use `juce::dsp::Oversampling<float>` with an array of pre-initialized oversamplers.**

The Gunk Lord `FuzzStage` currently hardcodes 2x oversampling at construction:
```cpp
juce::dsp::Oversampling<float> oversampling;
// constructor: oversampling(2, 1, filterHalfBandPolyphaseIIR, true)
```

For Claymore's selectable 2x/4x/8x, use the pattern from JUCE's official `DSPModulePluginDemo.h`:
```cpp
// Three oversamplers pre-initialized at construction
std::array<juce::dsp::Oversampling<float>, 3> oversamplers {
    {
        { 2, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, false }, // 2x
        { 2, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, false }, // 4x
        { 2, 3, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, false }, // 8x
    }
};
int currentOversamplingIndex = 0; // 0=2x, 1=4x, 2=8x
```

The `factor` parameter maps as: `factor=1` = 2x, `factor=2` = 4x, `factor=3` = 8x (formula: `2^factor`).

**Filter type choice: `filterHalfBandPolyphaseIIR`** — same as Gunk Lord. This minimizes latency at the cost of non-linear phase near Nyquist, which is inaudible in distortion use and correct for real-time guitar monitoring. Do not use `filterHalfBandFIREquiripple` for this plugin; it adds significantly more latency with no audible benefit for distortion.

**Switching oversampling rate**: When the user changes the oversampling selection, call `prepareToPlay` again to re-initialize the selected oversampler. This is the established JUCE pattern — do not try to dynamically reinit mid-stream.

**Latency reporting**: Call `setLatencySamples(oversamplers[currentIndex].getLatencyInSamples())` after `initProcessing`. DAWs need this for delay compensation.

### CMake Pattern

Use `FetchContent` pointing to JUCE 8.0.12, matching the pattern in Bathysphere and Crucible but bumping the tag:

```cmake
cmake_minimum_required(VERSION 3.25)
project(Claymore VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# macOS Universal Binary + deployment target
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Build architectures")
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")

include(FetchContent)

if(EXISTS "${CMAKE_BINARY_DIR}/_deps/juce-src/CMakeLists.txt")
    set(FETCHCONTENT_SOURCE_DIR_JUCE "${CMAKE_BINARY_DIR}/_deps/juce-src")
endif()

FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.12
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(Claymore
    COMPANY_NAME "NathanMcMillan"
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
```

**Key CMake decisions:**
- **No `Standalone` in FORMATS** — explicitly excluded per project requirements. Do not add it.
- **No `CLAP`** — future addition, excluded from v1.
- **`PLUGIN_MANUFACTURER_CODE NMcM`** — matches Gunk Lord exactly for consistent developer identity.
- **`PLUGIN_CODE Clyr`** — unique 4-character code, different from Gunk (Gunk), Crucible (Cruc), Bathysphere (Bath). Must be unique per plugin.
- **`CMAKE_OSX_ARCHITECTURES "arm64;x86_64"`** — set at configure time for Universal Binary. Can also be passed via `-DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"` on command line.
- **`CMAKE_OSX_DEPLOYMENT_TARGET "11.0"`** — matches Bathysphere and Crucible. macOS 11 is the minimum for Apple Silicon support.

**Avoid `file(GLOB_RECURSE ...)`** — Gunk Lord uses it but Bathysphere/Crucible explicitly list source files. Explicit listing is safer for CMake's dependency tracking and IDE integration, and is the direction this developer has moved in newer projects.

### GUI Approach

**Custom `LookAndFeel` subclass with procedurally-drawn rotary knobs.** This is the approach used in Bathysphere (`BathysphereLookAndFeel`) and Crucible (`CrucibleLookAndFeel`).

Pattern:
1. Subclass `juce::LookAndFeel_V4`
2. Override `drawRotarySlider()` to draw a pedal-style knob with a pointer/indicator line
3. Override `drawLabel()` if custom label styling is needed
4. Set on the editor with `setLookAndFeel()`, cleared in destructor with `setLookAndFeel(nullptr)`

**Do not use image strips (KnobMan-style film strips)** — procedural drawing scales cleanly to any resolution, is easy to modify, has zero asset pipeline overhead, and is the approach this developer uses in all existing plugins.

**Knob component pattern from existing projects:** Create a thin `ClaymoreKnob` component (like `FilterKnob` in Crucible, `BrassKnob` in Bathysphere) that wraps `juce::Slider` with `setSliderStyle(Slider::RotaryVerticalDrag)` and a custom label below. Attach via `juce::AudioProcessorValueTreeState::SliderAttachment`.

### Supporting Libraries / Tools

| Tool | Purpose | Notes |
|------|---------|-------|
| Xcode AudioPlugin Validator (`auval`) | Validates AU plugin format compliance | Run `auval -a` to list AU plugins; run `auval -v aufx Clyr NMcM` after first install. Mandatory before distributing. |
| JUCE AudioPluginHost | Test VST3 in JUCE's own host without launching a full DAW | Located at `juce-src/extras/AudioPluginHost`. Build it once alongside the plugin. |
| Catch2 v3.7+ | Unit testing DSP code | Crucible already uses Catch2 via FetchContent. For Claymore v1, DSP testing is optional — the algorithm is proven in Gunk Lord. Add if phase plan includes DSP test coverage. |

---

## What NOT to Use

| Avoid | Why | Use Instead |
|-------|-----|-------------|
| Projucer | Generates CMakeLists that fight hand-authored CMake; this developer has fully moved to CMake-first | Hand-authored CMakeLists.txt as in Bathysphere/Crucible |
| `juce_opengl` / OpenGL rendering | Massive overhead for a 6-knob plugin with no real-time visualization needs | Procedural software rendering via JUCE's Graphics API |
| `filterHalfBandFIREquiripple` for oversampling | Adds ~60–120 samples of latency at 44.1kHz vs. ~4 samples for IIR; makes the plugin unusable for real-time guitar monitoring at 2x | `filterHalfBandPolyphaseIIR` — same as Gunk Lord |
| Image-strip knobs (KnobMan) | Asset pipeline complexity, resolution/DPI fragility | Procedural drawing in `drawRotarySlider()` |
| `juce_animation` module | Not needed for a static knob layout with no animated transitions | Omit from `target_link_libraries` |
| `add_subdirectory(JUCE)` (local path pattern) | Only works if JUCE is vendored locally; Bathysphere and Crucible moved to FetchContent for reproducible builds | `FetchContent_Declare(JUCE GIT_TAG 8.0.12)` |
| JUCE 8.0.4 (what Crucible/Bathysphere pin) | Misses 8 months of fixes including VST3 parameter migration support (8.0.5), Image blur API fixes (8.0.7), and AudioPluginFormatManager changes (8.0.9) | JUCE 8.0.12 |
| `FORMATS VST3 AU Standalone` | Standalone builds add build complexity and a desktop application target; project spec excludes it | `FORMATS VST3 AU` only |

---

## Alternatives Considered

| Recommended | Alternative | When to Use Alternative |
|-------------|-------------|-------------------------|
| JUCE 8 | iPlug2 | If targeting web (WAM), Faust DSP integration, or need CLAP from day one without waiting for JUCE 9 |
| JUCE 8 | DPF (DISTRHO Plugin Framework) | Lightweight projects targeting LV2/CLAP/JACK with no GUI complexity |
| `FetchContent` for JUCE | Vendoring JUCE as a git submodule | When offline builds or corporate proxy environments make FetchContent unreliable |
| `filterHalfBandPolyphaseIIR` | `filterHalfBandFIREquiripple` | When linear phase is required (mastering limiter, mid-side processing) and latency is not a concern |
| Procedural LookAndFeel | OpenGL shader rendering | When a plugin needs GPU-accelerated real-time waveform visualization (oscilloscope, spectrum analyzer) |

---

## Version Compatibility

| Component | Version | Compatibility Notes |
|-----------|---------|---------------------|
| JUCE | 8.0.12 | Requires macOS 10.13+ for the framework itself; AU builds require macOS 11.0+ for Universal Binary |
| CMake | 3.25+ | `cmake_minimum_required(VERSION 3.25)` matches Gunk Lord; CMake 3.22 also works but 3.25 adds better FetchContent improvements |
| C++ Standard | 17 | JUCE 8 supports both C++17 and C++20; C++17 chosen for compatibility with Gunk Lord DSP headers |
| Xcode | 15+ | Required for Apple Silicon + Intel Universal Binary on macOS |
| macOS Deployment Target | 11.0 | Required for `arm64` support; matches Bathysphere and Crucible |
| Catch2 (optional) | 3.7.1 | Only if adding unit tests; version matches what Crucible uses |

---

## JUCE 8 Changes Relevant to This Project

Changes between the Gunk Lord codebase era and current JUCE 8.0.12 that directly affect a new plugin build:

| Change | Version | Impact on Claymore |
|--------|---------|-------------------|
| `VBlankListener::onVBlank()` now takes a `double` timestamp | 8.0.4 | Low — only affects VBlank callback implementations. If editor uses `VBlankAttachment`, update signature. |
| `Font::getStringWidth()` deprecated | 8.0.2 | Low — use `Font::getStringWidthFloat()` or `GlyphArrangement` for text measurement in `drawRotarySlider()` |
| `AudioPluginFormatManager::addDefaultFormats()` removed | 8.0.9 | Not applicable — plugin project does not host other plugins |
| `AudioProcessor::TrackProperties::colour` replaced with `colourARGB` | 8.0.9 | Not applicable — Claymore does not use TrackProperties |
| `juce_javascript` moved to separate module | 8.0.4 | Not applicable — no JavaScript use |
| VST3 parameter migration support added | 8.0.5 | Beneficial — allows parameter ID migration if naming changes between plugin versions |

**Net assessment:** The Gunk Lord DSP code (FuzzCore.h, FuzzTone.h, FuzzStage.h) uses only `juce_dsp` primitives (`FirstOrderTPTFilter`, `Oversampling`, `SmoothedValue`, `NoiseGate`) with no breaking API changes between Gunk Lord's JUCE version and 8.0.12. The port is clean.

---

## Sources

- JUCE GitHub releases page — confirmed 8.0.12 as current release (December 2024): https://github.com/juce-framework/JUCE/releases
- `juce::dsp::Oversampling` official API reference — constructor signature, factor semantics, filter types: https://docs.juce.com/master/classjuce_1_1dsp_1_1Oversampling.html
- JUCE DSPModulePluginDemo.h — selectable oversampling array pattern (official JUCE example): https://github.com/juce-framework/JUCE/blob/master/examples/Plugins/DSPModulePluginDemo.h
- JUCE CMake API docs — `juce_add_plugin` parameters, format names, VST3_CATEGORIES, AU_MAIN_TYPE: https://github.com/juce-framework/JUCE/blob/master/docs/CMake%20API.md
- JUCE BREAKING_CHANGES.md — verified all 8.x breaking changes: https://github.com/juce-framework/JUCE/blob/master/BREAKING_CHANGES.md
- JUCE LookAndFeel tutorial — `drawRotarySlider()` override pattern: https://docs.juce.com/master/tutorial_look_and_feel_customisation.html
- JUCE Q3 2025 roadmap — JUCE 9 CLAP support confirmed upcoming (not in 8.x): https://juce.com/blog/juce-roadmap-update-q3-2025/
- Gunk Lord `FuzzStage.h` — current 2x oversampling implementation pattern (source project): `/Users/nathanmcmillan/Projects/gunklord/source/dsp/FuzzStage.h`
- Bathysphere `CMakeLists.txt` — FetchContent pattern for JUCE: `/Users/nathanmcmillan/Projects/Bathysphere/CMakeLists.txt`
- Crucible `CMakeLists.txt` — FetchContent + Catch2 + explicit source listing pattern: `/Users/nathanmcmillan/Projects/Crucible/CMakeLists.txt`
- Melatonin JUCE CMake guide — CMake best practices, FetchContent vs submodule tradeoffs: https://melatonin.dev/blog/how-to-use-cmake-with-juce/

---

*Stack research for: Claymore — JUCE 8 Rat distortion plugin*
*Researched: 2026-02-23*
