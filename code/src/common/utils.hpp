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

#include <cstdint>
#include "common/config.hpp"
#include "common/dsp/math/math_utils.hpp"
#include "common/dsp/math/qmath.hpp"

namespace kastle2
{

/**
 * @file utils.h
 * @brief Various utility functions for Kastle 2.
 * @author Vaclav mach (Bastl Instruments)
 * @date 2024-06-03
 */

static constexpr int32_t START_NOTE = 48;
static constexpr int32_t END_NOTE = 108;

/**
 * @brief Converts MIDI notes into PWM values for the "DAC" (actually a PWM).
 * @param x The MIDI note to convert (0-127)
 * @param start_note The start note of the range (default 48)
 * @param end_note The end note of the range (default 108)
 * @return The PWM value (0-4095)
 */
inline constexpr uint32_t midi_note_to_pwm(const uint32_t x, const uint32_t start_note = START_NOTE, uint32_t end_note = END_NOTE)
{
    uint32_t note = constrain(x, start_note, end_note); // constrain to the range
    return map(note, start_note, end_note, 0, DAC_MAX);
}

/**
 * @brief Applies a potentiometer modulation to a value.
 * @param val The value to modulate (positive or negative values)
 * @param mod The modulation value, usually from a potentiometer (0-4095)
 * @return The modulated value preserving the sign of val
 */
inline constexpr int32_t apply_pot_mod(const int32_t val, const int32_t mod)
{
    return (val * mod) / POT_RANGE;
}

/**
 * @brief Applies a potentiometer modulation to a value with the ability to invert if below middle.
 * @param val The value to modulate (positive or negative values)
 * @param mod The modulation value, usually from a potentiometer (0-4095)
 * @return The modulated value preserving or inverting the sign of val based on mod
 */
inline constexpr int32_t apply_pot_mod_attenuvert(const int32_t val, const int32_t mod)
{
    static constexpr int32_t precise_half = (POT_RANGE / 2);
    const int32_t normalized_mod = mod - precise_half;
    return (val * normalized_mod) / precise_half;
}

/**
 * @brief Applies voltage to a frequency (1V per octave style)
 * @param base_frequency The base frequency
 * @param voltage The voltage to apply
 * @return The resulting frequency
 */
inline constexpr float cv_to_freq(const float base_frequency, const float voltage)
{
    return base_frequency * std::pow(2.0, voltage);
}

/**
 * @brief Applies voltage (expects 5V as 4095 steps) to a frequency (1V per octave style)
 * @param base_frequency The base frequency
 * @param steps The steps to apply (1V = 819 steps)
 * @return The resulting frequency
 */
inline constexpr float cv_to_freq_raw(const float base_frequency, const int32_t steps)
{
    return cv_to_freq(base_frequency, steps / static_cast<float>(ADC_1V));
}

/**
 * @brief Converts number from potentiometer range (0-4095) to q15_t (0-32768).
 * @param input The potentiometer value (0-4095)
 * @return The q15_t representation of the potentiometer value (0-1)
 */
inline constexpr q15_t pot_to_q15(const int32_t input)
{
    int32_t result = constrain(input, POT_MIN, POT_MAX); // 0-4095
    return result << 3;
}

/**
 * @brief Converts a potentiometer value (0-4095) to q31_t (0-2147483647).
 * @param input The potentiometer value (0-4095)
 * @return The q31_t representation of the potentiometer value (0-1)
 */
inline constexpr q31_t pot_to_q31(const int32_t input)
{
    int32_t result = constrain(input, POT_MIN, POT_MAX);
    return result << 19;
}

/**
 * @brief Converts a potentiometer value (0-4095) to MIDI CC (0-127).
 * @param input The potentiometer value (0-4095)
 * @return The MIDI CC representation of the potentiometer value (0-127)
 */
inline constexpr uint8_t pot_to_cc(const int32_t input)
{
    int32_t result = constrain(input, POT_MIN, POT_MAX);
    return result >> 5;
}

/**
 * @brief Converts a half-potentiometer value (0-2047) to MIDI CC (0-127).
 * @param half_input The half potentiometer value (0-2047)
 * @return The MIDI CC representation of the potentiometer value (0-127)
 */
inline constexpr uint8_t halfpot_to_cc(const int32_t input)
{
    int32_t result = constrain(input, POT_MIN, POT_HALF);
    return result >> 4;
}

/**
 * @brief Converts a potentiometer value (0-4095) to 8-bit memory storage (0-255).
 * @param pot The potentiometer value (0-4095)
 * @note We loose precision here, but it's acceptable for storing pot values.
 * @return The memory storage representation of the potentiometer value (0-255)
 */
inline constexpr uint8_t pot_to_mem(const int32_t pot)
{
    return constrain(pot, POT_MIN, POT_MAX) >> 4;
}

/**
 * @brief Converts an 8-bit memory storage value (0-255) back to potentiometer value (0-4095).
 * @param mem The memory storage value (0-255)
 * @note We loose precision here, but it's acceptable for restoring pot values.
 * @return The potentiometer representation of the memory storage value (0-4095)
 */
inline constexpr int32_t mem_to_pot(const uint8_t mem)
{
    return static_cast<int32_t>(mem) << 4;
}

/**
 * @brief Converts a frequency to q31_t representation based on the sample rate.
 * @param x The frequency to convert
 * @return The q31_t representation of the frequency
 */
inline constexpr q31_t fq31(const float x)
{
    return freq_to_q31(x, SAMPLE_RATE);
}

}
