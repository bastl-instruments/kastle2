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

#include "AppCalibration.hpp"
#include "common/core/Kastle2.hpp"
#include "common/utils.hpp"
#include "Sentence.hpp"

using namespace kastle2;

void AppCalibration::Init()
{
    inited_ = false;
    voltages_ok_ = false;
    is_testing_voltage_ = false;
    current_calibration_step_ = 0;

    // Disable all base features
    Kastle2::base.SetAllFeaturesEnabled(false);

    // Load reference calibrations
    ref_calibrations_ = Kastle2::hw.GetReferenceCalibrations();

    // Initialize the calibrations to refence values (necessary if we skip some steps etc)
    for (Hardware::Calibration c : EnumRange<Hardware::Calibration>())
    {
        calibrations_[c] = ref_calibrations_[c];
    }

    // Disable HW calibrations so we get the raw ADC values
    Kastle2::hw.SetCalibrationsEnabled(false);

    // Sample player initialization
    sample_player_.Init(SAMPLE_RATE, 22050);
    sample_player_.SetHifi(true);

    // Envelope follower to get nice LED action
    env_follower_.Init(SAMPLE_RATE);
    env_follower_.SetAttackTime(0.01f);
    env_follower_.SetReleaseTime(0.3f);

    if (Kastle2::memory.AreCalibrationsValid())
    {
        SetStage(Stage::ALREADY_CALIBRATED);
    }
    else
    {
        SetStage(Stage::INTRO);
    }

    inited_ = true;
}

void AppCalibration::PlaySentence(SentenceName sentence_name)
{
    current_sentence_ = &sentences_[sentence_name];
    current_sentence_->Reset();
    Sentence::Word file = current_sentence_->GetNext();
    sample_player_.SetSample(file);
    sample_player_.Play();
}

void AppCalibration::DeInit()
{
    inited_ = false;
}

void AppCalibration::CheckSentencePlayState()
{
    if (sample_player_.IsPlaying())
    {
        return;
    }

    if (!current_sentence_->IsFinished())
    {
        Sentence::Word file = current_sentence_->GetNext();
        sample_player_.SetSample(file);
        sample_player_.Play();
    }
    else
    {
        // Sentence ended...
        switch (current_stage_)
        {
        case Stage::INTRO:
            StartCalibrationSequence();
            break;
        case Stage::CALIBRATING:
            if (is_testing_voltage_)
            {
                MoveToNextCalibrationStep();
            }
            break;
        default:
            break;
        }
    }
}

void AppCalibration::SetStage(Stage stage)
{
    current_stage_ = stage;
    voltages_ok_ = true;
    is_testing_voltage_ = false;

    switch (current_stage_)
    {
    case Stage::ALREADY_CALIBRATED:
        switch (Kastle2::hw.GetVersion())
        {
        case Hardware::Version::KASTLE2:
            PlaySentence(SentenceName::KASTLE_2_ALREADY_CALIBRATED);
            break;
        case Hardware::Version::CITADEL:
            PlaySentence(SentenceName::CITADEL_ALREADY_CALIBRATED);
            break;
        }
        break;
    case Stage::CALIBRATION_CLEARED:
        PlaySentence(SentenceName::CALIBRATION_CLEARED);
        break;
    case Stage::INTRO:
        switch (Kastle2::hw.GetVersion())
        {
        case Hardware::Version::KASTLE2:
            PlaySentence(SentenceName::KASTLE_2_CALIBRATION);
            break;
        case Hardware::Version::CITADEL:
            PlaySentence(SentenceName::CITADEL_CALIBRATION);
            break;
        }
        break;
    case Stage::CALIBRATING:
        ProcessCurrentCalibrationStep();
        break;
    case Stage::SUCCESS:
        switch (Kastle2::hw.GetVersion())
        {
        case Hardware::Version::KASTLE2:
            PlaySentence(SentenceName::KASTLE_2_CALIBRATED_ENJOY);
            break;
        case Hardware::Version::CITADEL:
            PlaySentence(SentenceName::CITADEL_CALIBRATED_ENJOY);
            break;
        }
        next_time_write_calibrations_ = true;
        break;
    default:
        break;
    }
}

bool AppCalibration::CheckVoltage(int32_t value, int32_t target)
{
    voltages_ok_ = false;
    if (value < target - kTolerance)
    {
        switch (Kastle2::hw.GetVersion())
        {
        case Hardware::Version::KASTLE2:
            // Kastle 2 has non-inverting opamp so the checks are normal
            PlaySentence(SentenceName::VOLTAGE_TOO_LOW);
            break;
        case Hardware::Version::CITADEL:
            // Citadel has inverting opamp so the checks are inverted
            PlaySentence(SentenceName::VOLTAGE_TOO_HIGH);
            break;
        }
        return false;
    }
    else if (value > target + kTolerance)
    {
        switch (Kastle2::hw.GetVersion())
        {
        case Hardware::Version::KASTLE2:
            // Kastle 2 has non-inverting opamp so the checks are normal
            PlaySentence(SentenceName::VOLTAGE_TOO_HIGH);
            break;
        case Hardware::Version::CITADEL:
            // Citadel has inverting opamp so the checks are inverted
            PlaySentence(SentenceName::VOLTAGE_TOO_LOW);
            break;
        }
        return false;
    }
    PlaySentence(SentenceName::GREAT);
    voltages_ok_ = true;
    return true;
}

void AppCalibration::AudioLoop([[maybe_unused]] q15_t *input, q15_t *output, size_t size)
{
    if (!inited_)
    {
        return;
    }
    for (size_t i = 0; i < size; i++)
    {
        // code that runs each sample
        int16_t data = sample_player_.Process();

        env_follower_.Track(data);

        // apply sw volume not to blow our ears off
        data = q15_mult(data, kVolume);

        // output
        output[2 * i] = data;
        output[2 * i + 1] = data;
    }
}

void AppCalibration::UiLoop()
{
    // Check whether we should play the next word in the sentence
    CheckSentencePlayState();

    // Write calibrations?
    if (next_time_write_calibrations_)
    {
        Kastle2::memory.WriteCalibrations(calibrations_);
        next_time_write_calibrations_ = false;
    }

    // Read ADCs
    adc_readings_[Input::FREE].Add(Kastle2::hw.GetAnalogValue(Hardware::AnalogInput::PITCH_1));
    adc_readings_[Input::NOTE].Add(Kastle2::hw.GetAnalogValue(Hardware::AnalogInput::PITCH_2));

    // Extra debouncing to prevent accidental double-triggers
    // Shouldn't be necessary, but let's be sure
    bool is_valid_press = false;
    if (Kastle2::hw.JustPressed(Hardware::Button::MODE))
    {
        absolute_time_t now = get_absolute_time();
        if (is_nil_time(last_mode_press_) || absolute_time_diff_us(last_mode_press_, now) > 50000) // 50ms minimum between presses
        {
            last_mode_press_ = now;
            is_valid_press = true;
        }
    }

    if (is_valid_press)
    {
        switch (current_stage_)
        {
        case Stage::CALIBRATION_CLEARED:
        case Stage::ALREADY_CALIBRATED:
            SetStage(Stage::INTRO);
            break;
        case Stage::INTRO:
            StartCalibrationSequence();
            break;
        case Stage::CALIBRATING:
            if (is_testing_voltage_)
            {
                MoveToNextCalibrationStep();
            }
            else
            {
                // Start testing the current voltage
                is_testing_voltage_ = true;
                ProcessCurrentCalibrationStep();
            }
            break;
        case Stage::SUCCESS:
            SetStage(Stage::ALREADY_CALIBRATED);
            break;
        default:
            break;
        }
    }

    // Replay the current sentence if the user pressed the SHIFT button
    // if (Kastle2::hw.JustPressed(Hardware::Button::SHIFT))
    // {
    //     SetStage(current_stage_);
    // }

    // If the SHIFT button is pressed for more than 3 seconds, reset the calibration
    if (current_stage_ == Stage::ALREADY_CALIBRATED &&
        Kastle2::hw.PressedMillis(Hardware::Button::SHIFT) > 3000 &&
        already_cleared_ == false)
    {
        already_cleared_ = true;
        if (Kastle2::memory.ClearCalibrations())
        {
            SetStage(Stage::CALIBRATION_CLEARED);
        }
    }

    // Main color
    uint32_t color = voltages_ok_ ? 0x00AA00 : 0xAA0000;

    // Waiting for user input (yellow)
    switch (current_stage_)
    {
    case Stage::ALREADY_CALIBRATED:
    case Stage::INTRO:
    case Stage::CALIBRATING:
        if (!is_testing_voltage_)
        {
            color = 0xAAAA00;
        }
        break;
    default:
        break;
    }

    // Get envelope but keep a minimum brightness
    uint8_t brightness = u8_saturate((env_follower_.GetEnvelope() >> 6) + 80);
    color = WS2812::ApplyBrightness(color, brightness);

    // Overriding all LEDs to prevent noise from LFO led
    Kastle2::hw.SetLed(Hardware::Led::LED_1, color);
    Kastle2::hw.SetLed(Hardware::Led::LED_2, color);
    Kastle2::hw.SetLed(Hardware::Led::LED_3, color);
}

void AppCalibration::StartCalibrationSequence()
{
    current_calibration_step_ = 0;
    SetStage(Stage::CALIBRATING);
}

void AppCalibration::ProcessCurrentCalibrationStep()
{
    if (current_calibration_step_ >= calibration_sequence_.size())
    {
        SetStage(Stage::SUCCESS);
        return;
    }
    const auto &step = calibration_sequence_[current_calibration_step_];

    // Skip step if version doesn't match (unless step version is COUNT which means all versions)
    if (step.version != Hardware::Version::COUNT && step.version != Kastle2::hw.GetVersion())
    {
        current_calibration_step_++;
        ProcessCurrentCalibrationStep();
        return;
    }

    if (is_testing_voltage_)
    {
        // Test the voltage
        int32_t current_reading = adc_readings_[step.input].GetAverage();
        Hardware::Calibration cal_type = GetCalibrationType(step.input, step.voltage);
        int32_t target_value = ref_calibrations_[cal_type];
        if (CheckVoltage(current_reading, target_value))
        {
            calibrations_[cal_type] = current_reading;
        }
    }
    else
    {
        // Play instruction sentence directly
        PlayInstructionSentence(step.input, step.voltage);
    }
}

void AppCalibration::MoveToNextCalibrationStep()
{
    if (is_testing_voltage_)
    {
        if (voltages_ok_)
        {
            // Move to next step
            current_calibration_step_++;
            is_testing_voltage_ = false;
            ProcessCurrentCalibrationStep();
        }
        else
        {
            // Retry current step
            is_testing_voltage_ = false;
            ProcessCurrentCalibrationStep();
        }
    }
}

void AppCalibration::PlayInstructionSentence(Input input, Voltage voltage)
{
    // Get the voltage sample
    SamplePlayer16bit::Sample voltage_sample = voltage_samples_[voltage];

    // Create sentence directly based on input type
    Sentence::Words words;
    words.push_back(voltage_sample);

    switch (input)
    {
    case Input::NOTE:
        words.push_back(sample_to_the_pitch_note_input);
        break;
    case Input::FREE:
        words.push_back(sample_to_the_pitch_free_input);
        break;
    }
    words.push_back(sample_press_right_button_to_continue);

    // Create and store the sentence
    voltage_instruction_sentence_ = Sentence(words);
    current_sentence_ = &voltage_instruction_sentence_;
    current_sentence_->Reset();
    Sentence::Word file = current_sentence_->GetNext();
    sample_player_.SetSample(file);
    sample_player_.Play();
}

Hardware::Calibration AppCalibration::GetCalibrationType(Input input, Voltage voltage)
{
    static const EnumArray<Voltage, Hardware::Calibration> free_calibrations = {
        Hardware::Calibration::PITCH1_0V,
        Hardware::Calibration::PITCH1_1V,
        Hardware::Calibration::PITCH1_2V,
        Hardware::Calibration::PITCH1_3V,
        Hardware::Calibration::PITCH1_4V};
    static const EnumArray<Voltage, Hardware::Calibration> step_calibrations = {
        Hardware::Calibration::PITCH2_0V,
        Hardware::Calibration::PITCH2_1V,
        Hardware::Calibration::PITCH2_2V,
        Hardware::Calibration::PITCH2_3V,
        Hardware::Calibration::PITCH2_4V};
    const auto &calibrations = (input == Input::FREE) ? free_calibrations : step_calibrations;
    return calibrations[voltage];
}