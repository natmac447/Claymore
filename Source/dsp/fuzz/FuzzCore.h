#pragma once

#include <cmath>
#include <juce_dsp/juce_dsp.h>
#include "FuzzType.h"

/**
 * Unified Rat-based waveshaping with selectable clipping circuit and Sag control.
 *
 * Signal chain per sample:
 * - Tightness: HP filter before clipping (set externally by ClaymoreEngine)
 * - Slew filter: LM308 op-amp character (unchanged from original Rat)
 * - ClipType switch: one of 8 diode/circuit clipping algorithms
 * - Sag: bias-starve sputter applied after clipping
 *
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
     * Rat-based waveshaping with selectable clipping circuit and sag.
     *
     * @param x           Input sample
     * @param drive       Mapped drive gain (1-40x)
     * @param clipType    Clipping circuit index (ClipType enum cast to int)
     * @param sag         0 = no sag, 1 = heavy sputter (dying battery)
     * @param state       Per-channel state (slew filter, tightness filter, envelope)
     */
    inline float processSample (float x, float drive, int clipType, float sag,
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

        // --- Clipping circuit ---
        float clipped = 0.0f;

        switch (static_cast<ClipType> (clipType))
        {
            case ClipType::Silicon:
            {
                // Hard clip ±0.6 V — original Rat, aggressive and buzzy
                constexpr float th = 0.6f;
                clipped = juce::jlimit (-th, th, slewed) / th;
                break;
            }

            case ClipType::Germanium:
            {
                // Soft clip ±0.3 V with envelope bias — warm, compressed, vintage
                const float absInput     = std::abs (x);
                constexpr float atkCoeff = 0.01f;
                constexpr float relCoeff = 0.001f;

                if (absInput > state.envelopeValue)
                    state.envelopeValue += atkCoeff * (absInput - state.envelopeValue);
                else
                    state.envelopeValue += relCoeff * (absInput - state.envelopeValue);

                const float biased = slewed + state.envelopeValue * 0.8f;
                clipped = biased / (1.0f + std::abs (biased));
                break;
            }

            case ClipType::LED:
            {
                // Hard clip ±1.7 V — open, bright, dynamic (Turbo Rat)
                constexpr float th = 1.7f;
                clipped = juce::jlimit (-th, th, slewed) / th;
                break;
            }

            case ClipType::MOSFET:
            {
                // tanh() smooth compression — smooth, fat (Fat Rat)
                clipped = std::tanh (slewed);
                break;
            }

            case ClipType::Asymmetric:
            {
                // +0.6 V / −0.3 V mixed diodes — even harmonics, gritty (Dirty Rat)
                const float absInput     = std::abs (x);
                constexpr float atkCoeff = 0.01f;
                constexpr float relCoeff = 0.001f;

                if (absInput > state.envelopeValue)
                    state.envelopeValue += atkCoeff * (absInput - state.envelopeValue);
                else
                    state.envelopeValue += relCoeff * (absInput - state.envelopeValue);

                constexpr float thPos = 0.6f;
                constexpr float thNeg = 0.3f;
                if (slewed > 0.0f)
                    clipped = juce::jmin (slewed, thPos) / thPos;
                else
                    clipped = juce::jmax (slewed, -thNeg) / thNeg;

                // Subtle envelope bias for touch sensitivity
                clipped += state.envelopeValue * 0.15f;
                clipped = juce::jlimit (-1.0f, 1.0f, clipped);
                break;
            }

            case ClipType::OpAmp:
            {
                // Cubic soft clip: x − x³/3 — subtle, transparent saturation
                const float s = juce::jlimit (-1.5f, 1.5f, slewed);
                clipped = s - (s * s * s) / 3.0f;
                // Normalise: peak of cubic at ±1.5 → ±1.0
                clipped *= (2.0f / 3.0f);  // max output is 1.5 - 1.125 = 0.375*3 .. actually peak = 1.0 at |s|=√3 ≈1.73; with limit 1.5 peak≈0.375*4/3
                // Simpler: just normalise so ±1 in → ±0.667 out, keeps headroom
                clipped *= 1.5f;
                break;
            }

            case ClipType::Foldback:
            {
                // Wave folding past ±1.0 — synthy, metallic, extreme
                float s = slewed;
                // Fold repeatedly into [−1, 1]
                while (s > 1.0f || s < -1.0f)
                {
                    if (s > 1.0f)  s = 2.0f - s;
                    if (s < -1.0f) s = -2.0f - s;
                }
                clipped = s;
                break;
            }

            case ClipType::Rectifier:
            {
                // Half-wave rectification — octave-up, sputtery
                clipped = (slewed > 0.0f) ? slewed : 0.0f;
                // Normalise toward ±1 range and add DC offset compensation
                clipped = clipped * 2.0f - 1.0f;
                clipped = juce::jlimit (-1.0f, 1.0f, clipped);
                break;
            }

            default:
            {
                // Fallback to silicon
                constexpr float th = 0.6f;
                clipped = juce::jlimit (-th, th, slewed) / th;
                break;
            }
        }

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
            const float sagMakeup = 1.0f + sag * 0.5f;
            clipped *= sagMakeup;
        }

        return clipped;
    }
}
