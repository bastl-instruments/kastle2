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

#include "Sequencer.hpp"
#include "common/dsp/math/bit_utils.hpp"
#include "common/dsp/math/math_utils.hpp"

using namespace kastle2;

#define SWING_EVEN_CONTINOUS 1  // Inverted each step number (can be unpredictable when turning the swing on/off)
#define SWING_EVEN_CONSISTENT 2 // Calculated from actual step number (always reflects the steps)
#define SWING_EVEN SWING_EVEN_CONSISTENT

void Sequencer::Init(const size_t length)
{
    reaching_next_cycle_ = false;
    SetLength(length);

    // Fill all triggers up to kMaxLength
    TriggerGenerator::FillTriggers(triggers_, kMaxLength);

    // Generate random values
    for (size_t i = 0; i < kMaxLength; i++)
    {
        cv_bits_[i] = rand() & 1u;
    }
}

void Sequencer::SetLength(const size_t length)
{
    length_ = length > kMaxLength ? kMaxLength : (length < kMinLength ? kMinLength : length);
    Reset();
}

size_t Sequencer::GetLength() const
{
    return length_;
}

void Sequencer::Reset()
{
    current_step_ = 0;
    reaching_next_cycle_ = false;

    // A swing step still waiting to fire belongs to the position we just left, keeping
    // it would advance us off step 0 right after the reset.
    CancelSwingStep();
    swing_even_step_ = false;
    humanize_index_ = 0;

    UpdateCvOutput();
    UpdateTriggerOutput();
}

void Sequencer::RealignTo(const size_t steps)
{
    current_step_ = steps % length_;

    // The realigned position supersedes a swing step still waiting to fire, letting it
    // through would immediately advance us one step past where we were told to be.
    CancelSwingStep();

    UpdateCvOutput();
    UpdateTriggerOutput();
}

void Sequencer::NextStep(const Feed trigger_feed, const Feed cv_feed)
{
    reaching_next_cycle_ = false;

    // This call consumes whatever swing step was scheduled, no matter who triggered it.
    // Without this a step fired ahead of its delay (tempo jump) would still be fired a
    // second time by ProcessSwingTick().
    CancelSwingStep();

    current_step_ = (current_step_ + 1) % length_;

    // Read current trigger
    bool new_trigger = triggers_[current_step_];

    // Change it
    switch (trigger_feed)
    {
    case Feed::INVERT:
        new_trigger = !new_trigger;
        break;
    case Feed::RANDOM:
        new_trigger = rand() & 1;
        break;
    }

    // Write it back
    triggers_[current_step_] = new_trigger;
    UpdateTriggerOutput();

    // Read current CV bit
    bool new_cv_bit = cv_bits_[current_step_];

    // Change it
    switch (cv_feed)
    {
    case Feed::INVERT:
        new_cv_bit = !new_cv_bit;
        break;
    case Feed::RANDOM:
        new_cv_bit = rand() & 1;
        break;
    }

    // Write it back
    cv_bits_[current_step_] = new_cv_bit;
    UpdateCvOutput();
}

void Sequencer::UpdateTriggerOutput()
{
    // samples and holds the current trigger value (for the duration of the step)
    trigger_output_ = triggers_[current_step_];
}

void Sequencer::UpdateCvOutput()
{
    // Reaching next cycle is used to have CV ready before the actual clock
    uint32_t step = current_step_;
    if (reaching_next_cycle_)
    {
        step = (current_step_ + 1) % length_;
    }

    // Make CV output index
    uint8_t map_index = 0;
    bit_write(map_index, 0, cv_bits_[(step + 0) % length_]);
    bit_write(map_index, 1, cv_bits_[(step + 3) % length_]);
    bit_write(map_index, 2, cv_bits_[(step + 5) % length_]);

    // Use the value from PWM map as the result
    cv_output_ = kVoltageMap[map_index];
}

bool Sequencer::ReachingNextCycle()
{
    if (reaching_next_cycle_)
    {
        return false;
    }

    reaching_next_cycle_ = true;
    UpdateCvOutput();
    return true;
}

bool Sequencer::GetTriggerOutput() const
{
    // While a swing step is pending the audible trigger has to be silenced;
    // it will be re-issued by the delayed NextStep() call.
    if (swing_pending_)
    {
        return false;
    }
    return trigger_output_;
}

void Sequencer::SetNowTrigger()
{
    triggers_[current_step_] = true;
}

void Sequencer::SetTrigger(const size_t step, const bool value)
{
    if (step < length_)
    {
        triggers_[step] = value;
    }
}

void Sequencer::GenerateTriggers(const TriggerGenerator::Type type, const q15_t input)
{
    TriggerGenerator::GenerateTriggers(type, input, triggers_, length_);
}

void Sequencer::GenerateTriggersUsingTable(const q15_t input)
{
    TriggerGenerator::PatternTriggerGenerator(input, triggers_, length_, generator_table_);
}

void Sequencer::SetTriggerGeneratorTable(std::span<const uint32_t> table)
{
    generator_table_ = table;
}

uint32_t Sequencer::GetCvOutput() const
{
    return cv_output_;
}

void Sequencer::SetSwing(float value)
{
    swing_value_ = value;

    // Pre-compute the swing type and the normalized strength so the
    // audio thread never has to do float compares or multiplies.
    if (value > kSwingActiveThreshold)
    {
        swing_type_ = SwingType::SWING;
        // Normalize so swing_amount_ reaches 1.0 at value == 1.0
        swing_amount_ = (value - kSwingActiveThreshold) / (1.0f - kSwingActiveThreshold);
    }
    else if (value < kSwingHumanizeThreshold)
    {
        swing_type_ = SwingType::HUMANIZE;
        // Normalize so swing_amount_ reaches 1.0 at value == 0.0
        swing_amount_ = (kSwingHumanizeThreshold - value) / kSwingHumanizeThreshold;
    }
    else
    {
        swing_type_ = SwingType::NONE;
        swing_amount_ = 0.0f;
    }
}

void Sequencer::ScheduleSwingStep(uint32_t step_ticks)
{
    uint32_t delay = 0;

    switch (swing_type_)
    {
    case SwingType::SWING:
    {
// Handles different swing "even step" methods:
#if SWING_EVEN == SWING_EVEN_CONTINOUS
        const bool even_step = (swing_even_step_);
#elif SWING_EVEN == SWING_EVEN_CONSISTENT
        const bool even_step = (current_step_ % 2 == 0);
#endif
        if (even_step)
        {
            // Cap delay at half a step so even steps stay before the midpoint
            // to the next odd step (musically meaningful swing range).
            delay = static_cast<uint32_t>(step_ticks * swing_amount_ * 0.5f);
        }
    }
    break;

    case SwingType::HUMANIZE:
    {
        const float factor = kHumanizePattern[humanize_index_];
        humanize_index_ = (humanize_index_ + 1) % kHumanizePattern.size();
        delay = static_cast<uint32_t>(step_ticks * swing_amount_ * factor);
        break;
    }

    case SwingType::NONE:
    default:
        break;
    }

    swing_delay_counter_ = delay;
    swing_pending_ = true;
    swing_even_step_ = !swing_even_step_;
}

bool Sequencer::ProcessSwingTick()
{
    if (!swing_pending_)
    {
        return false;
    }

    if (swing_delay_counter_ > 0)
    {
        swing_delay_counter_--;
        return false;
    }

    swing_pending_ = false;
    return true;
}