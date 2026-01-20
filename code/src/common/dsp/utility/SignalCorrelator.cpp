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

#include "SignalCorrelator.hpp"

namespace kastle2
{

void SignalCorrelator::AddSample(int16_t sample1, int16_t sample2)
{
    // add new samples
    buffer1_[arr_index_] = sample1;
    buffer2_[arr_index_] = sample2;
    arr_index_ = (arr_index_ + 1) % size_;
}

bool SignalCorrelator::FindCorrelation(int16_t threshold, Expand expand)
{
    int32_t threshold_expanded = threshold * size_; // scale threshold to the number of samples

    std::unique_ptr<int16_t[]> scaled_buffer1;
    std::unique_ptr<int16_t[]> scaled_buffer2;
    if (expand == Expand::TRUE)
    {
        scaled_buffer1 = std::make_unique<int16_t[]>(size_);
        scaled_buffer2 = std::make_unique<int16_t[]>(size_);
        
        auto max1 = *std::max_element(buffer1_.get(), buffer1_.get() + size_);
        auto max2 = *std::max_element(buffer2_.get(), buffer2_.get() + size_);
        
        for (uint32_t i = 0; i < size_; i++)
        {
            scaled_buffer1[i] = q15_div(buffer1_[i], max1);
            scaled_buffer2[i] = q15_div(buffer2_[i], max2);
        }
    }
    
    int32_t sum_diff = 0;
    for (uint32_t i = 0; i < size_; i++)
    {
        if (i < static_cast<uint32_t>(arr_index_ + arr_index_margin_) && i > static_cast<uint32_t>(arr_index_ - arr_index_margin_))
        {
            // skip the area around the current write index, it's not fully filled yet
            // I am aware of it probably not working around the buffer wrap, but it works ok as is
            continue;
        }
        int32_t diff = 0;
        if (expand == Expand::TRUE)
        {
            diff = scaled_buffer1[i] - scaled_buffer2[i];
        }
        else
        {
            diff = buffer1_[i] - buffer2_[i];
        }
        sum_diff += std::abs(diff);
        if (sum_diff > threshold_expanded)
        {
            return false;
        }
    }
    return true;
}


} // namespace kastle2
