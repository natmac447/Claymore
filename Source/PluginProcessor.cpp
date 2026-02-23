#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

// =============================================================================
ClaymoreProcessor::ClaymoreProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "ClaymoreParameters", createParameterLayout())
{
    // Cache all 11 raw parameter pointers for real-time safe reads in processBlock.
    // Must be done in constructor so pointers are valid immediately.

    // Distortion
    driveParam     = apvts.getRawParameterValue (ParamIDs::drive);
    symmetryParam  = apvts.getRawParameterValue (ParamIDs::symmetry);
    tightnessParam = apvts.getRawParameterValue (ParamIDs::tightness);
    sagParam       = apvts.getRawParameterValue (ParamIDs::sag);
    toneParam      = apvts.getRawParameterValue (ParamIDs::tone);
    presenceParam  = apvts.getRawParameterValue (ParamIDs::presence);

    // Signal chain
    inputGainParam     = apvts.getRawParameterValue (ParamIDs::inputGain);
    outputGainParam    = apvts.getRawParameterValue (ParamIDs::outputGain);
    mixParam           = apvts.getRawParameterValue (ParamIDs::mix);
    gateEnabledParam   = apvts.getRawParameterValue (ParamIDs::gateEnabled);
    gateThresholdParam = apvts.getRawParameterValue (ParamIDs::gateThreshold);

    // Quality
    oversamplingParam  = apvts.getRawParameterValue (ParamIDs::oversampling);
}

// =============================================================================
bool ClaymoreProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& mainInput  = layouts.getMainInputChannelSet();
    const auto& mainOutput = layouts.getMainOutputChannelSet();

    // Input and output must match (no upmix / downmix)
    if (mainOutput != mainInput)
        return false;

    // Accept mono and stereo only
    if (mainOutput != juce::AudioChannelSet::mono()
        && mainOutput != juce::AudioChannelSet::stereo())
        return false;

    return true;
}

// =============================================================================
void ClaymoreProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;
    spec.sampleRate       = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32> (samplesPerBlock);
    spec.numChannels      = static_cast<juce::uint32> (getTotalNumOutputChannels());

    // Prepare ClaymoreEngine (oversampling + fuzz DSP)
    engine.prepare (spec);

    // Apply the saved oversampling index before reporting latency (QUAL-01, QUAL-02)
    const int initialOsIndex = static_cast<int> (oversamplingParam->load (std::memory_order_relaxed));
    if (initialOsIndex != 0)
        engine.setOversamplingFactor (initialOsIndex);
    lastOversamplingIndex = initialOsIndex;

    // Prepare output limiter
    outputLimiter.prepare (spec);

    // Prepare dry/wet mixer — set wet latency to oversampling latency
    dryWetMixer.prepare (spec);
    dryWetMixer.setMixingRule (juce::dsp::DryWetMixingRule::linear);
    dryWetMixer.setWetLatency (engine.getLatencyInSamples());

    // Report oversampling latency to DAW for session-level compensation (Pitfall 2: std::round)
    setLatencySamples (static_cast<int> (std::round (engine.getLatencyInSamples())));

    // Prepare gain smoothers (5ms ramp at current sample rate)
    const float initialInputGainLinear  = juce::Decibels::decibelsToGain (
        inputGainParam->load  (std::memory_order_relaxed));
    const float initialOutputGainLinear = juce::Decibels::decibelsToGain (
        outputGainParam->load (std::memory_order_relaxed));

    inputGainSmoother.reset  (sampleRate, 0.005);
    inputGainSmoother.setCurrentAndTargetValue (initialInputGainLinear);

    outputGainSmoother.reset (sampleRate, 0.005);
    outputGainSmoother.setCurrentAndTargetValue (initialOutputGainLinear);

    // Signal that processBlock can run safely
    isInitialized.store (true, std::memory_order_release);
}

// =============================================================================
void ClaymoreProcessor::releaseResources()
{
    engine.reset();
    outputLimiter.reset();
    dryWetMixer.reset();
    isInitialized.store (false, std::memory_order_release);
}

// =============================================================================
void ClaymoreProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                       juce::MidiBuffer& /*midiMessages*/)
{
    juce::ScopedNoDenormals noDenormals;

    // --- isInitialized guard (Pitfall 4: processBlock before prepareToPlay) ---
    if (! isInitialized.load (std::memory_order_acquire))
    {
        buffer.clear();
        return;
    }

    const auto totalNumInputChannels  = getTotalNumInputChannels();
    const auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Clear extra output channels that have no corresponding input
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // --- Read all parameters (atomic loads, no string lookups) ---
    const float drive         = driveParam->load     (std::memory_order_relaxed);
    const float symmetry      = symmetryParam->load   (std::memory_order_relaxed);
    const float tightness     = tightnessParam->load  (std::memory_order_relaxed);
    const float sag           = sagParam->load        (std::memory_order_relaxed);
    const float tone          = toneParam->load       (std::memory_order_relaxed);
    const float presence      = presenceParam->load   (std::memory_order_relaxed);
    const float inputGainDB   = inputGainParam->load  (std::memory_order_relaxed);
    const float outputGainDB  = outputGainParam->load (std::memory_order_relaxed);
    const float mix           = mixParam->load        (std::memory_order_relaxed);
    const bool  gateOn        = gateEnabledParam->load (std::memory_order_relaxed) >= 0.5f;
    const float gateThreshDB  = gateThresholdParam->load (std::memory_order_relaxed);

    // --- Oversampling rate change detection (QUAL-01, QUAL-02) ---
    {
        const int newOversamplingIndex = static_cast<int> (oversamplingParam->load (std::memory_order_relaxed));
        if (newOversamplingIndex != lastOversamplingIndex)
        {
            lastOversamplingIndex = newOversamplingIndex;
            engine.setOversamplingFactor (newOversamplingIndex);

            const float newLatency = engine.getLatencyInSamples();
            dryWetMixer.setWetLatency (newLatency);
            setLatencySamples (static_cast<int> (std::round (newLatency)));
        }
    }

    // --- 1. Input Gain (pre-distortion level trim) ---
    {
        inputGainSmoother.setTargetValue (juce::Decibels::decibelsToGain (inputGainDB));
        const int numSamples   = buffer.getNumSamples();
        const int numChannels  = buffer.getNumChannels();

        for (int s = 0; s < numSamples; ++s)
        {
            const float gain = inputGainSmoother.getNextValue();
            for (int ch = 0; ch < numChannels; ++ch)
                buffer.setSample (ch, s, buffer.getSample (ch, s) * gain);
        }
    }

    // --- 2. Capture dry signal for latency-compensated mix ---
    {
        dryWetMixer.setWetMixProportion (mix);
        juce::dsp::AudioBlock<float> inputBlock (buffer);
        dryWetMixer.pushDrySamples (inputBlock);
    }

    // --- 3. ClaymoreEngine: noise gate → oversample → fuzz → tone ---
    engine.setDrive         (drive);
    engine.setSymmetry      (symmetry);
    engine.setTightness     (tightness);
    engine.setSag           (sag);
    engine.setTone          (tone);
    engine.setPresence      (presence);
    engine.setGateEnabled   (gateOn);
    engine.setGateThreshold (gateThreshDB);
    engine.process          (buffer);

    // --- 4. Blend wet and latency-compensated dry ---
    {
        juce::dsp::AudioBlock<float> wetBlock (buffer);
        dryWetMixer.mixWetSamples (wetBlock);
    }

    // --- 5. Output Gain (post-mix level trim) ---
    {
        outputGainSmoother.setTargetValue (juce::Decibels::decibelsToGain (outputGainDB));
        const int numSamples  = buffer.getNumSamples();
        const int numChannels = buffer.getNumChannels();

        for (int s = 0; s < numSamples; ++s)
        {
            const float gain = outputGainSmoother.getNextValue();
            for (int ch = 0; ch < numChannels; ++ch)
                buffer.setSample (ch, s, buffer.getSample (ch, s) * gain);
        }
    }

    // --- 6. Brickwall limiter (last in chain, SIG-04) ---
    outputLimiter.process (buffer);
}

// =============================================================================
// State persistence — APVTS copyState/replaceState (GunkLord pattern, verbatim)
// =============================================================================

void ClaymoreProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void ClaymoreProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));

    if (xmlState != nullptr)
        if (xmlState->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}

// =============================================================================
// Editor
// =============================================================================

juce::AudioProcessorEditor* ClaymoreProcessor::createEditor()
{
    return new ClaymoreEditor (*this);
}

// =============================================================================
// Plugin factory
// =============================================================================

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ClaymoreProcessor();
}
