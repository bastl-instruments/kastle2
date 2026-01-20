/*
MIT License

Copyright (c) 2020 Electrosmith, Corp
Copyright (c) 2024 Vaclav Mach (Bastl Instruments)
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

#include <cstdint>
#include "common/dsp/math/qmath.hpp"
#include "Oscillator.hpp"

namespace kastle2
{

/**
 * @class OscillatorQ15
 * @ingroup dsp_synthesis
 * @brief Lo-fi version of the Oscillator class.
 * @author Vaclav Mach (Bastl Instruments), Marek Mach (Bastl Instruments)
 * @note While this oscillator is faster, it cannot be used for low frequencies of 1Hz and lower!
 * @date 2024-07-30
 *
 */
class OscillatorQ15
{
public:
    /**
     * @brief Initializes the oscillator.
     */
    void Init(float sample_rate);

    /**
     * @brief Sets the frequency of the oscillator.
     * @param frequency The frequency in Hz.
     */
    void SetFrequency(const float frequency);

    /**
     * @brief Sets the "native frequency" of the oscillator.
     * @param native_frequency The frequency in q31_t format where 1.0 is the sample_rate.
     */
    void SetNativeFrequency(const q15_t native_frequency);

    /**
     * @brief Set the number of ticks per one period (number of Process calls until a period finishes)
     * @param ticks The number of ticks per period.
     */
    void SetTicks(const uint32_t ticks);

    /**
     * @brief Sets the amplitude of the oscillator.
     * @param amplitude The amplitude in q31_t format.
     */
    void SetAmplitude(const q15_t amplitude);

    /**
     * @brief Sets the waveform of the oscillator.
     * @param waveform The waveform to set.
     */
    void SetWaveform(const Oscillator::Waveform waveform);

    /**
     * @brief Sets the pulse width for the square waveform.
     * @param pulse_width The pulse width in q31_t format.
     */
    void SetPulseWidth(q15_t pulse_width);

    /**
     * @brief Returns true if the oscillator is at the end of the rise phase.
     * @return True if the oscillator is at the end of the rise phase, false otherwise.
     */
    bool IsEOR();

    /**
     * @brief Returns true if the oscillator is at the end of the cycle.
     * @return True if the oscillator is at the end of the cycle, false otherwise.
     */
    bool IsEOC();

    /**
     * @brief Returns true if the oscillator is in the rising phase.
     * @return True if the oscillator is in the rising phase, false otherwise.
     */
    bool IsRising();

    /**
     * @brief Returns true if the oscillator is in the falling phase.
     * @return True if the oscillator is in the falling phase, false otherwise.
     */
    bool IsFalling();

    /**
     * @brief Processes the waveform to be generated and returns one sample.
     * @return The generated sample in q31_t format.
     */
    q15_t Process();

    /**
     * @brief Adds a value to the current phase.
     * @param phase The value to add to the current phase in q31_t format.
     * This method is useful for phase modulation and "FM" synthesis.
     */
    void PhaseAdd(q15_t phase);

    /**
     * @brief Resets the phase of the oscillator.
     * @param _phase The phase to reset to in q31_t format. If no argument is provided, the phase will be reset to the start.
     */
    void Reset(q15_t _phase = Q15_MIN);

    /**
     * @brief Calculates the phase increment for a given frequency.
     * @param frequency The frequency in q31_t format.
     * @return The phase increment in q31_t format.
     */
    static q15_t CalcPhaseIncrement(q15_t frequency);

private:
    float sample_rate_;
    Oscillator::Waveform waveform_;
    q15_t amplitude_;
    q15_t phase_, phase_inc_;
    q15_t pulse_width_;
    q15_t native_frequency_;
    bool eor_, eoc_;
};
}
