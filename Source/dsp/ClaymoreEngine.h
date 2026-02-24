#pragma once

#include <array>
#include <memory>
#include <juce_dsp/juce_dsp.h>
#include "fuzz/FuzzType.h"
#include "fuzz/FuzzCore.h"
#include "fuzz/FuzzTone.h"

/**
 * Main DSP signal chain for Claymore.
 *
 * Signal chain (Phase 2 — selectable 2x/4x/8x oversampling):
 *   NoiseGate → [Oversample Up] → FuzzCore (per-sample) → [Oversample Down] → FuzzTone
 *
 * Three Oversampling objects are pre-allocated in prepare() (one per rate).
 * setOversamplingFactor() switches between them with zero allocation in process().
 *
 * Based on GunkLord FuzzStage.h with Claymore-specific changes:
 * - Tightness HPF extended: 20–800 Hz (was 20–300 Hz, per CONTEXT.md)
 * - Tone LPF extended: 2–20 kHz (handled inside FuzzTone.h, per CONTEXT.md)
 * - Custom noise gate state machine with hysteresis (instead of JUCE NoiseGate alone)
 * - Sag gain-makeup for gain-neutrality (CONTEXT.md requirement)
 *
 * Hidden gate parameters (setGateAttack, setGateRelease, setGateSidechainHPF, setGateRange,
 * setGateHysteresis) are exposed here but not shown in the main UI — Phase 3 will
 * surface them via a right-click context menu.
 */
class ClaymoreEngine
{
public:
    ClaymoreEngine() = default;

    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        sampleRate   = spec.sampleRate;
        numChannels  = static_cast<int> (spec.numChannels);
        maxBlockSize = static_cast<int> (spec.maximumBlockSize);

        // Pre-allocate all three oversampling objects (2x, 4x, 8x) — zero allocation in process()
        for (int i = 0; i < numOversamplingFactors; ++i)
        {
            oversamplingObjects[i] = std::make_unique<juce::dsp::Oversampling<float>> (
                static_cast<size_t> (numChannels),
                static_cast<size_t> (i + 1),  // 1=2x, 2=4x, 3=8x
                juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
                true,   // isMaxQuality
                false   // useIntegerLatency (consistent with Phase 1)
            );
            oversamplingObjects[i]->initProcessing (static_cast<size_t> (spec.maximumBlockSize));
        }

        // Prepare per-channel fuzz core state at oversampled rate
        const double oversampledRate = sampleRate * std::pow (2.0, currentOversamplingIndex + 1);
        for (int ch = 0; ch < maxChannels; ++ch)
            coreState[ch].prepare (oversampledRate);

        // Prepare tone filtering at original sample rate
        tone.prepare (spec);

        // Sidechain HPF: per-channel, highpass at 150 Hz default
        // Applied to the level-detection path ONLY — not to the audio path
        {
            juce::dsp::ProcessSpec monoSpec;
            monoSpec.sampleRate       = sampleRate;
            monoSpec.maximumBlockSize = spec.maximumBlockSize;
            monoSpec.numChannels      = 1;

            for (int ch = 0; ch < maxChannels; ++ch)
            {
                sidechainHPF[ch].prepare (monoSpec);
                sidechainHPF[ch].setType (juce::dsp::FirstOrderTPTFilterType::highpass);
                sidechainHPF[ch].setCutoffFrequency (gateSidechainHPFHz);
            }
        }

        // Noise gate — custom state machine (hysteresis, see pitfalls notes)
        // The JUCE NoiseGate has no hysteresis; we implement dual-threshold manually.
        recalculateGateCoefficients();
        gateEnvelope = 0.0f;
        gateGainSmoother.reset (static_cast<float> (sampleRate), 0.001);  // 1ms smoother
        gateGainSmoother.setCurrentAndTargetValue (1.0f);
        gateIsOpen = false;

        // Drive smoothing: 10ms ramp (at oversampled rate)
        driveSmoother.reset (oversampledRate, 0.010);
        driveSmoother.setCurrentAndTargetValue (targetDrive);

        // Other parameter smoothers: 5ms ramp (at oversampled rate)
        tightnessSmoother.reset (oversampledRate, 0.005);
        tightnessSmoother.setCurrentAndTargetValue (targetTightness);

        sagSmoother.reset (oversampledRate, 0.005);
        sagSmoother.setCurrentAndTargetValue (targetSag);
    }

    void process (juce::AudioBuffer<float>& buffer)
    {
        const int chCount = juce::jmin (numChannels, buffer.getNumChannels());

        // 1. Optional noise gate (pre-distortion, CONTEXT.md locked)
        if (gateEnabled)
            applyNoiseGate (buffer, chCount);

        // 2. Upsample
        juce::dsp::AudioBlock<float> block (buffer);
        auto& os = *oversamplingObjects[currentOversamplingIndex];
        auto oversampledBlock = os.processSamplesUp (block);

        // 3. Per-channel per-sample waveshaping in oversampled domain
        const int numSamples = static_cast<int> (oversampledBlock.getNumSamples());

        // Set smoother targets
        driveSmoother.setTargetValue (targetDrive);
        tightnessSmoother.setTargetValue (targetTightness);
        sagSmoother.setTargetValue (targetSag);

        for (int ch = 0; ch < chCount; ++ch)
        {
            auto* data = oversampledBlock.getChannelPointer (static_cast<size_t> (ch));

            for (int s = 0; s < numSamples; ++s)
            {
                float drive, tightness, sag;

                // Only advance smoothers on ch 0; read current value on remaining channels
                // (same pattern as GunkLord FuzzStage)
                if (ch == 0)
                {
                    drive     = driveSmoother.getNextValue();
                    tightness = tightnessSmoother.getNextValue();
                    sag       = sagSmoother.getNextValue();
                }
                else
                {
                    drive     = driveSmoother.getCurrentValue();
                    tightness = tightnessSmoother.getCurrentValue();
                    sag       = sagSmoother.getCurrentValue();
                }

                // Tightness filter cutoff: 0 = 20 Hz (full bass), 1 = 800 Hz (tight)
                // CLAYMORE CHANGE: extended from GunkLord's 20–300 Hz to 20–800 Hz
                const float tightCutoff = 20.0f + tightness * 780.0f;
                coreState[ch].tightnessFilter.setCutoffFrequency (tightCutoff);

                const float mappedDrive = FuzzConfig::mapDrive (drive);
                data[s] = FuzzCore::processSample (data[s], mappedDrive, targetClipType, sag,
                                                    coreState[ch]);
                data[s] *= FuzzConfig::outputCompensation;
            }
        }

        // 4. Downsample
        os.processSamplesDown (block);

        // 5. Apply tone filtering + presence + DC blocker (at original rate)
        tone.applyTone (buffer);
    }

    void reset()
    {
        for (int i = 0; i < numOversamplingFactors; ++i)
            if (oversamplingObjects[i])
                oversamplingObjects[i]->reset();

        for (int ch = 0; ch < maxChannels; ++ch)
        {
            coreState[ch].reset();
            sidechainHPF[ch].reset();
        }

        tone.reset();

        driveSmoother.setCurrentAndTargetValue (targetDrive);
        tightnessSmoother.setCurrentAndTargetValue (targetTightness);
        sagSmoother.setCurrentAndTargetValue (targetSag);

        gateEnvelope = 0.0f;
        gateIsOpen   = false;
        gateGainSmoother.setCurrentAndTargetValue (gateEnabled ? 1.0f : 1.0f);
    }

    float getLatencyInSamples() const
    {
        return oversamplingObjects[currentOversamplingIndex]->getLatencyInSamples();
    }

    // --- Parameter setters (called per-block by PluginProcessor) ---

    void setDrive     (float drive)    { targetDrive     = juce::jlimit (0.0f, 1.0f, drive); }
    void setClipType  (int type)       { targetClipType  = juce::jlimit (0, static_cast<int> (ClipType::Rectifier), type); }
    void setTightness (float tight)    { targetTightness = juce::jlimit (0.0f, 1.0f, tight); }
    void setSag       (float sagVal)   { targetSag       = juce::jlimit (0.0f, 1.0f, sagVal); }
    void setTone      (float toneVal)  { tone.setTone     (juce::jlimit (0.0f, 1.0f, toneVal)); }
    void setPresence  (float presence) { tone.setPresence (juce::jlimit (0.0f, 1.0f, presence)); }

    /**
     * Switch to a different oversampling rate.
     * index: 0 = 2x, 1 = 4x, 2 = 8x
     *
     * Called from PluginProcessor::processBlock() on rate change (QUAL-01).
     * Safe to call from the audio thread — no allocation, no locks.
     * Resets stale filter state, re-prepares smoothers and FuzzCoreState at new rate.
     */
    void setOversamplingFactor (int index)
    {
        const int newIndex = juce::jlimit (0, numOversamplingFactors - 1, index);
        if (newIndex == currentOversamplingIndex)
            return;

        // Reset the OLD oversampling object's filter state (prevents stale state artifacts)
        oversamplingObjects[currentOversamplingIndex]->reset();

        currentOversamplingIndex = newIndex;

        // Recalculate oversampled rate for smoothers and FuzzCoreState
        const double newOsRate = sampleRate * std::pow (2.0, currentOversamplingIndex + 1);

        driveSmoother.reset (newOsRate, 0.010);
        tightnessSmoother.reset (newOsRate, 0.005);
        sagSmoother.reset (newOsRate, 0.005);

        // Re-prepare FuzzCoreState at new oversampled rate
        for (int ch = 0; ch < maxChannels; ++ch)
            coreState[ch].prepare (newOsRate);
    }

    void setGateEnabled (bool enabled)
    {
        gateEnabled = enabled;
        if (! enabled)
        {
            // Reset gate state when disabled — avoids audible artifact on re-enable
            gateIsOpen   = false;
            gateEnvelope = 0.0f;
            gateGainSmoother.setCurrentAndTargetValue (1.0f);
        }
    }

    void setGateThreshold (float thresholdDB)
    {
        // thresholdDB is the open threshold; close threshold is hysteresis dB lower
        gateOpenThreshold  = juce::jlimit (-60.0f, -10.0f, thresholdDB);
        gateCloseThreshold = gateOpenThreshold - gateHysteresisDB;
    }

    // --- Hidden gate parameters (Phase 3 exposes via right-click context menu) ---

    /** Attack time in milliseconds. Faster = more transient click suppression. Default: 1ms. */
    void setGateAttack (float attackMs)
    {
        gateAttackMs = juce::jlimit (0.1f, 100.0f, attackMs);
        if (sampleRate > 0.0)
            recalculateGateCoefficients();
    }

    /** Release time in milliseconds. Longer = more natural tail. Default: 80ms. */
    void setGateRelease (float releaseMs)
    {
        gateReleaseMs = juce::jlimit (1.0f, 2000.0f, releaseMs);
        if (sampleRate > 0.0)
            recalculateGateCoefficients();
    }

    /**
     * Gate ratio: 1.0 = full gating (only rangeDB remains), 0.0 = no gating.
     * Scales the effective range: actualRange = rangeDB * ratio.
     * Default: 1.0 (full range attenuation).
     */
    void setGateRatio (float ratio)
    {
        gateRatio = juce::jlimit (0.0f, 1.0f, ratio);
    }

    /**
     * Sidechain HPF cutoff frequency in Hz.
     * Filters out low-frequency content from the level-detection path,
     * preventing bass from triggering the gate. Default: 150 Hz.
     */
    void setGateSidechainHPF (float hz)
    {
        gateSidechainHPFHz = juce::jlimit (20.0f, 2000.0f, hz);
        for (int ch = 0; ch < maxChannels; ++ch)
            sidechainHPF[ch].setCutoffFrequency (gateSidechainHPFHz);
    }

    /**
     * Gate range: maximum attenuation in dB when gate is closed.
     * Negative value (e.g., -60 dB). Default: -60 dB.
     */
    void setGateRange (float rangeDB)
    {
        gateRangeDB = juce::jlimit (-120.0f, -6.0f, rangeDB);
    }

    /**
     * Hysteresis: difference between open and close thresholds in dB.
     * Larger = less chatter on borderline signals. Default: 4 dB.
     */
    void setGateHysteresis (float hysteresisDB)
    {
        gateHysteresisDB = juce::jlimit (0.0f, 12.0f, hysteresisDB);
        // Recalculate close threshold based on new hysteresis
        gateCloseThreshold = gateOpenThreshold - gateHysteresisDB;
    }

    // --- Gate parameter getters (for advanced menu display) ---
    float getGateAttack()       const { return gateAttackMs; }
    float getGateRelease()      const { return gateReleaseMs; }
    float getGateHysteresis()   const { return gateHysteresisDB; }
    float getGateRange()        const { return gateRangeDB; }
    float getGateSidechainHPF() const { return gateSidechainHPFHz; }

private:
    // --- Noise gate with hysteresis ---
    // Custom state machine: envelope follower + dual-threshold logic.
    // JUCE's NoiseGate does not expose hysteresis; this avoids chatter on borderline signals.
    void applyNoiseGate (juce::AudioBuffer<float>& buffer, int chCount)
    {
        const int   numSamples         = buffer.getNumSamples();
        const float openThreshLinear   = juce::Decibels::decibelsToGain (gateOpenThreshold);
        const float closeThreshLinear  = juce::Decibels::decibelsToGain (gateCloseThreshold);
        // Range scales with ratio: 0 = no attenuation, 1 = full gateRangeDB attenuation
        const float effectiveRangeDB   = gateRangeDB * gateRatio;
        const float rangeGain          = juce::Decibels::decibelsToGain (effectiveRangeDB);

        for (int s = 0; s < numSamples; ++s)
        {
            // Measure peak level across all channels at this sample,
            // after applying sidechain HPF (level detection only — audio path unchanged)
            float peakLevel = 0.0f;
            for (int ch = 0; ch < chCount; ++ch)
            {
                const float rawSample    = buffer.getSample (ch, s);
                const float filteredSamp = sidechainHPF[ch].processSample (0, rawSample);
                peakLevel = juce::jmax (peakLevel, std::abs (filteredSamp));
            }

            // Envelope follower (first-order IIR)
            if (peakLevel > gateEnvelope)
                gateEnvelope = gateAttackCoeff * gateEnvelope + (1.0f - gateAttackCoeff) * peakLevel;
            else
                gateEnvelope = gateReleaseCoeff * gateEnvelope + (1.0f - gateReleaseCoeff) * peakLevel;

            // Dual-threshold hysteresis state machine
            if (! gateIsOpen && gateEnvelope >= openThreshLinear)
                gateIsOpen = true;
            else if (gateIsOpen && gateEnvelope < closeThreshLinear)
                gateIsOpen = false;

            // Set gain target: open = 1.0 (pass through), closed = rangeGain (attenuation)
            gateGainSmoother.setTargetValue (gateIsOpen ? 1.0f : rangeGain);

            const float gain = gateGainSmoother.getNextValue();
            for (int ch = 0; ch < chCount; ++ch)
                buffer.setSample (ch, s, buffer.getSample (ch, s) * gain);
        }
    }

    void recalculateGateCoefficients()
    {
        const float sr = static_cast<float> (sampleRate);
        gateAttackCoeff  = std::exp (-1.0f / (sr * (gateAttackMs  / 1000.0f)));
        gateReleaseCoeff = std::exp (-1.0f / (sr * (gateReleaseMs / 1000.0f)));
    }

    // -------------------------------------------------------------------------
    static constexpr int maxChannels = 8;

    // Pre-allocated oversampling objects: index 0 = 2x, 1 = 4x, 2 = 8x
    // All three are created in prepare(); switching is zero-allocation
    static constexpr int numOversamplingFactors = 3;
    std::array<std::unique_ptr<juce::dsp::Oversampling<float>>, numOversamplingFactors> oversamplingObjects;
    int currentOversamplingIndex = 0;  // 0 = 2x (default), 1 = 4x, 2 = 8x

    // Per-channel waveshaping state
    FuzzCoreState coreState[maxChannels];

    // Tone and presence filtering
    FuzzTone tone;

    // --- Noise gate state ---
    bool  gateEnabled = false;
    bool  gateIsOpen  = false;
    float gateEnvelope = 0.0f;
    float gateAttackCoeff  = 0.0f;
    float gateReleaseCoeff = 0.0f;

    // Gain smoother for attack/release ramping (avoids click on state change)
    juce::SmoothedValue<float> gateGainSmoother;

    // Gate thresholds (user-visible open; close = open - hysteresis)
    float gateOpenThreshold  = -40.0f;  // dBFS — default from APVTS
    float gateCloseThreshold = -44.0f;  // dBFS — 4 dB hysteresis

    // Hidden gate parameters (power-user; exposed via context menu in Phase 3)
    float gateAttackMs      = 1.0f;    // 1ms attack (fast, avoids click)
    float gateReleaseMs     = 80.0f;   // 80ms release (natural tail)
    float gateHysteresisDB  = 4.0f;    // 4 dB open-close difference
    float gateRangeDB       = -60.0f;  // -60 dB attenuation when closed
    float gateRatio         = 1.0f;    // 1.0 = full range applied
    float gateSidechainHPFHz = 150.0f; // Sidechain HPF cutoff: 150 Hz default

    // Sidechain HPF filters — per channel, highpass at gateSidechainHPFHz
    // Applied to the level-detection signal path only (NOT to the audio output)
    juce::dsp::FirstOrderTPTFilter<float> sidechainHPF[maxChannels];

    // --- DSP parameters ---
    float targetDrive     = 0.5f;
    int   targetClipType  = 0;     // ClipType::Silicon
    float targetTightness = 0.0f;
    float targetSag       = 0.0f;

    // Parameter smoothers (clipType is discrete — no smoother needed)
    juce::SmoothedValue<float> driveSmoother;
    juce::SmoothedValue<float> tightnessSmoother;
    juce::SmoothedValue<float> sagSmoother;

    // Spec
    double sampleRate  = 44100.0;
    int    numChannels = 2;
    int    maxBlockSize = 512;
};
