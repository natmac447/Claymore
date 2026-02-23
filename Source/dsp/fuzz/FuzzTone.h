#pragma once

#include <juce_dsp/juce_dsp.h>

/**
 * Tone and presence filtering for the unified Rat-based fuzz stage.
 *
 * Rat tone: Single LP filter sweep — extended to 2 kHz–20 kHz (CONTEXT.md)
 * (GunkLord original was 800 Hz–8 kHz; Claymore extends the range)
 * Presence: High-shelf boost/cut at ~4 kHz, ±6 dB range
 * DC blocker: 20 Hz high-pass IIR after all tone processing
 *
 * SmoothedValue for tone and presence (5ms ramp) to prevent zipper noise.
 *
 * Based on GunkLord FuzzTone.h — one change: Tone LP range 2000–20000 Hz
 * (line marked with "CLAYMORE CHANGE" comment).
 */
class FuzzTone
{
public:
    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        sampleRate  = spec.sampleRate;
        numChannels = static_cast<int> (spec.numChannels);

        // Rat tone filter
        ratToneFilter.prepare (spec);
        ratToneFilter.setType (juce::dsp::FirstOrderTPTFilterType::lowpass);

        // Presence high-shelf filter
        presenceFilter.prepare (spec);
        updatePresenceCoefficients (0.5f);

        // DC blocker: 20 Hz high-pass
        dcBlocker.prepare (spec);
        auto dcCoeffs = juce::dsp::IIR::Coefficients<float>::makeFirstOrderHighPass (
            sampleRate, 20.0f);
        *dcBlocker.state = *dcCoeffs;

        // Parameter smoothing: 5ms ramp
        toneSmoother.reset (sampleRate, 0.005);
        toneSmoother.setCurrentAndTargetValue (0.5f);

        presenceSmoother.reset (sampleRate, 0.005);
        presenceSmoother.setCurrentAndTargetValue (0.5f);
    }

    void reset()
    {
        ratToneFilter.reset();
        presenceFilter.reset();
        dcBlocker.reset();

        toneSmoother.setCurrentAndTargetValue (0.5f);
        presenceSmoother.setCurrentAndTargetValue (0.5f);
    }

    void setTone    (float tone) { toneSmoother.setTargetValue (tone); }
    void setPresence (float pres) { presenceSmoother.setTargetValue (pres); }

    /**
     * Apply Rat tone filtering, presence high-shelf, and DC blocking
     * to the buffer at the original (non-oversampled) sample rate.
     */
    void applyTone (juce::AudioBuffer<float>& buffer)
    {
        const int numSamples = buffer.getNumSamples();
        const int chCount    = juce::jmin (numChannels, buffer.getNumChannels());

        // Apply Rat tone circuit
        for (int sample = 0; sample < numSamples; ++sample)
        {
            const float tone     = toneSmoother.getNextValue();
            const float presence = presenceSmoother.getNextValue();

            updatePresenceCoefficients (presence);

            // CLAYMORE CHANGE: Rat tone LP sweep 2000–20000 Hz
            // (GunkLord original: 800.0f + tone * 7200.0f  →  800–8000 Hz)
            const float cutoff = 2000.0f + tone * 18000.0f;
            ratToneFilter.setCutoffFrequency (cutoff);

            for (int ch = 0; ch < chCount; ++ch)
            {
                float x = buffer.getSample (ch, sample);
                x = ratToneFilter.processSample (ch, x);
                buffer.setSample (ch, sample, x);
            }
        }

        // Apply presence high-shelf (block processing)
        {
            juce::dsp::AudioBlock<float>              block (buffer);
            juce::dsp::ProcessContextReplacing<float> context (block);
            presenceFilter.process (context);
        }

        // DC blocker: 20 Hz high-pass
        {
            juce::dsp::AudioBlock<float>              block (buffer);
            juce::dsp::ProcessContextReplacing<float> context (block);
            dcBlocker.process (context);
        }
    }

private:
    /**
     * Update presence high-shelf coefficients.
     * Presence 0 = -6 dB cut. Presence 0.5 = flat. Presence 1.0 = +6 dB boost.
     */
    void updatePresenceCoefficients (float presence)
    {
        const float gainDB     = (presence - 0.5f) * 12.0f;
        const float gainLinear = juce::Decibels::decibelsToGain (gainDB);

        auto coeffs = juce::dsp::IIR::Coefficients<float>::makeHighShelf (
            sampleRate, 4000.0f, 0.707f, gainLinear);
        *presenceFilter.state = *coeffs;
    }

    // Rat tone filter
    juce::dsp::FirstOrderTPTFilter<float> ratToneFilter;

    // Presence high-shelf
    juce::dsp::ProcessorDuplicator<
        juce::dsp::IIR::Filter<float>,
        juce::dsp::IIR::Coefficients<float>> presenceFilter;

    // DC blocker: 20 Hz high-pass IIR
    juce::dsp::ProcessorDuplicator<
        juce::dsp::IIR::Filter<float>,
        juce::dsp::IIR::Coefficients<float>> dcBlocker;

    // Parameter smoothers (5ms ramp)
    juce::SmoothedValue<float> toneSmoother;
    juce::SmoothedValue<float> presenceSmoother;

    double sampleRate  = 44100.0;
    int    numChannels = 2;
};
