/*
MIT License

Copyright (c) 2025 Marek Mach (Bastl Instruments)

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
#include "common/dsp/math/qmath.hpp"
#include "common/fastcode.hpp"

namespace kastle2
{

/**
 * @class MultiOscillator
 * @ingroup dsp_synthesis
 * @brief Audio oscillator providing all waveforms at once and FM features.
 * @author Marek Mach (Bastl Instruments)
 * @date 2025-06-24
 *
 * Defaults to 440Hz.
 * Built on Oscillator.hpp.
 *
 */
class MultiOscillator
{
public:
    typedef struct
    {
        q31_t sine = Q31_ZERO;
        q31_t square = Q31_ZERO;
        q31_t ramp = Q31_ZERO;
    } Outputs;

    /**
     * @brief Initializes the oscillator.
     */
    void Init(const float sample_rate);

    /**
     * @brief Sets the frequency of the oscillator.
     * @param frequency The frequency in Hz.
     */
    void SetFrequency(const float frequency);

    /**
     * @brief Sets the "native frequency" of the oscillator.
     * @param native_frequency The frequency in q31_t format where 1.0 is the sample_rate.
     */
    void SetNativeFrequency(const q31_t native_frequency);

    /**
     * @brief Sets the pulse width for the square waveform output.
     * @note Does not affect other waveforms
     * @param pulse_width The pulse width in q31_t format from Q31_ZERO to Q31_MAX (negative does nothing).
     */
    void SetPulseWidth(const q31_t pulse_width);

    /**
     * @brief Processes the waveform to be generated and returns one sample.
     */
    FASTCODE Outputs Process();

    /**
     * @brief Adds a value to the current phase.
     * @param phase The value to add to the current phase in q31_t format.
     * This method is useful for phase modulation and "FM" synthesis.
     */
    FASTCODE void PhaseAdd(const q31_t phase);

    /**
     * @brief returns the current phase counter value
     * @note To set the phase, use Reset()
     */
    FASTCODE q31_t GetPhase() const;

    /**
     * @brief Sets the amount of FM feedback of the oscillator
     * @param feedback The feedback amount in q31_t format.
     * This creates strange arefacting FM noises when set high enough.
     */
    void SetPhaseFeedback(const q31_t feedback);

    /**
     * @brief Resets the phase of the oscillator.
     * @param _phase The phase to reset to in q31_t format. If no argument is provided, the phase will be reset to the start.
     */
    void Reset(const q31_t _phase = Q31_MIN);

    /**
     * @brief Returns all waveforms in a struct.
     * @return A struct containing the sine, square, and ramp waveforms in q31
     */
    constexpr inline Outputs GetOutputs() {
        return outputs_;
    }

    private:
    float sample_rate_ = 0.0f;
    q31_t phase_ = Q31_ZERO;
    q31_t phase_inc_ = Q31_ZERO;
    q31_t pulse_width_ = Q31_ZERO;
    q31_t native_frequency_ = Q31_ZERO;
    q31_t feedback_ = Q31_ZERO;
    Outputs outputs_;

    /**
     * @brief Calculates the phase increment for a given frequency.
     * @param frequency The frequency in q31_t format.
     * @return The phase increment in q31_t format.
     */
    static q31_t CalcPhaseIncrement(const q31_t frequency);
};
}
