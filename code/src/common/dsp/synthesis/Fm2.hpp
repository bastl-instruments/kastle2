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
#include "Oscillator.hpp"

namespace kastle2
{

/**
 * @class Fm2
 * @ingroup dsp_synthesis
 * @brief Simple 2 operator FM synth voice.
 * @author Vaclav Mach (Bastl Instruments)
 * @date 2024-05-30
 *
 * Ported from DaisySP and rewritten into fixed point math.
 * Original Code: Ben Sergentanis, Electrosmith, Corp in November 2020 under MIT License.
 */

class Fm2
{
public:
    /**
     * @brief Initializes the FM synth voice.
     * @param sample_rate The sample rate of the audio engine.
     */
    void Init(const float sample_rate);

    /**
     * @brief Processes the FM synth voice and returns one sample.
     * @return The output sample in Q31 fixed point format.
     */
    q31_t Process();

    /**
     * @brief Sets the carrier frequency.
     * @param frequency The frequency in Hz.
     */
    void SetFrequency(const float frequency);

    /**
     * @brief Sets the carrier frequency.
     * @param frequency The frequency in Q31 fixed point format relative to the sample_rate.
     */
    void SetNativeFrequency(const q31_t freq);

    /**
     * @brief Sets modulator freq. relative to carrier
     * @param ratio New modulator freq = carrier freq. * ratio
     */
    void SetRatio(const q31_t ratio);

    /**
     * @brief Sets the index (FM depth) of the FM synth voice.
     * @param index FM depth in Q31 fixed point format.
     */
    void SetIndex(const q31_t index);

    /**
     * @brief Returns the current FM index.
     * @return The current FM index in Q31 fixed point format.
     */
    q31_t GetIndex() const;

    /**
     * @brief Resets the both oscillators of the FM synth voice.
     */
    void Reset();

private:
    float sample_rate_ = 0.0f;
    static constexpr q31_t kIndexScalar = (INT32_MAX / 8);
    Oscillator mod_, car_;
    q31_t index_ = 0;
    q31_t freq_ = 0;
    q31_t prev_freq_ = 0;
    q31_t ratio_ = 0;
    q31_t prev_ratio_ = 0;
};
}
