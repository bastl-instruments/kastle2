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

#include "pico/stdlib.h"
#include "I2S.hpp"

/**
 * @file config.hpp
 * @ingroup core
 * @brief Core project configuration and constants.
 * @author Vaclav Mach (Bastl Instruments)
 * @date 2024-03-27
 */

namespace kastle2
{

/**
 * Kastle 2's 8MB memory is divided into 2 sections:
 * 512 KB for the main code
 * 7.5 MB for the user data (eg. samples)
 */
#define USER_DATA_SECTION __attribute__((section(".user_data")))
#define USER_DATA_SECTION_BEGIN 0x10080000 // at 512 KB

/**
 * Close to 44100 - "weird" frequency, because we need the RP2040 to run at
 * a multiplied frequency and frequencies of RP2040 are limited
 * (need to be calculated by rp2040_freq.py in scrpts folder and confirmed by vcocalc.py in Pico SDK).
 */
static constexpr float SAMPLE_RATE = 44000.0f;

/**
 * Seconds to sample rate samples (s * SAMPLE_RATE)
 * @param s Seconds to convert
 * @return Number of samples in SAMPLE_RATE (44000 Hz)
 */
static consteval size_t s2sr(float s)
{
    return static_cast<size_t>(s * SAMPLE_RATE);
}

/**
 * Audio buffer frames for Kastle 2 in stereo
 * Actually is 96 "real" frames (2*48 frames)
 */
static constexpr size_t AUDIO_BUFFER_SIZE = I2S::kAudioBufferSize;

/**
 * Basically SAMPLE_RATE / buffer size (48)
 */
static constexpr float AUDIO_LOOP_RATE = SAMPLE_RATE / static_cast<float>(AUDIO_BUFFER_SIZE);

/**
 * Seconds to audio loop rate samples (s * AUDIO_LOOP_RATE)
 * @param s Seconds to convert
 * @return Number of samples in AUDIO_LOOP_RATE (916 Hz)
 */
static consteval size_t s2alr(float s)
{
    return static_cast<size_t>(s * AUDIO_LOOP_RATE);
}

/**
 * Converts Hz to audio loop rate samples
 * @param hz Frequency in Hz to convert
 * @return Number of samples in AUDIO_LOOP_RATE (916 Hz)
 */
static consteval size_t hz2alr(float hz)
{
    return static_cast<size_t>(AUDIO_LOOP_RATE / hz);
}

/**
 * Because SYSTEM_CLOCK_KHZ / SAMPLE_RATE needs to be round number
 * Slightly overclocked (base is 133 MHz), but should be all right
 */
static constexpr uint32_t SYSTEM_CLOCK_KHZ = 176000;

/**
 * Max value of the DAC output (PWM-based, filtered)
 */
static constexpr int32_t DAC_MAX = 1023;

/**
 * ADC values for standard voltages
 * On Kastle 2 default the ADC values are 0-4095,
 * but in Citadel it's scaled to -4095 to 4095 since the inputs are bipolar.
 */
static constexpr int32_t ADC_0V = 0;
static constexpr int32_t ADC_1V = 819;
static constexpr int32_t ADC_2V = 2 * ADC_1V;
static constexpr int32_t ADC_3V = 3 * ADC_1V;
static constexpr int32_t ADC_4V = 4 * ADC_1V;
static constexpr int32_t ADC_5V = 5 * ADC_1V;
static constexpr int32_t ADC_6V = 6 * ADC_1V;
static constexpr int32_t ADC_7V = 7 * ADC_1V;
static constexpr int32_t ADC_8V = 8 * ADC_1V;
static constexpr int32_t ADC_N1V = -1 * ADC_1V;
static constexpr int32_t ADC_N2V = -2 * ADC_1V;
static constexpr int32_t ADC_N3V = -3 * ADC_1V;
static constexpr int32_t ADC_N4V = -4 * ADC_1V;
static constexpr int32_t ADC_N5V = -5 * ADC_1V;

static constexpr int32_t ADC_N02V = -164;

/**
 * DAC values for 0V, 1V, 2V, 3V, 4V
 * Rough estimations, for USB powered (5.0V)
 */
static constexpr int32_t DAC_0V = 0;
static constexpr int32_t DAC_1V = 220;
static constexpr int32_t DAC_2V = 440;
static constexpr int32_t DAC_3V = 660;
static constexpr int32_t DAC_4V = 878;

/**
 * POT VALUES
 * Regular 12-bit ADC values (0-4095)
 */
static constexpr int32_t POT_MIN = 0;
static constexpr int32_t POT_MAX = 4095;
static constexpr int32_t POT_THIRD = (POT_MAX / 3);
static constexpr int32_t POT_HALF = (POT_MAX / 2);
static constexpr int32_t POT_TWO_THIRDS = (POT_MAX * 2 / 3);
static constexpr int32_t POT_MOVE_THRESHOLD = 48;
static constexpr int32_t POT_RANGE = 4096;

} // namespace kastle2