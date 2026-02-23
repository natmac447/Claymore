#include "ClaymoreTheme.h"
#include "BinaryData.h"

ClaymoreTheme::ClaymoreTheme()
{
    // Load embedded fonts from BinaryData
    // Note: JUCE juce_add_binary_data strips hyphens from filenames in symbol names
    cormorantTypeface = juce::Typeface::createSystemTypefaceFor (
        ClaymoreAssets::CormorantGaramondLight_ttf,
        ClaymoreAssets::CormorantGaramondLight_ttfSize);

    dmSansTypeface = juce::Typeface::createSystemTypefaceFor (
        ClaymoreAssets::DMSansRegular_ttf,
        ClaymoreAssets::DMSansRegular_ttfSize);

    dmSansLightTypeface = juce::Typeface::createSystemTypefaceFor (
        ClaymoreAssets::DMSansLight_ttf,
        ClaymoreAssets::DMSansLight_ttfSize);

    // Cairn colour tokens
    setColour (juce::Slider::rotarySliderFillColourId,    juce::Colour (ClaymoreColors::accent));
    setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour (ClaymoreColors::border));
    setColour (juce::Slider::thumbColourId,               juce::Colours::white);

    // Generate grain texture once (128x128 tile)
    grainTexture = generateGrainTexture (128, 128, 1.0f);
}

//==============================================================================
// drawRotarySlider — white matte knob with silver indicator + cold blue arc

void ClaymoreTheme::drawRotarySlider (juce::Graphics& g,
                                       int x, int y, int width, int height,
                                       float sliderPos,
                                       float rotaryStartAngle,
                                       float rotaryEndAngle,
                                       juce::Slider& /*slider*/)
{
    auto radius  = (float) juce::jmin (width / 2, height / 2) - 4.0f;
    auto centreX = (float) x + (float) width  * 0.5f;
    auto centreY = (float) y + (float) height * 0.5f;
    auto angle   = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    // 1. Knob body — off-white matte circle
    g.setColour (juce::Colour (ClaymoreColors::knobBody));
    g.fillEllipse (centreX - radius, centreY - radius, radius * 2.0f, radius * 2.0f);

    // 2. Subtle rim shadow
    g.setColour (juce::Colour (0x30000000));
    g.drawEllipse (centreX - radius, centreY - radius, radius * 2.0f, radius * 2.0f, 1.5f);

    // 3. Silver metallic indicator line — extends from centre outward ~60% of radius
    {
        juce::Path p;
        auto pointerLength = radius * 0.6f;
        p.addRectangle (-1.0f, -radius, 2.0f, pointerLength);
        p.applyTransform (juce::AffineTransform::rotation (angle).translated (centreX, centreY));
        g.setColour (juce::Colour (ClaymoreColors::indicator));
        g.fillPath (p);
    }

    // 4. Inactive arc track — full 270° just outside the knob
    {
        juce::Path arcTrack;
        arcTrack.addArc (centreX - radius - 4.0f, centreY - radius - 4.0f,
                         (radius + 4.0f) * 2.0f, (radius + 4.0f) * 2.0f,
                         rotaryStartAngle, rotaryEndAngle, true);
        g.setColour (juce::Colour (ClaymoreColors::border).withAlpha (0.4f));
        g.strokePath (arcTrack, juce::PathStrokeType (2.0f));
    }

    // 5. Active arc fill — from start to current angle, cold blue
    if (angle > rotaryStartAngle)
    {
        juce::Path arcFill;
        arcFill.addArc (centreX - radius - 4.0f, centreY - radius - 4.0f,
                        (radius + 4.0f) * 2.0f, (radius + 4.0f) * 2.0f,
                        rotaryStartAngle, angle, true);
        g.setColour (juce::Colour (ClaymoreColors::accent));
        g.strokePath (arcFill, juce::PathStrokeType (2.0f));
    }
}

//==============================================================================
// drawToggleButton — metallic flip-toggle switch for Gate Enabled

void ClaymoreTheme::drawToggleButton (juce::Graphics& g,
                                       juce::ToggleButton& button,
                                       bool shouldDrawButtonAsHighlighted,
                                       bool /*shouldDrawButtonAsDown*/)
{
    juce::ignoreUnused (shouldDrawButtonAsHighlighted);

    auto bounds = button.getLocalBounds().toFloat();

    // Draw a vertical pill/capsule shape proportional to bounds
    float pillW = juce::jmin (bounds.getWidth() * 0.6f, 16.0f);
    float pillH = juce::jmin (bounds.getHeight() * 0.8f, 30.0f);
    float pillX = bounds.getCentreX() - pillW * 0.5f;
    float pillY = bounds.getCentreY() - pillH * 0.5f;

    auto pillBounds = juce::Rectangle<float> (pillX, pillY, pillW, pillH);
    float cornerRadius = pillW * 0.5f;

    bool isOn = button.getToggleState();

    // Draw pill background — dark base
    g.setColour (juce::Colour (ClaymoreColors::surface));
    g.fillRoundedRectangle (pillBounds, cornerRadius);

    // Draw flip toggle position — silver half (top = ON, bottom = OFF)
    float halfH = pillH * 0.5f;
    auto silverBounds = isOn
        ? juce::Rectangle<float> (pillX, pillY, pillW, halfH)
        : juce::Rectangle<float> (pillX, pillY + halfH, pillW, halfH);

    // Clip silver fill to pill shape
    juce::Path clipPath;
    clipPath.addRoundedRectangle (pillBounds, cornerRadius);
    g.saveState();
    g.reduceClipRegion (clipPath);
    g.setColour (juce::Colour (ClaymoreColors::indicator));
    g.fillRect (silverBounds);
    g.restoreState();

    // Subtle border
    g.setColour (juce::Colour (ClaymoreColors::border).withAlpha (0.3f));
    g.drawRoundedRectangle (pillBounds, cornerRadius, 1.0f);

    // Button text label below/beside (if any)
    if (button.getButtonText().isNotEmpty())
    {
        g.setColour (juce::Colour (ClaymoreColors::labelText));
        auto font = getKnobLabelFont (10.0f);
        g.setFont (font);
        g.drawText (button.getButtonText().toUpperCase(),
                    button.getLocalBounds().removeFromBottom (14),
                    juce::Justification::centred, false);
    }
}

//==============================================================================
// drawLabel — DM Sans uppercase labels, no background fill

void ClaymoreTheme::drawLabel (juce::Graphics& g, juce::Label& label)
{
    if (! label.isBeingEdited())
    {
        g.setColour (juce::Colour (ClaymoreColors::labelText));
        auto font = getKnobLabelFont (label.getFont().getHeight());
        g.setFont (font);
        g.drawText (label.getText().toUpperCase(),
                    label.getLocalBounds(),
                    label.getJustificationType(),
                    false);
    }
}

//==============================================================================
// Font helpers — JUCE 8 FontOptions API

juce::Font ClaymoreTheme::getTitleFont (float height)
{
    return juce::Font { juce::FontOptions{}
        .withTypeface (cormorantTypeface)
        .withHeight (height) };
}

juce::Font ClaymoreTheme::getKnobLabelFont (float height)
{
    return juce::Font { juce::FontOptions{}
        .withTypeface (dmSansTypeface)
        .withHeight (height) };
}

juce::Font ClaymoreTheme::getKnobValueFont (float height)
{
    return juce::Font { juce::FontOptions{}
        .withTypeface (dmSansLightTypeface)
        .withHeight (height) };
}

//==============================================================================
// Static helpers

void ClaymoreTheme::drawLEDIndicator (juce::Graphics& g,
                                       juce::Rectangle<float> bounds,
                                       bool isActive)
{
    auto centre = bounds.getCentre();

    if (isActive)
    {
        // Outer glow bloom — radial gradient from cold blue to transparent
        juce::ColourGradient glow { juce::Colour (0x604a7cb5), centre.x, centre.y,
                                    juce::Colours::transparentBlack,
                                    centre.x + bounds.getWidth() * 1.5f, centre.y,
                                    true }; // isRadial = true
        g.setGradientFill (glow);
        g.fillEllipse (bounds.expanded (8.0f));

        // LED bulb core — bright cold blue
        g.setColour (juce::Colour (0xff6fb3f0));
    }
    else
    {
        // LED off — dark surface colour
        g.setColour (juce::Colour (ClaymoreColors::surface));
    }

    g.fillEllipse (bounds);

    // Subtle rim
    g.setColour (juce::Colour (ClaymoreColors::border).withAlpha (0.3f));
    g.drawEllipse (bounds, 1.0f);
}

juce::Image ClaymoreTheme::generateGrainTexture (int w, int h, float opacity)
{
    juce::Image img (juce::Image::ARGB, w, h, true);
    juce::Random rng;
    juce::Image::BitmapData bd (img, juce::Image::BitmapData::writeOnly);
    for (int row = 0; row < h; ++row)
    {
        for (int col = 0; col < w; ++col)
        {
            auto v = (uint8_t) rng.nextInt (255);
            bd.setPixelColour (col, row,
                juce::Colour (v, v, v).withAlpha (opacity * 0.08f));
        }
    }
    return img;
}
