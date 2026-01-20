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

#include "Lfo.hpp"
#include "math.h"

using namespace kastle2;

void Lfo::Init(const float sample_rate)
{
    sample_rate_ = sample_rate;
    SetClockTicks(500);
    SetRatio({1, 1});
    SetFrequency(10.f);
}

void Lfo::SetFrequency(const float frequency)
{
    synced_ = false;
    // Limit frequency to 440 Hz to limit LFO crash
    native_frequency_ = freq_to_q31(std::min(frequency, 440.f), sample_rate_);
    phase_inc_ = native_frequency_ << 1;
}

void Lfo::SetClockTicks(const uint32_t clock_ticks)
{
    clock_ticks_ = clock_ticks == 0 ? 1 : clock_ticks;
    ClockedUpdate();
}

void Lfo::SetRatio(const Fraction &ratio)
{
    ratio_ = ratio;
    synced_ = true;

    ClockedUpdate();
}

FASTCODE void Lfo::ClockedUpdate()
{
    if (ratio_prev_ == ratio_ && ticks_prev_ == clock_ticks_)
    {
        return;
    }

    ticks_prev_ = clock_ticks_;
    ratio_prev_ = ratio_;

    uint32_t ticks = clock_ticks_ * ratio_.d;
    ticks /= ratio_.n;

    phase_inc_ = Q31_MAX / (ticks / 2);
}

void Lfo::Reset()
{
    phase_ = Q31_MIN;
    phase_synced_ = phase_;
}

void Lfo::SetPhase(const q31_t phase)
{
    phase_ = phase;
    phase_synced_ = phase_;
}

FASTCODE void Lfo::SyncWithClock()
{
    if (synced_)
    {
        // we cannot sync all the time by resetting, ex.: lfo is running twice slower, we need to reset every other cycle
        // for special ratios (2/3, etc.), it is easiest to multiply both of those number together to get a common divisor
        // but for ratios where the denominator is just 1 it can be skipped
        uint32_t reset_delay_count = 1;
        if (ratio_.d != 1)
        {
            reset_delay_count = ratio_.n * ratio_.d;
        }

        if (reset_counter_ >= reset_delay_count)
        {
            synced_phase_offset_ = phase_; // save the phase of main osc at clock-sync to know which way to adjust the phase to sync
            phase_synced_ = Q31_MIN;       // reset the synced phase
            reset_counter_ = 0;
        }

        reset_counter_++;
    }
}

bool Lfo::IsSynced() const
{
    return synced_;
}

void Lfo::DisableSlowingDown(const bool disable)
{
    disable_lfo_slowdown_ = disable;
}

void Lfo::Start()
{
    if (stopped_)
    {
        stopped_ = false;
        phase_ = Q31_MIN;
    }
}

FASTCODE q31_t Lfo::Process()
{
    if (stopped_)
    {
        return GetTriangleOut();
    }

    // don't use arm_add_q31 to enable overflow
    phase_ = (int32_t)phase_ + (int32_t)phase_inc_;

    if (synced_ && clock_ticks_ > kMinSyncedTicks)
    {
        phase_synced_ = (int32_t)phase_synced_ + (int32_t)phase_inc_;

        if (synced_phase_offset_ >= Q31_MAX / 40) // the unsynced lfo is ahead, slow down
        {
            phase_ = (int32_t)phase_ + ((int32_t)phase_inc_ / 1);
        }
        else if (synced_phase_offset_ <= -Q31_MAX / 40) // the unsynced lfo is behind, speed up
        {
            if (disable_lfo_slowdown_)
            {
                phase_ = (int32_t)phase_ + ((int32_t)phase_inc_ / 1);
            }
            else
            {
                phase_ = (int32_t)phase_ - ((int32_t)phase_inc_ / 2);
            }
        }

        if (q31_abs(((phase_ / 2) + Q31_HALF) - ((phase_synced_ / 2) + Q31_HALF)) < Q31_MAX / 40) // close enough, snap
        {
            phase_ = phase_synced_;
            synced_phase_offset_ = Q31_ZERO;
        }
    }

    if (stop_after_cycle_ && IsLastSample())
    {
        stop_after_cycle_ = false;
        stopped_ = true;
    }

    return GetTriangleOut();
}

FASTCODE q31_t Lfo::GetTriangleOut() const
{
    q31_t out;
    if (phase_ < Q31_ZERO)
    {
        out = (phase_ + Q31_HALF) * 2;
    }
    else
    {
        out = (Q31_HALF - phase_) * 2;
    }

    return out;
}

FASTCODE bool Lfo::GetSquareOut() const
{
    return phase_ < Q31_ZERO;
}
