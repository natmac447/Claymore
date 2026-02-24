#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

/**
 * ClaymoreTheme — custom LookAndFeel_V4 for the Claymore distortion plugin.
 *
 * Industrial Functionalism aesthetic (Dieter Rams inspired):
 *   - Light warm gray matte surface (powder-coat)
 *   - Dark anthracite knobs with brushed aluminum indicator
 *   - LED dot arc position indicators (warm amber active, muted inactive)
 *   - Cormorant Garamond wordmark, DM Sans lowercase labels
 *   - Minimal decoration, maximum function
 */

namespace ClaymoreColors
{
    // Industrial Functionalism palette — light matte surface, dark controls
    constexpr juce::uint32 background    = 0xffd5d0c9;  // Warm light gray (powder-coat matte)
    constexpr juce::uint32 surface       = 0xffcac5bd;  // Slightly darker (zone differentiation)
    constexpr juce::uint32 border        = 0xff9a9490;  // Zone separators (use at 10-15% alpha)
    constexpr juce::uint32 labelText     = 0xff4a4540;  // Dark warm gray — labels
    constexpr juce::uint32 primaryText   = 0xff2a2622;  // Near-black — title, values
    constexpr juce::uint32 accent        = 0xffc27a52;  // Warm terracotta
    constexpr juce::uint32 knobBody      = 0xff2e2e2e;  // Dark anthracite
    constexpr juce::uint32 knobHighlight = 0xff3e3e3e;  // Top-lit sheen
    constexpr juce::uint32 indicator     = 0xffb8b8b8;  // Brushed aluminum line
    constexpr juce::uint32 ledActive     = 0xffd49060;  // Warm amber LED
    constexpr juce::uint32 ledInactive   = 0xff807870;  // Muted inactive dot
    constexpr juce::uint32 makersMark    = 0xff6a6460;  // Footer branding
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

    void drawComboBox (juce::Graphics& g, int width, int height,
                       bool isButtonDown,
                       int buttonX, int buttonY, int buttonW, int buttonH,
                       juce::ComboBox& box) override;
    juce::Font getComboBoxFont (juce::ComboBox& box) override;
    void positionComboBoxText (juce::ComboBox& box, juce::Label& label) override;
    juce::Font getPopupMenuFont() override;
    void drawPopupMenuBackground (juce::Graphics& g, int width, int height) override;
    void drawPopupMenuItem (juce::Graphics& g, const juce::Rectangle<int>& area,
                            bool isSeparator, bool isActive, bool isHighlighted,
                            bool isTicked, bool hasSubMenu,
                            const juce::String& text, const juce::String& shortcutKeyText,
                            const juce::Drawable* icon, const juce::Colour* textColour) override;

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
