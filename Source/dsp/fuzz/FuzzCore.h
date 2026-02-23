#pragma once

#include <juce_dsp/juce_dsp.h>
#include "FuzzType.h"

/**
 * Unified Rat-based waveshaping with Symmetry, Tightness, and Sag controls.
 *
 * Single processSample function replaces five separate fuzz type functions.
 * - Tightness: HP filter before clipping (set externally by ClaymoreEngine)
 * - Slew filter: LM308 op-amp character (unchanged from original Rat)
 * - Symmetry: blends hard clip (Rat) to soft clip with envelope bias (Germanium)
 * - Sag: bias-starve sputter applied after clipping
 *
 * Ported verbatim from GunkLord — do not modify this logic.
 * Range adjustments (Tightness 20–800 Hz, Tone 2–20 kHz) happen in ClaymoreEngine.
 */

/**
 * Per-channel state for stateful waveshaping operations.
 * Owned by ClaymoreEngine, one instance per channel.
 */
struct FuzzCoreState
{
    // Rat slew filter: first-order TPT lowpass for LM308 character
    juce::dsp::FirstOrderTPTFilter<float> slewFilter;

    // Tightness filter: first-order TPT highpass before clipping
    juce::dsp::FirstOrderTPTFilter<float> tightnessFilter;

    // Envelope follower state (for symmetry control)
    float envelopeValue = 0.0f;

    void prepare (double sampleRate)
    {
        // Prepare slew filter for single-channel per-sample use
        juce::dsp::ProcessSpec monoSpec;
        monoSpec.sampleRate       = sampleRate;
        monoSpec.maximumBlockSize = 1;
        monoSpec.numChannels      = 1;

        slewFilter.prepare (monoSpec);
        slewFilter.setType (juce::dsp::FirstOrderTPTFilterType::lowpass);
        slewFilter.setCutoffFrequency (3000.0f);

        // Prepare tightness filter (highpass, cutoff set per-block by ClaymoreEngine)
        tightnessFilter.prepare (monoSpec);
        tightnessFilter.setType (juce::dsp::FirstOrderTPTFilterType::highpass);
        tightnessFilter.setCutoffFrequency (20.0f);

        envelopeValue = 0.0f;
    }

    void reset()
    {
        slewFilter.reset();
        tightnessFilter.reset();
        envelopeValue = 0.0f;
    }
};

namespace FuzzCore
{
    /**
     * Unified Rat-based waveshaping with symmetry and sag controls.
     *
     * @param x           Input sample
     * @param drive       Mapped drive gain (1-40x)
     * @param symmetry    0 = hard clip (Rat), 1 = soft clip with envelope bias (Germanium)
     * @param sag         0 = no sag, 1 = heavy sputter (dying battery)
     * @param state       Per-channel state (slew filter, tightness filter, envelope)
     */
    inline float processSample (float x, float drive, float symmetry, float sag,
                                FuzzCoreState& state)
    {
        // Tightness filter: HP before gain (cutoff set externally by ClaymoreEngine)
        x = state.tightnessFilter.processSample (0, x);

        // Apply drive gain
        const float gained = x * drive;

        // Slew rate limiting: LM308 character
        // Cutoff decreases with drive for more "thickness" at high gain
        const float slewCutoff = 3000.0f * (1.0f - (drive - 1.0f) / 78.0f);
        state.slewFilter.setCutoffFrequency (juce::jmax (1500.0f, slewCutoff));
        const float slewed = state.slewFilter.processSample (0, gained);

        // --- Clipping blend controlled by symmetry ---

        // Hard clip path (Rat): silicon diode +/-0.6V
        const float threshold   = 0.6f;
        const float hardClipped = juce::jlimit (-threshold, threshold, slewed) / threshold;

        // Soft clip path with envelope bias (Germanium-like)
        // Envelope follower for touch-sensitive bias offset
        const float absInput     = std::abs (x);
        const float attackCoeff  = 0.01f;
        const float releaseCoeff = 0.001f;

        if (absInput > state.envelopeValue)
            state.envelopeValue += attackCoeff * (absInput - state.envelopeValue);
        else
            state.envelopeValue += releaseCoeff * (absInput - state.envelopeValue);

        const float biasOffset  = state.envelopeValue * 0.8f * symmetry;
        const float biased      = slewed + biasOffset;
        const float softClipped = biased / (1.0f + std::abs (biased));

        // Blend between hard and soft clip based on symmetry
        float clipped = hardClipped * (1.0f - symmetry) + softClipped * symmetry;

        // --- Sag: bias-starve sputter ---
        if (sag > 0.0f)
        {
            const float sagThreshold = 0.3f * sag;
            const float absClipped   = std::abs (clipped);

            if (absClipped < sagThreshold)
            {
                // Below threshold: exponential decay for dying-battery character
                const float ratio = absClipped / juce::jmax (sagThreshold, 0.001f);
                clipped = clipped * ratio * ratio;
            }

            // Gain-neutral makeup: compensate for level reduction at high sag
            // Linear approximation — tuned empirically (CONTEXT.md allows discretion)
            const float sagMakeup = 1.0f + sag * 0.5f;
            clipped *= sagMakeup;
        }

        return clipped;
    }
}
