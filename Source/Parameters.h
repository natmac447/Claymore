#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "dsp/fuzz/FuzzType.h"

/**
 * Claymore APVTS parameter IDs and layout factory.
 *
 * All 12 parameters:
 *   Distortion: drive, clipType, tightness, sag, tone, presence
 *   Signal chain: inputGain, outputGain, mix, gateEnabled, gateThreshold
 *   Quality: oversampling
 *
 * Parameter names use descriptive mixing-tool language (not GunkLord's creative names).
 */

namespace ParamIDs
{
    // Distortion controls
    inline constexpr const char* drive     = "drive";
    inline constexpr const char* clipType  = "clipType";
    inline constexpr const char* tightness = "tightness";
    inline constexpr const char* sag       = "sag";
    inline constexpr const char* tone      = "tone";
    inline constexpr const char* presence  = "presence";

    // Signal chain controls
    inline constexpr const char* inputGain      = "inputGain";
    inline constexpr const char* outputGain     = "outputGain";
    inline constexpr const char* mix            = "mix";
    inline constexpr const char* gateEnabled    = "gateEnabled";
    inline constexpr const char* gateThreshold  = "gateThreshold";

    // Quality controls
    inline constexpr const char* oversampling   = "oversampling";
}

/**
 * Factory function: creates the APVTS parameter layout for Claymore.
 *
 * All ranges and defaults match REQUIREMENTS.md and CONTEXT.md specs exactly.
 * Called once in ClaymoreProcessor constructor to initialize the APVTS.
 */
inline juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    using namespace juce;
    using APVTS = AudioProcessorValueTreeState;

    APVTS::ParameterLayout layout;

    // --- Distortion controls ---

    // Drive: normalized 0–1 (mapped internally to 1x–40x gain via FuzzConfig::mapDrive)
    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { ParamIDs::drive, 1 },
        "Drive",
        NormalisableRange<float> (0.0f, 1.0f, 0.01f),
        0.5f,
        AudioParameterFloatAttributes{}.withLabel ("Drive")));

    // Clip Type: selects diode clipping circuit (8 variants)
    layout.add (std::make_unique<AudioParameterChoice> (
        ParameterID { ParamIDs::clipType, 1 },
        "Clip Type",
        clipTypeNames,
        0  // default: Silicon (index 0)
    ));

    // Tightness: HPF before distortion (maps to 20–800 Hz in ClaymoreEngine)
    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { ParamIDs::tightness, 1 },
        "Tightness",
        NormalisableRange<float> (0.0f, 1.0f, 0.01f),
        0.0f,
        AudioParameterFloatAttributes{}.withLabel ("Tightness")));

    // Sag: bias-starve sputter (0 = clean LM308, 1 = dying battery)
    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { ParamIDs::sag, 1 },
        "Sag",
        NormalisableRange<float> (0.0f, 1.0f, 0.01f),
        0.0f,
        AudioParameterFloatAttributes{}.withLabel ("Sag")));

    // Tone: LP sweep post-distortion (maps to 2 kHz–20 kHz in FuzzTone)
    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { ParamIDs::tone, 1 },
        "Tone",
        NormalisableRange<float> (0.0f, 1.0f, 0.01f),
        0.5f,
        AudioParameterFloatAttributes{}.withLabel ("Tone")));

    // Presence: high-shelf +/-6 dB at 4 kHz (0=cut, 0.5=flat, 1=boost)
    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { ParamIDs::presence, 1 },
        "Presence",
        NormalisableRange<float> (0.0f, 1.0f, 0.01f),
        0.5f,
        AudioParameterFloatAttributes{}.withLabel ("Presence")));

    // --- Signal chain controls ---

    // Input Gain: pre-distortion level trim (-24 to +24 dB)
    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { ParamIDs::inputGain, 1 },
        "Input Gain",
        NormalisableRange<float> (-24.0f, 24.0f, 0.1f),
        0.0f,
        AudioParameterFloatAttributes{}.withLabel ("dB")));

    // Output Gain: post-mix level trim (-48 to +12 dB)
    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { ParamIDs::outputGain, 1 },
        "Output Gain",
        NormalisableRange<float> (-48.0f, 12.0f, 0.1f),
        0.0f,
        AudioParameterFloatAttributes{}.withLabel ("dB")));

    // Mix: dry/wet blend (0 = fully dry, 1 = fully wet)
    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { ParamIDs::mix, 1 },
        "Mix",
        NormalisableRange<float> (0.0f, 1.0f, 0.01f),
        1.0f,
        AudioParameterFloatAttributes{}.withLabel ("Mix")));

    // Gate Enabled: on/off toggle for noise gate
    layout.add (std::make_unique<AudioParameterBool> (
        ParameterID { ParamIDs::gateEnabled, 1 },
        "Gate",
        false));

    // Gate Threshold: noise gate open threshold (-60 to -10 dBFS)
    // Gate operates pre-distortion on input signal
    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { ParamIDs::gateThreshold, 1 },
        "Gate Threshold",
        NormalisableRange<float> (-60.0f, -10.0f, 0.1f),
        -40.0f,
        AudioParameterFloatAttributes{}.withLabel ("dB")));

    // Oversampling: selectable quality vs. CPU tradeoff (QUAL-01, QUAL-02)
    // Index 0 = 2x (lowest latency), 1 = 4x, 2 = 8x (highest quality)
    layout.add (std::make_unique<AudioParameterChoice> (
        ParameterID { ParamIDs::oversampling, 1 },
        "Oversampling",
        StringArray { "2x", "4x", "8x" },
        0  // default: 2x (index 0)
    ));

    return layout;
}
