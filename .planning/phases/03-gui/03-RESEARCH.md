# Phase 3: GUI - Research

**Researched:** 2026-02-23
**Domain:** JUCE 8 custom plugin GUI — LookAndFeel, APVTS attachments, pedal-style layout, Cairn design system
**Confidence:** HIGH

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions

**Visual aesthetic**
- Dark theme with subtle skeuomorphism — Neural DSP dark themes as reference, touch of physicality
- Warm Cairn base palette (Basalt/Hearth range) with a cool tint to match the steel/blade identity
- Powder-coated grain texture on the pedal body surface — subtle, felt more than seen
- Cold blue steel accent color for active states, LED indicators, and highlights
- White matte knobs with silver metallic indicator line — inverted EHX Memory Man style
- LED bulb-style indicator light with glow bloom for bypass/active state (not flat button)
- Product name "CLAYMORE" in Cormorant Garamond with subtle metallic/silver steel-etched treatment in header

**Knob layout**
- Drive-centric hero layout: large Drive knob centered, satellite knobs arranged around it
- Pre-distortion controls (Tightness, Symmetry, Input Gain, Sag) flanking left/below Drive
- Post-distortion controls (Tone, Presence, Noise Gate) flanking right/below Drive
- Output Gain and Mix in the Utility Zone per Cairn design system
- Parameter labels: name only below knob, value displayed on hover/tooltip (not persistent readout)

**Oversampling selector**
- Three options: 2x, 4x, 8x
- Label shows rate values only — no CPU hints or quality descriptors
- Placement and control type (segmented toggle, flip switch, etc.) at Claude's discretion

**Window size**
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

### Deferred Ideas (OUT OF SCOPE)
- Multiple Rat algorithm models with color-coded LED indicators (blue for base, other colors per model) — future phase
- Resizable GUI — explicitly out of scope for v1
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|-----------------|
| GUI-01 | Pedal-style minimal interface with knobs for all distortion and signal chain parameters (Drive, Symmetry, Tightness, Sag, Tone, Presence, Input Gain, Output Gain, Mix) and Noise Gate threshold; all controls respond to mouse interaction | JUCE Slider with RotaryHorizontalVerticalDrag + LookAndFeel_V4::drawRotarySlider for custom knobs; SliderAttachment for APVTS binding |
| GUI-02 | Oversampling rate selector (2x, 4x, 8x) accessible and functional from GUI; current rate clearly indicated | AudioParameterChoice already in APVTS; ComboBoxAttachment or custom radio button group with ParameterAttachment; items populated before attachment created |
| GUI-03 | All parameters accept DAW automation write and read correctly; knobs visually track automation playback | SliderAttachment handles bi-directional sync via APVTS listener mechanism; gesture lifecycle (beginGesture/setValueAsPartOfGesture/endGesture) must be respected for automation recording |
</phase_requirements>

---

## Summary

Phase 3 replaces the existing `GenericAudioProcessorEditor` stub with a full pedal-style GUI. The technology stack is entirely within JUCE 8 (already the project's framework at version 8.0.12 per CMakeLists.txt) — no new libraries are needed. The work divides into three areas: (1) custom `LookAndFeel_V4` subclass that overrides `drawRotarySlider` for knob rendering and any toggle/LED drawing, (2) `ClaymoreEditor` rebuild with Cairn four-zone layout, Drive-centric knob arrangement, and procedural grain texture, and (3) APVTS attachment wiring for all 12 parameters.

The Cairn design system is thoroughly documented at `~/projects/brand/` and defines non-negotiable standards: Cormorant Garamond for the product wordmark, DM Sans for knob labels, Basalt/Hearth backgrounds, and the four-zone Header/Primary/Utility/Footer structure. The Crucible mockup (`~/projects/brand/Mockups/Crucible Mockup 1.png`) is the strongest visual reference for the Cairn aesthetic: dark warm background, copper/amber accent knobs with arc readout, segment-style mode buttons, subtle zone dividers at 1px, and the Cairn maker's mark at ~25% opacity in the footer. Claymore adapts this with a cold blue accent instead of Crucible's warm amber.

Automation tracking (GUI-03) is handled automatically by JUCE's `SliderAttachment` mechanism — the attachment registers as an APVTS listener and updates the slider from the message thread when the audio thread writes parameter values. No custom automation polling is required. The only edge case is the `beginGesture/endGesture` lifecycle for mouse interactions: `SliderAttachment` handles this internally; custom controls using `ParameterAttachment` directly must call these explicitly.

**Primary recommendation:** Build the custom `LookAndFeel` and editor structure in one focused pass. The APVTS is already complete; the GUI phase is purely additive — no processor changes needed.

---

## Standard Stack

### Core

| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| JUCE `juce_gui_basics` | 8.0.12 (already linked via `juce_audio_utils`) | Slider, Button, ComboBox, Component, LookAndFeel | Already in project; transitive dependency of juce_audio_utils |
| JUCE `juce_graphics` | 8.0.12 (same) | Graphics, Path, ColourGradient, Image, Font | Core rendering; already linked |
| JUCE `LookAndFeel_V4` | 8.0.12 | Base class for custom knob/control rendering | Standard base for custom plugin GUIs |
| JUCE `AudioProcessorValueTreeState` attachments | 8.0.12 | `SliderAttachment`, `ButtonAttachment`, `ComboBoxAttachment` | Automatic bi-directional parameter sync including automation |

### Supporting

| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| `juce_add_binary_data` CMake function | JUCE 8 | Embed fonts (TTF/OTF) and SVG assets as compile-time binary data | Needed for Cormorant Garamond and DM Sans; also for Cairn makers-mark SVG |
| JUCE `Image` (pixel-level) | 8.0.12 | Procedural grain texture generation at editor startup | One-time Image generation in constructor, cached for paint calls |
| JUCE `ColourGradient` | 8.0.12 | Radial LED glow effect and knob arc fills | LED indicator bloom; subtle radial shadow on knob body |

### Alternatives Considered

| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Custom `drawRotarySlider` in LookAndFeel | Sprite-sheet / filmstrip knob image | Film-strip requires Photoshop pipeline; custom drawing is code-only, matches aesthetic exactly, scales perfectly on HiDPI |
| `ComboBoxAttachment` for oversampling | Custom radio `TextButton` group with `ParameterAttachment` | ComboBox is simpler but less visually aligned with the pedal segmented-button aesthetic; radio buttons require manual gesture lifecycle but allow fully custom look |
| Generating grain texture procedurally | PNG grain tile via BinaryData | Both work; procedural avoids extra asset; tiled PNG from BinaryData also valid (see Code Examples) |

**No new CMake dependencies needed.** `juce_gui_basics` and `juce_graphics` are already transitively linked through `juce_audio_utils`. Only additions are: (1) `juce_add_binary_data` target for fonts, and (2) linking that target to Claymore.

---

## Architecture Patterns

### Recommended Project Structure

```
Source/
├── dsp/                    # Unchanged from Phases 1-2
│   ├── ClaymoreEngine.h
│   └── fuzz/
├── gui/                    # NEW: all Phase 3 additions
│   ├── ClaymoreEditor.h    # Replaces root PluginEditor.h (or stays at root)
│   ├── ClaymoreEditor.cpp
│   ├── ClaymoreKnob.h      # Custom Slider subclass (RotaryHVD + LookAndFeel integration)
│   └── ClaymoreKnob.cpp
├── look/                   # NEW
│   ├── ClaymoreTheme.h     # LookAndFeel_V4 subclass — drawRotarySlider, colors
│   └── ClaymoreTheme.cpp
├── Parameters.h            # Unchanged
├── PluginProcessor.h       # Unchanged
└── PluginProcessor.cpp     # Unchanged
```

Alternatively, keep the flat structure (no `gui/` subfolder) to match current project conventions — the project currently has only `dsp/` as a subfolder. Either is fine; flat is simpler for a small GUI.

### Pattern 1: Custom LookAndFeel for Knob Rendering

**What:** Subclass `LookAndFeel_V4` and override `drawRotarySlider` to paint custom knobs. Apply globally to the editor component so all child sliders inherit.

**When to use:** Single consistent knob style across all controls; size differentiation handled by component bounds, not separate LookAndFeel classes.

**Example:**
```cpp
// Source: https://juce.com/tutorials/tutorial_look_and_feel_customisation
class ClaymoreTheme : public juce::LookAndFeel_V4
{
public:
    ClaymoreTheme()
    {
        // Cairn color tokens
        setColour (juce::Slider::rotarySliderFillColourId,  juce::Colour (0xff4a7cb5)); // cold blue accent
        setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour (0xff5c5147)); // Cairn mid
        setColour (juce::Slider::thumbColourId,             juce::Colours::white);
    }

    void drawRotarySlider (juce::Graphics& g,
                           int x, int y, int width, int height,
                           float sliderPos,
                           const float rotaryStartAngle,
                           const float rotaryEndAngle,
                           juce::Slider& slider) override
    {
        auto radius   = (float) juce::jmin (width / 2, height / 2) - 4.0f;
        auto centreX  = (float) x + (float) width  * 0.5f;
        auto centreY  = (float) y + (float) height * 0.5f;
        auto angle    = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

        // Knob body — white matte circle
        g.setColour (juce::Colour (0xffe8e4df)); // off-white, slightly warm
        g.fillEllipse (centreX - radius, centreY - radius, radius * 2.0f, radius * 2.0f);

        // Subtle rim shadow
        g.setColour (juce::Colour (0x30000000));
        g.drawEllipse (centreX - radius, centreY - radius, radius * 2.0f, radius * 2.0f, 1.5f);

        // Silver metallic indicator line (inverted EHX Memory Man style)
        juce::Path p;
        auto pointerLength = radius * 0.6f;
        p.addRectangle (-1.0f, -radius, 2.0f, pointerLength);
        p.applyTransform (juce::AffineTransform::rotation (angle).translated (centreX, centreY));
        g.setColour (juce::Colour (0xffc8c8c8)); // silver
        g.fillPath (p);

        // Arc track (inactive)
        juce::Path arcTrack;
        arcTrack.addArc (centreX - radius - 4.0f, centreY - radius - 4.0f,
                         (radius + 4.0f) * 2.0f, (radius + 4.0f) * 2.0f,
                         rotaryStartAngle, rotaryEndAngle, true);
        g.setColour (juce::Colour (0x405c5147));
        g.strokePath (arcTrack, juce::PathStrokeType (2.0f));

        // Arc track (active — filled to current position with cold blue)
        juce::Path arcFill;
        arcFill.addArc (centreX - radius - 4.0f, centreY - radius - 4.0f,
                        (radius + 4.0f) * 2.0f, (radius + 4.0f) * 2.0f,
                        rotaryStartAngle, angle, true);
        g.setColour (juce::Colour (0xff4a7cb5)); // cold blue
        g.strokePath (arcFill, juce::PathStrokeType (2.0f));
    }
};
```

### Pattern 2: APVTS Slider Attachment (Automation-Ready Knob)

**What:** Each `Slider` bound to its APVTS parameter via `SliderAttachment`. The attachment handles mouse gesture lifecycle, value sync, and automation read/write.

**When to use:** All continuous parameters (Drive, Symmetry, Tightness, Sag, Tone, Presence, Input Gain, Output Gain, Mix, Gate Threshold — 10 sliders total).

**Example:**
```cpp
// Source: https://docs.juce.com/master/classjuce_1_1AudioProcessorValueTreeState_1_1SliderAttachment.html
// In ClaymoreEditor.h:
class ClaymoreEditor : public juce::AudioProcessorEditor
{
    // Sliders
    juce::Slider driveKnob, symmetryKnob, tightnessKnob, sagKnob;
    juce::Slider toneKnob, presenceKnob;
    juce::Slider inputGainKnob, outputGainKnob, mixKnob;
    juce::Slider gateThresholdKnob;

    // Attachments — declared AFTER sliders (destruction order matters)
    juce::AudioProcessorValueTreeState::SliderAttachment driveAttach, symmetryAttach;
    // ... etc.
};

// In constructor:
ClaymoreEditor::ClaymoreEditor (ClaymoreProcessor& p)
    : AudioProcessorEditor (p),
      driveAttach  (p.apvts, ParamIDs::drive,    driveKnob),
      symmetryAttach (p.apvts, ParamIDs::symmetry, symmetryKnob)
      // ...
{
    driveKnob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    driveKnob.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    driveKnob.setDoubleClickReturnValue (true, 0.5);  // double-click resets to default
    driveKnob.setPopupDisplayEnabled (false, true, this); // show value on hover, parent = this
    addAndMakeVisible (driveKnob);

    setSize (700, 500);
}
```

**Critical ordering rule:** Attachments MUST be declared after their Slider members in the class definition. C++ destructs in reverse declaration order — the attachment must be destroyed before the slider. A dangling attachment callback on a dead slider causes a crash.

### Pattern 3: AudioParameterChoice → Segmented Button Selector (Oversampling)

**What:** Three `TextButton` instances (2x / 4x / 8x) in a mutually exclusive radio group, linked to the `oversampling` `AudioParameterChoice` via `ParameterAttachment`.

**Why not ComboBoxAttachment:** ComboBox requires extra LookAndFeel work to look like a pedal control; three radio-style buttons are more visually idiomatic for a 3-option discrete selector and match the "segmented toggle" aesthetic referenced in CONTEXT.md.

**Pattern:**
```cpp
// Requires manual ParameterAttachment — no built-in attachment for radio groups
// Source: https://docs.juce.com/master/classjuce_1_1ParameterAttachment.html

class OversamplingSelector : public juce::Component, public juce::Button::Listener
{
public:
    OversamplingSelector (juce::AudioProcessorValueTreeState& apvts)
        : attachment (*apvts.getParameter (ParamIDs::oversampling),
                      [this](float v) { updateButtons (v); },
                      nullptr)
    {
        juce::StringArray labels { "2x", "4x", "8x" };
        for (int i = 0; i < 3; ++i)
        {
            buttons[i] = std::make_unique<juce::TextButton> (labels[i]);
            buttons[i]->setRadioGroupId (1);
            buttons[i]->setClickingTogglesState (true);
            buttons[i]->addListener (this);
            addAndMakeVisible (*buttons[i]);
        }
        attachment.sendInitialUpdate(); // sync initial state
    }

    void buttonClicked (juce::Button* b) override
    {
        for (int i = 0; i < 3; ++i)
        {
            if (buttons[i].get() == b && b->getToggleState())
            {
                attachment.beginGesture();
                attachment.setValueAsPartOfGesture ((float) i / 2.0f); // normalized: 0, 0.5, 1.0
                attachment.endGesture();
            }
        }
    }

private:
    void updateButtons (float normValue)
    {
        int idx = juce::roundToInt (normValue * 2.0f);
        for (int i = 0; i < 3; ++i)
            buttons[i]->setToggleState (i == idx, juce::dontSendNotification);
    }

    std::unique_ptr<juce::TextButton> buttons[3];
    juce::ParameterAttachment attachment;
};
```

**Note on normalized value mapping:** `AudioParameterChoice` with items ["2x", "4x", "8x"] normalizes to 0.0/0.5/1.0 for indices 0/1/2. Verify exact normalization by checking `AudioParameterChoice::getNormalisableRange()` — it uses a 0-to-(numChoices-1) integer range normalized to 0.0-1.0.

### Pattern 4: Font Embedding via BinaryData

**What:** Embed Cormorant Garamond and DM Sans as compile-time binary data; load via `FontOptions` + `Typeface::createSystemTypefaceFor`.

**CMakeLists.txt addition:**
```cmake
# After juce_add_plugin(...)
juce_add_binary_data(ClaymoreAssets
    NAMESPACE ClaymoreAssets
    SOURCES
        Assets/Fonts/CormorantGaramond-Light.ttf
        Assets/Fonts/DMSans-Regular.ttf
        Assets/Fonts/DMSans-Light.ttf
        Assets/Logos/cairn-makers-mark.svg
)

target_link_libraries(Claymore PRIVATE ClaymoreAssets)
# Note: add ClaymoreAssets to target_sources as well if header path issues arise
```

**C++ usage (JUCE 8 FontOptions API):**
```cpp
// Source: https://forum.juce.com/t/embedding-fonts-in-juce-8/66430
// Create once and cache (e.g., in ClaymoreTheme constructor)
auto cormorantTypeface = juce::Typeface::createSystemTypefaceFor (
    ClaymoreAssets::CormorantGaramond_Light_ttf,
    ClaymoreAssets::CormorantGaramond_Light_ttfSize);

juce::Font titleFont { juce::FontOptions{}
    .withTypeface (cormorantTypeface)
    .withHeight (24.0f)
    .withStyle ("Light") };

// JUCE 8 deprecates bare Font constructors — use FontOptions everywhere
```

### Pattern 5: Grain Texture Generation

**What:** One-time `juce::Image` generation of a noise grain, cached and drawn as a tiled background in `paint()`.

**When to use:** Powder-coated pedal body surface texture — visible on close inspection, invisible at a glance.

```cpp
// Generate once in constructor or static method
static juce::Image generateGrainTexture (int w, int h, float opacity)
{
    juce::Image img (juce::Image::ARGB, w, h, true);
    juce::Random rng;
    juce::Image::BitmapData bd (img, juce::Image::BitmapData::writeOnly);
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x)
        {
            uint8_t v = (uint8_t) rng.nextInt (255);
            bd.setPixelColour (x, y, juce::Colour (v, v, v).withAlpha (opacity * 0.08f));
        }
    return img;
}

// In paint():
// g.drawImage (grainTexture, getLocalBounds().toFloat()); // or tiled
```

Alternative: Tile a small pre-generated noise Image (64×64 or 128×128) to fill the full bounds. `Graphics::setTiledImageFill` followed by `fillRect` achieves this efficiently.

### Pattern 6: Cairn Four-Zone Layout in resized()

**What:** Hard-coded zone heights following Cairn design system spec. No dynamic layout calculation — fixed pixel zones.

```cpp
void ClaymoreEditor::resized()
{
    auto bounds = getLocalBounds();

    // Zone heights per Cairn design system
    const int headerH  = 36;   // ~30-40px spec
    const int footerH  = 22;   // ~20-25px spec
    const int utilityH = 60;   // ~50-70px spec
    const int primaryH = bounds.getHeight() - headerH - footerH - utilityH;

    headerZone  = bounds.removeFromTop    (headerH);
    footerZone  = bounds.removeFromBottom (footerH);
    utilityZone = bounds.removeFromBottom (utilityH);
    primaryZone = bounds; // remainder

    // Position children into zones...
}
```

### Pattern 7: Fixed Window Size + HiDPI (macOS)

**What:** On macOS, JUCE handles Retina automatically via CoreGraphics backing store — `setSize(700, 500)` produces 1400×1000 actual pixels on a Retina display. No explicit scale factor handling needed for macOS-only target.

```cpp
ClaymoreEditor::ClaymoreEditor (ClaymoreProcessor& p)
    : AudioProcessorEditor (p), processor (p)
{
    setResizable (false, false);  // fixed size, not user-resizable
    setSize (700, 500);           // logical pixels; Retina doubles automatically on macOS
}
```

**Confidence:** HIGH (macOS HiDPI "just works" in JUCE per official forum — confirmed by multiple sources. This is macOS-only per project scope, Windows HiDPI issues are irrelevant.)

### Anti-Patterns to Avoid

- **Declaring attachments before sliders in the class:** Reverse destruction order causes use-after-free. Attachments MUST come after their Slider members.
- **Populating ComboBox after creating ComboBoxAttachment:** Attachment sends initial update before items exist; populate items first, then create attachment.
- **Calling `setSize()` inside `resized()`:** Infinite recursion. Set size in constructor only.
- **Creating the grain texture inside `paint()`:** Called every repaint — generate once in constructor or `prepareToShow()`.
- **Using deprecated `Font` constructors in JUCE 8:** Use `FontOptions` — bare `Font(name, size, style)` is deprecated as of JUCE 8.
- **Forgetting `addAndMakeVisible()` for child components:** Invisible components still receive mouse events; forgetting visibility is a silent bug.
- **Global LookAndFeel vs per-component LookAndFeel:** Setting `setLookAndFeel()` on the editor makes it available to all children. Calling `setLookAndFeel (nullptr)` in the editor's destructor is required to prevent dangling pointer if LookAndFeel outlives the component.

---

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Bi-directional parameter ↔ knob sync | Custom ValueTree listener + slider callback | `SliderAttachment` | Handles thread-safe value propagation, gesture lifecycle, automation recording, and undo correctly |
| Automation gesture lifecycle | Manual `beginChangeGesture`/`endChangeGesture` calls | `SliderAttachment` (automatically managed) | Attachment wraps `ParameterAttachment` which calls begin/end correctly on mouseDown/mouseUp |
| Parameter normalization for display | Custom dB↔linear conversion in UI | `Slider`'s built-in `textFromValueFunction` / `valueFromTextFunction` | These lambdas plug into the slider's existing text system; no custom text box needed |
| Font loading and caching | Custom font registry | `Typeface::createSystemTypefaceFor` + hold the `Typeface::Ptr` | JUCE typeface cache requires holding a reference; the raw create call handles caching internally |
| Mouse double-click default reset | Custom `mouseDoubleClick` override | `slider.setDoubleClickReturnValue(true, defaultVal)` | Built-in; also handles Option+click reset on macOS |
| Tooltip on hover | Custom Component overlay | `slider.setPopupDisplayEnabled(false, true, parentComponent)` | Built-in popup display; positions itself correctly relative to parent |

**Key insight:** `SliderAttachment` is the gravity center of this phase. Any custom control that replaces or bypasses it must re-implement the entire gesture/automation/thread-safety contract manually, which is significant scope. Only build custom attachment for the oversampling radio buttons (which have no built-in attachment type).

---

## Common Pitfalls

### Pitfall 1: Attachment Destruction Order Crash

**What goes wrong:** Plugin crashes on editor close, often only in specific hosts (Logic, Live) or at session quit.
**Why it happens:** C++ destructs class members in reverse declaration order. If `SliderAttachment` is declared before its `Slider`, the attachment outlives the slider and attempts to deregister from a destroyed object.
**How to avoid:** In the header, always declare all `Slider`/`Button`/`ComboBox` members before their corresponding `Attachment` members.
**Warning signs:** Crash in destructor stack trace showing `AudioProcessorValueTreeState` or `ValueTree` internals; crash only on editor close, not on parameter interaction.

### Pitfall 2: ComboBox / Radio Button Initial State Mismatch

**What goes wrong:** Oversampling selector shows wrong initial state (e.g., always shows "2x" even if session was saved with "8x").
**Why it happens:** `ComboBoxAttachment` (or manual `sendInitialUpdate()`) sends value before items are populated.
**How to avoid:** Populate items (or create buttons) before creating the attachment. For manual `ParameterAttachment`, call `sendInitialUpdate()` after all UI setup is complete.
**Warning signs:** Control looks wrong on plugin open but corrects itself on first interaction.

### Pitfall 3: Grain Texture in paint() Causes Jank

**What goes wrong:** Plugin repaints slowly; CPU spikes when window is dragged or a parameter changes.
**Why it happens:** `paint()` is called every repaint cycle. Generating a full-size `Image` via pixel iteration inside `paint()` is extremely slow.
**How to avoid:** Generate the grain `Image` once in the constructor (or the first `paint()` call, cached with a flag). Store as a member. In `paint()`, call `g.drawImageAt(grainTexture, 0, 0)`.
**Warning signs:** CPU usage above 5% in idle; plugin window feels sluggish.

### Pitfall 4: LookAndFeel Dangling Pointer

**What goes wrong:** Crash on session close or plugin destruction after the editor is destroyed.
**Why it happens:** `setLookAndFeel(&myTheme)` on the editor stores a raw pointer. If `myTheme` is destroyed before the editor calls `setLookAndFeel(nullptr)`, a dangling pointer remains.
**How to avoid:** In `ClaymoreEditor` destructor (or `~ClaymoreEditor`), call `setLookAndFeel(nullptr)`. Alternatively, store `ClaymoreTheme` as a member of the editor so it's destroyed at the same time.
**Warning signs:** Crash only happens during host session cleanup; hard to reproduce in standalone testing.

### Pitfall 5: Automation Write vs. Read Threading

**What goes wrong:** DAW automation playback moves the knob, but the audio thread also reads the parameter — parameter write from UI thread conflicts.
**Why it happens:** Not actually a problem if `SliderAttachment` is used correctly — it handles thread safety. Pitfall occurs only if bypassing `SliderAttachment` and calling `processor.apvts.getParameterAsValue()` directly from a timer or custom listener.
**How to avoid:** Use `SliderAttachment` for all continuous parameters. Never call `setValueNotifyingHost` from the audio thread (it's UI-only).
**Warning signs:** Zipper noise or automation jumps when playing back recorded automation.

### Pitfall 6: Missing CMakeLists.txt Update for New Source Files

**What goes wrong:** Build succeeds but new `.cpp` files in `gui/` or `look/` are not compiled — linker error or silent dead code.
**Why it happens:** The project uses an explicit `target_sources` list (comment in CMakeLists.txt: "Explicit source list — do NOT use GLOB_RECURSE"). New files must be added manually.
**How to avoid:** Every new `.cpp` file added in Phase 3 must also be added to `target_sources(Claymore PRIVATE ...)` in CMakeLists.txt.
**Warning signs:** Undefined symbol linker error; or the new class exists but does nothing.

---

## Code Examples

Verified patterns from official sources and current project context:

### Complete Attachment Declaration Pattern (Header)

```cpp
// Source: JUCE docs + JUCE forum pattern — attachment after slider is required
class ClaymoreEditor final : public juce::AudioProcessorEditor
{
public:
    explicit ClaymoreEditor (ClaymoreProcessor&);
    ~ClaymoreEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    ClaymoreProcessor& processor;
    ClaymoreTheme theme; // LookAndFeel — member of editor, same lifetime

    // --- All Sliders first ---
    juce::Slider driveKnob, symmetryKnob, tightnessKnob, sagKnob;
    juce::Slider toneKnob, presenceKnob;
    juce::Slider inputGainKnob, outputGainKnob, mixKnob;
    juce::Slider gateThresholdKnob;
    juce::ToggleButton gateEnabledButton;

    // --- Attachments after sliders (destruction order: attachments die first) ---
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    SliderAttachment driveAttach, symmetryAttach, tightnessAttach, sagAttach;
    SliderAttachment toneAttach, presenceAttach;
    SliderAttachment inputGainAttach, outputGainAttach, mixAttach, gateThresholdAttach;
    ButtonAttachment gateEnabledAttach;

    // Oversampling (custom radio group — no built-in attachment type)
    std::unique_ptr<OversamplingSelector> oversamplingSelector;

    // Grain texture (generated once)
    juce::Image grainTexture;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ClaymoreEditor)
};
```

### Constructor with setLookAndFeel

```cpp
ClaymoreEditor::ClaymoreEditor (ClaymoreProcessor& p)
    : AudioProcessorEditor (p),
      processor (p),
      // Initialize all attachments in member initializer list
      driveAttach      (p.apvts, ParamIDs::drive,         driveKnob),
      symmetryAttach   (p.apvts, ParamIDs::symmetry,      symmetryKnob),
      tightnessAttach  (p.apvts, ParamIDs::tightness,     tightnessKnob),
      sagAttach        (p.apvts, ParamIDs::sag,           sagKnob),
      toneAttach       (p.apvts, ParamIDs::tone,          toneKnob),
      presenceAttach   (p.apvts, ParamIDs::presence,      presenceKnob),
      inputGainAttach  (p.apvts, ParamIDs::inputGain,     inputGainKnob),
      outputGainAttach (p.apvts, ParamIDs::outputGain,    outputGainKnob),
      mixAttach        (p.apvts, ParamIDs::mix,           mixKnob),
      gateThresholdAttach (p.apvts, ParamIDs::gateThreshold, gateThresholdKnob),
      gateEnabledAttach   (p.apvts, ParamIDs::gateEnabled,   gateEnabledButton)
{
    setLookAndFeel (&theme);  // apply to all children

    for (auto* knob : { &driveKnob, &symmetryKnob, &tightnessKnob, &sagKnob,
                        &toneKnob, &presenceKnob,
                        &inputGainKnob, &outputGainKnob, &mixKnob, &gateThresholdKnob })
    {
        knob->setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        knob->setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        knob->setPopupDisplayEnabled (false, true, this);  // value tooltip on hover
        addAndMakeVisible (knob);
    }

    gateEnabledButton.setButtonText ("GATE");
    addAndMakeVisible (gateEnabledButton);

    oversamplingSelector = std::make_unique<OversamplingSelector> (p.apvts);
    addAndMakeVisible (*oversamplingSelector);

    grainTexture = generateGrainTexture (128, 128, 1.0f); // tile-sized grain

    setResizable (false, false);
    setSize (700, 500);
}

ClaymoreEditor::~ClaymoreEditor()
{
    setLookAndFeel (nullptr); // required: clears dangling pointer before theme is destroyed
}
```

### Paint Method with Grain Texture and Zone Backgrounds

```cpp
void ClaymoreEditor::paint (juce::Graphics& g)
{
    // Background — Basalt base (#1a1716) with slight cool tint for steel identity
    g.fillAll (juce::Colour (0xff1c1a1f));  // Basalt + subtle blue shift

    // Grain texture (tiled over pedal body / primary zone)
    if (grainTexture.isValid())
    {
        g.setTiledImageFill (grainTexture, 0, 0, 1.0f);
        g.fillRect (primaryZone);
        g.setFillType (juce::FillType (juce::Colour (0xff1c1a1f))); // reset fill
    }

    // Header separator line (1px, Cairn border style)
    g.setColour (juce::Colour (0x1a5c5147)); // Cairn at ~10% opacity
    g.drawHorizontalLine (headerZone.getBottom(), 0.0f, (float) getWidth());

    // Utility zone separator
    g.drawHorizontalLine (utilityZone.getY(), 0.0f, (float) getWidth());

    // Footer separator + Cairn makers-mark text (or SVG path)
    g.setColour (juce::Colour (0xff5c5147).withAlpha (0.25f)); // 25% opacity
    g.setFont (/* Cairn icon path or small text */);
    g.drawText ("~ CAIRN", footerZone.withTrimmedRight (50),
                juce::Justification::centred, false);

    // CLAYMORE title (Cormorant Garamond, steel-etched treatment)
    g.setColour (juce::Colour (0xffd8d3cc)); // Bone
    g.setFont (titleFont); // FontOptions-based, set up in constructor
    g.drawText ("CLAYMORE", headerZone.withTrimmedRight (80).withTrimmedLeft (16),
                juce::Justification::centredLeft, false);
}
```

### ColourGradient for LED Glow Bloom

```cpp
// LED indicator for bypass/active state — radial glow from cold blue
void drawLEDIndicator (juce::Graphics& g, juce::Rectangle<float> bounds, bool isActive)
{
    auto centre = bounds.getCentre();

    if (isActive)
    {
        // Outer glow bloom (radial, fades to transparent)
        juce::ColourGradient glow { juce::Colour (0x604a7cb5), centre.x, centre.y,
                                    juce::Colours::transparentBlack,
                                    centre.x + bounds.getWidth() * 1.5f, centre.y,
                                    true }; // isRadial = true
        g.setGradientFill (glow);
        g.fillEllipse (bounds.expanded (8.0f));

        // LED bulb core
        g.setColour (juce::Colour (0xff6fb3f0)); // bright cold blue
    }
    else
    {
        g.setColour (juce::Colour (0xff2a2523)); // dark, off state
    }

    g.fillEllipse (bounds);
    g.setColour (juce::Colour (0x205c5147));
    g.drawEllipse (bounds, 1.0f); // subtle rim
}
```

---

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| `Font(name, height, style)` constructor | `FontOptions{}.withName().withHeight()` | JUCE 8 | Old constructors deprecated; both work in 8.0.12 but FontOptions is the forward-compatible path |
| `Typeface::createSystemTypefaceFor` manual caching | Same call but hold `Typeface::Ptr` returned | JUCE 7+ | The ptr is reference-counted; typeface stays registered as long as ptr is alive |
| Manual `beginChangeGesture/endChangeGesture` | `SliderAttachment` / `ParameterAttachment` handles this | JUCE 5 | No need to call gesture methods on `AudioParameterFloat` directly from UI code |
| GenericAudioProcessorEditor (current Phase 1-2 stub) | Custom `AudioProcessorEditor` with LookAndFeel | This phase | Full visual and interaction control |

**Deprecated/outdated:**
- `juce::Font(juce::String name, float size, int style)`: Deprecated in JUCE 8; use `FontOptions`. Works in 8.0.12 but will warn.
- Projucer-based binary data setup: This project uses CMake — use `juce_add_binary_data()` in CMakeLists.txt.

---

## Open Questions

1. **Exact double-click reset value for each knob**
   - What we know: `setDoubleClickReturnValue(true, value)` takes the reset value in the slider's native range (not normalized)
   - What's unclear: The current APVTS parameters use normalized 0.0-1.0 ranges for most controls, but `inputGain`, `outputGain`, `gateThreshold` use actual dB values. Need to confirm what value `SliderAttachment` presents to the Slider — it should use the native parameter range, so defaults would be `0.0f` for inputGain (0 dB), `0.0f` for outputGain, `-40.0f` for gateThreshold, `0.5f` for drive/tone/presence.
   - Recommendation: Check `Slider::getValue()` after attachment initialization — if it returns a value in the parameter's declared range (not 0.0-1.0), then `setDoubleClickReturnValue` should use those native values. This should be confirmed on first build.

2. **Oversampling selector normalized value mapping**
   - What we know: `AudioParameterChoice` with StringArray {"2x", "4x", "8x"} normalizes to 0.0/0.5/1.0 for indices 0/1/2 based on JUCE's linear spacing rule
   - What's unclear: Whether `ParameterAttachment::setValueAsPartOfGesture` takes the normalized float or an integer index
   - Recommendation: Use `(float)idx / (float)(numChoices - 1)` as the normalized value (0.0, 0.5, 1.0 for a 3-choice parameter). Verify with a quick test against the APVTS parameter's `getNormalisableRange()`.

3. **Cairn makers-mark SVG rendering**
   - What we know: SVG source is at `~/projects/brand/Logos/cairn-makers-mark.svg`. JUCE can render SVGs via `juce::Drawable::createFromSVG`.
   - What's unclear: Whether the BinaryData-embedded SVG approach vs loading from file is preferred for plugin distribution
   - Recommendation: Embed via `juce_add_binary_data` and render with `juce::Drawable::createFromSVGElement` or `createFromImageData`. This ensures the mark is baked into the binary.

4. **gateEnabled button visual treatment**
   - What we know: It's an `AudioParameterBool` (on/off toggle); in CONTEXT.md it's listed as a "Noise Gate" with threshold knob access
   - What's unclear: Whether gateEnabled gets a flip toggle vs a momentary button vs a labeled toggle; the CONTEXT.md specifies "silver metallic flip toggle switches" as a specific idea
   - Recommendation: Implement as a `ToggleButton` with a custom LookAndFeel `drawToggleButton` override that renders a flip-switch appearance (up/down pill shape with silver metallic finish). This is at Claude's discretion per CONTEXT.md.

---

## Cairn Design System Reference (Extracted from ~/projects/brand/)

The following are binding standards for Claymore's GUI, drawn from brand reference files:

### Color Tokens (Claymore-Specific Adaptation)

| Token | Hex | Usage |
|-------|-----|-------|
| Background | `#1c1a1f` | Basalt (#1a1716) + subtle blue shift for steel identity |
| Surface | `#2a2523` | Hearth — elevated panels, zone fills |
| Border | `#5c5147` at 10-15% alpha | Zone separators, knob rims |
| Label text | `#b8b0a5` | Lichen — knob labels (DM Sans, uppercase) |
| Primary text | `#d8d3cc` | Bone — CLAYMORE title, value readouts |
| Accent (Claymore) | `#4a7cb5` | Cold blue steel — arc fill, LED, active states |
| Knob body | `#e8e4df` | Off-white matte (warm white to avoid pure clinical white) |
| Indicator line | `#c8c8c8` | Silver metallic — the pointer/tick on the knob |
| Maker's mark | `#5c5147` at 25% alpha | Footer Cairn mark |

### Typography (Plugin Use)

| Face | Use | JUCE Notes |
|------|-----|------------|
| Cormorant Garamond Light | "CLAYMORE" wordmark in header | Embed via BinaryData; JUCE 8 FontOptions |
| DM Sans Regular/Light | All knob labels and value readouts | Embed via BinaryData |
| System serif fallback | If font load fails | Georgia (JUCE will fall back automatically) |

### Cairn Zone Heights (700×500px window)

| Zone | Height | Content |
|------|--------|---------|
| Header | 36px | "CLAYMORE" wordmark left, LED bypass indicator right |
| Primary | 322px | Drive hero knob + satellite knobs (700-36-60-22-60=322... adjust as needed) |
| Utility | 80px | Output Gain, Mix, Oversampling selector |
| Footer | 22px | Cairn maker's mark centered, version right |

*Note: Heights are approximate; exact values at Claude's discretion. Total must equal 500px.*

---

## Sources

### Primary (HIGH confidence)

- JUCE 8.0.12 official documentation: `drawRotarySlider`, `SliderAttachment`, `ParameterAttachment`, `ColourGradient`, `Font`/`FontOptions`, `Typeface::createSystemTypefaceFor`, `Slider::setDoubleClickReturnValue`, `Slider::setPopupDisplayEnabled` — verified via official docs.juce.com
- [JUCE LookAndFeel customisation tutorial](https://juce.com/tutorials/tutorial_look_and_feel_customisation) — drawRotarySlider code pattern
- [JUCE SliderAttachment Class Reference](https://docs.juce.com/master/classjuce_1_1AudioProcessorValueTreeState_1_1SliderAttachment.html) — constructor signature
- [JUCE Slider Class Reference](https://docs.juce.com/master/classjuce_1_1Slider.html) — SliderStyle, TextBoxStyle, interaction methods
- [JUCE Graphics Class Reference](https://docs.juce.com/master/classjuce_1_1Graphics.html) — drawing API
- [JUCE ColourGradient Class Reference](https://docs.juce.com/master/classColourGradient.html) — gradient constructors
- Cairn Brand Reference (`~/projects/brand/Docs/CAIRN-BRAND-REFERENCE.md`) — color palette, typography
- Cairn Plugin Design System (`~/projects/brand/Docs/CAIRN-PLUGIN-DESIGN-SYSTEM.md`) — zone layout, control standards
- Crucible Mockup (`~/projects/brand/Mockups/Crucible Mockup 1.png`) — visual reference for Cairn dark theme

### Secondary (MEDIUM confidence)

- [JUCE Font Embedding in JUCE 8 forum](https://forum.juce.com/t/embedding-fonts-in-juce-8/66430) — FontOptions + BinaryData pattern verified
- [AudioParameterChoice ComboBox pattern](https://forum.juce.com/t/combobox-and-audioparameterchoice/31332) — populate items before attachment creation
- [ParameterAttachment Class Reference](https://docs.juce.com/master/classjuce_1_1ParameterAttachment.html) — gesture lifecycle methods
- [VST3 automation gesture discussion](https://forum.juce.com/t/vst3-parameter-updates-automation-vs-host-refresh/67373) — SliderAttachment handles this correctly
- macOS HiDPI behavior: multiple JUCE forum sources confirm automatic CoreGraphics 2x scaling on Retina

### Tertiary (LOW confidence)

- Specific normalized value mapping for AudioParameterChoice (0.0/0.5/1.0 for 3 choices) — derived from documented linear spacing rule; should be empirically verified on first build

---

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — all technology is existing JUCE 8.0.12 already in the project; no new dependencies beyond BinaryData for fonts
- Architecture: HIGH — APVTS attachment pattern is stable, well-documented, and matches existing project conventions
- Visual design: HIGH — Cairn design system is fully documented in brand files; Crucible mockup is a direct visual reference
- Pitfalls: HIGH — destruction order and LookAndFeel lifetime issues are documented in official JUCE forums with consistent reports
- Oversampling selector normalized mapping: MEDIUM — logic derived from documented behavior, needs empirical verification

**Research date:** 2026-02-23
**Valid until:** 2026-09-23 (JUCE GUI APIs are stable; 6-month validity is conservative)
