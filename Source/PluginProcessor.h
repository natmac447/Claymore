#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#include "Parameters.h"
#include "dsp/ClaymoreEngine.h"
#include "dsp/OutputLimiter.h"

/**
 * ClaymoreProcessor — main AudioProcessor subclass.
 *
 * Signal chain (processBlock):
 *   isInitialized guard
 *   → Input Gain (SmoothedValue, multiplicative)
 *   → DryWetMixer::pushDrySamples() (capture dry with latency compensation)
 *   → ClaymoreEngine::process() [gate → oversample → fuzz → tone]
 *   → DryWetMixer::mixWetSamples() (blend with latency-compensated dry)
 *   → Output Gain (SmoothedValue, multiplicative)
 *   → OutputLimiter::process() (brickwall, last in chain)
 *
 * All 11 APVTS parameters cached as std::atomic<float>* in prepareToPlay()
 * for real-time safe access in processBlock (no string lookups at runtime).
 */
class ClaymoreProcessor final : public juce::AudioProcessor
{
public:
    ClaymoreProcessor();
    ~ClaymoreProcessor() override = default;

    // AudioProcessor lifecycle
    void prepareToPlay  (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock   (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    // Editor
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    // Plugin identity
    const juce::String getName() const override { return "Claymore"; }

    bool acceptsMidi()  const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    // Programs (not used — single preset)
    int getNumPrograms() override                              { return 1; }
    int getCurrentProgram() override                           { return 0; }
    void setCurrentProgram (int) override                      {}
    const juce::String getProgramName (int) override           { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    // State persistence
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Bus layout
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    // APVTS (public — editor reads it directly)
    juce::AudioProcessorValueTreeState apvts;

private:
    // Initialization guard: some hosts call processBlock before prepareToPlay
    std::atomic<bool> isInitialized { false };

    // Cached atomic parameter pointers — set in prepareToPlay, read in processBlock
    // Distortion
    std::atomic<float>* driveParam    = nullptr;
    std::atomic<float>* symmetryParam = nullptr;
    std::atomic<float>* tightnessParam= nullptr;
    std::atomic<float>* sagParam      = nullptr;
    std::atomic<float>* toneParam     = nullptr;
    std::atomic<float>* presenceParam = nullptr;
    // Signal chain
    std::atomic<float>* inputGainParam     = nullptr;
    std::atomic<float>* outputGainParam    = nullptr;
    std::atomic<float>* mixParam           = nullptr;
    std::atomic<float>* gateEnabledParam   = nullptr;
    std::atomic<float>* gateThresholdParam = nullptr;

    // DSP objects
    ClaymoreEngine engine;
    OutputLimiter  outputLimiter;

    // Dry/wet mixer with latency compensation for oversampling (SIG-03)
    // 64-sample capacity: sufficient for 2x IIR oversampling latency (~4 samples)
    juce::dsp::DryWetMixer<float> dryWetMixer { 64 };

    // Input/output gain as SmoothedValues (multiplicative, not dB — converted on use)
    // Smooth at audio rate to prevent zipper noise on gain changes
    juce::SmoothedValue<float> inputGainSmoother;
    juce::SmoothedValue<float> outputGainSmoother;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ClaymoreProcessor)
};
