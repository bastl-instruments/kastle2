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

#include "TestMode.hpp"
#include "common/core/Kastle2.hpp"
#include "common/testmode/version_samples.hpp"

using namespace kastle2;

void TestMode::Init(float sample_rate)
{
    sample_rate_ = sample_rate;

    // Set up helper things
    osc_left_.Init(sample_rate);
    osc_right_.Init(sample_rate);
    startup_env_.Init(sample_rate);
    pitch_env_.Init(sample_rate);
    sample_player_.Init(sample_rate, 11025);

    osc_left_.SetFrequency(kTestFrequencyLeft);
    osc_left_.SetWaveform(Oscillator::Waveform::SINE);
    osc_right_.SetFrequency(kTestFrequencyRight);
    osc_right_.SetWaveform(Oscillator::Waveform::SINE);
    startup_env_.SetAttackTime(0.5f);
    startup_env_.SetDecayTime(0.1f);
    startup_env_.SetSustainLevel(Q31_MAX);
    pitch_env_.SetAttackTime(0.001f);
    pitch_env_.SetDecayTime(0.2f);
    sample_player_.SetHifi(true);

    // Set up the volumes
    // When changing volumes, make sure envelope follower minimum is adjusted (kEnvFollowerMinimum)
    Kastle2::codec.SetInputGain(63);
    Kastle2::codec.SetHpVolume(48);

    // Enable the serial debug
    Kastle2::debug.SetEnabled(true);

    // Print state now
    next_time_print_state_ = get_absolute_time();

    // Versions
    version_index_ = 0;

    // Wait for a bit
    sleep_ms(200);

    // Start with "calibrated" or "intro" stage
    if (Kastle2::memory.AreCalibrationsValid())
    {
        SetStage(Stage::CALIBRATED);
    }
    else
    {
        SetStage(Stage::INTRO);
    }
}

void TestMode::Ding()
{
    pitch_env_.Trigger();
}

void TestMode::SetLeds(uint32_t color)
{
    for (Hardware::Led led : EnumRange<Hardware::Led>())
    {
        Kastle2::hw.SetLed(led, color);
    }
}

void TestMode::SetStage(Stage stage)
{
    stage_ = stage;
    Hardware::Version hw_version = Kastle2::hw.GetVersion();
    switch (stage)
    {
    case Stage::CALIBRATED:
        sample_player_.SetSample(version::calibrated);
        sample_player_.Play();
        break;
    case Stage::INTRO:
        switch (hw_version)
        {
        case Hardware::Version::KASTLE2:
            sample_player_.SetSample(version::kastle2);
            sample_player_.Play();
            break;
        case Hardware::Version::CITADEL:
            sample_player_.SetSample(version::citadel);
            sample_player_.Play();
            break;
        default:
            break;
        }
        break;
    case Stage::VERSION:
        NextVersionSample();
        break;
    case Stage::TESTING:
        SetLeds(WS2812::RED);
        Kastle2::hw.LatchLeds();
        startup_env_.Trigger();
        break;
    case Stage::SUCCESS:
        PrintState();
        Kastle2::debug.PrintLine("\nSuccess!\n");
        Kastle2::debug.Flush();
        sample_player_.SetSample(version::test_success);
        sample_player_.Play();
        break;
    }
}

bool TestMode::NextVersionSample()
{
    while (version_index_ < version_chain_.size())
    {
        SamplePlayer16bit::Sample sample = version_chain_[version_index_];
        version_index_++;
        if (sample.data == nullptr || sample.length == 0)
        {
            continue; // Skip empty samples
        }
        sample_player_.SetSample(sample);
        sample_player_.Play();
        return true;
    }
    SetStage(Stage::TESTING);
    return false;
}

void TestMode::PrintState()
{
    Kastle2::debug.PrintLine("KASTLE2 Test Mode Results:");
    for (size_t i = 0; i < tests_.size(); i++)
    {
        bool passed = tests_[i].HasPassed();
        bool skipped = tests_[i].IsSkipped();
        char buff[kLongestTestName + 10]; // Test name + status + reserve
        sprintf(buff, "%s: %s", tests_[i].GetName(), passed ? "OK" : (skipped ? "SKIP" : "FAIL"));
        Kastle2::debug.PrintLine(buff);
    }
    Kastle2::debug.Flush();

    // each 1.2s print test results
    next_time_print_state_ = make_timeout_time_ms(1200);
};

void TestMode::AudioLoop(q15_t *input, q15_t *output, size_t size)
{
    // Test audio block
    if (stage_ == Stage::TESTING)
    {
        for (size_t j = 0; j < tests_.size(); j++)
        {
            tests_[j].TestAudioBlock(input, size);
        }
    }

    for (size_t i = 0; i < size; ++i)
    {
        q15_t out_left = 0;
        q15_t out_right = 0;
        q15_t env = 0;

        switch (stage_)
        {
        case Stage::CALIBRATED:
        case Stage::INTRO:
        case Stage::VERSION:
        case Stage::SUCCESS:
            out_left = sample_player_.Process();
            out_right = out_left;
            break;
        case Stage::TESTING:
            env = q31_to_q15(startup_env_.Process());
            out_left = q15_mult(q31_to_q15(osc_left_.Process()), env);
            out_right = q15_mult(q31_to_q15(osc_right_.Process()), env);
            break;
        };
        pitch_env_value_ = pitch_env_.Process();
        output[2 * i] = out_left;
        output[2 * i + 1] = out_right;
    }

    osc_left_.SetFrequency((pitch_env_value_ >> 21) + kTestFrequencyLeft);
    osc_right_.SetFrequency((pitch_env_value_ >> 21) + kTestFrequencyRight);
}

void TestMode::UiLoop()
{
    switch (stage_)
    {
    case Stage::CALIBRATED:
        if (!sample_player_.IsPlaying())
        {
            SetStage(Stage::INTRO);
        }
        break;
    case Stage::INTRO:
        StageIntro();
        break;
    case Stage::VERSION:
        if (!sample_player_.IsPlaying())
        {
            NextVersionSample();
        }
        break;
    case Stage::TESTING:
        StageTesting();
        break;
    case Stage::SUCCESS:
        StageSuccess();
        break;
    case Stage::CITADEL_PLAYGROUND:
        StageCitadelPlayground();
        break;
    }
}

void TestMode::StageIntro()
{
    Kastle2::hw.LatchLeds();
    for (size_t i = 0; i < kTestColors.size(); i++)
    {
        for (Hardware::Led led : EnumRange<Hardware::Led>())
        {
            Kastle2::hw.SetLed(led, kTestColors[i]);
            Kastle2::hw.LatchLeds();
            sleep_ms(130);
        }
    }

    // Wait for player to finish, move to testing
    if (!sample_player_.IsPlaying())
    {
        SetStage(Stage::VERSION);
    }
}

void TestMode::StageTesting()
{
    bool all_tests_passed = true;
    bool automatic_tests_passed = true;
    for (size_t i = 0; i < tests_.size(); i++)
    {
        // Skip tests that are skipped
        if (tests_[i].IsSkipped())
        {
            continue;
        }
        bool prev_passed = tests_[i].HasPassed();
        if (prev_passed)
        {
            continue;
        }
        tests_[i].Run();
        if (tests_[i].HasPassed())
        {
            if (!prev_passed)
            {
                Ding();
            }
        }
        else
        {
            all_tests_passed = false;
            if (!tests_[i].IsManual())
            {
                automatic_tests_passed = false;
            }
        }
    }

    uint32_t color = automatic_tests_passed ? WS2812::BLUE : WS2812::RED;
    SetLeds(WS2812::ApplyBrightness(color, 255 - (pitch_env_value_ >> 24)));

    if (absolute_time_diff_us(next_time_print_state_, get_absolute_time()) > 0)
    {
        PrintState();
    }

    if (all_tests_passed)
    {
        startup_env_.SetSustainLevel(0);
        sleep_ms(300);
        PrintState();
        SetStage(Stage::SUCCESS);
    }
}

void TestMode::StageSuccess()
{
    SetLeds(WS2812::GREEN);

    // If player ended and we are on Citadel, go to playground
    if (!sample_player_.IsPlaying() && Kastle2::hw.GetVersion() == Hardware::Version::CITADEL)
    {
        SetStage(Stage::CITADEL_PLAYGROUND);
    }
}

void TestMode::StageCitadelPlayground()
{
    // The top LEDs shows the switches state on Citadel

    // CV is the left LED on Citadel
    switch (Kastle2::hw.GetFeedValue(Hardware::AnalogInput::FEED_3))
    {
    case Hardware::FeedValue::LOW:
        Kastle2::hw.SetLed(Hardware::Led::LED_1, WS2812::RED);
        break;
    case Hardware::FeedValue::UNCONNECTED:
        Kastle2::hw.SetLed(Hardware::Led::LED_1, WS2812::BLUE);
        break;
    case Hardware::FeedValue::HIGH:
        Kastle2::hw.SetLed(Hardware::Led::LED_1, WS2812::GREEN);
        break;
    }

    // Gate is the right LED on Citadel
    switch (Kastle2::hw.GetFeedValue(Hardware::AnalogInput::FEED_1))
    {
    case Hardware::FeedValue::LOW:
        Kastle2::hw.SetLed(Hardware::Led::LED_2, WS2812::RED);
        break;
    case Hardware::FeedValue::UNCONNECTED:
        Kastle2::hw.SetLed(Hardware::Led::LED_2, WS2812::BLUE);
        break;
    case Hardware::FeedValue::HIGH:
        Kastle2::hw.SetLed(Hardware::Led::LED_2, WS2812::GREEN);
        break;
    }

    // The bottom LED shows audio input jack detection
    Kastle2::hw.SetLed(Hardware::Led::LED_3,
                       Kastle2::hw.IsAudioInJackProbablyPlugged() ? WS2812::GREEN : WS2812::RED);
}