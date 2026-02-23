#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

/**
 * ClaymoreTheme — custom LookAndFeel_V4 subclass for the Claymore distortion plugin.
 *
 * Implements the Cairn design system:
 *   - White matte knobs with silver metallic indicator line
 *   - Cold blue steel arc fill for active parameter range
 *   - Cormorant Garamond for wordmark / title text
 *   - DM Sans Regular/Light for knob labels and value readouts
 *   - Powder-coated grain texture background
 *   - LED bulb-style indicator with glow bloom
 */

namespace ClaymoreColors
{
    // Cairn color tokens (Claymore-specific cold blue adaptation)
    constexpr juce::uint32 background  = 0xff1c1a1f;  // Basalt + blue shift
    constexpr juce::uint32 surface     = 0xff2a2523;  // Hearth — elevated panels
    constexpr juce::uint32 border      = 0xff5c5147;  // Zone separators (use at 10-15% alpha)
    constexpr juce::uint32 labelText   = 0xffb8b0a5;  // Lichen — knob labels
    constexpr juce::uint32 primaryText = 0xffd8d3cc;  // Bone — title, values
    constexpr juce::uint32 accent      = 0xff4a7cb5;  // Cold blue steel
    constexpr juce::uint32 knobBody    = 0xffe8e4df;  // Off-white matte
    constexpr juce::uint32 indicator   = 0xffc8c8c8;  // Silver metallic
    constexpr juce::uint32 makersMark  = 0xff5c5147;  // At 25% alpha for footer
}

class ClaymoreTheme : public juce::LookAndFeel_V4
{
public:
    ClaymoreTheme();
    ~ClaymoreTheme() override = default;

    //==============================================================================
    // LookAndFeel overrides

    void drawRotarySlider (juce::Graphics& g,
                           int x, int y, int width, int height,
                           float sliderPos,
                           float rotaryStartAngle,
                           float rotaryEndAngle,
                           juce::Slider& slider) override;

    void drawToggleButton (juce::Graphics& g,
                           juce::ToggleButton& button,
                           bool shouldDrawButtonAsHighlighted,
                           bool shouldDrawButtonAsDown) override;

    void drawLabel (juce::Graphics& g, juce::Label& label) override;

    //==============================================================================
    // Font helpers — JUCE 8 FontOptions API
    // Note: getLabelFont(Label&) is a virtual in LookAndFeel_V2 — use distinct names.

    juce::Font getTitleFont (float height);
    juce::Font getKnobLabelFont (float height);
    juce::Font getKnobValueFont  (float height);

    //==============================================================================
    // Static helpers

    /** Draw a radial glow LED indicator for bypass/active state. */
    static void drawLEDIndicator (juce::Graphics& g,
                                  juce::Rectangle<float> bounds,
                                  bool isActive);

    /** Generate a tileable grain noise texture. Call once and cache. */
    static juce::Image generateGrainTexture (int w, int h, float opacity);

    /** Returns the cached grain texture (128x128 ARGB tile). */
    const juce::Image& getGrainTexture() const { return grainTexture; }

private:
    juce::Typeface::Ptr cormorantTypeface;
    juce::Typeface::Ptr dmSansTypeface;
    juce::Typeface::Ptr dmSansLightTypeface;

    juce::Image grainTexture;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ClaymoreTheme)
};
