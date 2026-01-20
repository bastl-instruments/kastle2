/*
MIT License

Copyright (c) 2016 Lennart Schierling (Bastl Instruments)
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

#include "common/dsp/math/math_utils.hpp"
#include "common/dsp/math/qmath.hpp"
#include "common/fastcode.hpp"

namespace kastle2
{

/**
 * @class EnvelopeFollower
 * @ingroup dsp_control
 * @brief An envelope follower with variable timing for attack and release.
 * Attack and release speeds are set in seconds.
 * @authors Lennart Schierling (Bastl Instruments), Marek Mach (Bastl Instruments)
 * @date 2024-07-10
 */
class EnvelopeFollower
{
public:
    /**
     * @brief Initializes the envelope follower
     * @param update_rate how frequently Track() is called in Hz.
     */
    void Init(float update_rate);

    /**
     * @brief Sets rising response time of the envelope follower.
     * @param attack rising speed in seconds.
     */
    void SetAttackTime(float attack);

    /**
     * @brief Sets falling response time of the envelope follower.
     * @param release falling speed in seconds.
     */
    void SetReleaseTime(float release);

    /**
     * @brief Smooths incoming signal based on attack and release times.
     * @note Feed a peak value over multiple samples instead of every sample. Less frequent calls make for a smoother but still responsive output.
     * @param in signal to be smoothed out.
     */
    FASTCODE void Track(q15_t in);

    /**
     * @brief Updates the peak value without calculating the next output value.
     * @note This function is to be used with TrackWithoutUpdate()
     * @param in signal to be smoothed out.
     */
    FASTCODE void UpdatePeak(q15_t in);

    /**
     * @brief Allows for better smoothing of the envelope while keeping the input signal rate same.
     * @note The increased call speed must be reflected in the init function.
     * @return envelope amount from 0 to 1 in Q15 format, same as GetEnvelope().
     */
    FASTCODE q15_t CalculateEnvelope();

    /**
     * @brief Finds peak value and uses it for shaping the amplitude envelope.
     * @remark An interlaced stereo buffer can be passed to detect the overall envelope (buffer_size must be change accordingly).
     * @param buffer block of signal to be smoothed out.
     * @param block_size size of the buffer.
     */
    FASTCODE void TrackBlock(q15_t *buffer, size_t block_size);

    /**
     * @brief Returns the smoothed out envelope signal.
     * @return envelope amount from 0 to 1 in Q15 format.
     */
    FASTCODE q15_t GetEnvelope() const;

private:
    q15_t new_value_ = Q15_ZERO;
    q15_t current_value_ = 0;
    q15_t attack_ = 0;
    q15_t release_ = 0;
    float update_rate_ = 0.0f;
};
}
