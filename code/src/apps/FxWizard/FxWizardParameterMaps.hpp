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
#include "common/utils.hpp"
#include "common/core/Hardware.hpp"
#include "common/dsp/math/Fraction.hpp"
#include "common/dsp/math/math_utils.hpp"
#include "common/dsp/math/qmath.hpp"

namespace kastle2
{

// --- GLOBAL PARAMETERS ---
static constexpr float kFeedbackFilterLeftFreq = 50.f;
static constexpr float kFeedbackFilterRightFreq = 50.f;
static constexpr float kFeedbackFilterLpLeftFreq = 15000.f;
static constexpr float kFeedbackFilterLpRightFreq = 15000.f;


/*
    All maps are defined using MapDef from math_utils.
    MapDef<type, size> contains two arrays: input and output.
*/

// --- GLOBAL ---
static constexpr auto kMapDjFilter = MapDef<int32_t, 4>{
    {pot(0.0f), pot(0.49f), pot(0.51f), pot(1.0f)},
    {q15(-0.8f), q15(0.0f), q15(0.0f), q15(0.9f)}};

// --- CRUSHER ---
static constexpr auto kMapCrusherThreshold = MapDef<int32_t, 3>{
    {pot(0.0f), pot(0.7f), pot(1.0f)},
    {q15(-1.0f), q15(0.9f), q15(0.9f)}};

static constexpr auto kMapCrusherFrequency = MapDef<int32_t, 5>{
    {pot(0.0f), pot(0.2f), pot(0.5f), pot(0.9f), pot(1.0f)},
    {fq31(15.f), fq31(100.f), fq31(1000.f), fq31(15000.f), fq31(18000.f)}};

static constexpr auto kMapCrusherThresholdCompensation = MapDef<int32_t, 3>{
    {0, 3000, 20000},
    {q15(0.9f), q15(0.9f), q15(0.0f)}};

static constexpr auto kMapCrusherStereoMix = MapDef<int32_t, 3>{
    {pot(0.0f), pot(0.2f), pot(1.0f)},
    {q15(1.0f), q15(0.0f), q15(0.0f)}};

static constexpr auto kMapCrusherStereoFrequencyLeft = MapDef<int32_t, 3>{
    {pot(0.0f), pot(0.1f), pot(1.0f)},
    {q31(0.5f), q31(0.375f), q31(0.166f)}};

static constexpr auto kMapCrusherStereoFrequencyRight = MapDef<int32_t, 3>{
    {pot(0.0f), pot(0.1f), pot(1.0f)},
    {q31(0.5f), q31(0.4f), q31(0.75f)}};

static constexpr auto kMapCrusherEnvMod = MapDef<int32_t, 3>{
    {pot(0.0f), pot(0.5f), pot(1.0f)},
    {10, 10, 10}};

static constexpr float kCrusherEnvAttack = 0.1f;

static constexpr auto kMapCrusherEnvDecay = MapDef<float, 3>{
    {pot(0.0f), pot(0.5f), pot(1.0f)},
    {0.4f, 0.4f, 0.4f}};

static constexpr auto kMapCrusherGlobalDryWet = MapDef<int32_t, 4>{
    {pot(0.0f), pot(0.15f), pot(0.6f), pot(1.0f)},
    {q15(0.0f), q15(1.0f), q15(1.0f), q15(1.0f)}};

static constexpr auto kMapCrusherXor = MapDef<int32_t, 5>{
    {pot(0.0f), pot(0.7f), pot(0.77f), pot(0.9f), pot(1.0f)},
    {0, 0, 1000, 2000, 4000}};

static constexpr auto kMapCrusherGlobalFeedbackDelay = MapDef<int32_t, 3>{
    {pot(0.0f), pot(0.55f), pot(1.0f)},
    {300, 636, 1000}};

static constexpr auto kMapCrusherGlobalFeedbackVolume = MapDef<int32_t, 4>{
    {pot(0.0f), pot(0.15f), pot(0.5f), pot(1.0f)},
    {q15(0.0f), q15(0.025f), q15(0.05f), q15(0.07f)}};

// --- FLANGER ---
static constexpr auto kMapFlangerDepth = MapDef<int32_t, 3>{
    {pot(0.0f), pot(0.2f), pot(1.0f)},
    {q15(0.0f), q15(0.5f), q15(1.0f)}};

static constexpr auto kMapFlangerDryWet = MapDef<int32_t, 2>{
    {pot(0.0f), pot(1.0f)},
    {q15(0.0f), q15(1.0f)}};

static constexpr auto kMapFlangerFrequency = MapDef<int32_t, 5>{
    {pot(0.0f), pot(0.3f), pot(0.6f), pot(0.75f), pot(1.0f)},
    {fq31(0.02f), fq31(0.1f), fq31(1.f), fq31(5.f), fq31(50.f)}};

static constexpr auto kMapFlangerStereoMix = MapDef<int32_t, 3>{
    {pot(0.0f), pot(0.2f), pot(1.0f)},
    {q15(1.0f), q15(0.0f), q15(0.0f)}};

static constexpr auto kMapFlangerStereoFrequency = MapDef<int32_t, 3>{
    {pot(0.0f), pot(0.1f), pot(1.0f)},
    {fq31(0.f), fq31(0.5f), fq31(0.2f)}};

static constexpr auto kMapFlangerGlobalDryWet = MapDef<int32_t, 3>{
    {pot(0.0f), pot(0.15f), pot(1.0f)},
    {q15(0.0f), q15(1.0f), q15(1.0f)}};

static constexpr auto kMapFlangerGlobalFeedbackDelay = MapDef<int32_t, 3>{
    {pot(0.0f), pot(0.55f), pot(1.0f)},
    {200, 400, 700}};

static constexpr auto kMapFlangerGlobalFeedbackVolume = MapDef<int32_t, 4>{
    {pot(0.0f), pot(0.23f), pot(0.5f), pot(1.0f)},
    {q15(0.0f), q15(0.04f), q15(0.05f), q15(0.08f)}};

// --- PANNER ---
static constexpr auto kMapPannerDepth = MapDef<int32_t, 4>{
    {pot(0.0f), pot(0.2f), pot(0.7f), pot(1.0f)},
    {Q15_ZERO, q15(0.3f), Q15_MAX, Q15_MAX}};

static constexpr auto kMapPannerDistortion = MapDef<int32_t, 3>{
    {pot(0.0f), pot(0.7f), pot(1.0f)},
    {Q15_ZERO, Q15_ZERO, q15(0.08f)}};

static constexpr auto kMapPannerFrequency = MapDef<int32_t, 7>{
    {pot(0.0f), pot(0.25f), pot(0.5f), pot(0.7f), pot(0.85f), pot(0.9f), pot(1.0f)},
    {fq31(0.1f), fq31(0.5f), fq31(2.f), fq31(15.f), fq31(100.f), fq31(1000.f), fq31(10000.f)}};

static constexpr auto kMapPannerStereoMix = MapDef<int32_t, 3>{
    {pot(0.0f), pot(0.2f), pot(1.0f)},
    {q15(1.0f), q15(0.0f), q15(0.0f)}};

static constexpr auto kMapPannerStereoTimeAdjust = MapDef<int32_t, 4>{
    {pot(0.0f), pot(0.5f), pot(0.75f), pot(1.0f)},
    {0, 8, 16, 48}};

static constexpr auto kMapPannerStereoLeft = MapDef<int32_t, 3>{
    {pot(0.0f), pot(0.1f), pot(1.0f)},
    {fq31(0.f), fq31(0.2f), fq31(4.f)}};

static constexpr auto kMapPannerStereoRight = MapDef<int32_t, 3>{
    {pot(0.0f), pot(0.1f), pot(1.0f)},
    {fq31(0.f), fq31(0.1f), fq31(1.f)}};

static constexpr auto kMapPannerGlobalDryWet = MapDef<int32_t, 3>{
    {pot(0.0f), pot(0.15f), pot(1.0f)},
    {q15(0.0f), q15(1.0f), q15(1.0f)}};

static constexpr auto kMapPannerGlobalFeedbackDelay = MapDef<int32_t, 3>{
    {pot(0.0f), pot(0.6f), pot(1.0f)},
    {100, 636, 2000}};

static constexpr auto kMapPannerGlobalFeedbackVolume = MapDef<int32_t, 4>{
    {pot(0.0f), pot(0.15f), pot(0.5f), pot(1.0f)},
    {q15(0.0f), q15(0.04f), q15(0.06f), q15(0.15f)}};

// --- FREEZER ---
static constexpr int32_t kMapFreezerSampleThreshold = pot(0.1f);

static constexpr auto kMapFreezerPitchTone = MapDef<int32_t, 2>{
    {pot(0.5f), pot(1.0f)},
    {880, 150}};

static constexpr auto kMapFreezerPitchRatio = MapDef<int32_t, 8>{
    {pot(0.04f), pot(0.1f), pot(0.16f), pot(0.22f), pot(0.28f), pot(0.34f), pot(0.4f), pot(0.46f)},
    {1, 2, 3, 4, 6, 8, 12, 16}};

static constexpr auto kMapFreezerStereo = MapDef<int32_t, 2>{
    {pot(0.0f), pot(1.0f)},
    {0, 2000}};

static constexpr auto kMapFreezerFeedback = MapDef<int32_t, 3>{
    {pot(0.0f), pot(0.75f), pot(1.0f)},
    {q15(0.0f), q15(0.3f), q15(0.8f)}};

static constexpr auto kMapFreezerFeedbackDry = MapDef<int32_t, 3>{
    {pot(0.0f), pot(0.75f), pot(1.0f)},
    {q15(1.0f), q15(1.0f), q15(0.9f)}};

static constexpr auto kMapFreezerGlobalDryWet = MapDef<int32_t, 2>{
    {pot(0.0f), pot(1.0f)},
    {q15(0.0f), q15(1.0f)}};

// --- REPLAYER ---
static constexpr auto kMapReplayerDryWet = MapDef<int32_t, 3>{
    {pot(0.0f), pot(0.9f), pot(1.0f)},
    {q15(0.0f), q15(1.0f), q15(1.0f)}};

static constexpr auto kMapReplayerOversampling = MapDef<int32_t, 6>{
    {pot(0.0f), pot(0.2f), pot(0.49f), pot(0.51f), pot(0.8f), pot(1.0f)},
    {0x000F0000, 0x00010000, 0x00003FFF, 0x00003FFF, 0x00010000, 0x000B0000}};

static constexpr int32_t kReplayerReverseThresholdLower = pot(0.49f);
static constexpr int32_t kReplayerReverseThresholdUpper = pot(0.51f);
static constexpr int32_t kReplayerHoldThreshold = pot(0.9f);

static constexpr auto kMapReplayerStereoOversampling = MapDef<int32_t, 2>{
    {pot(0.0f), pot(1.0f)},
    {0, 0x00001000}};

static constexpr auto kMapReplayerGlobalDryWet = MapDef<int32_t, 3>{
    {pot(0.0f), pot(0.15f), pot(1.0f)},
    {q15(0.0f), q15(0.5f), q15(1.0f)}};

static constexpr auto kMapReplayerGlobalFeedbackDelayMod = MapDef<int32_t, 5>{
    {pot(0.0f), pot(0.2f), pot(0.5f), pot(0.8f), pot(1.0f)},
    {10, 1, 1, 1, 10}};

static constexpr auto kMapReplayerGlobalFeedbackDelay = MapDef<int32_t, 3>{
    {pot(0.0f), pot(0.55f), pot(1.0f)},
    {638, 636, 2000}};

static constexpr auto kMapReplayerGlobalFeedbackVolume = MapDef<int32_t, 4>{
    {pot(0.0f), pot(0.15f), pot(0.5f), pot(1.0f)},
    {q15(0.0f), q15(0.05f), q15(0.08f), q15(0.25f)}};

// --- PITCHER ---
static constexpr auto kMapPitcherDryWet = MapDef<int32_t, 5>{
    {pot(0.0f), pot(0.15f), pot(0.5f), pot(0.85f), pot(1.0f)},
    {q15(0.0f), q15(0.62f), q15(0.62f), q15(0.62f), q15(1.0f)}};

static constexpr auto kMapPitcherFrequency = MapDef<int32_t, 5>{
    {pot(0.0f), pot(0.25f), pot(0.4f), pot(0.6f), pot(1.0f)},
    {fq31(2.f), fq31(10.f), fq31(40.f), fq31(100.f), fq31(1000.f)}};

static constexpr auto kMapPitcherDepth = MapDef<int32_t, 2>{
    {pot(0.0f), pot(1.0f)},
    {q15(0.0f), q15(1.0f)}};

static constexpr auto kMapPitcherStereoMix = MapDef<int32_t, 3>{
    {pot(0.0f), pot(0.2f), pot(1.0f)},
    {q15(1.0f), q15(0.0f), q15(0.0f)}};

static constexpr auto kMapPitcherStereoFrequency = MapDef<int32_t, 3>{
    {pot(0.0f), pot(0.1f), pot(1.0f)},
    {fq31(0.f), fq31(0.f), fq31(200.f)}};

static const float kPitcherEnvAttack = 0.1f;
static const float kPitcherEnvDecay = 1.f;

static constexpr auto kMapPitcherEnvDepth = MapDef<int32_t, 2>{
    {pot(0.0f), pot(1.0f)},
    {q15(1.0f), q15(1.0f)}};

static constexpr auto kMapPitcherGlobalDryWet = MapDef<int32_t, 3>{
    {pot(0.0f), pot(0.15f), pot(1.0f)},
    {q15(0.0f), q15(1.0f), q15(1.0f)}};

static constexpr auto kMapPitcherGlobalFeedbackDelay = MapDef<int32_t, 3>{
    {pot(0.0f), pot(0.55f), pot(1.0f)},
    {100, 636, 1000}};

static constexpr auto kMapPitcherGlobalFeedbackVolume = MapDef<int32_t, 4>{
    {pot(0.0f), pot(0.15f), pot(0.5f), pot(1.0f)},
    {q15(0.0f), q15(0.04f), q15(0.07f), q15(0.15f)}};

// --- SHIFTER ---
static constexpr auto kMapShifterFrequency = MapDef<int32_t, 5>{
    {pot(0.0f), pot(0.35f), pot(0.5f), pot(0.65f), pot(1.0f)},
    {fq31(100.f), fq31(1.2f), fq31(1.f), fq31(1.2f), fq31(260.f)}};

static constexpr auto kMapShifterTimeDry = MapDef<int32_t, 6>{
    {pot(0.0f), pot(0.47f), pot(0.48f), pot(0.52f), pot(0.53f), pot(1.0f)},
    {q15(0.0f), q15(0.0f), q15(1.0f), q15(1.0f), q15(0.0f), q15(0.0f)}};

static constexpr auto kMapShifterStereoFrequency = MapDef<int32_t, 2>{
    {pot(0.0f), pot(1.0f)},
    {fq31(0.f), fq31(20.f)}};

static constexpr auto kMapShifterEnvMod = MapDef<int32_t, 5>{
    {pot(0.0f), pot(0.45f), pot(0.5f), pot(0.55f), pot(1.0f)},
    {8, 0, 0, 0, 20}};

static constexpr float kShifterEnvAttack = 0.1f;
static constexpr float kShifterEnvDecay = 1.f;

static constexpr auto kMapShifterGlobalDryWet = MapDef<int32_t, 2>{
    {pot(0.0f), pot(1.0f)},
    {q15(0.0f), q15(1.0f)}};

static constexpr auto kMapShifterGlobalFeedbackDelay = MapDef<int32_t, 3>{
    {pot(0.0f), pot(0.55f), pot(1.0f)},
    {100, 636, 1000}};

static constexpr auto kMapShifterGlobalFeedbackVolume = MapDef<int32_t, 4>{
    {pot(0.0f), pot(0.1f), pot(0.5f), pot(1.0f)},
    {q15(0.0f), q15(0.05f), q15(0.08f), q15(0.35f)}};

// --- DELAY ---
static constexpr auto kMapDelayDry = MapDef<int32_t, 3>{
    {pot(0.0f), pot(0.5f), pot(1.0f)},
    {q15(1.0f), q15(1.0f), q15(0.0f)}};

static constexpr auto kMapDelayWet = MapDef<int32_t, 3>{
    {pot(0.0f), pot(0.5f), pot(1.0f)},
    {q15(0.0f), q15(0.5f), q15(1.0f)}};

static constexpr auto kMapDelayFeedback = MapDef<int32_t, 3>{
    {pot(0.0f), pot(0.3f), pot(1.0f)},
    {q15(0.0f), q15(0.2f), q15(1.0f)}};

static constexpr auto kMapDelayLength = MapDef<int32_t, 5>{
    {pot(0.0f), pot(0.45f), pot(0.65f), pot(0.85f), pot(1.0f)},
    {50799, 12000, 7000, 4000, 100}};

static constexpr auto kMapDelayStereo = MapDef<int32_t, 3>{
    {pot(0.0f), pot(0.5f), pot(1.0f)},
    {0, 20, 2000}};

static constexpr auto kMapDelayGlobalDryWet = MapDef<int32_t, 3>{
    {pot(0.0f), pot(0.4f), pot(1.0f)},
    {q15(0.0f), q15(1.0f), q15(1.0f)}};

static constexpr size_t kDelayTicksSyncUpperLimit = 50800 * 5;
static constexpr size_t kDelayTicksSyncLowerLimit = 6000;
static constexpr auto kDelaySyncedDividers = std::array<Fraction, 22>{
    Fraction{32, 1},
    Fraction{16, 1},
    Fraction{12, 1},
    Fraction{8, 1},
    Fraction{6, 1},
    Fraction{4, 1},
    Fraction{3, 1},
    Fraction{3, 2},
    Fraction{2, 1},
    Fraction{1, 1},
    Fraction{1, 2},
    Fraction{3, 2},
    Fraction{1, 3},
    Fraction{1, 4},
    Fraction{1, 6},
    Fraction{1, 8},
    Fraction{1, 12},
    Fraction{1, 16},
    Fraction{1, 32},
    Fraction{1, 64},
    Fraction{1, 128},
    Fraction{1, 256}};

// Compute the index of {1, 1} fraction in kDelaySyncedDividers
static constexpr size_t kDelaySyncedDividersCenter = []() constexpr
{
    for (size_t i = 0; i < kDelaySyncedDividers.size(); ++i)
    {
        if (kDelaySyncedDividers[i] == Fraction{1, 1})
        {
            return i;
        }
    }
    // Fallback (should never happen if array is properly defined)
    return size_t{0};
}();

static constexpr auto kMapDelayGlobalFeedbackDelay = MapDef<int32_t, 3>{
    {pot(0.0f), pot(0.55f), pot(1.0f)},
    {0, 0, 0}};

static constexpr auto kMapDelayGlobalFeedbackVolume = MapDef<int32_t, 3>{
    {pot(0.0f), pot(0.23f), pot(1.0f)},
    {q15(0.0f), q15(0.5f), q15(0.75f)}};

// If larger delay difference than that (in seconds), don't slew delay time and just jump to that time
static constexpr size_t kDelayJumpIfLargerThan = s2sr(0.02f);

// --- SLICER ---
static constexpr auto kMapSlicerGlobalDryWet = MapDef<int32_t, 3>{
    {pot(0.0f), pot(0.05f), pot(1.0f)},
    {q15(0.0f), q15(1.0f), q15(1.0f)}};

static constexpr auto kMapSlicerGlobalFeedbackDelay = MapDef<int32_t, 3>{
    {pot(0.0f), pot(0.55f), pot(1.0f)},
    {100, 636, 800}};

static constexpr auto kMapSlicerGlobalFeedbackVolume = MapDef<int32_t, 4>{
    {pot(0.0f), pot(0.15f), pot(0.35f), pot(1.0f)},
    {q15(0.0f), q15(0.0f), q15(0.0f), q15(0.1f)}};

static constexpr auto kSlicerPatterns = std::array<uint32_t, 8>{
    0b10000000,
    0b10001000,
    0b00100010,
    0b10000100,
    0b10010010,
    0b10101010,
    0b10101100,
    0b11111111,
};

static constexpr auto kMapSlicerPattern = MapDef<int32_t, kSlicerPatterns.size()>{
    {pot(0.06f), pot(0.18f), pot(0.3f), pot(0.42f), pot(0.54f), pot(0.66f), pot(0.78f), pot(0.9f)},
    {0, 1, 2, 3, 4, 5, 6, 7}};

static constexpr auto kMapSlicerDecay = MapDef<float, 2>{
    {pot(0.0f), pot(1.0f)},
    {1.f, 0.01f}};

static constexpr auto kMapSlicerStereo = MapDef<int32_t, 2>{
    {pot(0.0f), pot(1.0f)},
    {0, 7}};

static constexpr auto kMapSlicerChance = MapDef<int32_t, 2>{
    {pot(0.0f), pot(1.0f)},
    {q15(0.0f), q15(0.9f)}};

// --- SEQUENCER
static constexpr size_t kFxSequencerLength = 8; ///< See Sequencer.hpp for the max length

}