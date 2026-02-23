#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "../Parameters.h"

/**
 * OversamplingSelector — custom 3-button radio group for oversampling rate selection.
 *
 * Wraps three TextButton instances ("2x", "4x", "8x") in a radio group, linked to
 * the AudioProcessorValueTreeState oversampling AudioParameterChoice via ParameterAttachment.
 *
 * Ordering contract:
 *   - TextButtons are created and made visible BEFORE ParameterAttachment is constructed,
 *     because sendInitialUpdate() fires the callback which accesses the buttons.
 */
class OversamplingSelector : public juce::Component
{
public:
    explicit OversamplingSelector (juce::AudioProcessorValueTreeState& apvts)
        : attachment (*apvts.getParameter (ParamIDs::oversampling),
                      [this](float v) { updateButtons (v); },
                      nullptr)
    {
        // CRITICAL: Create buttons before attachment sendInitialUpdate fires
        juce::StringArray labels { "2x", "4x", "8x" };
        for (int i = 0; i < 3; ++i)
        {
            buttons[i] = std::make_unique<juce::TextButton> (labels[i]);
            buttons[i]->setRadioGroupId (1, juce::dontSendNotification);
            buttons[i]->setClickingTogglesState (true);

            // Capture index by value for the lambda
            const int idx = i;
            buttons[i]->onClick = [this, idx]
            {
                if (buttons[idx]->getToggleState())
                {
                    attachment.beginGesture();
                    attachment.setValueAsPartOfGesture ((float) idx / 2.0f);
                    attachment.endGesture();
                }
            };

            addAndMakeVisible (*buttons[i]);
        }

        // Sync initial state from parameter (buttons must exist first)
        attachment.sendInitialUpdate();
    }

    ~OversamplingSelector() override = default;

    void resized() override
    {
        auto bounds = getLocalBounds();
        const int gap = 4;
        int totalGap = gap * 2; // gaps between 3 buttons
        int btnW = (bounds.getWidth() - totalGap) / 3;
        int btnH = bounds.getHeight();

        for (int i = 0; i < 3; ++i)
        {
            buttons[i]->setBounds (i * (btnW + gap), 0, btnW, btnH);
        }
    }

private:
    void updateButtons (float normValue)
    {
        int idx = juce::roundToInt (normValue * 2.0f);
        idx = juce::jlimit (0, 2, idx);
        for (int i = 0; i < 3; ++i)
            buttons[i]->setToggleState (i == idx, juce::dontSendNotification);
    }

    std::unique_ptr<juce::TextButton> buttons[3];
    juce::ParameterAttachment attachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OversamplingSelector)
};
