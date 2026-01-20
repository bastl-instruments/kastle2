/*
MIT License

Copyright (c) 2026 Vaclav Mach (Bastl Instruments)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma once

#include <cstdint>
#include "common/dsp/math/math_utils.hpp"

namespace kastle2
{

// Base tuning frequency in Hz
static constexpr float kBaseTune = 32.71875f;

// Max pitch frequency in Hz (applied after all modulations and transpositions)
static constexpr float kMaxPitchHz = 15000.0f;

// Octave selection
static constexpr auto kMapFreePitch = MapDef<float, 7>{
    {pot(0.0f), pot(0.166f), pot(0.333f), pot(0.500f), pot(0.666f), pot(0.833f), pot(1.0f)},
    {0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f}};

// Fine tuning
static constexpr auto kMapPitchFine = MapDef<float, 3>{
    {pot(0.0f), pot(0.5f), pot(1.0f)},
    {0.943874313f, 1.f, 1.059463094f}};

// Filter frequency
static constexpr auto kMapFilterFreq = MapDef<float, 5>{
    {pot(0.0f), pot(0.25f), pot(0.5f), pot(0.75f), pot(1.0f)},
    {50.0f, 165.7f, 547.7f, 1814.2f, 6000.0f}};

// Filter resonance
static constexpr auto kMapResonance = MapDef<float, 3>{
    {pot(0.0f), pot(0.5f), pot(1.0f)},
    {0.1f, 0.6f, 0.95f}};

// FM index (depth)
static constexpr auto kMapFmIndex = MapDef<int32_t, 3>{
    {pot(0.0f), pot(0.5f), pot(1.0f)},
    {q31(0.01f), q31(0.5f), q31(0.8f)}};

// FM ratio (oscillator ratios)
static constexpr auto kMapFmRatio = MapDef<int32_t, 3>{
    {pot(0.0f), pot(0.5f), pot(1.0f)},
    {q31(0.2f), q31(0.5f), q31(0.75f)}};

// ENV pot to attack mapping
static constexpr auto kMapEnvAttack = MapDef<float, 5>{
    {pot(0.0f), pot(0.25f), pot(0.5f), pot(0.75f), pot(1.0f)},
    {1.0f, 0.2f, 0.005f, 0.005f, 0.005f}};

// ENV pot to decay mapping
static constexpr auto kMapEnvDecay = MapDef<float, 5>{
    {pot(0.0f), pot(0.25f), pot(0.5f), pot(0.75f), pot(1.0f)},
    {1.0f, 0.2f, 0.05f, 0.5f, 2.0f}};

// Default POT values
static constexpr int32_t kPitchScaleDefaultValue = pot(0.6f);
static constexpr int32_t kPitchRootDefaultValue = pot(0.0f);
static constexpr int32_t kPitchFineDefaultValue = pot(0.5f);
static constexpr int32_t kFxDefaultValue = pot(0.5f);
static constexpr int32_t kResonanceDefaultValue = pot(0.0f);
static constexpr int32_t kModeModDefaultValue = pot(1.0f);
static constexpr int32_t kEnvModDefaultValue = pot(1.0f);

// For the mode change you need to press it for less than 1.5s
static constexpr uint32_t kModeShortPressUnder = s2alr(1.5f);

// Delay
static constexpr Fraction kDelayRatio = {3, 2};
static constexpr auto kMapFxDelay = MapDef<int32_t, 6>{
    {pot(0.0f), pot(0.25f), pot(0.4f), pot(0.6f), pot(0.75f), pot(1.0f)},
    {q15(0.5f), q15(0.2f), q15(0.0f), q15(0.0f), q15(0.2f), q15(0.5f)}};

}