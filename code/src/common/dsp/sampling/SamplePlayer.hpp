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

#include <cmath>
#include <cstddef>
#include <cstdint>
#include "common/dsp/math/math_utils.hpp"
#include "common/fastcode.hpp"

namespace kastle2
{

/**
 * @class SamplePlayer
 * @ingroup dsp_sampling
 * @brief Simple helper class for playing samples from an allocated memory. You can use ready made aliases `SamplePlayer16bit` and `SamplePlayer32bit`.
 * @author Vaclav Mach (Bastl Instruments)
 * @date 2024-06-06
 */
template <typename T>
class SamplePlayer
{
public:
    /**
     * @brief How many channels the sample has
     */
    enum Channels
    {
        MONO = 1,  ///< Mono
        STEREO = 2 ///< Interleaved stereo
    };

    /**
     * @brief Simple wrapper for the audio sample
     */
    struct Sample
    {
        const T *data = nullptr;  ///< Pointer to the sample data
        size_t length = 0;        ///< Actual samples per channel (not bytes)
        Channels channels = MONO; ///< Number of channels
    };

    /**
     * @brief Initializes the SamplePlayer with the sample rate. Expects samples to have the same rate as the system.
     * @param sample_rate The sample rate of the samples and system
     */
    void Init(float sample_rate)
    {
        Init(sample_rate, sample_rate);
    }

    /**
     * @brief Initializes the SamplePlayer with a separate playback rate and sample rate.
     * @param playback_rate The sample rate of the system
     * @param samples_rate The sample rate of the samples
     */
    void Init(float playback_rate, float samples_rate)
    {
        playback_rate_ = playback_rate;
        samples_rate_ = samples_rate;
        speed_ = 1.f;
        start_point_normal_ = 0;
        start_point_reverse_ = 0;
        hifi_ = false;
        reverse_ = false;
        SetSpeed(1.0f);
        Reset();
    }

    /**
     * @brief Processes the sample player. Returns one sample. When lo-fi is disabled, it uses linear interpolation for smoother playback.
     * @return The output sample
     */
    FASTCODE T Process()
    {
        if (!playing_ || sample_.data == nullptr || sample_.length == 0)
        {
            return 0;
        }

        if (!hifi_)
        {
            if (sample_.channels == MONO)
            {
                output_left_ = sample_.data[static_cast<size_t>(index_)];
                output_right_ = output_left_;
            }
            else if (sample_.channels == STEREO)
            {
                output_left_ = sample_.data[static_cast<size_t>(index_) * 2];
                output_right_ = sample_.data[static_cast<size_t>(index_) * 2 + 1];
            }
        }
        else
        {
            if (sample_.channels == MONO)
            {
                float index_floor = floor(index_);
                float index_ceil = ceil(index_);
                index_floor = constrain(index_floor, 0, sample_.length - 1);
                index_ceil = constrain(index_ceil, 0, sample_.length - 1);
                // Linear interpolation
                output_left_ = (1.0f - (index_ - index_floor)) * sample_.data[(size_t)index_floor] +
                               (index_ - index_floor) * sample_.data[(size_t)index_ceil];
                output_right_ = output_left_;
            }
            else if (sample_.channels == STEREO)
            {
                float index_floor = floor(index_);
                float index_ceil = ceil(index_);
                index_floor = constrain(index_floor, 0, sample_.length - 1);
                index_ceil = constrain(index_ceil, 0, sample_.length - 1);
                // Linear interpolation
                output_left_ = (1.0f - (index_ - index_floor)) * sample_.data[(size_t)index_floor * 2] +
                               (index_ - index_floor) * sample_.data[(size_t)index_ceil * 2];
                output_right_ = (1.0f - (index_ - index_floor)) * sample_.data[(size_t)index_floor * 2 + 1] +
                                (index_ - index_floor) * sample_.data[(size_t)index_ceil * 2 + 1];
            }
        }

        if (reverse_)
        {
            index_ -= index_increment_;
            if (index_ <= 0)
            {
                index_ = 0;
                playing_ = false;
            }
        }
        else
        {
            index_ += index_increment_;
            if (index_ >= sample_.length)
            {
                index_ = sample_.length;
                playing_ = false;
            }
        }

        return output_left_;
    }

    /**
     * @brief Returns the current sample for the left channel.
     * @return Left channel sample.
     */
    T inline GetLeft() const
    {
        return output_left_;
    }

    /**
     * @brief Returns the current sample for the right channel.
     * @return Right channel sample.
     */
    T inline GetRight() const
    {
        return output_right_;
    }

    /**
     * @brief Sets the sample to be played. Expects you to provide its length. Resets playhead to the beginning.
     * @param sample The sample to be played (struct)
     */
    void SetSample(Sample sample)
    {
        sample_ = sample;
        index_ = 0.0f;
    }

    /**
     * @brief Sets the playback rate of the sample player. 1 = normal speed, 2 = double speed, 0.5 = half speed, etc.
     * @param speed The speed of the playback
     */
    void SetSpeed(float speed)
    {
        speed_ = speed;
        // Calculate the index increment based on the speed and rates
        index_increment_ = speed_ * samples_rate_ / playback_rate_;
    }

    /**
     * @brief Sets wether the sample should be played normally or in reverse
     * @param reverse If the sample is to be played backwards
     */
    void SetReverse(bool reverse)
    {
        reverse_ = reverse;
    }

    /**
     * @brief Returns wether the player is playing backwards or not
     * @return returns true if the player is reversed, false if not
     */
    bool GetReverse() const
    {
        return reverse_;
    }

    /**
     * @brief Sets from which sample is should start playing
     * @param start_point the exact sample to start playback from
     */
    void SetStart(size_t start_point)
    {
        start_point_normal_ = constrain(start_point, 0, sample_.length);
        start_point_reverse_ = sample_.length - start_point_normal_;
    }

    /**
     * @brief Sets from which sample is should start playing, adjusted for changes in speed.
     * @param start_point the sample to start playback from to reach the end (adjusted for different playback speed)
     */
    void SetStartSpeedAdjusted(size_t start_point)
    {
        size_t start_point_adjusted = (float)start_point * speed_;
        start_point_normal_ = constrain(start_point_adjusted, 0, sample_.length);
        start_point_reverse_ = sample_.length - start_point_normal_;
    }

    /**
     * @brief Returns the length of the entire sample that's loaded in ticks
     * @return Length of loaded sample
     */
    size_t inline GetLength() const
    {
        return sample_.length;
    }

    /**
     * @brief Returns the length of the entire sample that's loaded in ticks, multiplied by the speed multiplier (to get a number of ticks for playback at different speeds)
     * @return Length of loaded sample adjusted for speed change
     */
    size_t inline GetLengthSpeedAdjusted() const
    {
        return (float)sample_.length / speed_;
    }

    /**
     * @brief Resets the playhead to the beginning.
     */
    void Reset()
    {
        if (reverse_)
        {
            index_ = sample_.length;
        }
        else
        {
            index_ = 0.0f;
        }
        playing_ = false;
    }

    /**
     * @brief Starts playing the sample from the beginning.
     */
    void Play()
    {
        if (reverse_)
        {
            index_ = start_point_reverse_;
        }
        else
        {
            index_ = start_point_normal_;
        }
        playing_ = true;
    }

    /**
     * @brief Returns whether the sample player is currently playing.
     * @return True if playing, false if stopped.
     */
    bool IsPlaying() const
    {
        return playing_;
    }

    /**
     * @brief Enables the hi-fi mode (linear interpolation of samples). Off by default.
     * @param hifi True enables linear interpolation using floats.
     * @note Takes a lot more CPU, so use with care!
     */
    void SetHifi(bool hifi)
    {
        hifi_ = hifi;
    }

private:
    float samples_rate_ = 0.0f;
    float playback_rate_ = 0.0f;
    Sample sample_;
    float speed_ = 1.f;
    size_t start_point_normal_ = 0;
    size_t start_point_reverse_ = 0;
    float index_increment_ = 0.0f;
    float index_ = 0.0f;
    bool playing_ = false;
    bool hifi_ = false;
    bool reverse_ = false;
    T output_left_ = 0;
    T output_right_ = 0;
};

/**
 * @brief Type alias for 16-bit sample player
 */
using SamplePlayer16bit = SamplePlayer<int16_t>;

/**
 * @brief Type alias for 32-bit sample player
 */
using SamplePlayer32bit = SamplePlayer<int32_t>;
}
