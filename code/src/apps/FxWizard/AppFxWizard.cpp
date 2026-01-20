/*
MIT License

Copyright (c) 2024 Marek Mach (Bastl Instruments)
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

#include "AppFxWizard.hpp"
#include "common/core/Kastle2.hpp"
#include "common/core/MultiCore.hpp"
#include "common/core/UserDataFile.hpp"
#include "common/utils.hpp"
#include "FxWizardParameterMaps.hpp"

#include "cc.hpp"

// Needs AdvancedDynamicDelayLine::SetFastSlew to be implemented.
// When that was implemented, it caused crashes in some modes.
#define CHECK_DELAY_JUMP 0

using namespace kastle2;

void AppFxWizard::Init()
{
    inited_ = false;

    // Load custom scales and rhythms (if available)
    if (LoadFile())
    {
        // Update rhythm generator with the loaded rhythms
        Kastle2::base.GetSequencer().SetTriggerGeneratorTable(std::span{
            settings_file_.rhythms, settings_file_.num_rhythms});

        // Set sequencer length
        Kastle2::base.GetSequencer().SetLength(settings_file_.sequencer_length);
    }
    else
    {
        // Default length
        Kastle2::base.GetSequencer().SetLength(kFxSequencerLength);

        // Default rhythms are just the classic ones from Kastle2_params.hpp
        // ...
    }

    // Disable audio chain, since we process the audio ourselves
    Kastle2::base.SetFeatureEnabled(Base::Feature::AUDIO_CHAIN, false);

    // Fake Tempo and LFO LEDs
    Kastle2::base.GetFakeBlinker().SetEnabled(true);
    // If under 30 cycles
    Kastle2::base.GetFakeBlinker().SetTempoThreshold(30);
    Kastle2::base.GetFakeBlinker().SetLfoThreshold(q31(0.05f));

    // Initialize the parameters
    input_left_ = Q15_ZERO;
    input_right_ = Q15_ZERO;
    output_left_ = Q15_ZERO;
    output_right_ = Q15_ZERO;
    cv_step_ = 0;
    cv_mode_ = 0;
    cv_mode_prev_ = 0;
    cv_mode_in_use_ = 0;

    trigger_trigger_ = false;
    clock_trigger_ = false;
    mode_sh_trigger_ = false;
    time_sh_trigger_ = false;

    feedback_delay_left_ = std::make_unique<AdvancedDynamicDelayLine<q15least_t>>(2000);  // 44ms
    feedback_delay_right_ = std::make_unique<AdvancedDynamicDelayLine<q15least_t>>(2000); // 44ms

    feedback_clip_.SetDrive(Q15_MAX);

    feedback_filter_left_.Init(SAMPLE_RATE);
    feedback_filter_left_.SetFrequency(kFeedbackFilterLeftFreq);
    feedback_filter_left_.SetResonance(0.f, Svf::ForceValue::TRUE);
    feedback_filter_left_.SetType(Svf::Type::HIGHPASS);
    feedback_filter_left_.SetDrive(1.f);
    feedback_filter_right_.Init(SAMPLE_RATE);
    feedback_filter_right_.SetFrequency(kFeedbackFilterRightFreq);
    feedback_filter_right_.SetResonance(0.f, Svf::ForceValue::TRUE);
    feedback_filter_right_.SetType(Svf::Type::HIGHPASS);
    feedback_filter_right_.SetDrive(1.f);

    feedback_filter_lp_left_.Init(SAMPLE_RATE);
    feedback_filter_lp_left_.SetFrequency(kFeedbackFilterLpLeftFreq);
    feedback_filter_lp_left_.SetResonance(0.f, Svf::ForceValue::TRUE);
    feedback_filter_lp_left_.SetType(Svf::Type::LOWPASS);
    feedback_filter_lp_left_.SetDrive(1.f);
    feedback_filter_lp_right_.Init(SAMPLE_RATE);
    feedback_filter_lp_right_.SetFrequency(kFeedbackFilterLpRightFreq);
    feedback_filter_lp_right_.SetResonance(0.f, Svf::ForceValue::TRUE);
    feedback_filter_lp_right_.SetType(Svf::Type::LOWPASS);
    feedback_filter_lp_right_.SetDrive(1.f);

    delay_clip_.Init(SAMPLE_RATE);
    feedback_volume_ = Q15_ZERO;
    feedback_delay_ = 0;
    global_dry_wet_ = Q15_ZERO;
    global_dry_wet_queue_ = Q15_ZERO;
    trigger_blink_counter = 0;

    dj_filter_left_.Init(SAMPLE_RATE);
    dj_filter_right_.Init(SAMPLE_RATE);
    dj_filter_left_.SetResonance(0.0f, Svf::ForceValue::TRUE);
    dj_filter_right_.SetResonance(0.0f, Svf::ForceValue::TRUE);

    crusher_threshold_ = Q15_ZERO;
    crusher_left_ = Q15_ZERO;
    crusher_right_ = Q15_ZERO;
    crusher_frequency_ = 0.f;
    crusher_stereo_mix_ = Q15_ZERO;
    crusher_env_.Init(SAMPLE_RATE);
    crusher_env_.SetAttackTime(kCrusherEnvAttack);
    crusher_env_.SetDecayTime(1.f);
    crusher_env_.SetSustainLevel(Q31_ZERO);
    crusher_env_.SetReleaseTime(0.f);
    crusher_env_mod_ = Q15_ZERO;
    crusher_left_frequency_ = Q31_ZERO;
    crusher_right_frequency_ = Q31_ZERO;
    crusher_xor_ = 0;

    flanger_depth_ = Q15_ZERO;
    flanger_dry_wet_ = Q15_ZERO;
    flanger_frequency_ = 0.f;
    flanger_stereo_mix_ = Q15_ZERO;

    panner_clip_.Init(SAMPLE_RATE);
    panner_clip_.DisableVolumeCompensation(true);
    panner_depth_ = Q15_ZERO;
    panner_frequency_ = 0.f;
    panner_stereo_mix_ = Q15_ZERO;

    freezer_length_left_ = 0;
    freezer_length_right_ = 0;
    freezer_grab_continuous_ = false;
    freezer_grab_continuous_prev_ = false;
    freezer_grab_buffer_ = false;
    freezer_grab_prev_ = false;
    freezer_grab_cnt_ = 0;
    freezer_clock_triggered_ = false;
    freezer_wait_for_clock_ = false;
    freezer_feedback_ = Q15_ZERO;
    freezer_feedback_dry = Q15_ZERO;
    freezer_record_length_ = 0;
    freezer_can_use_feedback_prev_ = false;
    freezer_start_ptr_left_ = 0;
    freezer_start_ptr_right_ = 0;

    replayer_dry_wet_ = Q15_ZERO;
    replayer_oversampling_ = 0;
    replayer_hold_ = false;
    replayer_stereo_oversampling_ = 0;
    replayer_filling_ = true;

    pitcher_frequency_ = 0.f;
    pitcher_dry_wet_ = Q15_ZERO;
    pitcher_depth_ = Q15_ZERO;
    pitcher_stereo_mix_ = Q15_ZERO;
    pitcher_env_.Init(AUDIO_LOOP_RATE);
    pitcher_env_.SetAttackTime(kPitcherEnvAttack);
    pitcher_env_.SetDecayTime(kPitcherEnvDecay);
    pitcher_env_.SetSustainLevel(Q31_ZERO);
    pitcher_env_.SetReleaseTime(0.f);
    pitcher_env_depth_ = Q31_ZERO;
    pitcher_depth_modulated_ = Q31_ZERO;

    shifter_direction_ = false;
    shifter_dry_ = Q15_ZERO;
    shifter_frequency_ = 0.f;
    shifter_env_.Init(AUDIO_LOOP_RATE);
    shifter_env_.SetAttackTime(kShifterEnvAttack);
    shifter_env_.SetDecayTime(kShifterEnvDecay);
    shifter_env_.SetSustainLevel(Q31_ZERO);
    shifter_env_.SetReleaseTime(0.f);
    shifter_env_mod_ = Q15_ZERO;
    shifter_left_frequency_ = Q31_ZERO;
    shifter_right_frequency_ = Q31_ZERO;

    delay_left_ = std::make_unique<AdvancedDynamicDelayLine<q15least_t>>(50800);
    delay_right_ = std::make_unique<AdvancedDynamicDelayLine<q15least_t>>(50800);
    delay_compressor_.Init(SAMPLE_RATE / delay_peak_counter_max_);
    delay_compressor_.SetAttackTime(50.f / 1000.f);
    delay_compressor_.SetReleaseTime(100.f / 1000.f);
    delay_wet_ = Q15_ZERO;
    delay_dry_ = Q15_ZERO;
    delay_feedback_ = Q15_ZERO;
    delay_peak_counter_ = 0;
    delay_peak_ = Q15_ZERO;
    delay_stereo_ = 0;
    delay_length_ = 0;
    delay_ticks_time_ = 0;
    delay_ticks_time_last_ = 0;
    delay_ticks_time_last_prev_ = 0;
    delay_synced_ = false;
    delay_synced_first_measurement_ = false;
    delay_synced_highest_index_ = 0;
    for (auto &time : delay_synced_times_)
    {
        time = 0;
    }
    delay_time_left_prev_ = 0;
    delay_time_right_prev_ = 0;

    lfo_left_.Init(SAMPLE_RATE);
    lfo_right_.Init(SAMPLE_RATE);

    slicer_env_left_.Init(SAMPLE_RATE);
    slicer_env_left_.SetAttackTime(0.01f);
    slicer_env_left_.SetSustainLevel(Q31_ZERO);
    slicer_env_left_.SetReleaseTime(0.f);
    slicer_env_right_.Init(SAMPLE_RATE);
    slicer_env_right_.SetAttackTime(0.01f);
    slicer_env_right_.SetSustainLevel(Q31_ZERO);
    slicer_env_right_.SetReleaseTime(0.f);
    slicer_pattern_left_ = 0;
    slicer_pattern_right_ = 0;
    slicer_step_ = 0;
    slicer_clock_trigger = false;
    slicer_decay_ = 0.f;
    slicer_lfo_prev_ = false;
    slicer_stereo_ = 0;
    slicer_chance_ = 0;
    slicer_flip_bit_ = false;

    // Normal layer
    pots_[Pot::TIME_MOD] = FancyPot::Create(
        {.pot = Hardware::Pot::POT_1,
         .layer = Hardware::Layer::NORMAL,
         .initial_value = POT_HALF,
         .midi_cc = cc::TIME_MOD,
         .deadzone = true});
    pots_[Pot::FEEDBACK_MOD] = FancyPot::Create(
        {.pot = Hardware::Pot::POT_2,
         .layer = Hardware::Layer::NORMAL,
         .initial_value = POT_HALF,
         .midi_cc = cc::FEEDBACK_MOD,
         .deadzone = true});
    pots_[Pot::AMOUNT] = FancyPot::Create(
        {.pot = Hardware::Pot::POT_4,
         .layer = Hardware::Layer::NORMAL,
         .initial_value = POT_HALF,
         .midi_cc = cc::AMOUNT});
    pots_[Pot::TIME] = FancyPot::Create(
        {.pot = Hardware::Pot::POT_5,
         .layer = Hardware::Layer::NORMAL,
         .initial_value = POT_HALF,
         .midi_cc = cc::TIME,
         .freeze = true});
    pots_[Pot::FEEDBACK] = FancyPot::Create(
        {.pot = Hardware::Pot::POT_6,
         .layer = Hardware::Layer::NORMAL,
         .initial_value = POT_MIN,
         .midi_cc = cc::FEEDBACK});

    // Shift layer
    pots_[Pot::STEREO] = FancyPot::Create(
        {.pot = Hardware::Pot::POT_2,
         .layer = Hardware::Layer::SHIFT,
         .initial_value = POT_MIN,
         .midi_cc = cc::STEREO,
         .freeze = true});
    pots_[Pot::AMOUNT_MOD] = FancyPot::Create(
        {.pot = Hardware::Pot::POT_4,
         .layer = Hardware::Layer::SHIFT,
         .initial_value = POT_MAX,
         .midi_cc = cc::AMOUNT_MOD});
    pots_[Pot::FILTER] = FancyPot::Create(
        {.pot = Hardware::Pot::POT_6,
         .layer = Hardware::Layer::SHIFT,
         .initial_value = POT_HALF,
         .midi_cc = cc::FILTER,
         .deadzone = true});

    // Mode layer
    pots_[Pot::MODE_MOD] = FancyPot::Create(
        {.pot = Hardware::Pot::POT_4,
         .layer = Hardware::Layer::MODE,
         .initial_value = POT_MAX,
         .midi_cc = cc::MODE_MOD});

    // Init pots
    for (auto &pot : pots_)
    {
        pot->Init(AUDIO_LOOP_RATE);
    }

    // This disables the next change when the pots are moved
    // or when the current layer time is over the specified number of ticks
    mode_selector_.DisableNextChangeWhen(pots_, kModeShortPressUnder);

    // Set up the second core
    second_core_processed_samples_ = 0;

    // Set up MODE
    mode_selector_.Init();
    mode_ = static_cast<Mode>(mode_selector_.GetMode());
    ModeInit();

    inited_ = true;
}

void AppFxWizard::DeInit()
{
    inited_ = false;
}

FASTCODE void AppFxWizard::AudioLoop(q15_t *input, q15_t *output, size_t size)
{
    if (!inited_)
    {
        return;
    }

    // Autofreeze pots
    for (auto &pot : pots_)
    {
        pot->Process();
    }
    mode_selector_.Process();

    // Process clock triggers etc.
    if (Kastle2::base.GetClock().IsNowTrigger())
    {
        clock_trigger_ = true;
    }

    // Process triggers
    if (trigger_detect_.Process(Kastle2::hw.GetTriggerIn()))
    {
        trigger_trigger_ = true;
    }

    size_t elapsed_ticks = Kastle2::base.GetClock().GetCurrentTicks();
    // Approx 5ms after clock - time SH
    if (elapsed_ticks == 6)
    {
        time_sh_trigger_ = true;
    }
    // Approx 20ms after clock - mode SH
    else if (elapsed_ticks == 23)
    {
        mode_sh_trigger_ = true;
    }

    // Set data for the second core
    input_buffer_ = input;
    output_buffer_ = output;
    buffer_size_ = size;

    // Tell the second core we're starting a new block
    MultiCore::SendMessage(MultiCore::MessageType::BEGIN);

    // Do the processing for each sample
    for (size_t i = 0; i < size; i++)
    {
        input_left_ = input[2 * i];
        input_right_ = input[2 * i + 1];
        // Because of performance requirements, some things need to be calculated only a few times per buffer
        sample_being_processed_ = i;
        if (mode_ != Mode::DELAY && mode_ != Mode::REPLAYER && mode_ != Mode::FREEZER)
        {
            input_left_ = q15_add(
                q15_mult(q15_div(feedback_delay_left_->Read(), 27853), feedback_volume_),
                q15_mult(input_left_, (Q15_MAX - feedback_volume_) / 2 + Q15_HALF));
            input_right_ = q15_add(
                q15_mult(q15_div(feedback_delay_right_->Read(), 27853), feedback_volume_),
                q15_mult(input_right_, (Q15_MAX - feedback_volume_) / 2 + Q15_HALF));
        }

        // Output resetting just to be sure there is no junk there
        output_left_ = 0;
        output_right_ = 0;

        WorkLongDelay();

        switch (mode_)
        {
        case Mode::CRUSHER:
            ModeCrusher();
            break;

        case Mode::FLANGER:
            ModeFlanger();
            break;

        case Mode::PANNER:
            ModePanner();
            break;

        case Mode::FREEZER:
            ModeFreezer();
            break;

        case Mode::REPLAYER:
            ModeReplayer();
            output_left_ = q15_add(
                q15_mult(q15_div(feedback_delay_left_->Read(), 27853), feedback_volume_),
                q15_mult(output_left_, (Q15_MAX - feedback_volume_) / 2 + Q15_HALF));
            output_right_ = q15_add(
                q15_mult(q15_div(feedback_delay_right_->Read(), 27853), feedback_volume_),
                q15_mult(output_right_, (Q15_MAX - feedback_volume_) / 2 + Q15_HALF));
            break;

        case Mode::PITCHER:
            ModePitcher();
            break;

        case Mode::DELAY:
            ModeDelay();
            break;

        case Mode::SLICER:
            ModeSlicer();
            break;

        case Mode::SHIFTER:
            ModeShifter();
            break;
        }

        // Write the output.
        // The second core will continue with the output.
        // See above that output_buffer_ = output
        output[2 * i] = output_left_;
        output[2 * i + 1] = output_right_;

        // Waiting for dry/wet change?
        CheckDryWet();

        // Tell the second core it can process this sample
        MultiCore::SendMessage(MultiCore::MessageType::SAMPLE_REQUEST, i);
    }

    // Process counters and timers
    if (trigger_blink_counter > 0)
    {
        trigger_blink_counter--;
    }

    // Wait for samples to finish processing
    MultiCore::WaitForMessage(MultiCore::MessageType::DONE);
}

FASTCODE void AppFxWizard::SecondCoreProcess(size_t index)
{
    // Read samples from the buffer
    q15_t left = output_buffer_[2 * index];
    q15_t right = output_buffer_[2 * index + 1];

    // Apply DJ filter
    left = dj_filter_left_.Process(left);
    right = dj_filter_right_.Process(right);

    // Compensate filter volume drop
    left = q15_div(left, Q15_HALF);
    right = q15_div(right, Q15_HALF);

    // Write back feedback (after DJ filter)
    if (mode_ != Mode::DELAY && mode_ != Mode::FREEZER)
    {
        // feedback_delay_left_->Write(feedback_filter_lp_left_.Process(feedback_filter_left_.Process(feedback_clip_.Process(left))));
        // feedback_delay_right_->Write(feedback_filter_lp_right_.Process(feedback_filter_right_.Process(feedback_clip_.Process(right))));

        feedback_delay_left_->Write(feedback_filter_left_.Process(feedback_filter_lp_left_.Process(feedback_clip_.Process(left))));
        feedback_delay_right_->Write(feedback_filter_right_.Process(feedback_filter_lp_right_.Process(feedback_clip_.Process(right))));

        // feedback_delay_left_->Write(feedback_clip_.Process(feedback_filter_left_.Process(feedback_filter_lp_left_.Process(left))));
        // feedback_delay_right_->Write(feedback_clip_.Process(feedback_filter_right_.Process(feedback_filter_lp_right_.Process(right))));
    }

    // Do DryWet and write the processed samples back to the output buffer
    output_buffer_[2 * index] = q15_add(q15_mult(input_buffer_[2 * index], Q15_MAX - global_dry_wet_), q15_mult(left, global_dry_wet_));
    output_buffer_[2 * index + 1] = q15_add(q15_mult(input_buffer_[2 * index + 1], Q15_MAX - global_dry_wet_), q15_mult(right, global_dry_wet_));
}

FASTCODE void AppFxWizard::SecondCoreWorker()
{
    while (inited_)
    {
        if (MultiCore::HasMessage())
        {
            // Monitoring second core performance
            Kastle2::hw.SetDebugPin(1, 1);

            MultiCore::Message m = MultiCore::GetMessage();
            switch (m.type)
            {
            case MultiCore::MessageType::BEGIN:
                second_core_processed_samples_ = 0;
                break;
            case MultiCore::MessageType::SAMPLE_REQUEST:
                SecondCoreProcess(m.data);
                second_core_processed_samples_++;
                if (second_core_processed_samples_ == buffer_size_)
                {
                    MultiCore::SendMessage(MultiCore::MessageType::DONE);
                }
                break;
            }

            Kastle2::hw.SetDebugPin(1, 0);
        }
    }
}

void AppFxWizard::SetDryWet(q15_t dry_wet)
{
#if NEAR_ZERO_DRYWET
    // Small change, ignore
    if (q15_abs(global_dry_wet_queue_ - dry_wet) < 8)
    {
        return;
    }
    global_dry_wet_queue_ = dry_wet;
    // if after X samples the signal not in range, change the dry/wet
    dry_wet_change_counter_ = kDryWetChangeCounterMax;
#else
    global_dry_wet_ = dry_wet;
#endif
}

void AppFxWizard::CheckDryWet()
{
#if NEAR_ZERO_DRYWET
    if (global_dry_wet_ != global_dry_wet_queue_ && (((q15_abs(output_left_ - input_left_) < kLowSignal) &&
                                                      (q15_abs(output_right_ - input_right_) < kLowSignal)) ||
                                                     dry_wet_change_counter_ == 0))
    {
        global_dry_wet_ = global_dry_wet_queue_;
    }

    if (dry_wet_change_counter_ > 0)
    {
        dry_wet_change_counter_--;
    }
#endif
}

void AppFxWizard::UiLoop()
{
    for (auto &pot : pots_)
    {
        pot->ReadValue();
    }
    mode_selector_.ReadValue();

    // Enable zero cross update if volume not low
    Kastle2::codec.SetZeroCrossUpdate(Kastle2::base.GetInputEnvelopeFollower().GetEnvelope() > q15(0.05f));

    // Get clock and sh triggers
    bool clock_triggered = clock_trigger_;
    clock_trigger_ = false;

    bool time_sh_triggered = time_sh_trigger_;
    time_sh_trigger_ = false;

    bool mode_sh_triggered = mode_sh_trigger_;
    mode_sh_trigger_ = false;

    // Get trigger state
    bool trigger_triggered = trigger_trigger_;
    trigger_trigger_ = false;

    // Get the mode CV value
    cv_mode_ = Kastle2::hw.GetAnalogValue(CV_MODE);

    // Check for jump
    bool cv_mode_jumped = (diff(cv_mode_, cv_mode_prev_) > kModeCvJumped);

    // Store prev value
    cv_mode_prev_ = cv_mode_;

    // Force the mode selector to re-read the ADC
    // This is done so the mode changes in sync with the tempo
    if ((mode_sh_triggered && (diff(cv_mode_, cv_mode_in_use_) > kModeCvChanged)) || cv_mode_jumped)
    {
        mode_selector_.TriggerAdcRead();
        cv_mode_in_use_ = cv_mode_;
    }

    // S&H stepped CV on clock trigger
    if (time_sh_triggered)
    {
        cv_step_ = Kastle2::hw.GetAnalogValue(CV_STEP);
    }

    int32_t feedback = pots_[Pot::FEEDBACK]->GetValue();
    feedback += apply_pot_mod_attenuvert(Kastle2::hw.GetAnalogValue(CV_FEEDBACK), pots_[Pot::FEEDBACK_MOD]->GetValue());
    feedback = constrain(feedback, POT_MIN, POT_MAX);

    feedback_delay_left_->SetDelay(feedback_delay_);
    feedback_delay_right_->SetDelay(feedback_delay_);

    int32_t filter = pots_[Pot::FILTER]->GetValue();
    int32_t filter_crossfade = curve_map(filter, kMapDjFilter);
    dj_filter_left_.SetCrossfade(filter_crossfade);
    dj_filter_right_.SetCrossfade(filter_crossfade);

    int32_t dry_wet = pots_[Pot::AMOUNT]->GetValue();
    dry_wet += apply_pot_mod_attenuvert(Kastle2::hw.GetAnalogValue(CV_DRYWET), pots_[Pot::AMOUNT_MOD]->GetValue());
    dry_wet = constrain(dry_wet, POT_MIN, POT_MAX);

    int32_t time = pots_[Pot::TIME]->GetValue();
    int32_t stereo = pots_[Pot::STEREO]->GetValue();
    int32_t time_mod = pots_[Pot::TIME_MOD]->GetValue();
    time += apply_pot_mod_attenuvert(Kastle2::hw.GetAnalogValue(CV_FREE), time_mod);
    time += apply_pot_mod_attenuvert(cv_step_, time_mod);
    time = constrain(time, POT_MIN, POT_MAX);

    size_t mapped_time;
    size_t delay_time_left;
    size_t delay_time_right;

    switch (mode_)
    {
    case Mode::CRUSHER:
        SetDryWet(curve_map(dry_wet, kMapCrusherGlobalDryWet));
        feedback_delay_ = curve_map(feedback, kMapCrusherGlobalFeedbackDelay);
        feedback_volume_ = curve_map(feedback, kMapCrusherGlobalFeedbackVolume);
        crusher_frequency_ = curve_map(time, kMapCrusherFrequency, MapClamp::TRUE, MapSafe::TRUE) / 2; // make smaller so it can be expanded later
        crusher_threshold_ = curve_map(dry_wet, kMapCrusherThreshold);
        crusher_threshold_ = constrain(crusher_threshold_, Q15_MIN, curve_map(crusher_frequency_, kMapCrusherThresholdCompensation));
        crusher_stereo_mix_ = curve_map(stereo, kMapCrusherStereoMix);
        crusher_left_frequency_ = constrain(q31_mult(crusher_frequency_, curve_map(stereo, kMapCrusherStereoFrequencyLeft, MapClamp::TRUE, MapSafe::TRUE)), Q31_ZERO, Q31_MAX / 2) * 2;
        crusher_right_frequency_ = constrain(q31_mult(crusher_frequency_, curve_map(stereo, kMapCrusherStereoFrequencyRight, MapClamp::TRUE, MapSafe::TRUE)), Q31_ZERO, Q31_MAX / 2) * 2;
        crusher_env_mod_ = curve_map(time, kMapCrusherEnvMod);
        crusher_env_.SetDecayTime(curve_map(time, kMapCrusherEnvDecay));
        crusher_xor_ = curve_map(dry_wet, kMapCrusherXor);
        break;

    case Mode::FLANGER:
        SetDryWet(curve_map(dry_wet, kMapFlangerGlobalDryWet));
        feedback_delay_ = curve_map(feedback, kMapFlangerGlobalFeedbackDelay);
        feedback_volume_ = curve_map(feedback, kMapFlangerGlobalFeedbackVolume);
        flanger_frequency_ = curve_map(time, kMapFlangerFrequency, MapClamp::TRUE, MapSafe::TRUE);
        lfo_left_.SetNativeFrequency(flanger_frequency_);
        lfo_right_.SetNativeFrequency(q31_add(flanger_frequency_, curve_map(stereo, kMapFlangerStereoFrequency, MapClamp::TRUE, MapSafe::TRUE)));
        flanger_depth_ = curve_map(dry_wet, kMapFlangerDepth);
        flanger_dry_wet_ = curve_map(dry_wet, kMapFlangerDryWet);
        flanger_stereo_mix_ = curve_map(stereo, kMapFlangerStereoMix);
        // hold stereo lfo in reset for smoother transition
        if (stereo == 0)
        {
            lfo_right_.Reset();
        }
        break;

    case Mode::PANNER:
        SetDryWet(curve_map(dry_wet, kMapPannerGlobalDryWet));
        feedback_delay_ = curve_map(feedback, kMapPannerGlobalFeedbackDelay);
        feedback_volume_ = curve_map(feedback, kMapPannerGlobalFeedbackVolume);
        panner_depth_ = curve_map(dry_wet, kMapPannerDepth);
        panner_stereo_mix_ = curve_map(stereo, kMapPannerStereoMix);
        panner_clip_.SetDrive(curve_map(pots_[Pot::AMOUNT]->GetValue(), kMapPannerDistortion));

        panner_frequency_ = curve_map(time, kMapPannerFrequency, MapClamp::TRUE, MapSafe::TRUE);
        panner_stereo_left_ = curve_map(stereo, kMapPannerStereoLeft, MapClamp::TRUE, MapSafe::TRUE) * curve_map(time, kMapPannerStereoTimeAdjust);
        panner_stereo_right_ = curve_map(stereo, kMapPannerStereoRight, MapClamp::TRUE, MapSafe::TRUE) * curve_map(time, kMapPannerStereoTimeAdjust);
        lfo_left_.SetNativeFrequency(q31_add(panner_frequency_, panner_stereo_left_));
        lfo_right_.SetNativeFrequency(q31_add(panner_frequency_, panner_stereo_right_));
        break;

    case Mode::FREEZER:
        freezer_grab_continuous_ = dry_wet < kMapFreezerSampleThreshold;
        SetDryWet(curve_map(dry_wet, kMapFreezerGlobalDryWet));
        freezer_feedback_ = curve_map(feedback, kMapFreezerFeedback);
        freezer_feedback_dry = curve_map(feedback, kMapFreezerFeedbackDry);
        if (time < POT_HALF)
        {
            // convert ticks per clock beat to samples per clock beat, than make it a whole bar instead of a beat
            size_t ticks = Kastle2::base.GetClock().GetTargetTicks() * buffer_size_ * 4;
            // divide until it fits
            while (ticks > delay_left_->GetMaxLength() && ticks != 0)
            {
                ticks /= 2;
            }

            freezer_length_left_ = ticks / step_map(time, kMapFreezerPitchRatio) + curve_map(stereo, kMapFreezerStereo);
            freezer_length_right_ = ticks / step_map(time, kMapFreezerPitchRatio);

            size_t fit_to_buffer = ticks;

            // In rare case ticks == 0, skip this
            while ((fit_to_buffer + ticks < delay_left_->GetMaxLength()) && ticks != 0)
            {
                fit_to_buffer += ticks;
            }
            // we need this to fill as much of the buffer as we can, but than sync the pointers without noticing
            freezer_record_length_ = fit_to_buffer;
        }
        else
        {
            freezer_length_left_ = (curve_map(time, kMapFreezerPitchTone)) + curve_map(stereo, kMapFreezerStereo);
            freezer_length_right_ = curve_map(time, kMapFreezerPitchTone);
        }
        if (clock_triggered)
        {
            freezer_clock_triggered_ = true;
        }
        break;

    case Mode::REPLAYER:
        SetDryWet(curve_map(dry_wet, kMapReplayerGlobalDryWet));
        feedback_delay_ = curve_map(feedback, kMapReplayerGlobalFeedbackDelay);
        feedback_delay_ /= curve_map(time, kMapReplayerGlobalFeedbackDelayMod);
        feedback_volume_ = curve_map(feedback, kMapReplayerGlobalFeedbackVolume);
        replayer_dry_wet_ = curve_map(dry_wet, kMapReplayerDryWet);
        replayer_oversampling_ = curve_map(time, kMapReplayerOversampling);
        replayer_stereo_oversampling_ = curve_map(stereo, kMapReplayerStereoOversampling);
        delay_left_->SetOversampling(replayer_oversampling_ - replayer_stereo_oversampling_);
        delay_right_->SetOversampling(replayer_oversampling_ + replayer_stereo_oversampling_);

        if (time > kReplayerReverseThresholdUpper)
        {
            delay_left_->SetReverse(false);
            delay_right_->SetReverse(false);
        }
        else if (time < kReplayerReverseThresholdLower)
        {
            delay_left_->SetReverse(true);
            delay_right_->SetReverse(true);
        }
        if (dry_wet > kReplayerHoldThreshold)
        {
            replayer_hold_ = true;
        }
        else
        {
            replayer_hold_ = false;
        }
        break;

    case Mode::PITCHER:
        SetDryWet(curve_map(dry_wet, kMapPitcherGlobalDryWet));
        feedback_delay_ = curve_map(feedback, kMapPitcherGlobalFeedbackDelay);
        feedback_volume_ = curve_map(feedback, kMapPitcherGlobalFeedbackVolume);
        pitcher_dry_wet_ = curve_map(dry_wet, kMapPitcherDryWet);
        pitcher_depth_ = q15_to_q31(curve_map(dry_wet, kMapPitcherDepth));
        pitcher_frequency_ = curve_map(time, kMapPitcherFrequency, MapClamp::TRUE, MapSafe::TRUE);
        pitcher_stereo_mix_ = q15_to_q31(curve_map(stereo, kMapPitcherStereoMix));
        lfo_left_.SetNativeFrequency(pitcher_frequency_);
        lfo_right_.SetNativeFrequency(q31_add(pitcher_frequency_, curve_map(stereo, kMapPitcherStereoFrequency, MapClamp::TRUE, MapSafe::TRUE)));
        pitcher_env_depth_ = curve_map(dry_wet, kMapPitcherEnvDepth, MapClamp::TRUE);
        break;

    case Mode::DELAY:
        SetDryWet(curve_map(dry_wet, kMapDelayGlobalDryWet));
        feedback_delay_ = curve_map(feedback, kMapDelayGlobalFeedbackDelay);
        feedback_volume_ = curve_map(feedback, kMapDelayGlobalFeedbackVolume);
        delay_dry_ = curve_map(dry_wet, kMapDelayDry);
        delay_wet_ = curve_map(dry_wet, kMapDelayWet);
        delay_feedback_ = curve_map(feedback, kMapDelayFeedback);

        mapped_time = curve_map(time, kMapDelayLength);

        if (delay_ticks_time_ > kDelayTicksSyncUpperLimit || ((delay_ticks_time_ > (delay_ticks_time_last_ * 2 + delay_ticks_time_last_ / 2)) && !delay_synced_first_measurement_))
        {
            delay_synced_ = false;
        }
        else
        {
            if (delay_ticks_time_last_ != delay_ticks_time_last_prev_)
            {
                size_t i = 0;
                delay_synced_ = true;
                delay_synced_times_[kDelaySyncedDividersCenter] = delay_ticks_time_last_;
                delay_synced_highest_index_ = kDelaySyncedDividersCenter + 1;
                // fill the array in the multipliers zone
                for (i = kDelaySyncedDividersCenter; delay_synced_times_[i] < delay_left_->GetMaxLength() && i > 0; i--)
                {
                    delay_synced_times_[i - 1] = delay_synced_times_[i];
                    delay_synced_times_[i - 1] *= kDelaySyncedDividers[i - 1].n;
                    delay_synced_times_[i - 1] /= kDelaySyncedDividers[i - 1].d;
                    // set the highest index to check for match to avoid reading unset values
                    delay_synced_highest_index_--;
                }
                // fill the array in the dividers zone
                for (i = kDelaySyncedDividersCenter; i < kDelaySyncedDividers.size() - 1; i++)
                {
                    delay_synced_times_[i + 1] = delay_synced_times_[i];
                    delay_synced_times_[i + 1] *= kDelaySyncedDividers[i + 1].n;
                    delay_synced_times_[i + 1] /= kDelaySyncedDividers[i + 1].d;
                }
            }
            delay_ticks_time_last_prev_ = delay_ticks_time_last_;
        }
        if (delay_synced_)
        {
            // find a value that is lower than time value
            size_t i = delay_synced_highest_index_;
            while (delay_synced_times_[i] > mapped_time && i < kDelaySyncedDividers.size())
            {
                i++;
            }
            // select it
            delay_length_ = delay_synced_times_[i];
        }
        else
        {
            delay_length_ = mapped_time;
        }

        delay_stereo_ = curve_map(stereo, kMapDelayStereo);

        // Calculate delay times
        delay_time_left = constrain(static_cast<int32_t>(delay_length_) - static_cast<int32_t>(delay_stereo_), 0, delay_left_->GetMaxLength());
        delay_time_right = constrain(delay_length_ + delay_stereo_, 0, delay_right_->GetMaxLength());

#if CHECK_DELAY_JUMP
        // Check for how much the value changed
        if (diff(delay_time_left, delay_time_left_prev_) > kDelayJumpIfLargerThan)
        {
            // Big jump? Set slewing to be fast
            delay_left_->SetFastSlew(true);
        }

        // Check for how much the value changed
        if (diff(delay_time_right, delay_time_right_prev_) > kDelayJumpIfLargerThan)
        {
            // Big jump? Set slewing to be fast
            delay_right_->SetFastSlew(true);
        }

        delay_time_left_prev_ = delay_time_left;
        delay_time_right_prev_ = delay_time_right;
#endif
        delay_left_->SetDelay(delay_time_left);
        delay_right_->SetDelay(delay_time_right);

        break;

    case Mode::SLICER:
        SetDryWet(curve_map(dry_wet, kMapSlicerGlobalDryWet));
        feedback_delay_ = curve_map(feedback, kMapSlicerGlobalFeedbackDelay);
        feedback_volume_ = curve_map(feedback, kMapSlicerGlobalFeedbackVolume);
        slicer_chance_ = curve_map(feedback, kMapSlicerChance);
        slicer_stereo_ = curve_map(stereo, kMapSlicerStereo);
        slicer_pattern_left_ = (step_map(time, kMapSlicerPattern) + slicer_stereo_) % (kMapSlicerPattern.size());
        slicer_pattern_right_ = (((kMapSlicerPattern.size()) + step_map(time, kMapSlicerPattern)) - slicer_stereo_) % (kMapSlicerPattern.size());
        slicer_decay_ = curve_map(dry_wet, kMapSlicerDecay);
        slicer_env_left_.SetDecayTime(slicer_decay_);
        slicer_env_right_.SetDecayTime(slicer_decay_);
        // lfo used for tempo to allow multiplying the clock speed
        lfo_left_.SetTicks((Kastle2::base.GetClock().GetTargetTicks() * buffer_size_ * 2) / 8);
        if (clock_triggered)
        {
            slicer_clock_trigger = true;
        }
        break;

    case Mode::SHIFTER:
        SetDryWet(curve_map(dry_wet, kMapShifterGlobalDryWet));
        feedback_delay_ = curve_map(feedback, kMapShifterGlobalFeedbackDelay);
        feedback_volume_ = curve_map(feedback, kMapShifterGlobalFeedbackVolume);
        shifter_dry_ = curve_map(time, kMapShifterTimeDry);
        shifter_frequency_ = curve_map(time, kMapShifterFrequency, MapClamp::TRUE, MapSafe::TRUE);
        shifter_env_mod_ = curve_map(time, kMapShifterEnvMod);
        shifter_left_frequency_ = shifter_frequency_;
        shifter_right_frequency_ = q31_add(shifter_frequency_, curve_map(stereo, kMapShifterStereoFrequency, MapClamp::TRUE, MapSafe::TRUE));
        // change lfo shape
        if (time > POT_HALF)
        {
            shifter_direction_ = true;
        }
        else
        {
            shifter_direction_ = false;
        }
        break;
    }

    // Mode is the selected + offset (by CV)
    mode_ = static_cast<Mode>(mode_selector_.GetMode());

    if (mode_ != mode_prev_)
    {
        mode_prev_ = mode_;
        ModeInit();
    }

    // do Mode triggers
    if (trigger_triggered)
    {
        ModeTrigger();
    }

    // NORMAL LAYER UI
    if (Kastle2::hw.GetLayer() == Hardware::Layer::NORMAL || Kastle2::hw.GetLayer() == Hardware::Layer::MODE)
    {
        uint32_t color = kLedColors[mode_];

        // dip the LEDs with a detected trigger
        if (trigger_blink_counter > 0)
        {
            color = WS2812::ApplyBrightness(color, 128);
        }

        Kastle2::hw.SetLed(Hardware::Led::LED_1, color);
        Kastle2::hw.SetLed(Hardware::Led::LED_2, color);
    }
}

void AppFxWizard::ModeInit()
{
    mode_selector_.SendMidi();
    switch (mode_)
    {
    case Mode::CRUSHER:
        ModeCrusherInit();
        break;
    case Mode::FLANGER:
        ModeFlangerInit();
        break;
    case Mode::PANNER:
        ModePannerInit();
        break;
    case Mode::FREEZER:
        ModeFreezerInit();
        break;
    case Mode::REPLAYER:
        ModeReplayerInit();
        break;
    case Mode::PITCHER:
        ModePitcherInit();
        break;
    case Mode::DELAY:
        ModeDelayInit();
        break;
    case Mode::SLICER:
        ModeSlicerInit();
        break;
    case Mode::SHIFTER:
        ModeShifterInit();
        break;
    }

    // reset oversampling and direction of the long delay
    if (mode_ != Mode::REPLAYER)
    {
        delay_left_->SetOversampling(1 << 16);
        delay_right_->SetOversampling(1 << 16);
        delay_left_->SetReverse(false);
        delay_right_->SetReverse(false);
    }

    // reset length of the long delay
    if (mode_ != Mode::FREEZER)
    {
        delay_left_->SetLengthBothToMax();
        delay_right_->SetLengthBothToMax();
    }
}

void AppFxWizard::ModeTrigger()
{
    trigger_blink_counter = 40;

    switch (mode_)
    {
    case Mode::FREEZER:
        freezer_grab_buffer_ = true;
        break;

    case Mode::SLICER:
        slicer_env_left_.Trigger();
        slicer_env_right_.Trigger();
        break;

    case Mode::DELAY:
        if (delay_ticks_time_ >= kDelayTicksSyncLowerLimit)
        {
            if (delay_synced_ || delay_synced_first_measurement_)
            {
                delay_ticks_time_last_ = delay_ticks_time_;
                delay_synced_first_measurement_ = false;
            }
            else
            {
                delay_synced_first_measurement_ = true;
            }
            delay_ticks_time_ = 0;
        }
        break;

    case Mode::REPLAYER:
        delay_left_->StartRecordProgress();
        replayer_filling_ = true;
        break;

    case Mode::SHIFTER:
        shifter_env_.Trigger();
        break;

    case Mode::CRUSHER:
        crusher_env_.Trigger();
        break;

    case Mode::PANNER:
        // reset the LFOs to the next zero crossing
        if (lfo_left_.GetPhase() > Q31_ZERO)
        {
            lfo_left_.Reset(Q31_MIN / 2);
        }
        else if (lfo_left_.GetPhase() < Q31_ZERO)
        {
            lfo_left_.Reset(Q31_MAX / 2);
        }

        if (lfo_right_.GetPhase() > Q31_ZERO)
        {
            lfo_right_.Reset(Q31_MIN / 2);
        }
        else if (lfo_right_.GetPhase() < Q31_ZERO)
        {
            lfo_right_.Reset(Q31_ZERO / 2);
        }
        break;

    case Mode::PITCHER:
        pitcher_env_.Trigger();
        [[fallthrough]]; // purposelly keeping fallthrough

    case Mode::FLANGER:
        lfo_left_.Reset();
        lfo_right_.Reset();
        break;
    }
}

void AppFxWizard::WorkLongDelay()
{
    // the delay has to be fed with newest data when not directly being used

    switch (mode_)
    {
    case Mode::FREEZER:
    case Mode::REPLAYER:
    case Mode::DELAY:
    case Mode::FLANGER:
    case Mode::PITCHER:
    case Mode::SLICER:
    case Mode::SHIFTER:
        break;

    default:
        delay_left_->Write(input_left_);
        delay_right_->Write(input_right_);
    }
}

void AppFxWizard::ModeCrusherInit()
{
    lfo_left_.SetWaveform(Oscillator::Waveform::RAMP);
    lfo_right_.SetWaveform(Oscillator::Waveform::RAMP);
}

void AppFxWizard::ModeCrusher()
{
    // xor for more digital feel
    input_left_ ^= crusher_xor_;
    input_right_ ^= crusher_xor_;

    q15_t env = q15_mult(q31_to_q15(crusher_env_.Process()), crusher_env_mod_);
    env = constrain(env, 1, 100);

    lfo_left_.SetNativeFrequency(crusher_left_frequency_ / env);
    lfo_right_.SetNativeFrequency(crusher_right_frequency_ / env);

    q15_t lfo_l = q31_to_q15(lfo_left_.Process());
    q15_t lfo_r = q15_mult(lfo_l, crusher_stereo_mix_) + q15_mult(q31_to_q15(lfo_right_.Process()), Q15_MAX - crusher_stereo_mix_);

    if (lfo_l >= crusher_threshold_)
    {
        crusher_left_ = input_left_;
    }

    if (lfo_r >= crusher_threshold_)
    {
        crusher_right_ = input_right_;
    }

    output_left_ = crusher_left_;
    output_right_ = crusher_right_;
}

void AppFxWizard::ModeFlangerInit()
{
    lfo_left_.SetWaveform(Oscillator::Waveform::TRI);
    lfo_right_.SetWaveform(Oscillator::Waveform::TRI);
}

void AppFxWizard::ModeFlanger()
{
    delay_left_->Write(input_left_);
    delay_right_->Write(input_right_);

    q15_t lfo_l = q31_to_q15(lfo_left_.Process());
    q15_t lfo_r = q15_mult(lfo_l, flanger_stereo_mix_smooth_) + q15_mult(q31_to_q15(lfo_right_.Process()), Q15_MAX - flanger_stereo_mix_smooth_);

    // smooth out the stereo_mix
    if ((flanger_mix_timeout_ > 1000) || (q15_abs(lfo_r - lfo_l) < q15(0.2f)))
    {
        flanger_mix_timeout_ = 0;
        flanger_stereo_mix_smooth_ = flanger_stereo_mix_;
    }
    flanger_mix_timeout_++;

    // scale from nothing to divided by 32 (I know it's hard to understand)
    delay_left_->SetDelaySnap(511 + q15_mult(lfo_l, flanger_depth_ / 64));
    delay_right_->SetDelaySnap(511 + q15_mult(lfo_r, flanger_depth_ / 64));

    output_left_ = q15_mult(input_left_, Q15_MAX - flanger_dry_wet_) + q15_mult(delay_left_->Read(), flanger_dry_wet_);
    output_right_ = q15_mult(input_right_, Q15_MAX - flanger_dry_wet_) + q15_mult(delay_right_->Read(), flanger_dry_wet_);
}

void AppFxWizard::ModePannerInit()
{
    lfo_left_.SetWaveform(Oscillator::Waveform::SINE);
    lfo_right_.SetWaveform(Oscillator::Waveform::SINE);
}

void AppFxWizard::ModePanner()
{
    // change it to 0 to 1 waveform in q15
    q15_t lfo_l = q31_to_q15(lfo_left_.Process());
    q15_t lfo_r = q15_mult(lfo_l, panner_stereo_mix_) + q15_mult(q31_to_q15(lfo_right_.Process()), Q15_MAX - panner_stereo_mix_);
    lfo_l = q15_add(q15_mult(panner_clip_.Process(lfo_l), Q15_HALF), Q15_HALF);
    lfo_r = q15_add(q15_mult(panner_clip_.Process(lfo_r), Q15_HALF), Q15_HALF);
    q15_t left_mod = q15_sub(Q15_MAX, q15_mult(lfo_l, panner_depth_));
    q15_t right_mod = q15_sub(Q15_MAX, q15_mult(q15_sub(Q15_MAX, lfo_r), panner_depth_));

    output_left_ = q15_mult(input_left_, left_mod);
    output_right_ = q15_mult(input_right_, right_mod);
}

void AppFxWizard::ModeFreezerInit()
{
    freezer_grab_buffer_ = false;
    freezer_grab_prev_ = freezer_grab_buffer_;
    delay_left_->SetOversampling(AdvancedDynamicDelayLine<q15least_t>::kDefaultSampling);
    delay_right_->SetOversampling(AdvancedDynamicDelayLine<q15least_t>::kDefaultSampling);
    delay_left_->SetDelaySnap(0);
    delay_right_->SetDelaySnap(0);
}

void AppFxWizard::ModeFreezer()
{
    bool can_use_feedback = true;
    // something was changing this on boot even after the init ran (We should look into this)
    delay_left_->SetDelaySnap(0);
    delay_right_->SetDelaySnap(0);

    if (freezer_grab_buffer_ != freezer_grab_prev_ && freezer_grab_buffer_)
    {
        freezer_grab_cnt_ = 0;
        freezer_grab_buffer_ = false;
        can_use_feedback = false;
        // align buffers (if they were detuned with stereo pot)
        delay_left_->SyncReadPointer();
        delay_right_->SyncReadPointer();
        delay_left_->SetLengthBoth(1);
        delay_right_->SetLengthBoth(1);
    }
    freezer_grab_prev_ = freezer_grab_buffer_;

    // fill buffer (I'm assuming both buffers are the same size because they are)
    // please don't murder me for this else if tree :(     (I hate it too)
    // only other way I can think of doing this is by reacting to changes of flags (too complex)
    if (freezer_grab_continuous_) // drywet pot is at 0
    {
        if (freezer_grab_continuous_ != freezer_grab_continuous_prev_)
        {
            // align buffers (if they were detuned with stereo pot)
            delay_left_->SyncReadPointer();
            delay_right_->SyncReadPointer();
            delay_left_->SetLengthBoth(1);
            delay_right_->SetLengthBoth(1);
        }
        delay_left_->Write(input_left_);
        delay_right_->Write(input_right_);
        // record to the whole buffer
        delay_left_->SetLengthBoth(freezer_record_length_);
        delay_right_->SetLengthBoth(freezer_record_length_);
        freezer_grab_cnt_ = freezer_record_length_;
        freezer_wait_for_clock_ = true;
        can_use_feedback = false;
    }
    else
    {
        if (freezer_wait_for_clock_ && !freezer_clock_triggered_)
        // drywet is above 0, we're waiting for a clock trigger to make the loop end on a beat
        {
            delay_left_->Write(input_left_);
            delay_right_->Write(input_right_);
            // record to the whole buffer
            delay_left_->SetLengthWrite(freezer_record_length_);
            delay_right_->SetLengthWrite(freezer_record_length_);
            freezer_grab_cnt_ = freezer_record_length_;
            can_use_feedback = false;
        }
        else if (freezer_grab_cnt_ < freezer_record_length_)
        // drywet is above 0 but we're filling the buffer completely (probably because of a trigger)
        {
            delay_left_->Write(input_left_);
            delay_right_->Write(input_right_);
            // record to the whole buffer
            delay_left_->SetLengthWrite(freezer_record_length_);
            delay_right_->SetLengthWrite(freezer_record_length_);
            freezer_grab_cnt_++;
            can_use_feedback = false;
        }

        // playback
        if (can_use_feedback)
        {
            delay_left_->SetLengthBoth(freezer_length_left_);
            delay_right_->SetLengthBoth(freezer_length_right_);
            // keep both pointers in sync
            delay_left_->SyncReadPointer();
            delay_right_->SyncReadPointer();

            // now that the pointers are synced, we can record in feedback
            q31_t tmp = q15_mult(delay_left_->Read(), freezer_feedback_dry);
            tmp += q15_mult(input_left_, freezer_feedback_);
            tmp = q15_saturate(tmp);
            delay_left_->Write(tmp);

            tmp = q15_mult(delay_right_->Read(), freezer_feedback_dry);
            tmp += q15_mult(input_right_, freezer_feedback_);
            tmp = q15_saturate(tmp);
            delay_right_->Write(tmp);
        }
        else
        {
            // set only the playback length because it's getting filled up
            delay_left_->SetLengthRead(freezer_length_left_);
            delay_right_->SetLengthRead(freezer_length_right_);
        }

        freezer_wait_for_clock_ = false;
    }

    freezer_clock_triggered_ = false;
    freezer_grab_continuous_prev_ = freezer_grab_continuous_;

    output_left_ = delay_left_->Read();
    output_right_ = delay_right_->Read();
}

void AppFxWizard::ModeReplayerInit()
{
    delay_left_->SetLengthBothToMax();
    delay_right_->SetLengthBothToMax();
    delay_left_->SetDelay(50800);
    delay_right_->SetDelay(50800);
}

void AppFxWizard::ModeReplayer()
{
    q15_t delay_reading_l;
    q15_t delay_reading_r;

    // fill the buffer fully at 1x oversampling and normal direction
    if (delay_left_->GetRecordProgress() < delay_left_->GetMaxLength() && replayer_filling_)
    {
        delay_left_->SetReverse(false);
        delay_right_->SetReverse(false);
        delay_left_->Write(input_left_);
        delay_right_->Write(input_right_);
        delay_reading_l = input_left_;
        delay_reading_r = input_right_;
    }
    else
    {
        replayer_filling_ = false;
        // if the drywet is fully wet, do not update the buffer at all (lossless looping)
        if (replayer_hold_)
        {
            delay_left_->WriteEmpty();
            delay_right_->WriteEmpty();
            delay_reading_l = delay_left_->Read();
            delay_reading_r = delay_right_->Read();
        }
        else
        {
            delay_reading_l = delay_left_->Read();
            delay_reading_r = delay_right_->Read();
            delay_left_->Write(q15_mult(input_left_, Q15_MAX - replayer_dry_wet_) + q15_mult(delay_reading_l, replayer_dry_wet_));
            delay_right_->Write(q15_mult(input_right_, Q15_MAX - replayer_dry_wet_) + q15_mult(delay_reading_r, replayer_dry_wet_));
        }
    }

    output_left_ = delay_reading_l;
    output_right_ = delay_reading_r;
}

void AppFxWizard::ModePitcherInit()
{
    lfo_left_.SetWaveform(Oscillator::Waveform::SAW);
    lfo_right_.SetWaveform(Oscillator::Waveform::SAW);
}

void AppFxWizard::ModePitcher()
{
    delay_left_->Write(input_left_);
    delay_right_->Write(input_right_);

    q31_t lfo_l = lfo_left_.Process();
    q31_t lfo_r = q31_mult(lfo_l, pitcher_stereo_mix_) + q31_mult(lfo_right_.Process(), Q31_MAX - pitcher_stereo_mix_);

    // modulate modulation depth with pitcher envelope
    if (sample_being_processed_ == 0)
    {
        q31_t tmp = q31_mult(pitcher_env_.Process(), q31(0.8f));
        pitcher_depth_modulated_ = q31_mult(pitcher_depth_, Q31_MAX - tmp);
    }
    // control modulation depth
    lfo_l = q31_mult(lfo_l / 2 + Q31_HALF, q31_mult(lfo_left_.GetTicks(), pitcher_depth_modulated_));
    lfo_r = q31_mult(lfo_r / 2 + Q31_HALF, q31_mult(lfo_right_.GetTicks(), pitcher_depth_modulated_));

    // increase modulation depth
    lfo_l *= 4;
    lfo_r *= 4;

    // limit the range for the effect to not drop out
    lfo_l %= delay_left_->GetMaxLength();
    lfo_r %= delay_right_->GetMaxLength();

    delay_left_->SetDelaySnap(lfo_l);
    delay_right_->SetDelaySnap(lfo_r);

    int32_t left = delay_left_->Read();
    int32_t right = delay_right_->Read();

    // fade the volume near the lfo phase reset to remove clicks - left channel
    int32_t remaining_ticks = lfo_left_.GetTicks() - lfo_left_.GetElapsedTicks();
    if (remaining_ticks < 0)
    {
        remaining_ticks = 0;
    }
    size_t elapsed_ticks = lfo_left_.GetElapsedTicks();
    if (elapsed_ticks < 64)
    {
        left = q15_mult(left, elapsed_ticks << (15 - 6));
    }
    else if (remaining_ticks < 64)
    {
        left = q15_mult(left, remaining_ticks << (15 - 6));
    }
    // fade the volume near the lfo phase reset to remove clicks - right channel
    remaining_ticks = lfo_right_.GetTicks() - lfo_right_.GetElapsedTicks();
    if (remaining_ticks < 0)
    {
        remaining_ticks = 0;
    }
    elapsed_ticks = lfo_right_.GetElapsedTicks();
    if (elapsed_ticks < 64)
    {
        right = q15_mult(right, elapsed_ticks << (15 - 6));
    }
    else if (remaining_ticks < 64)
    {
        right = q15_mult(right, remaining_ticks << (15 - 6));
    }

    output_left_ = q15_mult(input_left_, Q15_MAX - pitcher_dry_wet_) + q15_mult(left, pitcher_dry_wet_);
    output_right_ = q15_mult(input_right_, Q15_MAX - pitcher_dry_wet_) + q15_mult(right, pitcher_dry_wet_);
}

void AppFxWizard::ModeShifterInit()
{
    // nothing to init
}

void AppFxWizard::ModeShifter()
{
    if (sample_being_processed_ == 0)
    {
        q15_t env = q15_mult(q31_to_q15(shifter_env_.Process()), shifter_env_mod_);
        env = constrain(env, 1, 100);
        int64_t lf = (int64_t)shifter_left_frequency_ * (int64_t)env;
        int64_t rf = (int64_t)shifter_right_frequency_ * (int64_t)env;
        lfo_left_.SetNativeFrequency(q31_saturate(lf));
        lfo_right_.SetNativeFrequency(q31_saturate(rf));
    }

    if (shifter_direction_)
    {
        lfo_left_.SetWaveform(Oscillator::Waveform::SAW);
        lfo_right_.SetWaveform(Oscillator::Waveform::SAW);
    }
    else
    {
        lfo_left_.SetWaveform(Oscillator::Waveform::RAMP);
        lfo_right_.SetWaveform(Oscillator::Waveform::RAMP);
    }

    delay_left_->Write(input_left_);
    delay_right_->Write(input_right_);

    // scale lfo to +-255
    q15_t lfo_l = 255 + (q31_to_q15(lfo_left_.Process()) / 128);
    q15_t lfo_r = 255 + (q31_to_q15(lfo_right_.Process()) / 128);

    // lfo_l = q15_mult(lfo_l, Q15_MAX - shifter_dry_);
    // lfo_r = q15_mult(lfo_r, Q15_MAX - shifter_dry_);

    /*if (shifter_direction_ == 0)
    {
        lfo_l = 0;
        lfo_r = 0;
    }*/

    delay_left_->SetDelaySnap(lfo_l);
    delay_right_->SetDelaySnap(lfo_r);

    int32_t left = delay_left_->Read();
    int32_t right = delay_right_->Read();

    // fade the volume near the lfo phase reset to remove clicks - left channel
    int32_t remaining_ticks = lfo_left_.GetTicks() - lfo_left_.GetElapsedTicks();
    if (remaining_ticks < 0)
    {
        remaining_ticks = 0;
    }
    size_t elapsed_ticks = lfo_left_.GetElapsedTicks();
    if (elapsed_ticks < 64)
    {
        left = q15_mult(left, elapsed_ticks << (15 - 6));
    }
    else if (remaining_ticks < 64)
    {
        left = q15_mult(left, remaining_ticks << (15 - 6));
    }
    // fade the volume near the lfo phase reset to remove clicks - right channel
    remaining_ticks = lfo_right_.GetTicks() - lfo_right_.GetElapsedTicks();
    if (remaining_ticks < 0)
    {
        remaining_ticks = 0;
    }
    elapsed_ticks = lfo_right_.GetElapsedTicks();
    if (elapsed_ticks < 64)
    {
        right = q15_mult(right, elapsed_ticks << (15 - 6));
    }
    else if (remaining_ticks < 64)
    {
        right = q15_mult(right, remaining_ticks << (15 - 6));
    }

    output_left_ = q15_mult(input_left_, shifter_dry_) + q15_mult(left, Q15_MAX - shifter_dry_);
    output_right_ = q15_mult(input_right_, shifter_dry_) + q15_mult(right, Q15_MAX - shifter_dry_);
    // output_left_ = left;
    // output_right_ = right;
}

void AppFxWizard::ModeDelayInit()
{
    delay_clip_.SetDrive(120);
    delay_peak_counter_ = 0;
}

void AppFxWizard::ModeDelay()
{
    q15_t tmp_left, tmp_right;
    if (delay_ticks_time_ < SIZE_MAX)
    {
        delay_ticks_time_++;
    }

    q15_t delay_reading_l = delay_left_->Read();
    q15_t delay_reading_r = delay_right_->Read();

    tmp_left = q15_add((input_left_ / 2), (delay_clip_.Process(q15_mult(delay_reading_l, delay_feedback_)) / 2));
    tmp_right = q15_add((input_right_ / 2), (delay_clip_.Process(q15_mult(delay_reading_r, delay_feedback_)) / 2));

    if (q15_abs(tmp_left) > delay_peak_)
    {
        delay_peak_ = q15_abs(tmp_left);
    }
    if (q15_abs(tmp_right) > delay_peak_)
    {
        delay_peak_ = q15_abs(tmp_right);
    }
    delay_peak_counter_++;

    if (delay_peak_counter_ >= delay_peak_counter_max_)
    {
        delay_compressor_.Track(delay_peak_);
        delay_peak_ = Q15_ZERO;
        delay_peak_counter_ = 0;
    }

    q15_t compress_amount_ = Q15_MAX - delay_compressor_.GetEnvelope();

    tmp_left = q15_mult(tmp_left, compress_amount_);
    tmp_right = q15_mult(tmp_right, compress_amount_);

    delay_left_->Write(q15_div(tmp_left, Q15_HALF));
    delay_right_->Write(q15_div(tmp_right, Q15_HALF));

    output_left_ = q15_add(q15_mult(input_left_, delay_dry_), q15_mult(delay_reading_l, delay_wet_));
    output_right_ = q15_add(q15_mult(input_right_, delay_dry_), q15_mult(delay_reading_r, delay_wet_));

    // might need a compressor at the end here too, I'm not sure
}

void AppFxWizard::ModeSlicerInit()
{
    lfo_left_.SetWaveform(Oscillator::Waveform::SQUARE);
}

void AppFxWizard::ModeSlicer()
{
    // use lfo for clocking instead of the clock itself so we can multiply the timing
    bool tmp_lfo = lfo_left_.Process() > Q15_ZERO;
    if (tmp_lfo != slicer_lfo_prev_ && tmp_lfo)
    {
        if (slicer_chance_ > rand() % Q15_MAX)
        {
            slicer_flip_bit_ = true;
        }
        else
        {
            slicer_flip_bit_ = false;
        }

        slicer_step_ = (slicer_step_ + 1) % kSlicerPatterns.size();
        if (((kSlicerPatterns[slicer_pattern_left_] & (1 << slicer_step_)) >> slicer_step_) ^ slicer_flip_bit_)
        {
            slicer_env_left_.Trigger();
        }
        if (((kSlicerPatterns[slicer_pattern_right_] & (1 << slicer_step_)) >> slicer_step_) ^ slicer_flip_bit_)
        {
            slicer_env_right_.Trigger();
        }
    }
    slicer_lfo_prev_ = tmp_lfo;

    // sync lfo to the clock
    if (slicer_clock_trigger)
    {
        slicer_clock_trigger = false;
        lfo_left_.Reset();
    }

    output_left_ = q15_mult(input_left_, q31_to_q15(slicer_env_left_.Process()));
    output_right_ = q15_mult(input_right_, q31_to_q15(slicer_env_right_.Process()));
}

void AppFxWizard::MemoryInitialization()
{
    Kastle2::memory.Write8(kMemMode, 0);
}

void AppFxWizard::MidiCallback(midi::Message *msg)
{
    for (auto &pot : pots_)
    {
        pot->MidiCallback(msg);
    }
    mode_selector_.MidiCallback(msg);
    if (msg->IsNoteOn())
    {
        uint8_t note = msg->GetData1();
        if (note >= kMidiMinNote)
        {
            ModeTrigger();
        }
    }
}

bool AppFxWizard::LoadFile()
{
    UserDataFile file_reader;

    // Validate file header
    if (!file_reader.Validate("k2fx"))
    {
        return false;
    }

    // Read the header data
    file_reader.Read(&settings_file_, 10);

    // Validate counts (it's also checked by the file reader, but just in case)
    if (!between(settings_file_.num_rhythms, 1, UserDataFile::kMaxRhythms))
    {
        return false;
    }

    // Validate sequencer length
    if (!between(settings_file_.sequencer_length, Sequencer::kMinLength, Sequencer::kMaxLength))
    {
        return false;
    }

    // Jump to the rhythms data
    file_reader.JumpTo(20);

    // Load rhythms
    settings_file_.rhythms = file_reader.LoadRhythms(settings_file_.num_rhythms);
    if (!settings_file_.rhythms)
    {
        return false;
    }

    return true;
}