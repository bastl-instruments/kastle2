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

#include "common/dsp/math/qmath.hpp"
#include "common/fastcode.hpp"

namespace kastle2
{

/**
 * @class SvfStereo
 * @ingroup dsp_filters
 * @brief State Variable Filter
 * @author: Vaclav Mach (Bastl Instruments), Marek Mach (Bastl Instruments)
 * @date 2024-11-21
 *
 * State Variable Filter with q15_t inputs and outputs.
 * Uses 14 bit (cobmination of q15_t and kDownsample) depth math for internal calculations.
 * If it crashes, try to increase kDownsample.
 *
 * Original source:
 *
 * Double Sampled, Stable State Variable Filter
 * Credit to Andrew Simper from musicdsp.org
 * This is his "State Variable Filter (Double Sampled, Stable)"
 * Additional thanks to Laurent de Soras for stability limit, and
 * Stefan Diedrichsen for the correct notch output
 * Ported by: Stephen Hensley to Daisy
 */

class SvfStereo
{
public:
    enum class Type
    {
        LOWPASS,
        HIGHPASS,
        BANDPASS,
        BYPASS,
        NOTCH
    };

    enum class ForceValue
    {
        FALSE,
        TRUE
    };

    /**
     * @brief Initializes a new State Variable Filter instance
     * @param sample_rate - The sample rate of the audio
     */
    void Init(float sample_rate);

    /**
     * @brief Processes the input signal, updating all of the outputs.
     * @param input_left - The left channel input signal to process
     * @param input_right - The right channel input signal to process
     */
    FASTCODE void Process(q15_t input_left, q15_t input_right);

    /**
     * @brief Returns the left processed signal.
     * @return Processed left channel
     */
    q15_t GetLeft() const;

    /**
     * @brief Returns the right processed signal.
     * @return Processed right channel
     */
    q15_t GetRight() const;

    /**
     * @brief Sets the frequency of the cutoff frequency.
     * @param frequency Must be between 0.0 and sample_rate / 3
     */
    void SetFrequency(float frequency);

    /**
     * @brief Sets the resonance of the filter.
     * @param resonance Must be between 0.0 and 1.0 to ensure stability.
     * @param force If true, allows setting resonance to values below 0.005 which will cause instability for low-pass mode with high frequencies. Use only if you want to set a perfect zero, which is fine and doesn't cause issues.
     */
    void SetResonance(float resonance, ForceValue force = ForceValue::FALSE);

    /**
     * @brief Sets the type of the filter. Used just for returning from process.
     * @param type - LOWPASS, HIGHPASS, BANDPASS, NOTCH, BYPASS
     */
    void SetType(Type type);

    /**
     * @brief Returns the type of the filter
     * @return Type - LOWPASS, HIGHPASS, BANDPASS, NOTCH, BYPASS
     */
    Type GetType() const;

    /**
     * @brief Sets the drive of the filter
     * Affects the response of the resonance of the filter
     * @param drive - 0.0 to 1.0
     */
    void SetDrive(float drive);

private:
    float sample_rate_ = 0.0f;
    float resonance_ = 0.0f;
    float pre_drive_ = 0.0f;
    float max_frequency_ = 0.0f;
    float internal_frequency_ = 0.0f;

    Type type_ = Type::LOWPASS;

    // Using these for calculations
    int32_t tmp_qdrive_ = 0;
    int32_t tmp_qdamp_ = 0;
    int32_t tmp_qinternal_frequency_ = 0;

    // These are set at once in FinishValueSetting to prevent glitches
    int32_t qdrive_ = 0;
    int32_t qdamp_ = 0;
    int32_t qinternal_frequency_ = 0;

    int32_t qnotch_left_ = 0;
    int32_t qlow_left_ = 0;
    int32_t qhigh_left_ = 0;
    int32_t qband_left_ = 0;
    int32_t qinput_left_ = 0;

    int32_t qnotch_right_ = 0;
    int32_t qlow_right_ = 0;
    int32_t qhigh_right_ = 0;
    int32_t qband_right_ = 0;
    int32_t qinput_right_ = 0;

    q15_t qout_raw_left_ = 0;
    q15_t qout_low_left_ = 0;
    q15_t qout_high_left_ = 0;
    q15_t qout_band_left_ = 0;
    q15_t qout_notch_left_ = 0;

    q15_t qout_raw_right_ = 0;
    q15_t qout_low_right_ = 0;
    q15_t qout_high_right_ = 0;
    q15_t qout_band_right_ = 0;
    q15_t qout_notch_right_ = 0;

    static constexpr int32_t kDownsample = 1;
    static constexpr int32_t kMultMax = 1 << 22; // Safety for not overflowing

    void RecalculateDamp();
    void RecalculateDrive();
    void FinishValueSetting();

    static int32_t mult(int32_t a, int32_t b)
    {
        int32_t val = (a * b) >> 15;
        if (val > kMultMax)
        {
            return kMultMax;
        }
        if (val < -kMultMax)
        {
            return -kMultMax;
        }
        return val;
    }
};
}
