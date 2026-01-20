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
#include <cstdint>
#include <memory>
#include "common/dsp/math/qmath.hpp"

namespace kastle2
{

/**
 * @class Fft
 * @ingroup dsp_math
 * @brief Fast Fourier Transform (FFT) class for audio frequency analysis.
 * @author Vaclav Mach (Bastl Instruments)
 * @warning This implementation is really slow on RP2040 and can't be used in the audio loop.
 * @date 2024-07-24
 */
class Fft
{
public:
    /**
     * @brief Complex number structure.
     */
    struct Complex
    {
        float real;
        float imag;
    };

    /**
     * @brief Initialize the FFT.
     * @param samples Circular buffer size for processing the samples.
     * @param sample_rate Sample rate of the audio.
     */
    Fft(size_t samples, float sample_rate);

    /**
     * @brief Adds a 16-bit sample to the FFT buffer.
     * @param sample Q15 sample.
     */
    void AddSample(q15_t sample);

    /**
     * @brief Resets the buffers.
     */
    void Reset();

    /**
     * @brief Computes the FFT. SLOW! Should be called only in UiLoop.
     */
    void Compute();

    /**
     * @brief Returns the dominant frequency (needs to be called after Compute).
     * @return Dominant frequency.
     */
    float GetDominantFrequency();

private:
    size_t samples_ = 0;
    float sample_rate_ = 0.0f;
    std::unique_ptr<Complex[]> fft_buffer_;
    std::unique_ptr<float[]> magnitude_;

    std::unique_ptr<q15_t[]> source_buffer_;
    size_t source_buffer_index_ = 0;
    size_t source_buffer_count_ = 0;

    static void PerformFft(Complex *v, int n);
    static void BitReverse(Complex *v, int n);
    static void Swap(Complex &a, Complex &b);
};
}
