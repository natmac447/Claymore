#include "ClaymoreTheme.h"
#include "BinaryData.h"

ClaymoreTheme::ClaymoreTheme()
{
    // Load embedded fonts from BinaryData
    cormorantTypeface = juce::Typeface::createSystemTypefaceFor (
        ClaymoreAssets::CormorantGaramond_ttf,
        ClaymoreAssets::CormorantGaramond_ttfSize);

    dmSansTypeface = juce::Typeface::createSystemTypefaceFor (
        ClaymoreAssets::DMSansRegular_ttf,
        ClaymoreAssets::DMSansRegular_ttfSize);

    dmSansLightTypeface = juce::Typeface::createSystemTypefaceFor (
        ClaymoreAssets::DMSansLight_ttf,
        ClaymoreAssets::DMSansLight_ttfSize);

    // Slider colour tokens
    setColour (juce::Slider::rotarySliderFillColourId,    juce::Colour (ClaymoreColors::accent));
    setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour (ClaymoreColors::border));
    setColour (juce::Slider::thumbColourId,               juce::Colour (ClaymoreColors::indicator));

    // PopupMenu colours — light surface
    setColour (juce::PopupMenu::backgroundColourId,            juce::Colour (ClaymoreColors::background));
    setColour (juce::PopupMenu::textColourId,                  juce::Colour (ClaymoreColors::labelText));
    setColour (juce::PopupMenu::highlightedBackgroundColourId, juce::Colour (ClaymoreColors::accent).withAlpha (0.15f));
    setColour (juce::PopupMenu::highlightedTextColourId,       juce::Colour (ClaymoreColors::primaryText));

    // Generate grain texture once (128x128 tile, dark speckling for light surface)
    grainTexture = generateGrainTexture (128, 128, 1.0f);
}

//==============================================================================
// drawRotarySlider — dark anthracite knob with LED dot arc

void ClaymoreTheme::drawRotarySlider (juce::Graphics& g,
                                       int x, int y, int width, int height,
                                       float sliderPos,
                                       float rotaryStartAngle,
                                       float rotaryEndAngle,
                                       juce::Slider& /*slider*/)
{
    auto componentRadius = (float) juce::jmin (width / 2, height / 2);
    auto centreX = (float) x + (float) width  * 0.5f;
    auto centreY = (float) y + (float) height * 0.5f;

    // Sizing — LEDs on chassis surface, knob (cap+skirt) through cutout hole
    auto dotRadius     = juce::jmax (2.5f, componentRadius * 0.055f);
    auto dotArcRadius  = componentRadius - dotRadius - 1.0f;
    auto surfaceMargin = juce::jmax (5.0f, componentRadius * 0.14f);
    auto cutoutRadius  = dotArcRadius - surfaceMargin;
    auto cutoutGap     = juce::jmax (2.0f, componentRadius * 0.04f);
    auto radius        = cutoutRadius - cutoutGap;
    auto skirtWidth    = juce::jmax (2.5f, radius * 0.09f);
    auto capRadius     = radius - skirtWidth;
    auto angle         = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    // 1. Glass-dome LED arc — surface-mounted on chassis
    {
        const int numDots = 11;
        for (int i = 0; i < numDots; ++i)
        {
            float dotNorm  = (float) i / (float) (numDots - 1);
            float dotAngle = rotaryStartAngle + dotNorm * (rotaryEndAngle - rotaryStartAngle);
            float dotX     = centreX + dotArcRadius * std::sin (dotAngle);
            float dotY     = centreY - dotArcRadius * std::cos (dotAngle);
            bool isActive  = dotNorm <= sliderPos + 0.001f;

            g.setColour (isActive ? juce::Colour (ClaymoreColors::ledActive)
                                  : juce::Colour (ClaymoreColors::ledInactive));
            g.fillEllipse (dotX - dotRadius, dotY - dotRadius,
                           dotRadius * 2.0f, dotRadius * 2.0f);

            // Glass dome highlight
            juce::ColourGradient dome (
                juce::Colour (0xffffffff).withAlpha (isActive ? 0.20f : 0.10f),
                dotX, dotY - dotRadius * 0.5f,
                juce::Colours::transparentBlack,
                dotX, dotY + dotRadius * 0.6f, false);
            g.setGradientFill (dome);
            g.fillEllipse (dotX - dotRadius, dotY - dotRadius,
                           dotRadius * 2.0f, dotRadius * 2.0f);

            if (isActive)
            {
                float specR = dotRadius * 0.28f;
                g.setColour (juce::Colour (0x50ffffff));
                g.fillEllipse (dotX - dotRadius * 0.22f - specR,
                               dotY - dotRadius * 0.28f - specR,
                               specR * 2.0f, specR * 2.0f);
            }
        }
    }

    // 2. Chassis cutout — warm dark recess, not black void (#5)
    {
        g.setColour (juce::Colour (0xff2a2520));
        g.fillEllipse (centreX - cutoutRadius, centreY - cutoutRadius,
                       cutoutRadius * 2.0f, cutoutRadius * 2.0f);

        juce::ColourGradient cutShadow (
            juce::Colour (0x50000000), centreX, centreY - cutoutRadius,
            juce::Colours::transparentBlack, centreX, centreY - cutoutRadius * 0.3f, false);
        g.setGradientFill (cutShadow);
        g.fillEllipse (centreX - cutoutRadius, centreY - cutoutRadius,
                       cutoutRadius * 2.0f, cutoutRadius * 2.0f);

        juce::ColourGradient cutRim (
            juce::Colours::transparentBlack, centreX, centreY + cutoutRadius * 0.3f,
            juce::Colour (0x0cffffff), centreX, centreY + cutoutRadius, false);
        g.setGradientFill (cutRim);
        g.fillEllipse (centreX - cutoutRadius, centreY - cutoutRadius,
                       cutoutRadius * 2.0f, cutoutRadius * 2.0f);
    }

    // 3. Knob drop shadow — heavy, directional (#2)
    {
        float shadowOffset = juce::jmax (3.0f, radius * 0.07f);
        float shadowSpread = juce::jmax (4.0f, radius * 0.10f);
        for (int pass = 4; pass >= 0; --pass)
        {
            float spread = shadowSpread * ((float) pass / 4.0f);
            float alpha  = 0.06f + 0.19f * (1.0f - (float) pass / 4.0f);
            g.setColour (juce::Colours::black.withAlpha (alpha));
            g.fillEllipse (centreX - radius - spread,
                           centreY - radius - spread + shadowOffset,
                           (radius + spread) * 2.0f, (radius + spread) * 2.0f);
        }
    }

    // 4. Knob skirt (sidewall) — darker band = visible cylinder edge (#1)
    {
        juce::ColourGradient skirtGrad (
            juce::Colour (ClaymoreColors::knobBody).darker (0.15f),
            centreX, centreY - radius * 0.5f,
            juce::Colour (ClaymoreColors::knobBody).darker (0.6f),
            centreX, centreY + radius, false);
        g.setGradientFill (skirtGrad);
        g.fillEllipse (centreX - radius, centreY - radius,
                       radius * 2.0f, radius * 2.0f);
    }

    // 5. Knurled edge — radial ticks on skirt, rotate with knob (#8, large knobs only)
    if (radius > 18.0f)
    {
        int numTicks = juce::jmax (24, (int) (radius * 0.7f));
        float tickInner = capRadius + skirtWidth * 0.15f;
        float tickOuter = radius - 0.5f;

        for (int t = 0; t < numTicks; ++t)
        {
            float tickAngle = angle + (float) t / (float) numTicks * juce::MathConstants<float>::twoPi;
            float cosA = std::cos (tickAngle);
            float sinA = std::sin (tickAngle);
            float x1 = centreX + tickInner * sinA;
            float y1 = centreY - tickInner * cosA;
            float x2 = centreX + tickOuter * sinA;
            float y2 = centreY - tickOuter * cosA;

            float brightness = (t % 2 == 0) ? 0.10f : 0.04f;
            g.setColour (juce::Colours::white.withAlpha (brightness));
            g.drawLine (x1, y1, x2, y2, 0.5f);
        }
    }

    // 5b. Cap shadow on skirt — cap raised above skirt casts inward shadow
    {
        float shadowInner = capRadius - 1.0f;
        float shadowOuter = capRadius + skirtWidth * 0.6f;
        juce::ColourGradient capShadow (
            juce::Colour (0x30000000), centreX, centreY, // dark at cap edge
            juce::Colours::transparentBlack, centreX + shadowOuter, centreY, true);
        capShadow.addColour ((double) shadowInner / (double) shadowOuter, juce::Colour (0x28000000));
        g.setGradientFill (capShadow);
        // Clip to skirt ring only (don't darken the cap itself)
        juce::Path skirtRing;
        skirtRing.addEllipse (centreX - radius, centreY - radius, radius * 2.0f, radius * 2.0f);
        skirtRing.addEllipse (centreX - capRadius, centreY - capRadius, capRadius * 2.0f, capRadius * 2.0f);
        skirtRing.setUsingNonZeroWinding (false);
        g.saveState();
        g.reduceClipRegion (skirtRing);
        g.fillEllipse (centreX - shadowOuter, centreY - shadowOuter,
                       shadowOuter * 2.0f, shadowOuter * 2.0f);
        g.restoreState();
    }

    // 6. Knob cap (top face) — radial gradient, light source top-left (#1)
    {
        // Highlight offset toward top-left light source
        float hlX = centreX - capRadius * 0.25f;
        float hlY = centreY - capRadius * 0.30f;

        juce::ColourGradient capGrad (
            juce::Colour (ClaymoreColors::knobHighlight).brighter (0.12f),
            hlX, hlY,
            juce::Colour (ClaymoreColors::knobBody).darker (0.40f),
            hlX + capRadius * 1.1f, hlY + capRadius * 1.1f, true);
        capGrad.addColour (0.25, juce::Colour (ClaymoreColors::knobHighlight));
        capGrad.addColour (0.55, juce::Colour (ClaymoreColors::knobBody));
        capGrad.addColour (0.85, juce::Colour (ClaymoreColors::knobBody).darker (0.25f));
        g.setGradientFill (capGrad);
        g.fillEllipse (centreX - capRadius, centreY - capRadius,
                       capRadius * 2.0f, capRadius * 2.0f);
    }

    // 7. Cap-to-skirt lip — bright hairline at cap edge (#1)
    g.setColour (juce::Colour (0x18ffffff));
    g.drawEllipse (centreX - capRadius, centreY - capRadius,
                   capRadius * 2.0f, capRadius * 2.0f, 0.7f);

    // 8. Edge vignette on cap
    {
        juce::ColourGradient vignette (juce::Colours::transparentBlack, centreX, centreY,
                                        juce::Colour (0x14000000), centreX, centreY + capRadius, true);
        vignette.addColour (0.6, juce::Colours::transparentBlack);
        g.setGradientFill (vignette);
        g.fillEllipse (centreX - capRadius, centreY - capRadius,
                       capRadius * 2.0f, capRadius * 2.0f);
    }

    // 9. Outer rim
    g.setColour (juce::Colour (0x28000000));
    g.drawEllipse (centreX - radius, centreY - radius,
                   radius * 2.0f, radius * 2.0f, 0.8f);

    // 10. Indicator groove — shadow/highlight flanking (#7)
    {
        auto pointerLength = capRadius * 0.6f;
        float lineWidth = juce::jmax (1.5f, capRadius * 0.05f);

        juce::Path shadowP;
        shadowP.addRectangle (-lineWidth * 0.5f - 0.7f, -capRadius, 0.7f, pointerLength);
        shadowP.applyTransform (juce::AffineTransform::rotation (angle).translated (centreX, centreY));
        g.setColour (juce::Colour (0x28000000));
        g.fillPath (shadowP);

        juce::Path highlightP;
        highlightP.addRectangle (lineWidth * 0.5f, -capRadius, 0.7f, pointerLength);
        highlightP.applyTransform (juce::AffineTransform::rotation (angle).translated (centreX, centreY));
        g.setColour (juce::Colour (0x12ffffff));
        g.fillPath (highlightP);

        juce::Path p;
        p.addRectangle (-lineWidth * 0.5f, -capRadius, lineWidth, pointerLength);
        p.applyTransform (juce::AffineTransform::rotation (angle).translated (centreX, centreY));
        g.setColour (juce::Colour (ClaymoreColors::indicator));
        g.fillPath (p);
    }
}

//==============================================================================
// drawToggleButton — dark anthracite flip-toggle

void ClaymoreTheme::drawToggleButton (juce::Graphics& g,
                                       juce::ToggleButton& button,
                                       bool shouldDrawButtonAsHighlighted,
                                       bool /*shouldDrawButtonAsDown*/)
{
    juce::ignoreUnused (shouldDrawButtonAsHighlighted);

    auto bounds = button.getLocalBounds().toFloat();

    float pillW = juce::jmin (bounds.getWidth() * 0.6f, 16.0f);
    float pillH = juce::jmin (bounds.getHeight() * 0.8f, 30.0f);
    float pillX = bounds.getCentreX() - pillW * 0.5f;
    float pillY = bounds.getCentreY() - pillH * 0.5f;

    auto pillBounds = juce::Rectangle<float> (pillX, pillY, pillW, pillH);
    float cornerRadius = pillW * 0.5f;

    bool isOn = button.getToggleState();

    // Drop shadow — smaller than knobs (toggle is lower profile)
    for (int pass = 2; pass >= 0; --pass)
    {
        float spread = 2.0f * ((float) pass / 2.0f);
        float alpha  = 0.04f + 0.10f * (1.0f - (float) pass / 2.0f);
        g.setColour (juce::Colours::black.withAlpha (alpha));
        g.fillRoundedRectangle (pillBounds.expanded (spread).translated (0.0f, 1.5f + spread * 0.5f),
                                cornerRadius + spread);
    }

    // Inset recess — toggle sits IN the surface
    auto recessBounds = pillBounds.expanded (2.5f);
    g.setColour (juce::Colour (ClaymoreColors::background).darker (0.04f));
    g.fillRoundedRectangle (recessBounds, cornerRadius + 1.5f);
    g.setColour (juce::Colour (0x10000000));
    g.drawRoundedRectangle (recessBounds, cornerRadius + 1.5f, 0.5f);

    // Pill background — dark anthracite
    g.setColour (juce::Colour (ClaymoreColors::knobBody));
    g.fillRoundedRectangle (pillBounds, cornerRadius);

    // Active half — brushed aluminum indicator
    float halfH = pillH * 0.5f;
    auto activeBounds = isOn
        ? juce::Rectangle<float> (pillX, pillY, pillW, halfH)
        : juce::Rectangle<float> (pillX, pillY + halfH, pillW, halfH);

    juce::Path clipPath;
    clipPath.addRoundedRectangle (pillBounds, cornerRadius);
    g.saveState();
    g.reduceClipRegion (clipPath);
    g.setColour (isOn ? juce::Colour (ClaymoreColors::indicator)
                      : juce::Colour (ClaymoreColors::indicator).withAlpha (0.55f));
    g.fillRect (activeBounds);
    g.restoreState();

    // Subtle rim
    g.setColour (juce::Colour (0x18000000));
    g.drawRoundedRectangle (pillBounds, cornerRadius, 1.0f);

    // LED indicator above pill — always visible, dormant when off
    {
        float ledSize = 6.0f;
        float ledX = pillBounds.getCentreX() - ledSize * 0.5f;
        float ledY = pillBounds.getY() - 6.0f - ledSize;
        drawLEDIndicator (g, juce::Rectangle<float> (ledX, ledY, ledSize, ledSize), isOn);
    }

    // Label below — lowercase
    if (button.getButtonText().isNotEmpty())
    {
        g.setColour (juce::Colour (ClaymoreColors::labelText));
        auto font = getKnobLabelFont (10.0f);
        g.setFont (font);
        g.drawText (button.getButtonText().toLowerCase(),
                    button.getLocalBounds().removeFromBottom (14),
                    juce::Justification::centred, false);
    }
}

//==============================================================================
// drawLabel — DM Sans lowercase

void ClaymoreTheme::drawLabel (juce::Graphics& g, juce::Label& label)
{
    if (! label.isBeingEdited())
    {
        auto font = getKnobLabelFont (label.getFont().getHeight());
        g.setFont (font);
        auto text = label.getText().toLowerCase();
        auto bounds = label.getLocalBounds();
        auto just = label.getJustificationType();

        // Silk-screen shadow — text printed onto surface (#10)
        g.setColour (juce::Colour (0x10000000));
        g.drawText (text, bounds.translated (0, 1), just, false);

        g.setColour (juce::Colour (ClaymoreColors::labelText));
        g.drawText (text, bounds, just, false);
    }
}

//==============================================================================
// ComboBox / PopupMenu overrides

juce::Font ClaymoreTheme::getComboBoxFont (juce::ComboBox& /*box*/)
{
    return getKnobLabelFont (11.0f);
}

void ClaymoreTheme::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
{
    label.setBounds (0, 0, box.getWidth() - 16, box.getHeight());
    label.setJustificationType (juce::Justification::centred);
    label.setFont (getComboBoxFont (box));
}

juce::Font ClaymoreTheme::getPopupMenuFont()
{
    return getKnobLabelFont (11.0f);
}

void ClaymoreTheme::drawComboBox (juce::Graphics& g, int width, int height,
                                   bool /*isButtonDown*/,
                                   int /*buttonX*/, int /*buttonY*/, int /*buttonW*/, int /*buttonH*/,
                                   juce::ComboBox& box)
{
    auto bounds = juce::Rectangle<int> (0, 0, width, height).toFloat();

    g.setColour (box.findColour (juce::ComboBox::backgroundColourId));
    g.fillRoundedRectangle (bounds, 3.0f);

    g.setColour (box.findColour (juce::ComboBox::outlineColourId));
    g.drawRoundedRectangle (bounds.reduced (0.5f), 3.0f, 1.0f);

    {
        juce::Path arrow;
        float arrowX = (float) width - 12.0f;
        float arrowY = (float) height * 0.5f - 2.0f;
        arrow.addTriangle (arrowX, arrowY,
                           arrowX + 5.0f, arrowY,
                           arrowX + 2.5f, arrowY + 4.0f);
        g.setColour (box.findColour (juce::ComboBox::arrowColourId));
        g.fillPath (arrow);
    }
}

void ClaymoreTheme::drawPopupMenuBackground (juce::Graphics& g, int width, int height)
{
    g.fillAll (juce::Colour (ClaymoreColors::background));

    g.setColour (juce::Colour (ClaymoreColors::border).withAlpha (0.20f));
    g.drawRect (0, 0, width, height, 1);
}

void ClaymoreTheme::drawPopupMenuItem (juce::Graphics& g, const juce::Rectangle<int>& area,
                                        bool isSeparator, bool isActive, bool isHighlighted,
                                        bool isTicked, bool /*hasSubMenu*/,
                                        const juce::String& text, const juce::String& /*shortcutKeyText*/,
                                        const juce::Drawable* /*icon*/, const juce::Colour* /*textColour*/)
{
    if (isSeparator)
    {
        auto sepArea = area.reduced (8, 0);
        g.setColour (juce::Colour (ClaymoreColors::border).withAlpha (0.15f));
        g.fillRect (sepArea.getX(), area.getCentreY(), sepArea.getWidth(), 1);
        return;
    }

    if (isHighlighted && isActive)
    {
        g.setColour (juce::Colour (ClaymoreColors::accent).withAlpha (0.15f));
        g.fillRect (area);
        g.setColour (juce::Colour (ClaymoreColors::primaryText));
    }
    else
    {
        g.setColour (isActive ? juce::Colour (ClaymoreColors::labelText)
                              : juce::Colour (ClaymoreColors::labelText).withAlpha (0.4f));
    }

    auto textArea = area.reduced (10, 0);

    if (isTicked)
    {
        auto tickArea = textArea.removeFromLeft (14);
        g.drawText (juce::String::charToString (0x2713), tickArea,
                    juce::Justification::centred, false);
    }

    g.setFont (getKnobLabelFont (11.0f));
    g.drawFittedText (text, textArea, juce::Justification::centredLeft, 1);
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
    auto r = bounds.getWidth() * 0.5f;

    // Drop shadow — small, LED sits just above surface
    for (int pass = 1; pass >= 0; --pass)
    {
        float spread = 1.0f + 1.0f * (float) pass;
        float alpha  = 0.06f + 0.08f * (1.0f - (float) pass);
        g.setColour (juce::Colours::black.withAlpha (alpha));
        g.fillEllipse (centre.x - r - spread,
                       centre.y - r - spread + 1.0f,
                       (r + spread) * 2.0f, (r + spread) * 2.0f);
    }

    if (isActive)
    {
        // Radial glow — gradient fade, no hard clip
        float glowR = r * 3.0f;
        juce::ColourGradient glow (
            juce::Colour (ClaymoreColors::ledActive).withAlpha (0.4f),
            centre.x, centre.y,
            juce::Colours::transparentBlack,
            centre.x + glowR, centre.y, true);
        g.setGradientFill (glow);
        g.fillEllipse (centre.x - glowR, centre.y - glowR,
                       glowR * 2.0f, glowR * 2.0f);

        // LED core — bright amber
        g.setColour (juce::Colour (0xffe8b070));
    }
    else
    {
        g.setColour (juce::Colour (ClaymoreColors::border));
    }

    g.fillEllipse (bounds);

    // Glass dome gradient — top-lit highlight
    {
        juce::ColourGradient dome (
            juce::Colour (0xffffffff).withAlpha (isActive ? 0.22f : 0.12f),
            centre.x, centre.y - r * 0.5f,
            juce::Colours::transparentBlack,
            centre.x, centre.y + r * 0.6f, false);
        g.setGradientFill (dome);
        g.fillEllipse (bounds);
    }

    // Specular highlight — bright dot offset toward light source
    if (isActive)
    {
        float specR = r * 0.25f;
        float specX = centre.x - r * 0.2f;
        float specY = centre.y - r * 0.25f;
        g.setColour (juce::Colour (0x55ffffff));
        g.fillEllipse (specX - specR, specY - specR,
                       specR * 2.0f, specR * 2.0f);
    }

    // Rim
    g.setColour (juce::Colour (0x18000000));
    g.drawEllipse (bounds, 0.5f);
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
            // Dark speckling for light powder-coat surface
            auto v = (uint8_t) rng.nextInt (80);
            bd.setPixelColour (col, row,
                juce::Colour (v, v, v).withAlpha (opacity * 0.07f));
        }
    }
    return img;
}
