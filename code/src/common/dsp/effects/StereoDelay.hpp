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

#include <cstddef>
#include <memory>
#include "common/dsp/filters/DjFilter.hpp"
#include "common/dsp/math/qmath.hpp"
#include "common/dsp/utility/AdvancedDynamicDelayLine.hpp"
#include "common/fastcode.hpp"

namespace kastle2
{

/**
 * @class StereoDelay
 * @ingroup dsp_effects
 * @brief Simple stereo delay effect
 * @author Vaclav Mach (Bastl Instruments)
 * @date 2024-05-30
 *
 * Processes stereo audio through a delay line with feedback and wet/dry mix
 * and crossfades between lowpass and highpass filters.
 *
 * The Delay line is heap allocated so the StereoDelay can be initialized on the fly (changing Apps etc.)
 *
 */
class StereoDelay
{
public:
    /**
     * @brief Output structure containing left and right channel samples
     */
    struct Output
    {
        q15_t left;  ///< Left channel output sample
        q15_t right; ///< Right channel output sample
    };

    /**
     * @brief Construct a new StereoDelay object
     * @param max_delay Maximum delay time in samples (default: kMaxDelay)
     */
    StereoDelay(size_t max_delay = kMaxDelay) : max_delay_(max_delay)
    {
    }

    /**
     * @brief Initialize the delay effect
     * @param sample_rate The audio sample rate in Hz
     */
    void Init(float sample_rate);

    /**
     * @brief Process stereo audio samples through the delay effect
     * @param left Left channel input sample
     * @param right Right channel input sample
     * @return Output struct containing processed left and right samples
     */
    FASTCODE Output Process(q15_t left, q15_t right);

    /**
     * @brief Set the delay time for left and right channels
     * @param delay_left Delay time for left channel in samples
     * @param delay_right Delay time for right channel in samples
     */
    void SetDelay(size_t delay_left, size_t delay_right);

    /**
     * @brief Set the wet/dry mix amount
     * @param wet Wet signal amount (0 = dry, Q15_MAX = fully wet)
     */
    void SetWet(q15_t wet);

    /**
     * @brief Set the feedback amount for both channels
     * @param feedback Feedback amount for both left and right channels
     */
    void SetFeedback(q15_t feedback);

    /**
     * @brief Set individual feedback amounts for left and right channels
     * @param feedback_left Feedback amount for left channel
     * @param feedback_right Feedback amount for right channel
     */
    void SetFeedback(q15_t feedback_left, q15_t feedback_right);

    /**
     * @brief Enable or disable the filter in the feedback path
     * @param filter_enabled True to enable filtering, false to disable
     */
    void SetFilterEnabled(bool filter_enabled);

    /**
     * @brief Set the filter resonance amount
     * @param resonance Filter resonance value (0.0 to 1.0)
     */
    void SetFilterResonance(float resonance);

    /**
     * @brief Set the crossfade between lowpass and highpass filters
     * @param crossfade -1.0 is lowpass, 0.0 is bandpass, 1.0 is highpass
     */
    void SetFilterCrossfade(q15_t crossfade);

    /**
     * @brief Get the last processed output
     * @return Output struct containing the last processed left and right samples
     */
    Output GetOutput() const;

    /**
     * @brief Maximum delay time in samples
     *
     * Examples of delay times in samples:
     * - 1 second delay: 44000 * 2 (q15_t size) * 2 (stereo) = 176000 bytes
     * - 1.2 second delay: 52800 * 2 (q15_t size) * 2 (stereo) = 211200 bytes
     */
    static constexpr size_t kMaxDelay = 52800;

private:
    float sample_rate_ = 0.0f;
    q15_t feedback_l_ = 0;
    q15_t feedback_r_ = 0;
    q15_t wet_ = 0;
    Output out_{0, 0};
    size_t max_delay_ = kMaxDelay;
    bool filter_enabled_ = false;
    DjFilter filter_l_;
    DjFilter filter_r_;
    std::unique_ptr<AdvancedDynamicDelayLine<q15least_t>> delay_l_;
    std::unique_ptr<AdvancedDynamicDelayLine<q15least_t>> delay_r_;
};
}
