#include "PluginEditor.h"
#include "BinaryData.h"

//==============================================================================
ClaymoreEditor::ClaymoreEditor (ClaymoreProcessor& p)
    : AudioProcessorEditor (p),
      processor (p),
      // Attachments initialised in member-init list — CRITICAL ordering:
      // Each attachment calls sendInitialUpdate() which posts back to the slider,
      // so the slider members above must already be constructed.
      driveAttach         (p.apvts, ParamIDs::drive,         driveKnob),
      symmetryAttach      (p.apvts, ParamIDs::symmetry,      symmetryKnob),
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
    for (auto* knob : { &driveKnob, &symmetryKnob, &tightnessKnob, &sagKnob,
                        &toneKnob, &presenceKnob,
                        &inputGainKnob, &outputGainKnob, &mixKnob, &gateThresholdKnob })
    {
        knob->setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        knob->setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        knob->setPopupDisplayEnabled (true, true, this);
        addAndMakeVisible (knob);
    }

    //==========================================================================
    // Double-click reset defaults
    // Values match Parameters.h default + CONTEXT.md intent
    driveKnob.setDoubleClickReturnValue         (true, 0.5f);
    symmetryKnob.setDoubleClickReturnValue      (true, 0.0f);
    tightnessKnob.setDoubleClickReturnValue     (true, 0.0f);
    sagKnob.setDoubleClickReturnValue           (true, 0.0f);
    toneKnob.setDoubleClickReturnValue          (true, 0.5f);
    presenceKnob.setDoubleClickReturnValue      (true, 0.5f);
    inputGainKnob.setDoubleClickReturnValue     (true, 0.0f);
    outputGainKnob.setDoubleClickReturnValue    (true, 0.0f);
    mixKnob.setDoubleClickReturnValue           (true, 1.0f);
    gateThresholdKnob.setDoubleClickReturnValue (true, -40.0f);

    //==========================================================================
    // Set up labels — DM Sans uppercase, Lichen colour, no mouse intercept
    auto setupLabel = [this](juce::Label& label, const juce::String& text)
    {
        label.setText (text, juce::dontSendNotification);
        label.setJustificationType (juce::Justification::centred);
        label.setFont (theme.getKnobLabelFont (11.0f));
        label.setColour (juce::Label::textColourId, juce::Colour (0xffb8b0a5));
        label.setInterceptsMouseClicks (false, false);
        addAndMakeVisible (label);
    };

    setupLabel (driveLabel,         "DRIVE");
    setupLabel (symmetryLabel,      "SYMMETRY");
    setupLabel (tightnessLabel,     "TIGHTNESS");
    setupLabel (sagLabel,           "SAG");
    setupLabel (toneLabel,          "TONE");
    setupLabel (presenceLabel,      "PRESENCE");
    setupLabel (inputGainLabel,     "INPUT");
    setupLabel (outputGainLabel,    "OUTPUT");
    setupLabel (mixLabel,           "MIX");
    setupLabel (gateThresholdLabel, "THRESHOLD");
    setupLabel (gateLabel,          "GATE");

    //==========================================================================
    // Gate enabled toggle
    gateEnabledButton.setButtonText ("GATE");
    addAndMakeVisible (gateEnabledButton);

    //==========================================================================
    // Oversampling selector
    oversamplingSelector = std::make_unique<OversamplingSelector> (p.apvts);
    addAndMakeVisible (*oversamplingSelector);

    //==========================================================================
    // Load Cairn makers-mark from BinaryData
    // Symbol: cairnmakersmark_svg (hyphens stripped by juce_add_binary_data)
    makersMark = juce::Drawable::createFromImageData (
        ClaymoreAssets::cairnmakersmark_svg,
        ClaymoreAssets::cairnmakersmark_svgSize);

    //==========================================================================
    // Fixed 700x500 window — no resize handle
    setResizable (false, false);
    setSize (700, 500);
}

ClaymoreEditor::~ClaymoreEditor()
{
    // CRITICAL: Clear LookAndFeel pointer before theme member is destroyed.
    // If omitted, JUCE may call theme methods on a dangling pointer during teardown.
    setLookAndFeel (nullptr);
}

//==============================================================================
void ClaymoreEditor::paint (juce::Graphics& g)
{
    // 1. Background fill
    g.fillAll (juce::Colour (ClaymoreColors::background));

    // 2. Grain texture over primary zone
    const auto& grain = theme.getGrainTexture();
    if (grain.isValid())
    {
        g.setTiledImageFill (grain, 0, 0, 1.0f);
        g.fillRect (primaryZone);
    }

    // 3. Zone separators — very subtle, 10% alpha
    g.setColour (juce::Colour (ClaymoreColors::border).withAlpha (0.10f));
    g.fillRect (headerZone.getX(), headerZone.getBottom(), headerZone.getWidth(), 1);
    g.fillRect (utilityZone.getX(), utilityZone.getY(), utilityZone.getWidth(), 1);

    // 4. CLAYMORE title — steel-etched treatment
    //    Shadow pass (1px offset, darker), then primary pass (normal position)
    {
        const juce::String title = "CLAYMORE";
        const auto titleFont = theme.getTitleFont (22.0f);
        const int leftPad = 16;
        const auto textArea = headerZone.withTrimmedLeft (leftPad);

        // Shadow pass — darker colour, 1px offset down/right for engraved look
        g.setFont (titleFont);
        g.setColour (juce::Colour (0xff0a0a0d));
        g.drawText (title,
                    textArea.translated (1, 1),
                    juce::Justification::centredLeft,
                    false);

        // Primary pass — Bone text
        g.setColour (juce::Colour (ClaymoreColors::primaryText));
        g.drawText (title,
                    textArea,
                    juce::Justification::centredLeft,
                    false);
    }

    // 5. LED bypass indicator — always active in v1 (no bypass parameter yet)
    {
        const float ledSize = 10.0f;
        const float ledX = (float) headerZone.getRight() - 16.0f - ledSize;
        const float ledY = (float) headerZone.getCentreY() - ledSize * 0.5f;
        auto ledBounds = juce::Rectangle<float> (ledX, ledY, ledSize, ledSize);
        ClaymoreTheme::drawLEDIndicator (g, ledBounds, true);
    }

    // 6. Cairn makers-mark in footer — centred at 25% opacity
    if (makersMark != nullptr)
    {
        const float padding = 4.0f;
        const float maxH = (float) footerZone.getHeight() - padding * 2.0f;
        const auto  footerBounds = footerZone.toFloat().reduced (padding);

        // Scale to fit footer height while preserving aspect ratio
        auto markBounds = makersMark->getDrawableBounds();
        float scale = maxH / markBounds.getHeight();
        float scaledW = markBounds.getWidth() * scale;
        float centreX = footerBounds.getCentreX() - scaledW * 0.5f;
        auto  drawBounds = juce::Rectangle<float> (centreX, footerBounds.getY(), scaledW, maxH);

        makersMark->drawWithin (g, drawBounds,
                                juce::RectanglePlacement::centred,
                                0.25f);
    }

    // 7. Version text — right-aligned in footer, subtle
    {
        g.setFont (theme.getKnobValueFont (9.0f));
        g.setColour (juce::Colour (ClaymoreColors::makersMark).withAlpha (0.35f));
        g.drawText ("v1.0.0",
                    footerZone.withTrimmedRight (8),
                    juce::Justification::centredRight,
                    false);
    }
}

//==============================================================================
void ClaymoreEditor::resized()
{
    auto bounds = getLocalBounds();

    const int headerH  = 36;
    const int footerH  = 22;
    const int utilityH = 80;

    headerZone  = bounds.removeFromTop    (headerH);
    footerZone  = bounds.removeFromBottom (footerH);
    utilityZone = bounds.removeFromBottom (utilityH);
    primaryZone = bounds; // remainder ~362px

    //==========================================================================
    // Helper: position a rotary knob and its label
    //   cx, cy = centre of knob
    //   size   = knob diameter
    auto placeKnob = [](juce::Slider& knob, juce::Label& label, int cx, int cy, int size)
    {
        knob.setBounds (cx - size / 2, cy - size / 2, size, size);
        label.setBounds (cx - size / 2 - 8, cy + size / 2 + 2, size + 16, 14);
    };

    const int pTop = primaryZone.getY(); // top of primary zone

    //==========================================================================
    // Primary Zone — Drive-centric hero layout
    //
    //   LEFT column (pre-distortion controls):
    //     Tightness  (cx=120, row1)   Symmetry  (cx=220, row1)
    //     Input Gain (cx=120, row2)   Sag       (cx=220, row2)
    //
    //   CENTRE:
    //     Drive hero knob (cx=350, centred slightly above vertical midpoint)
    //
    //   RIGHT column (post-distortion controls):
    //     Tone     (cx=480, row1)   Presence (cx=580, row1)
    //     Gate Threshold (cx=530, row2)   Gate toggle (cx=617, row2)

    placeKnob (tightnessKnob,  tightnessLabel,  120, pTop + 60,  60);
    placeKnob (symmetryKnob,   symmetryLabel,   220, pTop + 60,  60);
    placeKnob (inputGainKnob,  inputGainLabel,  120, pTop + 160, 60);
    placeKnob (sagKnob,        sagLabel,        220, pTop + 160, 60);

    placeKnob (driveKnob,      driveLabel,      350, pTop + 100, 110);

    placeKnob (toneKnob,       toneLabel,       480, pTop + 60,  60);
    placeKnob (presenceKnob,   presenceLabel,   580, pTop + 60,  60);
    placeKnob (gateThresholdKnob, gateThresholdLabel, 530, pTop + 160, 60);

    // Gate enabled toggle — vertical pill, near Gate Threshold
    gateEnabledButton.setBounds (603, pTop + 135, 30, 50);
    gateLabel.setBounds (595, pTop + 188, 46, 14);

    //==========================================================================
    // Utility Zone — horizontal thirds: Output Gain | Mix | Oversampling
    {
        const int uTop  = utilityZone.getY();
        const int uMidY = utilityZone.getCentreY();

        placeKnob (outputGainKnob, outputGainLabel, 117, uMidY,    50);
        placeKnob (mixKnob,        mixLabel,        350, uMidY,    50);

        // OversamplingSelector — right third, centred vertically
        const int selW = 120;
        const int selH = 28;
        oversamplingSelector->setBounds (
            532, uTop + (utilityZone.getHeight() - selH) / 2, selW, selH);
    }
}
