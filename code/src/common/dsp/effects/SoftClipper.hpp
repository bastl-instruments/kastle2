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

#include "common/fastcode.hpp"
#include "common/dsp/filters/Svf.hpp"
#include "common/dsp/math/qmath.hpp"

namespace kastle2
{

/**
 * @class SoftClipper
 * @ingroup dsp_effects
 * @brief Soft clipping overdrive effect using tanh.
 * @authors Marek Mach (Bastl Instruments)
 * @date 2024-07-22
 */
class SoftClipper
{

public:
    /**
     * @brief Initializes the SoftClipper object.
     *
     * @param sample_rate The sample rate of the audio signal.
     */
    void Init(float sample_rate);

    /**
     * @brief Disable automatic lowering of the volume at higher drive values.
     *
     * @param disable Set true to disable the feature.
     */
    void DisableVolumeCompensation(bool disable);

    /**
     * @brief Processes an input sample and applies the Soft clipping and bass boost.
     *
     * @param input The input sample to be processed.
     * @return The processed output sample.
     */
    FASTCODE q15_t Process(q15_t input);

    /**
     * @brief Sets the drive level of the soft clipping.
     *
     * @param drive The drive level to be set. (0-Q15_MAX)
     */
    void SetDrive(q15_t drive);

    /**
     * @brief Gets the current drive level of the soft clipping.
     *
     * @return The current drive level.
     */
    q15_t GetDrive() const;

private:
    float sample_rate_ = 0.0f;  /* The sample rate of the audio signal. */
    q15_t drive_ = 0;           /* The drive level of the effect. */
    bool compensation_ = false; /* If the volume should be lowered at higher drive*/
};
}
