/*
MIT License

Copyright (c) 2024 Vaclav Mach (Bastl Instruments)

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
#include "common/dsp/math/Fraction.hpp"
#include "common/dsp/math/math_utils.hpp"
#include "common/dsp/math/qmath.hpp"
#include "common/peripherals/WS2812.hpp"

namespace kastle2
{

// --- COLORS ---

static constexpr uint32_t kBaseColorGate = WS2812::WARM_WHITE;

static constexpr uint32_t kBaseColorLfoSynced = WS2812::COLD_WHITE;
static constexpr uint32_t kBaseColorLfoFree = WS2812::WARM_WHITE;

static constexpr uint32_t kBaseColorTempoInternal = WS2812::MEDIUM_MAGENTA;
static constexpr uint32_t kBaseColorTempoExternal = WS2812::MEDIUM_CYAN;
static constexpr uint32_t kBaseColorTempoMidi = WS2812::BLUE;

static constexpr uint32_t kBaseColorTempoMidiDisabled = WS2812::KHAKI;
static constexpr uint32_t kBaseColorTempoNoneDisabled = WS2812::WHITE;
static constexpr uint32_t kBaseColorTempoExternalDisabled = WS2812::ORANGE;

static constexpr uint32_t kBaseColorMonoLeft = WS2812::BLUE;
static constexpr uint32_t kBaseColorStereo = WS2812::WHITE;
static constexpr uint32_t kBaseColorMonoRight = WS2812::RED;

static constexpr uint32_t kBaseColorSyncEnabled = WS2812::GREEN;
static constexpr uint32_t kBaseColorSyncDisabled = WS2812::RED;

static constexpr auto kMapLoudnessLedGreen = MapDef<q15_t, 4>{
    {q15(0.0f), q15(0.5f), q15(0.75f), q15(1.0f)},
    {0, 255, 0, 0}};
static constexpr auto kMapLoudnessLedRed = MapDef<q15_t, 4>{
    {q15(0.0f), q15(0.5f), q15(0.75f), q15(1.0f)},
    {0, 0, 255, 255}};

// --- LFO ---
static constexpr auto kBaseLfoMap = MapDef<float, 5>{
    {pot(0.5f), pot(0.63f), pot(0.75f), pot(0.87f), pot(1.0f)},
    {0.02f, 0.2f, 1.0f, 3.0f, 30.0f}};

static constexpr auto kBaseLfoRatios = std::array<Fraction, 15>{
    Fraction{1, 256},
    Fraction{1, 128},
    Fraction{1, 64},
    Fraction{1, 32},
    Fraction{1, 16},
    Fraction{1, 12},
    Fraction{1, 8},
    Fraction{1, 6},
    Fraction{1, 4},
    Fraction{1, 3},
    Fraction{1, 2},
    Fraction{1, 1},
    Fraction{2, 1},
    Fraction{3, 1},
    Fraction{4, 1}};

static constexpr auto kBaseLfoRatioMap = MapDef<int32_t, 2>{
    {0, pot(0.5f)},
    {kBaseLfoRatios.size() - 1, 0}};

// --- TEMPO/CLOCK RELATED ---

// UNUSED
// static constexpr uint32_t kExternalTempoChangedCountdownMax = 1000; ///< The countdown (in ticks) for external tempo change indication.
// static constexpr int32_t kExternalTempoChangedThreshold = 20;       ///< The threshold (in ticks) for external tempo change indication.

static constexpr uint32_t kSyncOutTicks = 5;                        ///< The number of ticks for sync out pulse. Approximates to ms.
static constexpr uint32_t kTapTempoMinTicks = s2alr(0.1f);          ///< Minimum tap in ticks.
static constexpr uint32_t kTapTempoMaxTicks = s2alr(2.0f);          ///< 2 seconds for tap tempo max.
static constexpr uint32_t kTapTempoJustHappenedTicks = s2alr(0.2f); ///< Prevent double trigger when user taps just a tiny bit late.

static constexpr uint32_t kTapTempoMultiplier = 4; ///< The tempo is this times faster than the tapping.

static constexpr uint32_t kExtClockProbablyStopped = s2alr(2.0f); ///< If 2 seconds ext clock didn't tick, it's probably stopped.

static constexpr std::array<uint8_t, 4> kDividersMultipliers = {1, 2, 4, 8}; ///< The ext clock dividers and multipliers.

/**
 * @brief Potentiometer mapping for internal tempo.
 */
static constexpr auto kTempoMap = MapDef<int32_t, 5>{
    {pot(0.0f), pot(0.125f), pot(0.5f), pot(0.87f), pot(1.0f)},
    {hz2alr(0.1f), hz2alr(3.0f), hz2alr(6.0f), hz2alr(14.0f), hz2alr(60.0f)}};

/**
 * @brief Potentiometer mapping for external MIDI tempo.
 */
static constexpr std::array<size_t, 5> kMidiTempoDividers = {
    24, // ¼ note
    12, // 8th note
    6,  // 16th note
    3,  // 32th note
    1,  // 1:1 clock
};
static constexpr size_t kMidiTempoDividerDefault = 6; // real value, not index

// --- GATE SIZE ---
static constexpr uint32_t kBaseGateLength = 75; ///< Length of the gate output in percents.

// --- TRIGGER GENERATOR TABLE (can be overwritten by firmwares) ---
static constexpr std::array<uint32_t, 16> kBaseRhythmTable = {
    0b1000000010000000,
    0b1000100010001000,
    0b1000101010001010,
    0b1000001001001000,
    0b1000000000100000,
    0b1010000110100000,
    0b1000100000101000,
    0b1000100010010010,
    0b0010001000100010,
    0b0010010001000110,
    0b0010001000110010,
    0b1111101010111010,
    0b0000111100001100,
    0b1010110010101100,
    0b0011001100110010,
    0b0111011101110111};

// --- OTHERS ---

// Startup attack envelope not to blow ears off
static constexpr float kBaseStartupFadeIn = 0.75f;
static constexpr uint32_t kBaseStartupFadeInDelay = s2alr(0.75f);

// Whether the output sync should copy input sync (if connected)
// Meaning not divided/multiplied
static constexpr bool kBaseSyncThru = true;

// To enter Advanced Settings
static constexpr size_t kBaseTicksEnterSettings = s2alr(2); // 2 seconds

// To clear memory
static constexpr size_t kBaseTicksClearMemory = s2alr(10); // 10 seconds

// Pots running average
static constexpr size_t kBasePotsRunningAverage = 8;

// When switching to SHIFT layer, ignore pot readings for a while
static constexpr size_t kShiftShortPressTicks = s2alr(0.2f);

// Deadzone for FancyPot implementation
static constexpr auto kMapDeadzone = MapDef<int32_t, 4>{{pot(0.0f), pot(0.45f), pot(0.55f), pot(1.0f)},
                                                        {pot(0.0f), pot(0.50f), pot(0.50f), pot(1.0f)}};

// --- DC OFFSETS (Citadel only) ---

static constexpr bool kCitadelInputDcOffsetRemove = true;
static constexpr bool kCitadelOutputDcOffsetRemove = true;

static constexpr q15_t kCitadelInputDcOffset = q15(0.01f);
static constexpr q15_t kCitadelOutputDcOffset = q15(-0.008f);

}
