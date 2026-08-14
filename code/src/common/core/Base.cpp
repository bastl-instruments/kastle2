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

#include "Base.hpp"
#include "hardware/watchdog.h"
#include "common/core/midi/Handler.hpp"
#include "common/utils.hpp"
#include "Kastle2.hpp"
#include "Kastle2_cc.hpp"

using namespace kastle2;

void Base::Init()
{
    // By default, enable all features
    SetAllFeaturesEnabled(true);

    // Disable gate indication (we agreed not to include it since it's too distracting)
    SetFeatureEnabled(Feature::GATE_INDICATION, false);

    // Initialize LFO and Clock
    lfo_.Init(AUDIO_LOOP_RATE);
    lfo_.SetFrequency(1.0f);
    clock_.Init(AUDIO_LOOP_RATE);
    fake_blinker_.Init();

    // Startup envelope not to blow your ears off
    startup_env_.Init(AUDIO_LOOP_RATE);
    startup_env_.SetTargetRatioA(0.95f);
    startup_env_.SetAttackTime(kBaseStartupFadeIn);
    startup_env_.Trigger();
    startup_env_state_ = StartupEnvState::OUTPUT;

    // Initialize sequencer
    sequencer_.Init();
    sequencer_.SetTriggerGeneratorTable(kBaseRhythmTable);

    // Set up gains
    sw_input_gain_ = Q15_HALF;
    sw_output_gain_ = Q15_MAX;
    SetMaxVolume(kDefaultMaxVolume);

    // POTS

    // Normal layer
    pots_[Pot::LFO_MOD] = FancyPot::Create({.pot = Hardware::Pot::POT_3,
                                            .layer = Hardware::Layer::NORMAL,
                                            .midi_cc = cc::LFO_MOD,
                                            .deadzone = true});

    pots_[Pot::LFO] = FancyPot::Create({.pot = Hardware::Pot::POT_7,
                                        .layer = Hardware::Layer::NORMAL,
                                        .midi_cc = cc::LFO,
                                        .deadzone = true});

    // Shift layer
    pots_[Pot::INPUT] = FancyPot::Create({.pot = Hardware::Pot::POT_1,
                                          .layer = Hardware::Layer::SHIFT,
                                          .midi_cc = cc::INPUT_GAIN,
                                          .memory_addr = Memory::ADDR_INPUT_GAIN});
    pots_[Pot::RHYTHM] = FancyPot::Create({.pot = Hardware::Pot::POT_3,
                                           .layer = Hardware::Layer::SHIFT,
                                           .midi_cc = cc::RHYTHM});
    pots_[Pot::OUTPUT] = FancyPot::Create({.pot = Hardware::Pot::POT_5,
                                           .layer = Hardware::Layer::SHIFT,
                                           .midi_cc = cc::MAIN_VOLUME,
                                           .memory_addr = Memory::ADDR_OUTPUT_GAIN});
    pots_[Pot::TEMPO] = FancyPot::Create({.pot = Hardware::Pot::POT_7,
                                          .layer = Hardware::Layer::SHIFT,
                                          .midi_cc = cc::TEMPO});

    // Mode
    pots_[Pot::SWING] = FancyPot::Create({.pot = Hardware::Pot::POT_7,
                                          .layer = Hardware::Layer::MODE,
                                          .initial_value = POT_HALF,
                                          .midi_cc = cc::SWING,
                                          .memory_addr = Memory::ADDR_SWING});

    // Settings
    pots_[Pot::SETTINGS_MONO_INPUT] = FancyPot::Create({.pot = Hardware::Pot::POT_1,
                                                        .layer = Hardware::Layer::SETTINGS,
                                                        .deadzone = true});
    pots_[Pot::SETTINGS_SYNC_INPUT] = FancyPot::Create({.pot = Hardware::Pot::POT_7,
                                                        .layer = Hardware::Layer::SETTINGS,
                                                        .deadzone = true});
    pots_[Pot::SETTINGS_LFO_MOD] = FancyPot::Create({.pot = Hardware::Pot::POT_3,
                                                     .layer = Hardware::Layer::SETTINGS});
    pots_[Pot::SETTINGS_NORMAL_2] = FancyPot::Create({.pot = Hardware::Pot::POT_2,
                                                      .layer = Hardware::Layer::SETTINGS});
    pots_[Pot::SETTINGS_NORMAL_4] = FancyPot::Create({.pot = Hardware::Pot::POT_4,
                                                      .layer = Hardware::Layer::SETTINGS});
    pots_[Pot::SETTINGS_NORMAL_6] = FancyPot::Create({.pot = Hardware::Pot::POT_6,
                                                      .layer = Hardware::Layer::SETTINGS});
    pots_[Pot::SETTINGS_SHIFT_1] = FancyPot::Create({.pot = Hardware::Pot::POT_1,
                                                     .layer = Hardware::Layer::SETTINGS_SHIFT});
    pots_[Pot::SETTINGS_SHIFT_2] = FancyPot::Create({.pot = Hardware::Pot::POT_2,
                                                     .layer = Hardware::Layer::SETTINGS_SHIFT});
    pots_[Pot::SETTINGS_SHIFT_3] = FancyPot::Create({.pot = Hardware::Pot::POT_3,
                                                     .layer = Hardware::Layer::SETTINGS_SHIFT});
    pots_[Pot::SETTINGS_SHIFT_4] = FancyPot::Create({.pot = Hardware::Pot::POT_4,
                                                     .layer = Hardware::Layer::SETTINGS_SHIFT});
    pots_[Pot::SETTINGS_SHIFT_5] = FancyPot::Create({.pot = Hardware::Pot::POT_5,
                                                     .layer = Hardware::Layer::SETTINGS_SHIFT});
    pots_[Pot::SETTINGS_SHIFT_6] = FancyPot::Create({.pot = Hardware::Pot::POT_6,
                                                     .layer = Hardware::Layer::SETTINGS_SHIFT});
    pots_[Pot::SETTINGS_SHIFT_7] = FancyPot::Create({.pot = Hardware::Pot::POT_7,
                                                     .layer = Hardware::Layer::SETTINGS_SHIFT});
    pots_[Pot::SETTINGS_MODE_1] = FancyPot::Create({.pot = Hardware::Pot::POT_1,
                                                    .layer = Hardware::Layer::SETTINGS_MODE});
    pots_[Pot::SETTINGS_MODE_2] = FancyPot::Create({.pot = Hardware::Pot::POT_2,
                                                    .layer = Hardware::Layer::SETTINGS_MODE});
    pots_[Pot::SETTINGS_MODE_3] = FancyPot::Create({.pot = Hardware::Pot::POT_3,
                                                    .layer = Hardware::Layer::SETTINGS_MODE});
    pots_[Pot::SETTINGS_MODE_4] = FancyPot::Create({.pot = Hardware::Pot::POT_4,
                                                    .layer = Hardware::Layer::SETTINGS_MODE});
    pots_[Pot::SETTINGS_MODE_5] = FancyPot::Create({.pot = Hardware::Pot::POT_5,
                                                    .layer = Hardware::Layer::SETTINGS_MODE});
    pots_[Pot::SETTINGS_MODE_6] = FancyPot::Create({.pot = Hardware::Pot::POT_6,
                                                    .layer = Hardware::Layer::SETTINGS_MODE});
    pots_[Pot::SETTINGS_MODE_7] = FancyPot::Create({.pot = Hardware::Pot::POT_7,
                                                    .layer = Hardware::Layer::SETTINGS_MODE});

    lfo_mod_pots_[LfoMod::Destination::DEFAULT] = nullptr;
    lfo_mod_pots_[LfoMod::Destination::NORMAL_1] = pots_[Pot::SETTINGS_MONO_INPUT].get();
    lfo_mod_pots_[LfoMod::Destination::NORMAL_2] = pots_[Pot::SETTINGS_NORMAL_2].get();
    lfo_mod_pots_[LfoMod::Destination::NORMAL_3] = pots_[Pot::SETTINGS_LFO_MOD].get();
    lfo_mod_pots_[LfoMod::Destination::NORMAL_4] = pots_[Pot::SETTINGS_NORMAL_4].get();
    lfo_mod_pots_[LfoMod::Destination::NORMAL_5] = nullptr;
    lfo_mod_pots_[LfoMod::Destination::NORMAL_6] = pots_[Pot::SETTINGS_NORMAL_6].get();
    lfo_mod_pots_[LfoMod::Destination::NORMAL_7] = pots_[Pot::SETTINGS_SYNC_INPUT].get();
    lfo_mod_pots_[LfoMod::Destination::SHIFT_1] = pots_[Pot::SETTINGS_SHIFT_1].get();
    lfo_mod_pots_[LfoMod::Destination::SHIFT_2] = pots_[Pot::SETTINGS_SHIFT_2].get();
    lfo_mod_pots_[LfoMod::Destination::SHIFT_3] = pots_[Pot::SETTINGS_SHIFT_3].get();
    lfo_mod_pots_[LfoMod::Destination::SHIFT_4] = pots_[Pot::SETTINGS_SHIFT_4].get();
    lfo_mod_pots_[LfoMod::Destination::SHIFT_5] = pots_[Pot::SETTINGS_SHIFT_5].get();
    lfo_mod_pots_[LfoMod::Destination::SHIFT_6] = pots_[Pot::SETTINGS_SHIFT_6].get();
    lfo_mod_pots_[LfoMod::Destination::SHIFT_7] = pots_[Pot::SETTINGS_SHIFT_7].get();
    lfo_mod_pots_[LfoMod::Destination::MODE_1] = pots_[Pot::SETTINGS_MODE_1].get();
    lfo_mod_pots_[LfoMod::Destination::MODE_2] = pots_[Pot::SETTINGS_MODE_2].get();
    lfo_mod_pots_[LfoMod::Destination::MODE_3] = pots_[Pot::SETTINGS_MODE_3].get();
    lfo_mod_pots_[LfoMod::Destination::MODE_4] = pots_[Pot::SETTINGS_MODE_4].get();
    lfo_mod_pots_[LfoMod::Destination::MODE_5] = pots_[Pot::SETTINGS_MODE_5].get();
    lfo_mod_pots_[LfoMod::Destination::MODE_6] = pots_[Pot::SETTINGS_MODE_6].get();
    lfo_mod_pots_[LfoMod::Destination::MODE_7] = pots_[Pot::SETTINGS_MODE_7].get();

    // Init pots
    for (auto &pot : pots_)
    {
        pot->Init(AUDIO_LOOP_RATE);
    }

    // Force "changed" flag to load values
    pots_[Pot::LFO]->ForceChanged();
    pots_[Pot::RHYTHM]->ForceChanged();
    pots_[Pot::SWING]->ForceChanged();

    // MIDI Out Pots
    midi_pots_[Hardware::Pot::POT_1] = FancyPot::Create({.pot = Hardware::Pot::POT_1,
                                                         .layer = Hardware::Layer::NORMAL,
                                                         .midi_output_cc = cc::OUT_POT_1});
    midi_pots_[Hardware::Pot::POT_2] = FancyPot::Create({.pot = Hardware::Pot::POT_2,
                                                         .layer = Hardware::Layer::NORMAL,
                                                         .midi_output_cc = cc::OUT_POT_2});
    midi_pots_[Hardware::Pot::POT_3] = FancyPot::Create({.pot = Hardware::Pot::POT_3,
                                                         .layer = Hardware::Layer::NORMAL,
                                                         .midi_output_cc = cc::OUT_POT_3});
    midi_pots_[Hardware::Pot::POT_4] = FancyPot::Create({.pot = Hardware::Pot::POT_4,
                                                         .layer = Hardware::Layer::NORMAL,
                                                         .midi_output_cc = cc::OUT_POT_4});
    midi_pots_[Hardware::Pot::POT_5] = FancyPot::Create({.pot = Hardware::Pot::POT_5,
                                                         .layer = Hardware::Layer::NORMAL,
                                                         .midi_output_cc = cc::OUT_POT_5});
    midi_pots_[Hardware::Pot::POT_6] = FancyPot::Create({.pot = Hardware::Pot::POT_6,
                                                         .layer = Hardware::Layer::NORMAL,
                                                         .midi_output_cc = cc::OUT_POT_6});
    midi_pots_[Hardware::Pot::POT_7] = FancyPot::Create({.pot = Hardware::Pot::POT_7,
                                                         .layer = Hardware::Layer::NORMAL,
                                                         .midi_output_cc = cc::OUT_POT_7});
    for (auto pot_type : EnumRange<Hardware::Pot>())
    {
        // Init pot
        midi_pots_[pot_type]->Init(AUDIO_LOOP_RATE);
        // Enable pot
        midi_pots_enabled_.set(static_cast<size_t>(pot_type), true);
    }

    // Other LFO stuff
    lfo_sync_ = false;
    lfo_pot_ratio_ = 0;

    // Default settings
    mono_setting_ = Memory::MonoSetting::STEREO;
    sync_setting_ = Memory::SyncSetting::NONE_DISABLED;
    uint8_t mono_setting_byte;
    if (Kastle2::memory.Read8(Memory::ADDR_MONO_SETTINGS, &mono_setting_byte))
    {
        if (mono_setting_byte < static_cast<uint8_t>(Memory::MonoSetting::COUNT))
        {
            mono_setting_ = static_cast<Memory::MonoSetting>(mono_setting_byte);
        }
    }
    uint8_t sync_setting_byte;
    if (Kastle2::memory.Read8(Memory::ADDR_SYNC_SETTINGS, &sync_setting_byte))
    {
        if (sync_setting_byte < static_cast<uint8_t>(Memory::SyncSetting::COUNT))
        {
            sync_setting_ = static_cast<Memory::SyncSetting>(sync_setting_byte);
        }
    }

    clock_.LoadFromMemory();

    SetAllLfoModSelectionEnabled(false);                            // disable all secondary pot mappings
    SetLfoModSelectionEnabled(true, LfoMod::Destination::NORMAL_3); // enable LFO frequency pot by default
    lfo_mod_.SetDestination(LfoMod::Destination::DEFAULT);          // Default LFO modulation destination is LFO frequency
    if (IsFeatureEnabled(Feature::LFO_MOD_MAPPING))
    {
        SetLfoModDefaultSelectionEnabled(); // enable all secondary pots + lfo mod by default
        uint8_t lfo_mod_assignment_byte;
        if (Kastle2::memory.Read8(Memory::ADDR_LFO_MOD_ASSIGNMENT, &lfo_mod_assignment_byte))
        {
            if (lfo_mod_assignment_byte < static_cast<uint8_t>(LfoMod::Destination::COUNT))
            {
                lfo_mod_.SetDestination(static_cast<LfoMod::Destination>(lfo_mod_assignment_byte));
            }
        }
    }

    // Default sync stuff
    prev_reset_ = false;
    sync_thru_ = kBaseSyncThru;

    // Settings stuff
    shift_and_mode_pressed_count_ = 0;
    settings_toggled_ = false;
    settings_pulse_ = 0;
    leds_should_be_off_ = false;

    // Layers
    prev_layer_ = Hardware::Layer::NORMAL;
    layer_timer_ = 0;
    prev_layer_timer_ = 0;

    // Input loudness indication
    input_envelope_follower_.Init(AUDIO_LOOP_RATE);
    input_envelope_follower_.SetAttackTime(.005f);
    input_envelope_follower_.SetReleaseTime(.040f);
}

void Base::SetMaxVolume(size_t max_volume)
{
    max_volume_ = max_volume <= 63 ? max_volume : 63;
}

void Base::SetFeatureEnabled(Feature feature, bool enabled)
{
    features_enabled_.set(static_cast<size_t>(feature), enabled);
}

bool Base::IsFeatureEnabled(Feature feature) const
{
    return features_enabled_.test(static_cast<size_t>(feature));
}

void Base::SetAllFeaturesEnabled(bool enabled)
{
    if (enabled)
    {
        features_enabled_.set(); // Set all bits to true
    }
    else
    {
        features_enabled_.reset(); // Reset all bits to false
    }
}

void Base::SetLfoModSelectionEnabledSingle(bool enabled, LfoMod::Destination destination)
{
    if (destination < LfoMod::Destination::DEFAULT || destination >= LfoMod::Destination::COUNT)
    {
        return;
    }

    lfo_mod_selection_enabled_[destination] = enabled;

    if (!enabled && lfo_mod_.GetDestination() == destination)
    {
        lfo_mod_.SetDestination(LfoMod::Destination::DEFAULT);
    }
}

void Base::SetAllLfoModSelectionEnabled(bool enabled)
{
    lfo_mod_selection_enabled_.fill(enabled);

    if (!enabled)
    {
        lfo_mod_.SetDestination(LfoMod::Destination::DEFAULT);
    }
}

void Base::SetLfoModDefaultSelectionEnabled()
{
    lfo_mod_selection_enabled_ = {
        true,  // DEFAULT
        false, // NORMAL_1
        false, // NORMAL_2
        true,  // NORMAL_3
        false, // NORMAL_4
        false, // NORMAL_5
        false, // NORMAL_6
        false, // NORMAL_7
        true,  // SHIFT_1
        true,  // SHIFT_2
        true,  // SHIFT_3
        true,  // SHIFT_4
        true,  // SHIFT_5
        true,  // SHIFT_6
        true,  // SHIFT_7
        true,  // MODE_1
        true,  // MODE_2
        true,  // MODE_3
        true,  // MODE_4
        true,  // MODE_5
        true,  // MODE_6
        true   // MODE_7
    };
}

// Ticking LFO and TEMPO in precise timings.
FASTCODE void Base::BeforeAudioLoop(q15_t *input, size_t size)
{
    if (!IsFeatureEnabled(Feature::BASE))
    {
        // If all features are disabled, just return
        return;
    }

    // We don't use the pot freeze function so no need to call it here
    // Process pots in audio loop
    // for (auto &pot : pots_)
    // {
    //     pot->Process();
    // }
    // for (auto &pot : midi_pots_)
    // {
    //     pot->Process();
    // }

    // Do the input gain
    q15_scale_buffer(input, sw_input_gain_, kInputGainShiftLeft, 2 * size);

    if (IsFeatureEnabled(Feature::LFO_MOD_MAPPING))
    {
        for (size_t i = 0; i < 2 * size; i++)
        {
            input[i] = q15_mult(input[i], input_amplitude_multipler_);
            input[i] = q15_div(input[i], input_amplitude_divider_);
        }
    }

    // apply startup volume fade in to avoid clicks in buffer
    if (startup_env_state_ != StartupEnvState::FINISHED)
    {
        startup_env_.Process();
    }
    if (startup_env_state_ == StartupEnvState::OUTPUT)
    {
        // Zero input
        for (uint32_t i = 0; i < 2 * size; i++)
        {
            input[i] = 0;
        }
    }
    if (startup_env_state_ == StartupEnvState::INPUT)
    {
        if (startup_env_.GetState() == AdsrEnv::State::DECAY)
        {
            startup_env_state_ = StartupEnvState::FINISHED;
        }
        q15_scale_buffer(input, q31_to_q15(startup_env_.GetOutput()), 0, 2 * size);
    }

    // MONO SETTING FOR KASTLE2
    if (Kastle2::hw.GetVersion() == Hardware::Version::KASTLE2)
    {

        // Copy the left channel into the right
        if (mono_setting_ == Memory::MonoSetting::LEFT)
        {
            for (size_t i = 0; i < size; i++)
            {
                input[i * 2 + 1] = input[i * 2];
            }
        }
        // Copy the right channel into the left
        else if (mono_setting_ == Memory::MonoSetting::RIGHT)
        {
            for (size_t i = 0; i < size; i++)
            {
                input[i * 2] = input[i * 2 + 1];
            }
        }
    }
    // MONO SETTING FOR CITADEL
    else if (Kastle2::hw.GetVersion() == Hardware::Version::CITADEL)
    {
        // Halve and mix
        if (mono_setting_ == Memory::MonoSetting::LEFT)
        {
            for (size_t i = 0; i < size; i++)
            {
                q15_t mixed = input[i * 2] / 2 + input[i * 2 + 1] / 2;
                input[i * 2] = mixed;
                input[i * 2 + 1] = mixed;
            }
        }
        // Mix and clip
        else if (mono_setting_ == Memory::MonoSetting::RIGHT)
        {
            for (size_t i = 0; i < size; i++)
            {
                q15_t mixed = q15_add(input[i * 2], input[i * 2 + 1]);
                input[i * 2] = mixed;
                input[i * 2 + 1] = mixed;
            }
        }
    }

    // Update the input loudness indication envelope follower
    if (IsFeatureEnabled(Feature::INPUT_INDICATION))
    {
        input_envelope_follower_.TrackBlock(input, size * 2);
    }

    // Received Reset, jump to middle of phase
    bool reset = Kastle2::hw.GetResetIn();
    if (reset && !prev_reset_)
    {
        lfo_.SetPhase(Q31_ZERO);
    }
    prev_reset_ = reset;

    // Receive the value from the Oscillator as a 32-bit signed integer
    int32_t lfo_value = lfo_.Process();

    // Scale down to 10 bits and shift to 0-1023
    lfo_value = (lfo_value >> 22) + 512;

    // Store as a class variable
    lfo_triangle_value_ = lfo_value;
    lfo_pulse_value_ = lfo_.GetSquareOut();

    // Tap tempo
    bool tap_state = Kastle2::hw.GetLayer() == Hardware::Layer::SHIFT &&
                     Kastle2::hw.GetRawButtonState(Hardware::Button::MODE);

    // Set external sync
    bool sync_enabled = sync_setting_ != Memory::SyncSetting::EXTERNAL_DISABLED;
    clock_.SetSyncJackPlugged(sync_enabled && Kastle2::hw.IsSyncInJackProbablyPlugged());
    bool sync_in = sync_enabled && Kastle2::hw.GetSyncIn();

    // Only true for the audio loop the step actually starts on, see IsNowStep().
    step_now_ = false;

    // The gate runs on its own countdown, restarted by StartGate() on every step. Ticked
    // down before the step handling so a step firing this tick keeps its full length.
    if (gate_ticks_remaining_ > 0)
    {
        gate_ticks_remaining_--;
    }

    // Reset sequencer?
    bool do_cv_update = false;
    Hardware::FeedValue feed2 = Kastle2::hw.GetFeedValue(Hardware::AnalogInput::FEED_2);
    if (sequencer_edge_detector_.Process(feed2 == Hardware::FeedValue::HIGH))
    {
        clock_.NextCycleReset();
        sequencer_.Reset();
        do_cv_update = true;
    }

    // Check clock for the new clock tick
    if (clock_.Process(tap_state, sync_in, clock_midi_pulse_))
    {
        // Clock received a reset?
        bool now_reset = clock_.IsNowReset();
        if (now_reset)
        {
            // Reset the sequencer
            sequencer_.Reset();
            OnStepStarted();
            do_cv_update = true;
        }
        else
        {
            Hardware::FeedValue feed1 = Kastle2::hw.GetFeedValue(Hardware::AnalogInput::FEED_1);
            Sequencer::Feed trigger_feed;

            switch (feed1)
            {
            case Hardware::FeedValue::LOW:
                trigger_feed = Sequencer::Feed::INVERT;
                break;
            case Hardware::FeedValue::HIGH:
                trigger_feed = Sequencer::Feed::RANDOM;
                break;
            default:
                trigger_feed = Sequencer::Feed::SAME;
                break;
            }

            Hardware::FeedValue feed3 = Kastle2::hw.GetFeedValue(Hardware::AnalogInput::FEED_3);
            Sequencer::Feed cv_feed;

            switch (feed3)
            {
            case Hardware::FeedValue::LOW:
                cv_feed = Sequencer::Feed::INVERT;
                break;
            case Hardware::FeedValue::HIGH:
                cv_feed = Sequencer::Feed::RANDOM;
                break;
            default:
                cv_feed = Sequencer::Feed::SAME;
                break;
            }

            // A swing step can still be waiting here when the tempo jumped up and the
            // averaged step length used for the delay overshot the real one. Fire it
            // now, otherwise scheduling over it would silently drop a step.
            if (sequencer_.IsSwingStepPending())
            {
                sequencer_.NextStep(pending_trigger_feed_, pending_cv_feed_);
                OnStepStarted();
                do_cv_update = true;
            }

            if (sequencer_.GetSwingType() != Sequencer::SwingType::NONE)
            {
                pending_trigger_feed_ = trigger_feed;
                pending_cv_feed_ = cv_feed;
                sequencer_.ScheduleSwingStep(clock_.GetAverageTargetTicks());
            }
            else
            {
                sequencer_.NextStep(trigger_feed, cv_feed);
                OnStepStarted();
                do_cv_update = true;
            }
        }

        // clocked LFO sync
        lfo_.SyncWithClock();
    }
    clock_midi_pulse_ = false;

    if (sequencer_.ProcessSwingTick())
    {
        sequencer_.NextStep(pending_trigger_feed_, pending_cv_feed_);
        OnStepStarted();
        do_cv_update = true;
    }

    if (lfo_.IsSynced())
    {
        bool clk_stopped = clock_.GetState() != ClockSource::State::RUNNING;
        bool lfo_stopped = lfo_.IsStopped();
        if (clk_stopped && !lfo_stopped)
        {
            lfo_.StopAfterCycle();
        }
        else if (!clk_stopped && lfo_stopped)
        {
            lfo_.Start();
        }
    }
    else
    {
        lfo_.Resume();
    }

    // Publish the CV of the upcoming step slightly ahead of it. With swing active the
    // step no longer lands on the clock grid, so the lookahead has to track the swung
    // position - otherwise CV would jump on the straight grid while the trigger waits.
    bool reaching_next_step = sequencer_.GetSwingType() == Sequencer::SwingType::NONE
                                  ? clock_.IsReachingNextCycle()
                                  : sequencer_.IsReachingSwungStep();

    if (reaching_next_step)
    {
        if (sequencer_.ReachingNextCycle())
        {
            do_cv_update = true;
        }
    }

    if (do_cv_update)
    {
        UpdateCvOut();
    }

    // Update Gate Out
    if (IsFeatureEnabled(Feature::GATE_OUT))
    {
        UpdateGateOut();
    }

    // Check sync out countdown
    if (IsFeatureEnabled(Feature::SYNC_OUT))
    {
        // If externally synced, just copy the input (if sync_thru_ is enabled)
        if (clock_.GetSyncType() == Clock::Sync::EXTERNAL && sync_thru_)
        {
            Kastle2::hw.SetSyncOut(Kastle2::hw.GetSyncIn());
        }
        else
        {
            Kastle2::hw.SetSyncOut(clock_.GetSyncOutput());
        }
    }

    // Set LFO output
    if (IsFeatureEnabled(Feature::LFO_OUT))
    {
        Kastle2::hw.SetTriOut(lfo_triangle_value_);
        Kastle2::hw.SetPulseOut(lfo_pulse_value_);
    }

    // Stuff for the layers swiching
    if (shift_and_mode_pressed_count_ < SIZE_MAX)
    {
        shift_and_mode_pressed_count_++;
    }
    settings_pulse_++;
    if (settings_pulse_ > 127)
    {
        settings_pulse_ = -128;
    }

    if (input_clipping_counter_ > 0)
    {
        input_clipping_counter_--;
    }

    midi_number_flasher_.Process();

    if (IsFeatureEnabled(Feature::LFO_MOD_MAPPING))
    {
        DecrementLfoModDestChangeCounter();
    }
}

void Base::UpdateCvOut()
{
    if (IsFeatureEnabled(Feature::CV_OUT))
    {
        Kastle2::hw.SetCvOut(sequencer_.GetCvOutput());
    }
}

void Base::OnStepStarted()
{
    step_now_ = true;
    StartGate();
}

bool Base::IsNowStep() const
{
    return step_now_ && clock_.IsOutputEnabled();
}

void Base::StartGate()
{
    const uint32_t target_ticks = clock_.GetTargetTicks();
    const uint32_t nominal_ticks = (target_ticks * kBaseGateLength) / 100;

    // The next step lands on the clock tick at the earliest (swing only ever delays it),
    // so that is how much room this gate has. A swung step starts late into the cycle and
    // would otherwise run its full length straight into the following trigger.
    const uint32_t current_ticks = clock_.GetCurrentTicks();
    const uint32_t until_next_ticks = target_ticks > current_ticks ? target_ticks - current_ticks : 0;

    // Never eat more than the duty cycle already reserves, that keeps the gate audible at
    // fast tempos and leaves unswung timing untouched.
    const uint32_t gap_cap_ticks = (target_ticks * (100 - kBaseGateLength)) / 100;
    const uint32_t gap_ticks = kBaseGateMinGapTicks < gap_cap_ticks ? kBaseGateMinGapTicks : gap_cap_ticks;

    const uint32_t max_ticks = until_next_ticks > gap_ticks ? until_next_ticks - gap_ticks : 0;

    gate_ticks_remaining_ = nominal_ticks < max_ticks ? nominal_ticks : max_ticks;
}

void Base::UpdateGateOut()
{
    Kastle2::hw.SetGateOut(
        clock_.IsOutputEnabled() &&
        gate_ticks_remaining_ > 0 &&
        sequencer_.GetTriggerOutput());
}

FASTCODE void Base::AfterAudioLoop(q15_t *input, q15_t *output, size_t size)
{
    if (!IsFeatureEnabled(Feature::BASE))
    {
        // If all features are disabled, just return
        return;
    }

    // apply sw volume
    if (sw_output_gain_ != Q15_MAX)
    {
        q15_scale_buffer(output, sw_output_gain_, 0, 2 * size);
    }

    // mix in input audio
    // since the output volume is also harware based,
    // it's not completely independent of the volume control
    if (IsFeatureEnabled(Feature::AUDIO_CHAIN) && Kastle2::hw.IsAudioInJackProbablyPlugged())
    {
        for (size_t i = 0; i < 2 * size; i++)
        {
            output[i] = q15_add(input[i], output[i]);
        }
    }

    if (IsFeatureEnabled(Feature::LFO_MOD_MAPPING))
    {
        for (size_t i = 0; i < 2 * size; i++)
        {
            output[i] = q15_mult(output[i], output_amplitude_multipler_);
            output[i] = q15_div(output[i], output_amplitude_divider_);
        }
    }

    // apply startup volume fade in (so the user can react to too high volume)
    // using the same envelope as for input fading
    if (startup_env_state_ == StartupEnvState::OUTPUT)
    {
        if (startup_env_.GetState() == AdsrEnv::State::DECAY)
        {
            startup_env_state_ = StartupEnvState::INPUT;
            startup_env_.Trigger();
        }
        q15_scale_buffer(input, q31_to_q15(startup_env_.GetOutput()), 0, 2 * size);
    }

    // Do time counters
    if (layer_timer_ < SIZE_MAX)
    {
        layer_timer_++;
    }
}

void Base::LayersHandling()
{
    bool settings_toggle = false;

    if (Kastle2::hw.Pressed(Hardware::Button::SHIFT) && Kastle2::hw.Pressed(Hardware::Button::MODE))
    {
        if (shift_and_mode_pressed_count_ > kBaseTicksEnterSettings)
        {
            if (!settings_toggled_)
            {
                settings_toggle = true;
                settings_toggled_ = true;

                // Reset stuff for settings here:
                midi_channel_tapping_active = false;
                midi_learn_start_allowed_ = false;
            }
        }
    }
    else
    {
        settings_toggled_ = false;
        shift_and_mode_pressed_count_ = 0;
    }

    prev_layer_ = Kastle2::hw.GetLayer();

    switch (Kastle2::hw.GetLayer())
    {
    case Hardware::Layer::NORMAL:

        if (Kastle2::hw.JustPressed(Hardware::Button::SHIFT))
        {
            Kastle2::hw.ChangeLayer(Hardware::Layer::SHIFT);
            clock_.ClearTaps();
        }
        if (Kastle2::hw.JustPressed(Hardware::Button::MODE))
        {
            Kastle2::hw.ChangeLayer(Hardware::Layer::MODE);
        }
        if (settings_toggle)
        {
            Kastle2::hw.ChangeLayer(Hardware::Layer::SETTINGS);
        }
        break;

    case Hardware::Layer::SHIFT:

        if (Kastle2::hw.JustReleased(Hardware::Button::SHIFT))
        {
            Kastle2::hw.ChangeLayer(Hardware::Layer::NORMAL);

            // save Pots into memory
            // Kastle2::memory.QueueUpdate16(Memory::ADDR_INPUT_GAIN, input_pot_);
            // Kastle2::memory.QueueUpdate16(Memory::ADDR_OUTPUT_GAIN, output_pot_);
            clock_.SaveToMemory();
        }
        if (settings_toggle)
        {
            Kastle2::hw.ChangeLayer(Hardware::Layer::SETTINGS);
        }
        break;

    case Hardware::Layer::MODE:
        if (Kastle2::hw.JustReleased(Hardware::Button::MODE))
        {
            Kastle2::hw.ChangeLayer(Hardware::Layer::NORMAL);
        }
        if (settings_toggle)
        {
            Kastle2::hw.ChangeLayer(Hardware::Layer::SETTINGS);
        }
        break;

    case Hardware::Layer::SETTINGS:
        if (settings_toggle)
        {
            Kastle2::hw.ChangeLayer(Hardware::Layer::NORMAL);
        }
        if (IsFeatureEnabled(Feature::LFO_MOD_MAPPING))
        {
            if (Kastle2::hw.JustPressed(Hardware::Button::SHIFT) && !Kastle2::hw.Pressed(Hardware::Button::MODE))
            {
                Kastle2::hw.ChangeLayer(Hardware::Layer::SETTINGS_SHIFT);
            }
            if (Kastle2::hw.JustPressed(Hardware::Button::MODE) && !Kastle2::hw.Pressed(Hardware::Button::SHIFT))
            {
                Kastle2::hw.ChangeLayer(Hardware::Layer::SETTINGS_MODE);
            }
        }
        break;

    case Hardware::Layer::SETTINGS_SHIFT:
        if (settings_toggle)
        {
            Kastle2::hw.ChangeLayer(Hardware::Layer::NORMAL);
        }
        if (Kastle2::hw.JustReleased(Hardware::Button::SHIFT) || Kastle2::hw.JustReleased(Hardware::Button::MODE))
        {
            Kastle2::hw.ChangeLayer(Hardware::Layer::SETTINGS);
        }
        break;

    case Hardware::Layer::SETTINGS_MODE:
        if (settings_toggle)
        {
            Kastle2::hw.ChangeLayer(Hardware::Layer::NORMAL);
        }
        if (Kastle2::hw.JustReleased(Hardware::Button::SHIFT) || Kastle2::hw.JustReleased(Hardware::Button::MODE))
        {
            Kastle2::hw.ChangeLayer(Hardware::Layer::SETTINGS);
        }
        break;
    }

    if (prev_layer_ != Kastle2::hw.GetLayer())
    {
        prev_layer_timer_ = layer_timer_;
        layer_timer_ = 0;
    }
}

void Base::BeforeUiLoop()
{
    if (!IsFeatureEnabled(Feature::BASE))
    {
        // If all features are disabled, just return
        return;
    }

    // Read pot values
    for (auto &pot : pots_)
    {
        pot->ReadValue();
    }
    for (auto pot_type : EnumRange<Hardware::Pot>())
    {
        if (midi_pots_enabled_.test(static_cast<size_t>(pot_type)))
        {
            midi_pots_[pot_type]->ReadValue();
        }
    }
    lfo_mod_.UpdateValues(pots_[Pot::LFO_MOD]->GetValue(), Kastle2::hw.GetAnalogValue(Hardware::AnalogInput::PARAM_2));

    // Switch layers
    LayersHandling();

    // INPUT AND OUTPUT GAINS
    // Combining software volume and codec setting

    // INPUT
    if (IsFeatureEnabled(Feature::INPUT_GAIN))
    {
        uint32_t input_pot = pots_[Pot::INPUT]->GetValue();
        Kastle2::codec.SetInputGain(input_pot >> 6); // hw gain
        sw_input_gain_ = pot_to_q15(input_pot);      // sw gain
    }

    // INPUT amplitude lfo modulation
    if (IsFeatureEnabled(Feature::LFO_MOD_MAPPING))
    {
        int32_t value = lfo_mod_.GetModValue(pots_[Pot::INPUT].get());
        if (value > 0)
        {
            input_amplitude_divider_ = map(value, 0, POT_MAX, Q15_MAX, q15(1.0f / 16.0f), MapClamp::TRUE);
            input_amplitude_multipler_ = Q15_MAX;
        }
        else if (value < 0)
        {
            input_amplitude_divider_ = Q15_MAX;
            input_amplitude_multipler_ = map(value, -POT_MAX, 0, Q15_ZERO, Q15_MAX, MapClamp::TRUE);
        }
        else
        {
            input_amplitude_divider_ = Q15_MAX;
            input_amplitude_multipler_ = Q15_MAX;
        }
    }

    // OUTPUT
    if (IsFeatureEnabled(Feature::OUTPUT_GAIN))
    {
        uint32_t output_pot = pots_[Pot::OUTPUT]->GetValue();
        if (output_pot < POT_HALF)
        {
            // if less than half, apply digital volume lowering of the volume
            sw_output_gain_ = pot_to_q15(output_pot * 2);
        }
        else
        {
            sw_output_gain_ = Q15_MAX; // sw to max
        }
        // scaling 0-1023 into 0-52 range (63 is the max, but we don't want to go that high)
        // POT_MAX + 10 to prevent noise when pot all the way to the max
        uint32_t hp_volume = map(output_pot, POT_MIN, POT_MAX + 10, 0, max_volume_);
        Kastle2::codec.SetHpVolume(hp_volume); // hw
    }

    // OUTPUT amplitude lfo modulation
    if (IsFeatureEnabled(Feature::LFO_MOD_MAPPING))
    {
        int32_t value = lfo_mod_.GetModValue(pots_[Pot::OUTPUT].get());
        if (value > 0)
        {
            output_amplitude_divider_ = map(value, 0, POT_MAX, Q15_MAX, q15(1.0f / 16.0f), MapClamp::TRUE);
            output_amplitude_multipler_ = Q15_MAX;
        }
        else if (value < 0)
        {
            output_amplitude_divider_ = Q15_MAX;
            output_amplitude_multipler_ = map(value, -POT_MAX, 0, Q15_ZERO, Q15_MAX, MapClamp::TRUE);
        }
        else
        {
            output_amplitude_divider_ = Q15_MAX;
            output_amplitude_multipler_ = Q15_MAX;
        }
    }

    // Input envelope follower from ENV out by default
    if (IsFeatureEnabled(Feature::ENV_OUT))
    {
        // downsize to 10 bits - resolution of the DAC)
        Kastle2::hw.SetEnvOut(input_envelope_follower_.GetEnvelope() >> (15 - 10));
    }

    // TEMPO
    uint32_t tempo_pot = lfo_mod_.AdjustWithMod(pots_[Pot::TEMPO].get(), pots_[Pot::TEMPO]->GetValue());
    clock_.SetPot(tempo_pot);

    // LFO
    int32_t lfo_pot = pots_[Pot::LFO]->GetValue();

    // Allowing a little bit of hysteresis
    if (lfo_pot > POT_HALF + 5)
    {
        lfo_sync_ = false;
    }
    if (lfo_pot < POT_HALF - 5)
    {
        lfo_sync_ = true;
    }

    // LFO in free mode (non-synced)
    if (!lfo_sync_)
    {
        float freq = curve_map(lfo_pot, kBaseLfoMap);

        // Apply mod (proper 1V/Oct)
        int32_t lfo_mod_val = lfo_mod_.GetModValue(pots_[Pot::LFO_MOD].get());
        if (lfo_mod_val)
        {

            freq = cv_to_freq_raw(freq, lfo_mod_val);
        }

        // update frequency
        lfo_.SetFrequency(freq);
    }
    else
    // LFO in tempo-synced mode
    {
        if (pots_[Pot::LFO]->HasChanged())
        {
            lfo_pot_ratio_ = curve_map(lfo_pot, kBaseLfoRatioMap);
        }
        lfo_.SetClockTicks(clock_.GetTargetTicks()); // update clock speed (ticks)

        int32_t ratio = lfo_pot_ratio_;

        // Apply mod
        int32_t lfo_mod_val = lfo_mod_.GetModValue(pots_[Pot::LFO_MOD].get());
        if (lfo_mod_val)
        {
            int32_t abs_lfo_mod_val = map(abs(lfo_mod_val), 0, ADC_5V + 1, 0, kBaseLfoRatios.size());
            if (lfo_mod_val < 0)
            {
                abs_lfo_mod_val *= -1;
            }
            lfo_mod_val = abs_lfo_mod_val;
        }
        ratio = constrain(ratio + lfo_mod_val, 0, kBaseLfoRatios.size() - 1);

        // detect if the lfo is self patched
        if ((lfo_.GetSquareOut() > 0) != lfo_state_prev_)
        {
            lfo_state_prev_ = lfo_.GetSquareOut() > 0;
            if (lfo_change_timer_ == 0)
            {
                lfo_change_timer_ = 1000;
                lfo_self_patched_ = 0;
            }
            else if (lfo_last_timer_source_ == true)
            {
                lfo_self_patched_++;
                lfo_change_timer_ = 0;
            }
            lfo_last_timer_source_ = false;
        }

        if (diff(lfo_mod_state_prev, Kastle2::hw.GetAnalogValue(Hardware::AnalogInput::PARAM_2)) > (ADC_5V / 10))
        {
            lfo_mod_state_prev = Kastle2::hw.GetAnalogValue(Hardware::AnalogInput::PARAM_2);
            if (lfo_change_timer_ == 0)
            {
                lfo_change_timer_ = 1000;
                lfo_self_patched_ = 0;
            }
            else if (lfo_last_timer_source_ == false)
            {
                lfo_self_patched_++;
                lfo_change_timer_ = 0;
            }
            lfo_last_timer_source_ = true;
        }

        if (lfo_change_timer_ > 0)
        {
            lfo_change_timer_--;
        }

        lfo_.DisableSlowingDown(lfo_self_patched_ >= 1);
        lfo_.SetRatio(kBaseLfoRatios[ratio]);
    }

    // Generate triggers/rhythms
    int32_t rhythm_modulation = lfo_mod_.GetModValue(pots_[Pot::RHYTHM].get());
    if (pots_[Pot::RHYTHM]->HasChanged() || diff(rhythm_modulation, rhythm_modulation_prev_) > 10)
    {
        rhythm_modulation_prev_ = rhythm_modulation;
        sequencer_.GenerateTriggersUsingTable(
            pot_to_q15(pots_[Pot::RHYTHM]->GetValue() + rhythm_modulation));
    }

    int32_t swing_modulation = lfo_mod_.GetModValue(pots_[Pot::SWING].get());
    if (pots_[Pot::SWING]->HasChanged() || diff(swing_modulation, swing_modulation_prev_) > 10)
    {
        swing_modulation_prev_ = swing_modulation;
        int32_t swing_value = pots_[Pot::SWING]->GetValue() + swing_modulation;
        swing_value = constrain(swing_value, 0, POT_MAX);
        sequencer_.SetSwing(static_cast<float>(swing_value) / static_cast<float>(POT_MAX));
    }

    leds_should_be_off_ = shift_and_mode_pressed_count_ > 1000;

    if (Kastle2::hw.HasLedsJustUpdated())
    {
        fake_blinker_.Process();
    }

    // Gate indication
    if (Kastle2::hw.GetLayer() == Hardware::Layer::SHIFT)
    {
        if (
            IsFeatureEnabled(Feature::GATE_INDICATION) &&
            !leds_should_be_off_ &&
            sequencer_.GetTriggerOutput() &&
            clock_.GetHalfDutyOutput())
        {
            Kastle2::hw.SetLed(Hardware::Led::LED_2, kBaseColorGate);
        }
        else
        {
            Kastle2::hw.SetLed(Hardware::Led::LED_2, WS2812::NONE);
        }
    }

    // LFO/TEMPO LED
    if (
        Kastle2::hw.GetLayer() == Hardware::Layer::MODE ||
        Kastle2::hw.GetLayer() == Hardware::Layer::SHIFT ||
        Kastle2::hw.GetLayer() == Hardware::Layer::NORMAL)
    {
        if (!leds_should_be_off_)
        {
            bool show_tempo = Kastle2::hw.GetLayer() == Hardware::Layer::SHIFT;
            if (show_tempo)
            {
                // Showing TEMPO

                // Pick the color
                uint32_t color = kBaseColorTempoInternal;
                switch (sync_setting_)
                {
                case Memory::SyncSetting::EXTERNAL_DISABLED:
                    color = kBaseColorTempoExternalDisabled;
                    break;
                case Memory::SyncSetting::MIDI_DISABLED:
                    color = kBaseColorTempoMidiDisabled;
                    break;
                }
                switch (clock_.GetSyncType())
                {
                case Clock::Sync::EXTERNAL:
                    color = kBaseColorTempoExternal;
                    break;
                case Clock::Sync::MIDI:
                    color = kBaseColorTempoMidi;
                    break;
                }

                // We use the FakeBlinker class to solve the interferences
                bool led_state = fake_blinker_.GetTempoLedState(clock_);
                Kastle2::hw.SetLed(Hardware::Led::LED_3, led_state ? color : 0);
            }
            else if (Kastle2::hw.GetLayer() == Hardware::Layer::MODE)
            {
                // BANK/MODE layer: show swing state on LED_3, or keep it black
                // when swing is in the dead zone.
                switch (sequencer_.GetSwingType())
                {
                case Sequencer::SwingType::SWING:
                case Sequencer::SwingType::HUMANIZE:
                {
                    uint32_t color = (sequencer_.GetSwingType() == Sequencer::SwingType::SWING)
                                         ? WS2812::ORANGE
                                         : WS2812::CYAN;
                    uint8_t brightness = static_cast<uint8_t>(sequencer_.GetSwingAmount() * 255.0f);
                    Kastle2::hw.SetLed(Hardware::Led::LED_3, WS2812::ApplyBrightness(color, brightness));
                    break;
                }
                case Sequencer::SwingType::NONE:
                default:
                    Kastle2::hw.SetLed(Hardware::Led::LED_3, WS2812::NONE);
                    break;
                }
            }
            else
            {
                // Showing LFO LED (only when the outputs are enabled)
                if (IsFeatureEnabled(Feature::LFO_OUT))
                {

                    // Pick the color
                    uint32_t color = lfo_.IsSynced() ? kBaseColorLfoSynced : kBaseColorLfoFree;

                    // We use the FakeBlinker class to solve the interferences
                    uint8_t brightness = fake_blinker_.GetLfoLedBrightness(lfo_);

                    // Set the LED
                    Kastle2::hw.SetLed(Hardware::Led::LED_3, WS2812::ApplyBrightness(color, brightness));
                }
            }
            if (IsFeatureEnabled(Feature::INPUT_INDICATION))
            {
                q15_t in_env = input_envelope_follower_.GetEnvelope();
                uint8_t green = curve_map(in_env, kMapLoudnessLedGreen);
                uint8_t red = curve_map(in_env, kMapLoudnessLedRed);
                Kastle2::hw.SetLed(Hardware::Led::LED_1, red, green, 0);
            }
        }
        else
        {
            Kastle2::hw.SetLed(Hardware::Led::LED_3, WS2812::NONE);
        }
    }

    // Clipping counter (visible across all layers)
    if (input_envelope_follower_.GetEnvelope() > q15(0.9f))
    {
        input_clipping_counter_ = kClippingShowTicks;
    }

    // Lfo Modulation destination change indication (visible across all settings modes)
    if (IsFeatureEnabled(Feature::LFO_MOD_MAPPING))
    {
        if (lfo_mod_.DestinationChanged())
        {
            Kastle2::memory.Write8(Memory::ADDR_LFO_MOD_ASSIGNMENT, static_cast<unsigned int>(lfo_mod_.GetDestination()));
        }
    }

    // Settings layer stuff
    if (Kastle2::hw.GetLayer() == Hardware::Layer::SETTINGS)
    {
        MidiAdvancedSettings(cancel_midi_learn_tap_, cancel_midi_learn_tap_);

        if (!Kastle2::hw.Pressed(Hardware::Button::SHIFT) && !Kastle2::hw.Pressed(Hardware::Button::MODE))
        {
            cancel_midi_learn_tap_ = false; // Reset the cancel flag set by lfo mod assigning
                                            // (but only when both buttons are released)
        }

        // Other settings
        Memory::MonoSetting prev_mono_setting = mono_setting_;
        Memory::SyncSetting prev_sync_setting = sync_setting_;
        if (pots_[Pot::SETTINGS_MONO_INPUT]->HasChanged())
        {
            uint32_t mono_input_pot = pots_[Pot::SETTINGS_MONO_INPUT]->GetValue();
            if (mono_input_pot < (POT_THIRD - kSettingsHysteresis))
            {
                mono_setting_ = Memory::MonoSetting::LEFT;
            }
            if (mono_input_pot > (POT_THIRD + kSettingsHysteresis) && mono_input_pot < (POT_TWO_THIRDS - kSettingsHysteresis))
            {
                mono_setting_ = Memory::MonoSetting::STEREO;
            }
            if (mono_input_pot > (POT_TWO_THIRDS + kSettingsHysteresis))
            {
                mono_setting_ = Memory::MonoSetting::RIGHT;
            }
        }
        if (pots_[Pot::SETTINGS_SYNC_INPUT]->HasChanged())
        {
            uint32_t sync_input_pot = pots_[Pot::SETTINGS_SYNC_INPUT]->GetValue();
            if (sync_input_pot < (POT_THIRD - kSettingsHysteresis))
            {
                sync_setting_ = Memory::SyncSetting::MIDI_DISABLED;
            }
            if (sync_input_pot > (POT_THIRD + kSettingsHysteresis) && sync_input_pot < (POT_TWO_THIRDS - kSettingsHysteresis))
            {
                sync_setting_ = Memory::SyncSetting::NONE_DISABLED;
            }
            if (sync_input_pot > (POT_TWO_THIRDS + kSettingsHysteresis))
            {
                sync_setting_ = Memory::SyncSetting::EXTERNAL_DISABLED;
            }
        }

        // Save to memory
        if (prev_mono_setting != mono_setting_)
        {
            Kastle2::memory.Write8(Memory::ADDR_MONO_SETTINGS, static_cast<unsigned int>(mono_setting_));
        }
        if (prev_sync_setting != sync_setting_)
        {
            Kastle2::memory.Write8(Memory::ADDR_SYNC_SETTINGS, static_cast<unsigned int>(sync_setting_));
        }

        // Display the settings
        uint32_t sync_color = WS2812::NONE;
        switch (sync_setting_)
        {
        case Memory::SyncSetting::MIDI_DISABLED:
            sync_color = kBaseColorTempoMidiDisabled;
            break;
        case Memory::SyncSetting::EXTERNAL_DISABLED:
            sync_color = kBaseColorTempoExternalDisabled;
            break;
        case Memory::SyncSetting::NONE_DISABLED:
            sync_color = kBaseColorTempoNoneDisabled;
            break;
        }

        uint32_t input_color = WS2812::NONE;
        switch (mono_setting_)
        {
        case Memory::MonoSetting::LEFT:
            input_color = kBaseColorMonoLeft;
            break;
        case Memory::MonoSetting::RIGHT:
            input_color = kBaseColorMonoRight;
            break;
        case Memory::MonoSetting::STEREO:
            input_color = kBaseColorStereo;
            break;
        }

        uint8_t brightness = GetSettingsLedPulse();
        Kastle2::hw.SetLed(Hardware::Led::LED_1, WS2812::ApplyBrightness(input_color, brightness));
        Kastle2::hw.SetLed(Hardware::Led::LED_2, WS2812::NONE);
        Kastle2::hw.SetLed(Hardware::Led::LED_3, WS2812::ApplyBrightness(sync_color, brightness));
    }

    if (IsFeatureEnabled(Feature::LFO_MOD_MAPPING))
    {
        if (Kastle2::hw.GetLayer() == Hardware::Layer::SETTINGS_SHIFT ||
            Kastle2::hw.GetLayer() == Hardware::Layer::SETTINGS_MODE)
        {
            // Clear the LEDs when in the LFO MOD assignment
            Kastle2::hw.SetLed(Hardware::Led::LED_1, 0);
            Kastle2::hw.SetLed(Hardware::Led::LED_2, 0);
            Kastle2::hw.SetLed(Hardware::Led::LED_3, 0);

            MidiAdvancedSettings(cancel_midi_learn_tap_, cancel_midi_learn_tap_);

            if(Kastle2::hw.JustPressed(Hardware::Button::SHIFT) || Kastle2::hw.JustPressed(Hardware::Button::MODE))
            {
                ui_lfo_mod_last_destination_ = LfoMod::Destination::COUNT;
            }
        }

        bool cancel_midi_learn_tap = false;
        for (auto destination : EnumRange<LfoMod::Destination>())
        {
            FancyPot *destination_pot = lfo_mod_pots_[destination];
            if (destination_pot == nullptr)
            {
                continue;
            }

            if (!destination_pot->HasMoved(FancyPot::Move::MOVE))
            {
                continue;
            }

            if (!lfo_mod_selection_enabled_[destination])
            {
                if (Kastle2::hw.GetLayer() == Hardware::Layer::SETTINGS_MODE)
                {
                    cancel_midi_learn_tap = true;
                }
                continue;
            }

            lfo_mod_.SetDestination(destination);
            cancel_midi_learn_tap_ = true;

            if (destination != ui_lfo_mod_last_destination_)
            {
                ui_lfo_mod_last_destination_ = destination;
                IndicateLfoModDestChange();
            }
        }

        if (cancel_midi_learn_tap)
        {
            cancel_midi_learn_tap_ = true;
        }
    }

    // Memory clear (Factory reset)
    if (shift_and_mode_pressed_count_ > kBaseTicksClearMemory)
    {
        sleep_ms(200);
        if (Kastle2::memory.WriteDefaultSettings())
        {
            if (Kastle2::app != nullptr)
            {
                Kastle2::app->MemoryInitialization();
            }
            // Blink all LEDs green until both buttons are released
            while (Kastle2::hw.GetRawButtonState(Hardware::Button::SHIFT) || Kastle2::hw.GetRawButtonState(Hardware::Button::MODE))
            {
                Kastle2::hw.SetLed(Hardware::Led::LED_1, WS2812::GREEN);
                Kastle2::hw.SetLed(Hardware::Led::LED_2, WS2812::GREEN);
                Kastle2::hw.SetLed(Hardware::Led::LED_3, WS2812::GREEN);
                Kastle2::hw.LatchLeds();
                sleep_ms(200);
                Kastle2::hw.SetLed(Hardware::Led::LED_1, WS2812::NONE);
                Kastle2::hw.SetLed(Hardware::Led::LED_2, WS2812::NONE);
                Kastle2::hw.SetLed(Hardware::Led::LED_3, WS2812::NONE);
                Kastle2::hw.LatchLeds();
                sleep_ms(200);
            }
        }
        else
        {
            Kastle2::hw.ShowStartupMessage(Hardware::StartupMessage::EEPROM_INIT_FAIL);
        }
        watchdog_reboot(0, 0, 10);
    }
}

void Base::AfterUiLoop()
{
    // If all features are disabled, just return
    if (!IsFeatureEnabled(Feature::BASE))
    {
        return;
    }

    // Show the MIDI learn LED
    if (Kastle2::hw.GetLayer() == Hardware::Layer::SETTINGS ||
        Kastle2::hw.GetLayer() == Hardware::Layer::SETTINGS_SHIFT ||
        Kastle2::hw.GetLayer() == Hardware::Layer::SETTINGS_MODE)
    {
        // Learn LED
        switch (Kastle2::midi.GetLearnState())
        {
        case midi::Handler::LearnState::LEARNING:
            Kastle2::hw.SetLed(Hardware::Led::LED_2, WS2812::NONE);
            break;
        case midi::Handler::LearnState::LEARNED:
            Kastle2::hw.SetLed(Hardware::Led::LED_2, WS2812::ORANGE);
            break;
        }
        // This actually overrides the value set by the app
        if (midi_number_flasher_.IsActive())
        {
            Kastle2::hw.SetLed(Hardware::Led::LED_2, midi_number_flasher_.GetLedState() ? WS2812::ORANGE : WS2812::NONE);
        }
    }

    // Show red clipping LED in non-SHIFT layers
    if (IsFeatureEnabled(Feature::INPUT_INDICATION_CLIP) && input_clipping_counter_ > 0)
    {
        if (Kastle2::hw.GetLayer() == Hardware::Layer::NORMAL ||
            Kastle2::hw.GetLayer() == Hardware::Layer::MODE)
        {
            Kastle2::hw.SetLed(Hardware::Led::LED_1, WS2812::RED);
        }
    }

    if (IsFeatureEnabled(Feature::LFO_MOD_MAPPING))
    {
        if (Kastle2::hw.GetLayer() == Hardware::Layer::SETTINGS ||
            Kastle2::hw.GetLayer() == Hardware::Layer::SETTINGS_SHIFT ||
            Kastle2::hw.GetLayer() == Hardware::Layer::SETTINGS_MODE)
        {
            if (IsLfoModDestChangeActive())
            {
                int32_t color = LfoModDestChangeColor();
                Kastle2::hw.SetLed(Hardware::Led::LED_1, color);
                Kastle2::hw.SetLed(Hardware::Led::LED_2, color);
                Kastle2::hw.SetLed(Hardware::Led::LED_3, color);
            }
        }
    }
}

void Base::MidiAdvancedSettings(bool cancel_learning, bool cancel_tapping)
{
    // MIDI LEARN

    if (cancel_learning)
    {
        Kastle2::midi.CancelLearning();
    }
    else
    {
        // Mode must be released at the start
        if (!Kastle2::hw.Pressed(Hardware::Button::MODE))
        {
            midi_learn_start_allowed_ = true;
        }

        // Hold at least 1 second to start MIDI learn
        if (!Kastle2::hw.Pressed(Hardware::Button::SHIFT) && Kastle2::hw.PressedMillis(Hardware::Button::MODE) > 1000 && midi_learn_start_allowed_)
        {
            // Start MIDI learn
            Kastle2::midi.StartLearning();
        }
        if (Kastle2::hw.JustReleased(Hardware::Button::MODE))
        {
            // Stop MIDI learn
            if (Kastle2::midi.StopLearning())
            {
                midi_number_flasher_.FlashNumber(Kastle2::midi.GetChannel() + 1);
            }
        }
    }

    // MIDI TAP CHANNEL
    if (cancel_tapping)
    {
        midi_channel_tapping_active = false;
    }
    else
    {
        if (Kastle2::hw.JustPressed(Hardware::Button::SHIFT))
        {
            midi_channel_taps_ = 0;
            midi_channel_tapping_active = true;
        }
        if (Kastle2::hw.Pressed(Hardware::Button::SHIFT) && Kastle2::hw.JustReleased(Hardware::Button::MODE))
        {
            if (midi_channel_tapping_active && midi_channel_taps_ < 16)
            {
                midi_channel_taps_++;
            }
        }
        if (Kastle2::hw.JustReleased(Hardware::Button::SHIFT))
        {
            if (midi_channel_tapping_active)
            {
                midi_channel_tapping_active = false;
                // Save the MIDI tap channel
                if (midi_channel_taps_ > 0)
                {
                    // -1 because MIDI channels are 0-15, but we tap 1-16
                    if (Kastle2::midi.SetChannel(midi_channel_taps_ - 1))
                    {
                        midi_number_flasher_.FlashNumber(Kastle2::midi.GetChannel() + 1);
                    }
                }
            }
        }
    }
}

void Base::MidiCallback(midi::Message *msg)
{
    for (auto &pot : pots_)
    {
        pot->MidiCallback(msg);
    }

    if (IsFeatureEnabled(Feature::MIDI_CLOCK) &&
        sync_setting_ != Memory::SyncSetting::MIDI_DISABLED)
    {
        // Only for time-related messages
        if (msg->IsClock() ||
            msg->IsStart() ||
            msg->IsStop() ||
            msg->IsContinue())
        {
            // Does currently selected MIDI source match?
            if (clock_.CheckMidiSource(msg->GetSource()))
            {
                if (msg->IsClock())
                {
                    clock_midi_pulse_ = true;
                }
                if (msg->IsStart())
                {
                    clock_.Start();
                    if (lfo_.IsSynced())
                    {
                        lfo_.SetPhase(Q31_MIN);
                    }
                }
                if (msg->IsStop())
                {
                    clock_.Stop();
                }
                if (msg->IsContinue())
                {
                    clock_.Resume();
                }
            }
        }
    }
}

Clock &Base::GetClock()
{
    return clock_;
}

Sequencer &Base::GetSequencer()
{
    return sequencer_;
}

Lfo &Base::GetLfo()
{
    return lfo_;
}

void Base::IndicateLfoModDestChange()
{
    ui_lfo_mod_change_indication_counter_ = kUiIndicateLfoModChangeTimeDark * 2 + kUiIndicateLfoModChangeTimeLight;
}

void Base::DecrementLfoModDestChangeCounter()
{
    if (ui_lfo_mod_change_indication_counter_ > 0)
    {
        ui_lfo_mod_change_indication_counter_--;
    }
}

bool Base::IsLfoModDestChangeActive()
{
    return ui_lfo_mod_change_indication_counter_ > 0;
}

uint32_t Base::LfoModDestChangeColor()
{
    if (ui_lfo_mod_change_indication_counter_ > kUiIndicateLfoModChangeTimeDark &&
        ui_lfo_mod_change_indication_counter_ < kUiIndicateLfoModChangeTimeLight + kUiIndicateLfoModChangeTimeDark)
    {
        // We don't use the custom colors for now
        // return lfo_mod_.GetTarget().color;

        // Global color signal for LFO MOD destination change
        return kBaseColorLfoModDestChange;

    }
    return WS2812::BLACK;
}
