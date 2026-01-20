/*
MIT License

Copyright (c) 2020 Electrosmith, Corp
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
#include "common/dsp/math/qmath.hpp"
#include "common/fastcode.hpp"

namespace kastle2
{

/**
 * @class Oscillator
 * @ingroup dsp_synthesis
 * @brief Simple oscillator that generates classic waveforms.
 * @author Vaclav Mach (Bastl Instruments)
 * @date 2024-04-18
 *
 * Default oscillator is a sine wave, 100 Hz, 1.0 amplitude.
 * Inspired by DaisySP's Oscillator class.
 *
 */
class Oscillator
{
public:
    /**
     * @brief The Waveform enum represents the available waveforms for the oscillator.
     */
    enum class Waveform
    {
        SINE,   /**< Sine waveform */
        TRI,    /**< Triangle waveform */
        SAW,    /**< Sawtooth waveform */
        RAMP,   /**< Ramp waveform */
        SQUARE, /**< Square waveform */
        COUNT   /**< Number of waveforms */
    };

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
     * @brief Set the number of ticks per one period (number of Process() calls until a period finishes)
     * @param ticks The number of ticks per period.
     */
    void SetTicks(const uint32_t ticks);

    /**
     * @brief Returns the length of a period in ticks.
     * @return Number of ticks per one full cycle.
     */
    uint32_t GetTicks() const;

    /**
     * @brief Returns the number of ticks elapsed since the phase reset.
     * @return Number of ticks since the start of the cycle.
     */
    uint32_t GetElapsedTicks() const;

    /**
     * @brief Sets the amplitude of the oscillator.
     * @param amplitude The amplitude in q31_t format.
     */
    void SetAmplitude(const q31_t amplitude);

    /**
     * @brief Sets the waveform of the oscillator.
     * @param waveform The waveform to set.
     */
    void SetWaveform(const Waveform waveform);

    /**
     * @brief Sets the pulse width for the square waveform.
     * @param pulse_width The pulse width in q31_t format.
     */
    void SetPulseWidth(const q31_t pulse_width);

    /**
     * @brief Returns true if the oscillator is at the end of the rise phase.
     * @return True if the oscillator is at the end of the rise phase, false otherwise.
     */
    bool IsEOR() const;

    /**
     * @brief Returns true if the oscillator is at the end of the cycle.
     * @return True if the oscillator is at the end of the cycle, false otherwise.
     */
    bool IsEOC() const;

    /**
     * @brief Returns true if the oscillator is in the rising phase.
     * @return True if the oscillator is in the rising phase, false otherwise.
     */
    bool IsRising() const;

    /**
     * @brief Returns true if the oscillator is in the falling phase.
     * @return True if the oscillator is in the falling phase, false otherwise.
     */
    bool IsFalling() const;

    /**
     * @brief Processes the waveform to be generated and returns one sample.
     * @return The generated sample in q31_t format.
     */
    FASTCODE q31_t Process();

    /**
     * @brief Get the current output of the oscillator
     * @return The current output in q31_t format.
     */
    q31_t GetOutput() const;

    /**
     * @brief Get the current phase of the lfo
     * @return phase in q31_t format
     */
    q31_t GetPhase() const;

    /**
     * @brief Adds a value to the current phase.
     * @param phase The value to add to the current phase in q31_t format.
     * This method is useful for phase modulation and "FM" synthesis.
     */
    void PhaseAdd(const q31_t phase);

    /**
     * @brief Resets the phase of the oscillator.
     * @param _phase The phase to reset to in q31_t format. If no argument is provided, the phase will be reset to the start.
     */
    void Reset(const q31_t _phase = Q31_MIN);

    /**
     * @brief Calculates the phase increment for a given frequency.
     * @param frequency The frequency in q31_t format.
     * @return The phase increment in q31_t format.
     */
    static q31_t CalcPhaseIncrement(const q31_t frequency);

private:
    float sample_rate_ = 0.0f;
    Waveform waveform_ = Waveform::TRI;
    q31_t amplitude_ = 0;
    q31_t phase_ = 0;
    q31_t phase_inc_ = 0;
    q31_t pulse_width_ = 0;
    q31_t native_frequency_ = 0;
    q31_t out_ = 0;
    bool eor_ = false;
    bool eoc_ = false;
    uint32_t ticks_ = 0;
    uint32_t ticks_counter_ = 0;
};
}
