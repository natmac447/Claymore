#include "PluginEditor.h"

//==============================================================================
ClaymoreEditor::ClaymoreEditor (ClaymoreProcessor& p)
    : AudioProcessorEditor (p),
      processor (p),
      // Attachments initialised in member-init list — CRITICAL ordering:
      // Each attachment calls sendInitialUpdate() which posts back to the slider,
      // so the slider members above must already be constructed.
      driveAttach         (p.apvts, ParamIDs::drive,         driveKnob),
      tightnessAttach     (p.apvts, ParamIDs::tightness,     tightnessKnob),
      sagAttach           (p.apvts, ParamIDs::sag,           sagKnob),
      toneAttach          (p.apvts, ParamIDs::tone,          toneKnob),
      presenceAttach      (p.apvts, ParamIDs::presence,      presenceKnob),
      inputGainAttach     (p.apvts, ParamIDs::inputGain,     inputGainKnob),
      outputGainAttach    (p.apvts, ParamIDs::outputGain,    outputGainKnob),
      mixAttach           (p.apvts, ParamIDs::mix,           mixKnob),
      gateThresholdAttach (p.apvts, ParamIDs::gateThreshold, gateThresholdKnob),
      gateEnabledAttach   (p.apvts, ParamIDs::gateEnabled,   gateEnabledButton)
{
    //==========================================================================
    // Apply LookAndFeel
    setLookAndFeel (&theme);

    //==========================================================================
    // Configure all rotary knobs
    for (auto* knob : { &driveKnob, &tightnessKnob, &sagKnob,
                        &toneKnob, &presenceKnob,
                        &inputGainKnob, &outputGainKnob, &mixKnob, &gateThresholdKnob,
                        &clipTypeKnob })
    {
        knob->setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        knob->setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        knob->setPopupDisplayEnabled (true, true, this);
        addAndMakeVisible (knob);
    }

    //==========================================================================
    // Double-click reset defaults
    driveKnob.setDoubleClickReturnValue         (true, 0.5f);
    tightnessKnob.setDoubleClickReturnValue     (true, 0.0f);
    sagKnob.setDoubleClickReturnValue           (true, 0.0f);
    toneKnob.setDoubleClickReturnValue          (true, 0.5f);
    presenceKnob.setDoubleClickReturnValue      (true, 0.5f);
    inputGainKnob.setDoubleClickReturnValue     (true, 0.0f);
    outputGainKnob.setDoubleClickReturnValue    (true, 0.0f);
    mixKnob.setDoubleClickReturnValue           (true, 1.0f);
    gateThresholdKnob.setDoubleClickReturnValue (true, -40.0f);
    clipTypeKnob.setDoubleClickReturnValue      (true, 0.0);

    //==========================================================================
    // Set up labels — DM Sans, lowercase via ClaymoreTheme::drawLabel
    auto setupLabel = [this](juce::Label& label, const juce::String& text)
    {
        label.setText (text, juce::dontSendNotification);
        label.setJustificationType (juce::Justification::centred);
        label.setFont (theme.getKnobLabelFont (11.0f));
        label.setColour (juce::Label::textColourId, juce::Colour (ClaymoreColors::labelText));
        label.setInterceptsMouseClicks (false, false);
        addAndMakeVisible (label);
    };

    setupLabel (driveLabel,         "DRIVE");
    setupLabel (tightnessLabel,     "TIGHTNESS");
    setupLabel (sagLabel,           "SAG");
    setupLabel (toneLabel,          "TONE");
    setupLabel (presenceLabel,      "PRESENCE");
    setupLabel (inputGainLabel,     "INPUT");
    setupLabel (outputGainLabel,    "OUTPUT");
    setupLabel (mixLabel,           "MIX");
    setupLabel (gateThresholdLabel, "THRESHOLD");

    //==========================================================================
    // Gate enabled toggle
    gateEnabledButton.setButtonText ("GATE");
    gateEnabledButton.addMouseListener (this, false);
    addAndMakeVisible (gateEnabledButton);

    //==========================================================================
    // Oversampling — header ComboBox
    oversamplingBox.addItem ("2x", 1);
    oversamplingBox.addItem ("4x", 2);
    oversamplingBox.addItem ("8x", 3);
    oversamplingBox.setColour (juce::ComboBox::backgroundColourId, juce::Colour (ClaymoreColors::surface));
    oversamplingBox.setColour (juce::ComboBox::textColourId,       juce::Colour (ClaymoreColors::primaryText));
    oversamplingBox.setColour (juce::ComboBox::outlineColourId,    juce::Colour (ClaymoreColors::border).withAlpha (0.3f));
    oversamplingBox.setColour (juce::ComboBox::arrowColourId,      juce::Colour (ClaymoreColors::labelText));
    addAndMakeVisible (oversamplingBox);
    oversamplingAttach = std::make_unique<ComboBoxAttachment> (p.apvts, ParamIDs::oversampling, oversamplingBox);

    //==========================================================================
    // Clip Type — detented rotary knob
    setupLabel (clipTypeLabel, "CIRCUIT");
    clipTypeAttach = std::make_unique<SliderAttachment> (p.apvts, ParamIDs::clipType, clipTypeKnob);

    //==========================================================================
    // Fixed 700x500 window — no resize handle
    setResizable (false, false);
    setSize (700, 500);
}

ClaymoreEditor::~ClaymoreEditor()
{
    gateEnabledButton.removeMouseListener (this);

    // CRITICAL: Clear LookAndFeel pointer before theme member is destroyed.
    setLookAndFeel (nullptr);
}

//==============================================================================
void ClaymoreEditor::paint (juce::Graphics& g)
{
    // 1. Background fill — warm light gray (powder-coat matte)
    g.fillAll (juce::Colour (ClaymoreColors::background));

    // 2. Utility zone — slightly darker surface band
    {
        g.setColour (juce::Colour (ClaymoreColors::surface));
        g.fillRect (utilityZone);
    }

    // 3. Grain texture over ALL zones — subtle dark speckling
    const auto& grain = theme.getGrainTexture();
    if (grain.isValid())
    {
        g.setTiledImageFill (grain, 0, 0, 1.0f);
        g.fillRect (getLocalBounds());
    }

    // 4. Zone separators — beveled edges for machined panel feel
    auto drawBevel = [&](int xPos, int yPos, int w)
    {
        g.setColour (juce::Colour (0x18000000));
        g.fillRect (xPos, yPos, w, 1);
        g.setColour (juce::Colour (0x10ffffff));
        g.fillRect (xPos, yPos + 1, w, 1);
    };

    drawBevel (headerZone.getX(), headerZone.getBottom(), headerZone.getWidth());
    drawBevel (utilityZone.getX(), utilityZone.getY(), utilityZone.getWidth());
    drawBevel (utilityZone.getX(), utilityZone.getBottom(), utilityZone.getWidth());

    // 5. CLAYMORE title — embossed/debossed text on light surface
    {
        const juce::String title = "CLAYMORE";
        const auto titleFont = theme.getTitleFont (22.0f);
        const int leftPad = 16;
        const auto textArea = headerZone.withTrimmedLeft (leftPad);

        g.setFont (titleFont);

        // Deboss shadow
        g.setColour (juce::Colour (0x18000000));
        g.drawText (title, textArea.translated (0, 1).toFloat(), juce::Justification::centredLeft, false);

        // Deboss highlight
        g.setColour (juce::Colour (0x0cffffff));
        g.drawText (title, textArea.toFloat().translated (0.0f, -0.5f), juce::Justification::centredLeft, false);

        // Main text
        g.setColour (juce::Colour (ClaymoreColors::primaryText));
        g.drawText (title, textArea, juce::Justification::centredLeft, false);
    }

    // 6. "oversampling" label — lowercase, dark text
    {
        g.setFont (theme.getKnobLabelFont (9.0f));
        g.setColour (juce::Colour (ClaymoreColors::labelText));
        g.drawText ("oversampling",
                    juce::Rectangle<int> (490, headerZone.getY(), 106, headerZone.getHeight()),
                    juce::Justification::centredRight, false);
    }

    // 7. LED bypass indicator — warm amber (always active in v1)
    {
        const float ledSize = 10.0f;
        const float ledX = (float) headerZone.getRight() - 16.0f - ledSize;
        const float ledY = (float) headerZone.getCentreY() - ledSize * 0.5f;
        auto ledBounds = juce::Rectangle<float> (ledX, ledY, ledSize, ledSize);
        ClaymoreTheme::drawLEDIndicator (g, ledBounds, true);
    }

    // 8. Cairn icon + "CAIRN" text — centred in footer, dark on light
    {
        const float iconH   = 14.0f;
        const float iconW   = iconH * 1.15f;
        const float lineExt = 3.0f;
        const float gap     = 8.0f;

        const auto  cairnFont = theme.getTitleFont (16.0f);
        juce::GlyphArrangement glyphs;
        glyphs.addLineOfText (cairnFont, "CAIRN", 0.0f, 0.0f);
        const float textW = glyphs.getBoundingBox (0, glyphs.getNumGlyphs(), false).getWidth();

        float totalW = (iconW + lineExt * 2.0f) + gap + textW;
        float startX = footerZone.toFloat().getCentreX() - totalW * 0.5f;
        float centreY = footerZone.toFloat().getCentreY();

        // Equilateral triangle (stroke only)
        float triLeft   = startX + lineExt;
        float triTop    = centreY - iconH * 0.5f;
        float triBottom = centreY + iconH * 0.5f;
        float triMidX   = triLeft + iconW * 0.5f;

        juce::Path triangle;
        triangle.startNewSubPath (triMidX, triTop);
        triangle.lineTo (triLeft + iconW, triBottom);
        triangle.lineTo (triLeft, triBottom);
        triangle.closeSubPath();

        g.setColour (juce::Colour (ClaymoreColors::primaryText).withAlpha (0.20f));
        g.strokePath (triangle, juce::PathStrokeType (1.0f, juce::PathStrokeType::mitered,
                                                       juce::PathStrokeType::square));

        // Bisecting horizontal line
        float lineY = triTop + iconH * 0.62f;
        g.setColour (juce::Colour (ClaymoreColors::primaryText).withAlpha (0.12f));
        g.drawLine (startX, lineY, startX + iconW + lineExt * 2.0f, lineY, 0.6f);

        // "CAIRN" text
        float textX = startX + iconW + lineExt * 2.0f + gap;
        g.setFont (cairnFont);
        g.setColour (juce::Colour (ClaymoreColors::primaryText).withAlpha (0.35f));
        g.drawText ("CAIRN",
                    juce::Rectangle<float> (textX, (float) footerZone.getY(),
                                            textW + 4.0f, (float) footerZone.getHeight()),
                    juce::Justification::centredLeft, false);
    }

    // 9. Version text — right-aligned in footer
    {
        g.setFont (theme.getKnobValueFont (9.0f));
        g.setColour (juce::Colour (ClaymoreColors::primaryText).withAlpha (0.30f));
        g.drawText ("v1.0.0",
                    footerZone.withTrimmedRight (8),
                    juce::Justification::centredRight,
                    false);
    }

    // 10. Panel enclosure edge — beveled metal border
    {
        auto fullBounds = getLocalBounds().toFloat();
        g.setColour (juce::Colour (0x30000000));
        g.drawRect (fullBounds, 1.0f);
        g.setColour (juce::Colour (0x10ffffff));
        g.drawRect (fullBounds.reduced (1.0f), 1.0f);
    }
}

//==============================================================================
void ClaymoreEditor::resized()
{
    auto bounds = getLocalBounds();

    const int headerH  = 36;
    const int footerH  = 34;
    const int utilityH = 90;

    headerZone  = bounds.removeFromTop    (headerH);
    footerZone  = bounds.removeFromBottom (footerH);
    utilityZone = bounds.removeFromBottom (utilityH);
    primaryZone = bounds; // remainder ~362px

    //==========================================================================
    // Helper: position a rotary knob and its label
    auto placeKnob = [](juce::Slider& knob, juce::Label& label, int cx, int cy, int size)
    {
        knob.setBounds (cx - size / 2, cy - size / 2, size, size);
        label.setBounds (cx - size / 2 - 8, cy + size / 2 + 2, size + 16, 14);
    };

    const int pTop = primaryZone.getY();

    //==========================================================================
    // Primary Zone — Drive-centric hero layout (vertically spread)
    placeKnob (tightnessKnob,  tightnessLabel,  120, pTop + 78,  60);
    placeKnob (clipTypeKnob,   clipTypeLabel,   220, pTop + 78,  60);

    placeKnob (sagKnob,        sagLabel,        170, pTop + 250, 60);

    placeKnob (driveKnob,      driveLabel,      350, pTop + 138, 110);

    placeKnob (toneKnob,       toneLabel,       480, pTop + 78,  60);
    placeKnob (presenceKnob,   presenceLabel,   580, pTop + 78,  60);
    placeKnob (gateThresholdKnob, gateThresholdLabel, 530, pTop + 250, 60);

    // Gate enabled toggle
    gateEnabledButton.setBounds (603, pTop + 218, 30, 64);

    //==========================================================================
    // Utility Zone — Input | Mix | Output
    {
        const int uMidY = utilityZone.getCentreY() - 4;

        placeKnob (inputGainKnob,  inputGainLabel,  117, uMidY, 48);
        placeKnob (mixKnob,        mixLabel,        350, uMidY, 48);
        placeKnob (outputGainKnob, outputGainLabel, 583, uMidY, 48);
    }

    //==========================================================================
    // Header — oversampling ComboBox
    oversamplingBox.setBounds (608, 7, 44, 22);
}

//==============================================================================
void ClaymoreEditor::mouseDown (const juce::MouseEvent& e)
{
    if (e.originalComponent == &gateEnabledButton && e.mods.isPopupMenu())
    {
        // Status header
        bool gateOn = gateEnabledButton.getToggleState();
        auto* threshParam = processor.apvts.getParameter (ParamIDs::gateThreshold);
        float threshVal = threshParam != nullptr ? threshParam->getValue() : 0.0f;
        juce::String threshText = threshParam != nullptr
            ? threshParam->getText (threshVal, 64) : "N/A";

        auto approxEq = [] (float a, float b) { return std::abs (a - b) < 0.01f; };

        juce::PopupMenu menu;
        menu.addItem (1, juce::String ("Gate: ") + (gateOn ? "Enabled" : "Disabled"), false);
        menu.addItem (2, juce::String ("Threshold: ") + threshText, false);
        menu.addSeparator();

        // Attack submenu
        {
            juce::PopupMenu sub;
            float cur = processor.getGateAttack();
            for (float v : { 0.5f, 1.0f, 5.0f, 10.0f, 50.0f })
            {
                juce::String label = juce::String (v, (v == (int) v) ? 0 : 1) + " ms";
                if (approxEq (v, 1.0f)) label += " (default)";
                sub.addItem (label, true, approxEq (cur, v),
                    [this, v] { processor.setGateAttack (v); });
            }
            menu.addSubMenu ("Attack", sub);
        }

        // Release submenu
        {
            juce::PopupMenu sub;
            float cur = processor.getGateRelease();
            for (float v : { 20.0f, 50.0f, 80.0f, 200.0f, 500.0f, 1000.0f })
            {
                juce::String label = juce::String ((int) v) + " ms";
                if (approxEq (v, 80.0f)) label += " (default)";
                sub.addItem (label, true, approxEq (cur, v),
                    [this, v] { processor.setGateRelease (v); });
            }
            menu.addSubMenu ("Release", sub);
        }

        // Hysteresis submenu
        {
            juce::PopupMenu sub;
            float cur = processor.getGateHysteresis();
            for (float v : { 0.0f, 2.0f, 4.0f, 6.0f, 8.0f, 12.0f })
            {
                juce::String label = juce::String ((int) v) + " dB";
                if (approxEq (v, 4.0f)) label += " (default)";
                sub.addItem (label, true, approxEq (cur, v),
                    [this, v] { processor.setGateHysteresis (v); });
            }
            menu.addSubMenu ("Hysteresis", sub);
        }

        // Sidechain HPF submenu
        {
            juce::PopupMenu sub;
            float cur = processor.getGateSidechainHPF();
            for (float v : { 20.0f, 80.0f, 150.0f, 300.0f, 500.0f, 1000.0f })
            {
                juce::String label = juce::String ((int) v) + " Hz";
                if (approxEq (v, 150.0f)) label += " (default)";
                sub.addItem (label, true, approxEq (cur, v),
                    [this, v] { processor.setGateSidechainHPF (v); });
            }
            menu.addSubMenu ("Sidechain HPF", sub);
        }

        // Range submenu
        {
            juce::PopupMenu sub;
            float cur = processor.getGateRange();
            for (float v : { -120.0f, -80.0f, -60.0f, -40.0f, -20.0f })
            {
                juce::String label = juce::String ((int) v) + " dB";
                if (approxEq (v, -60.0f)) label += " (default)";
                sub.addItem (label, true, approxEq (cur, v),
                    [this, v] { processor.setGateRange (v); });
            }
            menu.addSubMenu ("Range", sub);
        }

        menu.addSeparator();

        // Reset All Defaults
        menu.addItem ("Reset All Defaults", [this]
        {
            processor.setGateAttack (1.0f);
            processor.setGateRelease (80.0f);
            processor.setGateHysteresis (4.0f);
            processor.setGateSidechainHPF (150.0f);
            processor.setGateRange (-60.0f);
            gateThresholdKnob.setValue (gateThresholdKnob.getDoubleClickReturnValue(), juce::sendNotification);
        });

        menu.showMenuAsync (juce::PopupMenu::Options()
            .withTargetComponent (&gateEnabledButton)
            .withParentComponent (this));
    }
}
