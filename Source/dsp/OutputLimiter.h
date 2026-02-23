#pragma once

#include <juce_dsp/juce_dsp.h>

/**
 * Brickwall output limiter — thin wrapper around juce::dsp::Limiter<float>.
 *
 * Placed last in the signal chain (after dry/wet mix and output gain).
 * Default threshold: 0 dBFS (prevents digital overs).
 * Uses lookahead + knee for transparent limiting behavior.
 *
 * SIG-04: Output is limited by brickwall limiter to prevent digital overs.
 */
class OutputLimiter
{
public:
    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        limiter.prepare (spec);
        limiter.setThreshold (0.0f);   // 0 dBFS brickwall
        limiter.setRelease   (50.0f);  // 50ms release — transparent for mixing use
    }

    void process (juce::AudioBuffer<float>& buffer)
    {
        juce::dsp::AudioBlock<float>              block (buffer);
        juce::dsp::ProcessContextReplacing<float> context (block);
        limiter.process (context);
    }

    void reset()
    {
        limiter.reset();
    }

private:
    juce::dsp::Limiter<float> limiter;
};
