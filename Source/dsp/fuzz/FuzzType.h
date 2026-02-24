#pragma once

#include <juce_core/juce_core.h>

/**
 * Fuzz configuration constants and clipping circuit types.
 *
 * ClipType selects the diode clipping behaviour applied after the LM308
 * slew-rate stage.  Each variant models a different hardware topology
 * found in classic Rat-family pedals (and a few wilder options).
 */

enum class ClipType : int
{
    Silicon = 0,   // Hard clip ±0.6 V  (original Rat)
    Germanium,     // Soft clip ±0.3 V + envelope bias (vintage)
    LED,           // Hard clip ±1.7 V  (Turbo Rat)
    MOSFET,        // tanh() smooth compression (Fat Rat)
    Asymmetric,    // +0.6 V / −0.3 V mixed diodes (Dirty Rat)
    OpAmp,         // Cubic soft clip  x − x³/3 (transparent)
    Foldback,      // Wave folding past ±1.0 (synthy, metallic)
    Rectifier      // Half-wave rectification (octave-up, sputtery)
};

inline const juce::StringArray clipTypeNames
{
    "Silicon", "Germanium", "LED", "MOSFET",
    "Asymmetric", "Op-amp", "Foldback", "Rectifier"
};

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
