#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"
#include "look/ClaymoreTheme.h"
#include "gui/OversamplingSelector.h"

/**
 * ClaymoreEditor — full pedal-style GUI with Cairn 4-zone layout.
 *
 * Cairn layout (700 x 500px fixed):
 *   - Header  (36px):   CLAYMORE title (Cormorant Garamond) + LED bypass indicator
 *   - Primary (~362px): Drive hero knob (110px) + 9 satellite knobs with labels
 *   - Utility (80px):   Output Gain, Mix knobs + OversamplingSelector
 *   - Footer  (22px):   Cairn makers-mark + version string (painted only)
 *
 * Destruction order:
 *   Attachments MUST be declared AFTER their corresponding sliders (C++ member
 *   destruction is reverse-declaration order — attachments must be destroyed first).
 *   ~ClaymoreEditor() calls setLookAndFeel(nullptr) before ClaymoreTheme is destroyed.
 */
class ClaymoreEditor final : public juce::AudioProcessorEditor
{
public:
    explicit ClaymoreEditor (ClaymoreProcessor& p);
    ~ClaymoreEditor() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    //===========================================================================
    // 1. Processor reference
    ClaymoreProcessor& processor;

    // 2. LookAndFeel — same lifetime as editor (set/cleared explicitly in ctor/dtor)
    ClaymoreTheme theme;

    // 3. Zone rectangles — computed in resized(), read in paint()
    juce::Rectangle<int> headerZone;
    juce::Rectangle<int> primaryZone;
    juce::Rectangle<int> utilityZone;
    juce::Rectangle<int> footerZone;

    // 4. Cairn makers-mark SVG
    std::unique_ptr<juce::Drawable> makersMark;

    // 5. Sliders — declared BEFORE attachments
    juce::Slider driveKnob;
    juce::Slider symmetryKnob;
    juce::Slider tightnessKnob;
    juce::Slider sagKnob;

    juce::Slider toneKnob;
    juce::Slider presenceKnob;

    juce::Slider inputGainKnob;
    juce::Slider outputGainKnob;
    juce::Slider mixKnob;
    juce::Slider gateThresholdKnob;

    // 6. Gate toggle
    juce::ToggleButton gateEnabledButton;

    // 7. Labels — one per knob
    juce::Label driveLabel;
    juce::Label symmetryLabel;
    juce::Label tightnessLabel;
    juce::Label sagLabel;

    juce::Label toneLabel;
    juce::Label presenceLabel;

    juce::Label inputGainLabel;
    juce::Label outputGainLabel;
    juce::Label mixLabel;
    juce::Label gateThresholdLabel;
    juce::Label gateLabel;

    // 8. Attachments — declared AFTER sliders (destroyed first, disconnects before slider dies)
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    SliderAttachment driveAttach;
    SliderAttachment symmetryAttach;
    SliderAttachment tightnessAttach;
    SliderAttachment sagAttach;

    SliderAttachment toneAttach;
    SliderAttachment presenceAttach;

    SliderAttachment inputGainAttach;
    SliderAttachment outputGainAttach;
    SliderAttachment mixAttach;
    SliderAttachment gateThresholdAttach;

    ButtonAttachment gateEnabledAttach;

    // 9. OversamplingSelector
    std::unique_ptr<OversamplingSelector> oversamplingSelector;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ClaymoreEditor)
};
