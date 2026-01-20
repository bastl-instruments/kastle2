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

#include "TestEntry.hpp"
#include "common/config.hpp"
#include "common/core/Kastle2.hpp"

using namespace kastle2;

TestEntry::TestEntry(const Config &config) : config_(config)
{
    if (config_.type == Type::FFT)
    {
        // Set up the fft
        fft_ = std::make_unique<Fft>(kFftBufferSize, SAMPLE_RATE);
        env_follower_.Init(AUDIO_LOOP_RATE);
        env_follower_.SetAttackTime(0.005f);
        env_follower_.SetReleaseTime(0.200f);
    }
}

void TestEntry::Run()
{
    if (passed_ || IsSkipped())
    {
        return;
    }

    switch (config_.type)
    {
    case Type::BUTTON:
        RunButton();
        break;
    case Type::POT:
        RunPot();
        break;
    case Type::DIGITAL_IN:
        RunDigitalInput();
        break;
    case Type::DIGITAL_COMBO:
        RunDigitalCombo();
        break;
    case Type::ANALOG_COMBO:
        RunAnalogCombo();
        break;
    case Type::DIGITAL_OUT_ANALOG_IN:
        RunDigitalOutputAnalogInput();
        break;
    case Type::FFT:
        RunFft();
        break;
    case Type::MIDI_IN:
        RunMidiIn();
        break;
    default:
        // Handle unknown type
        break;
    }
}

bool TestEntry::IsManual() const
{
    return config_.type == Type::BUTTON || config_.type == Type::POT;
}

bool TestEntry::HasPassed() const
{
    return passed_;
}

void TestEntry::RunButton()
{
    if (Kastle2::hw.JustPressed(config_.button))
    {
        passed_ = true;
    }
}

void TestEntry::RunPot()
{
    int32_t val = Hardware::kPotUndefined;
    Kastle2::hw.ReadPot(&val, config_.pot, Hardware::Layer::NORMAL);
    if (val != Hardware::kPotUndefined)
    {
        if (val < kLow)
        {
            low_reached_ = true;
        }
        if (val > kHigh)
        {
            high_reached_ = true;
        }
    }
    if (low_reached_ && high_reached_)
    {
        passed_ = true;
    }
}

void TestEntry::RunDigitalInput()
{
    bool state = Kastle2::hw.GetDigitalIn(config_.digital_input);
    if (state != prev_state_)
    {
        passed_ = true;
    }
    prev_state_ = state;
}

void TestEntry::RunDigitalCombo()
{
    // test low & high
    for (size_t i = 0; i < 2; i++)
    {
        bool val = i;
        Kastle2::hw.SetDigitalOut(config_.digital_output, val);
        sleep_ms(5);
        bool result = Kastle2::hw.GetDigitalIn(config_.digital_input);
        if (result != val)
        {
            return;
        }
    }
    passed_ = true;
}

void TestEntry::RunAnalogCombo()
{
    bool result = true;
    for (size_t i = 0; i < kAnalogTestsCount; i++)
    {
        Kastle2::hw.SetAnalogOut(config_.analog_output, kAnalogTestValues[i]);

        // Need to call it multiple times to get the correct value (2 normal ADCs, 2*8 ADCs multiplexed) etc.
        UpdateAllAdcs();

        volatile int32_t reading = Kastle2::hw.GetAnalogValue(config_.analog_input);
        volatile int32_t should_match = kAnalogTestResults[i];
        if (diff(reading, should_match) > kAnalogTestsTolerance)
        {
            result = false;
        }
    }
    passed_ = result;
}

void TestEntry::RunDigitalOutputAnalogInput()
{
    Kastle2::hw.SetDigitalOut(config_.digital_output, true);
    UpdateAllAdcs();
    volatile int32_t reading = Kastle2::hw.GetAnalogValue(config_.analog_input);
    bool result = true;
    if (reading < kHigh)
    {
        result = false;
    }
    Kastle2::hw.SetDigitalOut(config_.digital_output, false);
    UpdateAllAdcs();
    reading = Kastle2::hw.GetAnalogValue(config_.analog_input);
    if (reading > kLow)
    {
        result = false;
    }
    passed_ = result;
}

void TestEntry::RunFft()
{
    fft_->Compute();
    float dominant_frequency = fft_->GetDominantFrequency();
    if (abs(dominant_frequency - config_.match_frequency) < kFftTestFrequencyTolerance)
    {
        fft_hold_count_++;
        if (fft_hold_count_ > kFftHoldCount && env_follower_.GetEnvelope() > config_.match_volume)
        {
            passed_ = true;
        }
    }
    else
    {
        // Reset the hold count if frequency doesn't match
        fft_hold_count_ = 0;
    }
}

void TestEntry::RunMidiIn()
{
    // If we received the expected MIDI message, mark the test as passed
    if (!passed_ && midi_message_received_)
    {
        passed_ = true;
        // Reset the flag for the next test (not necessary but I'm leaving it here...)
        midi_message_received_ = false;
    }
}

void TestEntry::UpdateAllAdcs()
{
    // Allow some time for ADC to take readings
    sleep_ms(10);
}

const char *TestEntry::GetName() const
{
    return config_.name;
}

void TestEntry::TestAudioBlock(q15_t *audio_block, size_t size)
{
    if (config_.type != Type::FFT || passed_)
    {
        return;
    }

    // Find maximum value
    q15_t max = 0;
    for (size_t i = 0; i < size; i++)
    {
        size_t offset = config_.channel == Channel::LEFT ? 0 : 1;
        q15_t a = q15_abs(audio_block[2 * i + offset]);
        if (a > max)
        {
            max = a;
        }
    }

    // Track envelope follower
    env_follower_.Track(max);

    // Add to frequency measurements
    for (size_t i = 0; i < size; i++)
    {
        size_t offset = config_.channel == Channel::LEFT ? 0 : 1;
        fft_->AddSample(audio_block[2 * i + offset]);
    }
}

void TestEntry::MidiCallback(midi::Message *msg)
{
    if (config_.type != Type::MIDI_IN)
    {
        return;
    }

    // Check if the message type matches
    if (msg->GetType() == config_.midi_message_type)
    {
        midi_message_received_ = true;
    }
}

bool TestEntry::IsSkipped() const
{
    return (config_.version != Hardware::Version::COUNT && config_.version != Kastle2::hw.GetVersion());
}