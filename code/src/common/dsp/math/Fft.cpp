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

#include "Fft.hpp"
#include <numbers>

using namespace kastle2;

Fft::Fft(size_t samples, float sample_rate)
    : samples_(samples), sample_rate_(sample_rate)
{
    source_buffer_ = std::make_unique<q15_t[]>(samples);
    fft_buffer_ = std::make_unique<Complex[]>(samples);
    magnitude_ = std::make_unique<float[]>(samples / 2);
    Reset();
}

void Fft::Reset()
{
    source_buffer_count_ = 0;
    source_buffer_index_ = 0;
}

void Fft::AddSample(q15_t sample)
{
    source_buffer_[source_buffer_index_] = sample;
    source_buffer_index_ = (source_buffer_index_ + 1) % samples_;
    if (source_buffer_count_ < samples_)
    {
        source_buffer_count_++;
    }
}

void Fft::Compute()
{
    // Convert the q15_t samples to float numbers for FFT
    for (size_t i = 0; i < source_buffer_count_; i++)
    {
        fft_buffer_[i].real = q15_to_float(source_buffer_[i]);
        fft_buffer_[i].imag = 0.0;
    }

    // Perform FFT
    PerformFft(fft_buffer_.get(), samples_);

    // Calculate magnitude of the FFT output
    for (size_t i = 0; i < samples_ / 2; i++)
    {
        magnitude_[i] = sqrtf(fft_buffer_[i].real * fft_buffer_[i].real + fft_buffer_[i].imag * fft_buffer_[i].imag);
    }
}

float Fft::GetDominantFrequency()
{
    // Find the bin with the highest magnitude_
    float maxMagnitude = 0;
    int maxIndex = 0;
    for (size_t i = 1; i < samples_ / 2; i++)
    { // Start at 1 to skip the DC component
        if (magnitude_[i] > maxMagnitude)
        {
            maxMagnitude = magnitude_[i];
            maxIndex = i;
        }
    }

    // Calculate the frequency of the bin with the highest magnitude_
    return (float)maxIndex * (sample_rate_ / samples_);
}

void Fft::PerformFft(Complex *v, int n)
{
    // Bit reversal
    BitReverse(v, n);

    // Cooley-Tukey Fft
    for (int length = 2; length <= n; length <<= 1)
    {
        float angle = -2.0f * std::numbers::pi / length;
        Complex wlen = {cosf(angle), sinf(angle)};

        for (int i = 0; i < n; i += length)
        {
            Complex w = {1.0f, 0.0f};
            for (int j = 0; j < length / 2; ++j)
            {
                Complex u = v[i + j];
                Complex t = {w.real * v[i + j + length / 2].real - w.imag * v[i + j + length / 2].imag,
                             w.real * v[i + j + length / 2].imag + w.imag * v[i + j + length / 2].real};

                v[i + j] = {u.real + t.real, u.imag + t.imag};
                v[i + j + length / 2] = {u.real - t.real, u.imag - t.imag};

                Complex temp = {w.real * wlen.real - w.imag * wlen.imag,
                                w.real * wlen.imag + w.imag * wlen.real};
                w = temp;
            }
        }
    }
}

void Fft::BitReverse(Complex *v, int n)
{
    int j = 0;
    for (int i = 0; i < n; ++i)
    {
        if (i < j)
        {
            Swap(v[i], v[j]);
        }
        int m = n >> 1;
        while (j >= m && m >= 2)
        {
            j -= m;
            m >>= 1;
        }
        j += m;
    }
}

void Fft::Swap(Complex &a, Complex &b)
{
    Complex temp = a;
    a = b;
    b = temp;
}