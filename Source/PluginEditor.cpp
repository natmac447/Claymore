#include "PluginEditor.h"

ClaymoreEditor::ClaymoreEditor (ClaymoreProcessor& p)
    : AudioProcessorEditor (p),
      processor (p)
{
    // Create generic editor showing all parameters
    genericEditor = std::make_unique<juce::GenericAudioProcessorEditor> (p);
    addAndMakeVisible (*genericEditor);

    // Set initial size (400 x 300) — generic editor will fill bounds in resized()
    setSize (400, 300);
}

void ClaymoreEditor::resized()
{
    // Give the generic editor full bounds
    if (genericEditor != nullptr)
        genericEditor->setBounds (getLocalBounds());
}
