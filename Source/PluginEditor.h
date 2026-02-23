#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"

/**
 * ClaymoreEditor — minimal editor for Phase 1.
 *
 * Displays a GenericAudioProcessorEditor with all APVTS parameters visible.
 * This allows full parameter control in the DAW for Phase 1 validation.
 *
 * Phase 3 will replace this with the full pedal-style GUI.
 */
class ClaymoreEditor final : public juce::AudioProcessorEditor
{
public:
    explicit ClaymoreEditor (ClaymoreProcessor& p);
    ~ClaymoreEditor() override = default;

    void resized() override;

private:
    ClaymoreProcessor& processor;

    // Generic editor: shows all APVTS parameters as sliders/buttons
    // Sufficient for Phase 1 validation — no custom GUI required
    std::unique_ptr<juce::GenericAudioProcessorEditor> genericEditor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ClaymoreEditor)
};
