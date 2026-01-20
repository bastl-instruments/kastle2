/*
MIT License

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

#include <cstddef>
#include "common/dsp/math/qmath.hpp"
#include "common/fastcode.hpp"
#include "SvfStereo.hpp"

namespace kastle2
{

/**
 * @class DjFilterStereo
 * @ingroup dsp_filters
 * @brief DJ-style filter that crossfades between a lowpass and highpass filter. Uses two Svf filters per channel.
 * @note Slightly faster than two separate DjFilters.
 * @author Vaclav Mach (Bastl Instruments), Marek Mach (Bastl Instruments)
 * @date 2024-05-28
 */
class DjFilterStereo
{
public:
    /**
     * @brief Initializes the filter with a given sample rate
     * @param sample_rate The sample rate of the audio
     */
    void Init(float sample_rate);

    /**
     * @brief Processes the input through the two filters
     * @param input_left The left channel input to process
     * @param input_right The right channel input to process
     */
    FASTCODE void Process(q15_t input_left, q15_t input_right);

    /**
     * @brief Returns the left processed signal.
     * @return Processed left channel
     */
    q15_t GetLeft();

    /**
     * @brief Returns the right processed signal.
     * @return Processed right channel
     */
    q15_t GetRight();

    /**
     * @brief Sets the filter crossfade
     * @param crossfade -1.0 is lowpass, 0.0 is bandpass, 1.0 is highpass
     */
    void SetCrossfade(q15_t crossfade);

    /**
     * @brief Sets the filter resonance
     * @param resonance The resonance (0-1)
     * @param force If true, allows setting resonance to values below 0.005 which will cause instability for low-pass mode with high frequencies. Use only if you want to set a perfect zero, which is fine and doesn't cause issues.
     */
    void SetResonance(float resonance, SvfStereo::ForceValue force = SvfStereo::ForceValue::FALSE);

private:
    // Filters
    SvfStereo lowpass_;
    SvfStereo highpass_;

    // Crossfade between filters
    q15_t crossfade_ = Q15_ZERO;

    // Frequencies
    static constexpr auto kLowpassMap = MapDef<float, 5>{
        {q15(0.0f), q15(0.25f), q15(0.5f), q15(0.75f), q15(1.0f)},
        {50.0, 100.0, 400.0, 2000.0, 15000.0}};

    static constexpr auto kHighpassMap = MapDef<float, 5>{
        {q15(0.0f), q15(0.25f), q15(0.5f), q15(0.75f), q15(1.0f)},
        {20.0, 80.0, 300.0, 1000.0, 5000.0}};

    static constexpr float kLowPassMax = kLowpassMap.output[kLowpassMap.size() - 1];
    static constexpr float kHighPassMax = kHighpassMap.output[kHighpassMap.size() - 1];

    // Output
    q15_t output_left_ = 0;
    q15_t output_right_ = 0;

    // Zone management
    enum class Zone
    {
        LOWPASS = 0,
        NONE = 1,
        HIGHPASS = 2
    };
    static constexpr q15_t kCenterDeadzone = q15(0.2);
    static constexpr q15_t kCenterDeadzoneThreshold = q15(0.05);
    Zone prev_zone_ = Zone::NONE;
    Zone zone_ = Zone::NONE;

    // Crossfade management
    static constexpr uint32_t kCrossfadeLength = 2048;
    Zone crossfade_from_ = Zone::NONE;
    uint32_t crossfade_index_ = kCrossfadeLength; // Initialize to kCrossfadeLength to indicate no crossfade in progress
};
}
