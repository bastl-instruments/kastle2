/*
MIT License

Copyright (c) 2024 Marek Mach (Bastl Instruments)

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

#include <array>
#include "common/config.hpp"
#include "common/core/Hardware.hpp"
#include "common/dsp/math/Fraction.hpp"
#include "common/dsp/math/math_utils.hpp"
#include "common/dsp/math/qmath.hpp"
#include "common/dsp/utility/Quantizer.hpp"
#include "common/peripherals/WS2812.hpp"

namespace kastle2
{

// namespace kastle2

// ---SETTINGS---
static constexpr uint32_t kTimeBetweenTriggers = 20000;          // 20ms (please leave at 20 ms, to prevent glitches when modulating eg. both Sample Mod and TRIG)
static constexpr int32_t kPotMoveDetectThreshold = pot(0.1f);    // 10%
static constexpr int32_t kCVChangeDetectThreshold = pot(0.1f);   // 10%
static constexpr uint32_t kUiIndicateChangeTime = s2alr(0.035f); // 35ms
static constexpr uint32_t kUiIndicateChangeColor = WS2812::NONE; // Dip to none color when indicating scale/root change
static constexpr uint32_t kModeShortPressUnder = s2alr(1.5f);    // For the bank change you need to press it for less than 1.5s

// ---INPUT SIDECHAIN / COMPRESSOR---
static constexpr float kSidechainAttack = 0.005f;  // Sidechain attack time in seconds (must be short, if it's slow it clips)
static constexpr float kSidechainRelease = 0.04f;  // Sidechain release time in seconds
static constexpr float kCompressorAttack = 0.005f; // Compressor attack time in seconds (must be set correctly, to not follow the signal too closely - causes ring mod)
static constexpr float kCompressorRelease = 0.4f;  // Compressor release time in seconds

// ---FX DELAY---
static constexpr auto kMapLowerVolumeWithInput = MapDef<int32_t, 3>{
    {pot(0.0f), pot(0.6f), pot(1.0f)},
    {q15(1.0f), q15(1.0f), q15(0.5f)}};

static constexpr auto kMapDelayMix = MapDef<int32_t, 7>{
    {pot(0.0f), pot(0.1f), pot(0.4f), pot(0.45f), pot(0.55f), pot(0.6f), pot(1.0f)},
    {q15(0.6f), q15(0.25f), q15(0.1f), q15(0.0f), q15(0.0f), q15(0.4f), q15(0.44f)}};

static constexpr auto kMapDelayFeedback = MapDef<int32_t, 6>{
    {pot(0.0f), pot(0.15f), pot(0.35f), pot(0.55f), pot(0.75f), pot(1.0f)},
    {q15(1.0f), q15(0.5f), q15(0.0f), q15(0.0f), q15(0.6f), q15(0.9f)}};

static constexpr Fraction kDelayRatio = {3, 2}; // tempo multiplier for delay length (fraction numerator/denominator)

static constexpr auto kMapDelayStereoLeft = MapDef<int32_t, 6>{
    {pot(0.0f), pot(0.05f), pot(0.1f), pot(0.2f), pot(0.35f), pot(1.0f)},
    {0, 0, -8, 15, 0, 0}};

static constexpr auto kMapDelayStereoRight = MapDef<int32_t, 6>{
    {pot(0.0f), pot(0.05f), pot(0.1f), pot(0.15f), pot(0.3f), pot(1.0f)},
    {0, 0, 13, -7, 0, 0}};

// ---FX CHORUS---
static constexpr auto kMapChorusFreq = MapDef<float, 5>{
    {pot(0.0f), pot(0.55f), pot(0.75f), pot(0.9f), pot(1.0f)},
    {0.13f, 2.0f, 0.4f, 0.2f, 0.1f}};

static constexpr auto kMapChorusBaseLength = MapDef<int32_t, 4>{
    {pot(0.0f), pot(0.55f), pot(0.75f), pot(1.0f)},
    {10, 20, 40, 60}};

static constexpr auto kMapChorusLfoDepthLeft = MapDef<float, 5>{
    {pot(0.0f), pot(0.55f), pot(0.6f), pot(0.9f), pot(1.0f)},
    {0.0f, 0.0f, 0.8f, 0.8f, 0.8f}};

static constexpr auto kMapChorusLfoDepthRight = MapDef<float, 5>{
    {pot(0.0f), pot(0.55f), pot(0.6f), pot(0.9f), pot(1.0f)},
    {0.0f, 0.0f, 0.8f, 0.8f, 0.8f}};

// ---FX DISTORTION---
static constexpr auto kMapDistortionAmount = MapDef<int32_t, 5>{
    {pot(0.0f), pot(0.55f), pot(0.85f), pot(0.95f), pot(1.0f)},
    // The clipper always drops output a little, we just pust it little bit higher with 0.003.
    {q15(0.003f), q15(0.003f), q15(0.04f), q15(0.25f), q15(0.6f)}};

static constexpr auto kMapInputDistortionAmount = MapDef<int32_t, 4>{
    {pot(0.0f), pot(0.55f), pot(0.85f), pot(1.0f)},
    {q15(0.0f), q15(0.0f), q15(0.04f), q15(0.3f)}};

static constexpr auto kMapFXVolumeCompensation = MapDef<int32_t, 5>{
    {pot(0.0f), pot(0.55f), pot(0.85f), pot(0.95f), pot(1.0f)},
    {q15(1.0f), q15(1.0f), q15(0.55f), q15(0.45f), q15(0.45f)}};

// Resonance compensation for the filter
// 0.5 is the ideal value
// 0.768 is compensation adjusted for resonance to not clip
// (calculated by measuring the output of the filter and comparing it to the input)
static constexpr auto kMapFilterVolumeCompensation = MapDef<int32_t, 5>{
    {pot(0.0f), pot(0.15f), pot(0.5f), pot(0.85f), pot(1.0f)},
    {q15(0.6f), q15(0.768), q15(0.55f), q15(0.768), q15(0.6f)}};

// ---ENVELOPE---
static constexpr auto kMapEnvelopeDecay = MapDef<float, 8>{
    {pot(0.0f), pot(0.48f), pot(0.5f), pot(0.75f), pot(0.8f), pot(0.9f), pot(0.95f), pot(1.0f)},
    {0.005f, 0.01f, 0.005f, 0.3f, 0.5f, 1.f, 2.f, 8.f}};

static constexpr auto kMapEnvelopeAttack = MapDef<float, 8>{
    {pot(0.0f), pot(0.05f), pot(0.12f), pot(0.25f), pot(0.37f), pot(0.5f), pot(0.52f), pot(1.0f)},
    {2.f, 2.f, 1.f, 0.5f, 0.1f, 0.f, 0.f, 0.f}};

static constexpr auto kMapEnvelopeHold = MapDef<float, 9>{
    {pot(0.0f), pot(0.05f), pot(0.48f), pot(0.5f), pot(0.75f), pot(0.8f), pot(0.9f), pot(0.95f), pot(1.0f)},
    {8.f, 0.f, 0.f, 0.000625f, 0.0375f, 0.0625f, 0.125f, 0.5f, 8.0f}};

static constexpr auto kMapEnvelopeCV = MapDef<int32_t, 9>{
    {-pot(1.0f), -pot(0.65f), -pot(0.5f), -pot(0.35f), pot(0.0f), pot(0.35f), pot(0.5f), pot(0.65f), pot(1.0f)},
    {-pot(1.0f), -pot(0.75f), -pot(0.5f), -pot(0.25f), pot(0.0f), pot(0.25f), pot(0.5f), pot(0.75f), pot(1.0f)}};

static constexpr auto kMapEnvelopeNoteDuration = MapDef<int32_t, 10>{
    {pot(0.0f), pot(0.12f), pot(0.25f), pot(0.42f), pot(0.5f), pot(0.5f), pot(0.58f), pot(0.75f), pot(0.88f), pot(1.0f)},
    {s2alr(8.f), s2alr(2.0f), s2alr(0.8f), s2alr(0.25f), s2alr(0.01f), s2alr(0.01f), s2alr(0.25f), s2alr(0.8f), s2alr(2.0f), s2alr(8.f)}};

// ---PITCH---
static constexpr auto kMapPitch = MapDef<float, 3>{
    {pot(0.0f), pot(0.5f), pot(1.0f)},
    {0.25f, 1.0f, 4.f}};

static constexpr auto kMapPitchOctaves = MapDef<float, 5>{
    {pot(0.1f), pot(0.3f), pot(0.5f), pot(0.7f), pot(0.9f)},
    {0.25f, 0.5f, 1.f, 2.f, 4.f}};

// pot 0.0 = means -1V/Oct, pot 0.5 means no modulation, 1.0 = means +1V/Oct
// by mapping the values 0.15 to 0.3 etc. we keep more range for smaller values
// by mapping 0.005 to 0.0 etc we make sure we safely reach the end values
static constexpr auto kMapPitchMod = MapDef<int32_t, 7>{
    {pot(0.0f), pot(0.005f), pot(0.15f), pot(0.5f), pot(0.85f), pot(0.995f), pot(1.0f)},
    {pot(0.0f), pot(0.0f), pot(0.3f), pot(0.5f), pot(0.7f), pot(1.0f), pot(1.0f)}};

// this must match the number of octaves defined by the map above (kMapPitchOctaves)
static constexpr size_t kNumberOfOctaves = 5;
static constexpr int32_t kPitchSemitoneChangeThreshold = pot(1.0f) / (kNumberOfOctaves * 12);

static constexpr auto kMapPitchFine = MapDef<float, 4>{
    {pot(0.0f), pot(0.45f), pot(0.55f), pot(1.0f)},
    {0.943874313f, 1.f, 1.f, 1.059463094f}};

static constexpr auto kMapPitchRoot = MapDef<int32_t, 12>{
    {pot(0.04f), pot(0.12f), pot(0.2f), pot(0.28f), pot(0.36f), pot(0.44f), pot(0.52f), pot(0.6f), pot(0.68f), pot(0.76f), pot(0.84f), pot(0.92f)},
    {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}};

static constexpr auto kMapPitchRootTranspose = MapDef<float, 12>{
    {pot(0.04f), pot(0.12f), pot(0.2f), pot(0.28f), pot(0.36f), pot(0.44f), pot(0.52f), pot(0.6f), pot(0.68f), pot(0.76f), pot(0.84f), pot(0.92f)},
    {1.0f, 1.05946f, 1.12246f, 1.18920f, 1.2599f, 1.33483f, 1.41421f, 1.49830f, 1.58740f, 1.68179f, 1.78179f, 1.88774f}};

static constexpr auto kPitchRootTransposeLut = std::array<float, 12>{1.0f, 1.05946f, 1.12246f, 1.18920f, 1.2599f, 1.33483f, 1.41421f, 1.49830f, 1.58740f, 1.68179f, 1.78179f, 1.88774f};

// --- SEQUENCER
static constexpr size_t kSequencerLength = 16; ///< See Sequencer.hpp for the max length
}
