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

#include "common/dsp/math/math_utils.hpp"

namespace kastle2
{

/**
 * @class Portamento
 * @ingroup utility
 * @brief A slewer with change speed setting in seconds.
 * @note Based on the Envelope Follower class
 * @authors Marek Mach (Bastl Instruments)
 * @date 2025-07-10
 */
class Portamento
{
public:
    /**
     * @brief Initializes the portamento
     * @param update_rate how frequently Track() is called in Hz.
     */
    void Init(float update_rate);

    /**
     * @brief Sets response time of the portamento.
     * @param speed change speed in seconds.
     */
    void SetSpeed(float speed);

    /**
     * @brief Roughly tracks the UI loop calls compared to the audio processing rate for time-correct smoothing.
     *        Call this function from the audio loop, at the speed that was set in the Init() function.
     */
    void TimeTick();

    /**
     * @brief Smooths incoming signal based on set change time.
     * @param in signal to be smoothed out.
     */
    float Track(float in);

    /**
     * @brief Returns the smoothed out envelope signal.
     * @return envelope amount from 0 to 1 in Q15 format.
     */
    float GetOut() const;

private:
    float new_value_ = 0;
    float current_value_ = 0;
    float speed_ = 0;
    float update_rate_ = 0.0f;
    uint32_t timed_loop_calls_ = 0;
};
}
