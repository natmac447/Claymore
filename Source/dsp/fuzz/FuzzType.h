#pragma once

#include <juce_core/juce_core.h>

/**
 * Fuzz configuration constants for the unified Rat-based algorithm.
 *
 * Single fuzz type with three tonal controls (Symmetry, Tightness, Sag)
 * instead of five separate fuzz types with volume mismatch issues.
 *
 * Ported verbatim from GunkLord — no changes required.
 */

namespace FuzzConfig
{
    // Drive range: min gain at drive=0, max gain at drive=1
    inline constexpr float driveMin = 1.0f;
    inline constexpr float driveMax = 40.0f;

    // Output compensation gain
    inline constexpr float outputCompensation = 0.30f;

    // Map normalized drive (0-1) to gain value
    inline float mapDrive (float normalizedDrive)
    {
        return driveMin + normalizedDrive * (driveMax - driveMin);
    }
}
