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

#include "AppWaveBard.hpp"
#include "common/controls/ShiftTrigger.hpp"
#include "common/core/Kastle2.hpp"
#include "common/core/MultiCore.hpp"
#include "common/core/UserDataFile.hpp"
#include "common/peripherals/WS2812.hpp"
#include "common/utils.hpp"
#include "WaveBardParameterMaps.hpp"

#define DONT_RETRIGGER_IN_REVERSE                     // comment out to enable retriggering when playing in reverse
#define SAMPLE_DECAY_ON_TRIGGER                       // comment out to sample decay continuously
#define SAMPLE_NOTE_ON_TRIGGER                        // comment out to sample note continuously
#define OUT_ENVELOPE_SCALING                          // comment out to disable scaling of the output envelope in order to match the sample length
#define QUANTIZER_ROOT_TRANSPOSES                     // comment out to use the original style of quantizer control, where the root selection only transposes the scale, not the pitch
#define FILTER_VOLUME_COMPENSATION                    // increases the overall volume when filter is not active
#define PLAYBACK_CLIPPER                              // used eg. for FX Chorus distortion
#define PASSTHROUGH_MIDI_NOTE                         // if enabled the MIDI note is passed through to the sample playback, otherwise the patch pitch MIDI note is used
#define LOWEST_TWO_MIDI_OCTAVES_SELECT_ORIGINAL_PITCH // if enabled, the lowest two octaves (0-23) select the original pitch of the sample, otherwise the pitch kept as it is

// Currently we only support this
#define SUPPORTED_BIT_DEPTH 16

using namespace kastle2;

void AppWaveBard::Init()
{
    inited_ = false;

    // Fail if the samples are not loaded
    if (!LoadSamples() || samples_.bit_depth != SUPPORTED_BIT_DEPTH)
    {
        Kastle2::base.SetAllFeaturesEnabled(false);
        Kastle2::hw.SetLed(Hardware::Led::LED_3, 0);
        while (1)
        {
            Kastle2::hw.SetLed(Hardware::Led::LED_1, WS2812::RED);
            Kastle2::hw.SetLed(Hardware::Led::LED_2, 0);
            Kastle2::hw.LatchLeds();
            sleep_ms(250);
            Kastle2::hw.SetLed(Hardware::Led::LED_1, 0);
            Kastle2::hw.SetLed(Hardware::Led::LED_2, WS2812::RED);
            Kastle2::hw.LatchLeds();
            sleep_ms(250);
        }
        return;
    }

    // Disable audio chain, since we mix the audio ourselves
    Kastle2::base.SetFeatureEnabled(Base::Feature::AUDIO_CHAIN, false);
    Kastle2::base.SetFeatureEnabled(Base::Feature::ENV_OUT, false);
    Kastle2::base.SetFeatureEnabled(Base::Feature::INPUT_INDICATION_CLIP, false);

    // Tiny bit max higher volume to match FX Wizard
    Kastle2::base.SetMaxVolume(54);

    // Sequencer length from the loaded file
    Kastle2::base.GetSequencer().SetLength(samples_.sequencer_length);

    // Update rhythm generator with the loaded rhythms
    Kastle2::base.GetSequencer().SetTriggerGeneratorTable(
        std::span{samples_.rhythms, samples_.num_rhythms});

    // Fake Tempo and LFO LEDs
    Kastle2::base.GetFakeBlinker().SetEnabled(true);

    // We have two players to prevent clicks when retrigerring/switching samples
    for (auto &player : players_)
    {
        // Samples sample-rate is stored in the Wave Bard file
        // Can (and usually is) be different from the system sample rate
        player.Init(SAMPLE_RATE, samples_.sample_rate);
        player.SetHifi(true);
    }

    // The active player is the one which is currently outputting audio
    active_player_ = PlayerDeck::A;

    // Quantizer stuff
    quantizer_.Init();
    quantizer_.SetScaleTable(std::span{samples_.scales, samples_.num_scales});
    quantizer_.SetScale(samples_.num_scales / 2); // Selects the middle scale

    // Initialize the bank selector
    bank_select_.Init(samples_.num_banks);
    input_audio_through_fx_ = Kastle2::memory.Get8(kMemAudioRouting) != 0;

    // POTS

    // Normal layer
    pots_[Pot::PITCH_MOD] = FancyPot::Create({
        .pot = Hardware::Pot::POT_1,
        .layer = Hardware::Layer::NORMAL,
        .midi_cc = cc::PITCH_MOD,
        .deadzone = true,
    });
    pots_[Pot::SAMPLE_MOD] = FancyPot::Create({.pot = Hardware::Pot::POT_2,
                                               .layer = Hardware::Layer::NORMAL,
                                               .midi_cc = cc::SAMPLE_MOD,
                                               .deadzone = true,
                                               .freeze = true});
    pots_[Pot::ENVELOPE] = FancyPot::Create({.pot = Hardware::Pot::POT_4,
                                             .layer = Hardware::Layer::NORMAL,
                                             .midi_cc = cc::LENGTH,
                                             .deadzone = true});
    pots_[Pot::PITCH] = FancyPot::Create({.pot = Hardware::Pot::POT_5,
                                          .layer = Hardware::Layer::NORMAL,
                                          .midi_cc = cc::PITCH});
    pots_[Pot::SAMPLE] = FancyPot::Create({.pot = Hardware::Pot::POT_6,
                                           .layer = Hardware::Layer::NORMAL,
                                           .map_size = samples_.num_samples,
                                           .midi_cc = cc::SAMPLE,
                                           .midi_note_control = {
                                               .start = 0,                       // Start at note 0
                                               .repeat = kMidiNoteControlRepeat, // Repeat each 12 notes
                                               .end = kMidiMinNote,              // End at note 36 (exclusive)
                                           },
                                           .freeze = true});

    // Shift layer
    pots_[Pot::ENVELOPE_MOD] = FancyPot::Create({.pot = Hardware::Pot::POT_4,
                                                 .layer = Hardware::Layer::SHIFT,
                                                 .initial_value = POT_MAX,
                                                 .midi_cc = cc::LENGTH_MOD,
                                                 .deadzone = true});
    pots_[Pot::FX] = FancyPot::Create({.pot = Hardware::Pot::POT_2,
                                       .layer = Hardware::Layer::SHIFT,
                                       .midi_cc = cc::FX,
                                       .deadzone = true,
                                       .freeze = true});
    pots_[Pot::FILTER] = FancyPot::Create({.pot = Hardware::Pot::POT_6,
                                           .layer = Hardware::Layer::SHIFT,
                                           .midi_cc = cc::FILTER,
                                           .deadzone = true});

    // Mode layer
    pots_[Pot::PITCH_SCALE] = FancyPot::Create({.pot = Hardware::Pot::POT_1,
                                                .layer = Hardware::Layer::MODE,
                                                .map_size = samples_.num_scales,
                                                .midi_cc = cc::SCALE,
                                                .deadzone = true,
                                                .memory_addr = kMemPotScale});
    pots_[Pot::PITCH_ROOT] = FancyPot::Create({.pot = Hardware::Pot::POT_2,
                                               .layer = Hardware::Layer::MODE,
                                               .initial_value = POT_MIN,
                                               .map_size = 12, // 12 semitones
                                               .midi_cc = cc::ROOT,
                                               .memory_addr = kMemPotRoot});
    pots_[Pot::PITCH_FINE] = FancyPot::Create({.pot = Hardware::Pot::POT_3,
                                               .layer = Hardware::Layer::MODE,
                                               .midi_cc = cc::FINE,
                                               .deadzone = true,
                                               .memory_addr = kMemPotFine});
    pots_[Pot::BANK_MOD] = FancyPot::Create({.pot = Hardware::Pot::POT_4,
                                             .layer = Hardware::Layer::MODE,
                                             .initial_value = POT_MAX,
                                             .midi_cc = cc::BANK_MOD});
    pots_[Pot::PITCH_QUANTIZED] = FancyPot::Create({.pot = Hardware::Pot::POT_5,
                                                    .layer = Hardware::Layer::MODE});

    // Advanced settings
    pots_[Pot::AUDIO_ROUTE] = FancyPot::Create({.pot = Hardware::Pot::POT_5,
                                                .layer = Hardware::Layer::SETTINGS});

    for (auto &pot : pots_)
    {
        pot->Init(AUDIO_LOOP_RATE);
    }

    // This disables the next change when the pots are moved
    // or when the current layer time is over the specified number of ticks
    bank_select_.DisableNextChangeWhen(pots_, kModeShortPressUnder);

    // Disable Length CC pot sending - we send our own length manually as split values
    Kastle2::base.SetMidiOutPotEnabled(pots_[Pot::ENVELOPE]->GetPot(), false);

    sample_bank_selected_ = bank_select_.GetMode();
    sample_num_selected_ = 0;
    sample_num_selected_prev_ = 0;
    sample_play_on_change_ = false;

    prev_sample_cv_ = 0;
    sample_cv_ = 0;

    fx_volume_compensation_ = Q15_ZERO;

    pitch_note_cv_ = 0;
    pitch_note_cv_prev_ = 0;

    quantization_enabled_ = false;

    delay_feedback_ = Q15_ZERO;
    delay_mix_ = Q15_ZERO;
    delay_stereo_left_ = 0;
    delay_stereo_right_ = 0;

    ui_indicate_change_time_ = 0;

    delay_left_ = std::make_unique<AdvancedDynamicDelayLine<q15least_t>>(22000);
    delay_right_ = std::make_unique<AdvancedDynamicDelayLine<q15least_t>>(22000);

#ifdef PLAYBACK_CLIPPER
    playback_clipper_.Init(SAMPLE_RATE);
    playback_clipper_.SetDrive(0);
#endif
    fx_input_clipper_.Init(SAMPLE_RATE);
    fx_input_clipper_.SetDrive(0);
    soft_clipper_.Init(SAMPLE_RATE);
    soft_clipper_.SetDrive(0);
    filter_.Init(SAMPLE_RATE);

    // Filter volume compensation because of the resonance
    filter_volume_compensation_slewer_.Init();
    filter_volume_compensation_slewer_.SetSpeed(3);

    fx_compressor_.Init(SAMPLE_RATE);
    fx_compressor_.SetAttackTime(kCompressorAttack);
    fx_compressor_.SetReleaseTime(kCompressorRelease);

    envelope_.Init(SAMPLE_RATE);
    envelope_.SetAttackTime(0.001f);
    envelope_.SetHoldTime(0.2f);
    envelope_.SetDecayTime(0.5f);
    envelope_.SetNonResetting(AdsrEnv::NonResetting::NONE);

    // Setting the sample frequency at the audio loop call rate to save processing
    // power as we only need to call it once, not 48 times
    envelope_indicator_.Init(AUDIO_LOOP_RATE);
    envelope_indicator_.SetAttackTime(0.001);
    envelope_indicator_.SetDecayTime(0.5);
    envelope_indicator_.SetNonResetting(AdsrEnv::NonResetting::NONE);

    envelope_out_.Init(AUDIO_LOOP_RATE);
    envelope_out_.SetAttackTime(0.001);
    envelope_out_.SetHoldTime(0.f);
    envelope_out_.SetDecayTime(0.7);
    envelope_out_.SetNonResetting(AdsrEnv::NonResetting::NONE);

    // Fadeout envelope that prevents clicks
    fadeout_envelope_.Init(SAMPLE_RATE);
    fadeout_envelope_.SetAttackTime(0.0000001f);
    fadeout_envelope_.SetDecayTime(0.01f);
    fadeout_envelope_.SetNonResetting(AdsrEnv::NonResetting::NONE);
    fadeout_envelope_max_ = Q15_MAX;

    note_sender_.Init(AUDIO_LOOP_RATE);
    note_sender_.SetDuration(1.0f);

    chorus_lfo_.Init(AUDIO_LOOP_RATE);
    chorus_lfo_.SetWaveform(Oscillator::Waveform::TRI);
    chorus_lfo_.SetFrequency(0.5f);

    last_trigger_time_ = get_absolute_time();
    trigger_spacing_us_ = 0;

    pitch_source_ = PitchSource::PATCH;

    inited_ = true;
}

void AppWaveBard::DeInit()
{
    inited_ = false;
}

FASTCODE void AppWaveBard::AudioLoop(q15_t *input, q15_t *output, size_t size)
{
    if (!inited_)
    {
        return;
    }

    for (auto &pot : pots_)
    {
        pot->Process();
    }

    bank_select_.Process();

    if (trigger_.Process(Kastle2::hw.GetTriggerIn()))
    {
#ifndef DONT_RETRIGGER_IN_REVERSE
        Trigger();
#else
        // Trigger the sample (if not in reverse playback)
        if (!(players_[active_player_].IsPlaying() && players_[active_player_].GetReverse()))
        {
            Trigger();
        }
#endif
    }

    // Handle MIDI note on/off sending
    note_sender_.Process();

    // Set data for the second core
    input_buffer_ = input;
    output_buffer_ = output;
    buffer_size_ = size;

    // Update the compressor
    q15_t peak;
    // cut off anything that's below the distortion threshold
    peak = std::max(fx_compressor_max_ - q15(0.166f), Q15_ZERO);
    // expand it to full-range
    peak = q15_div(peak, q15(0.4f));
    fx_compressor_.UpdatePeak(peak);
    fx_compressor_max_ = Q15_ZERO;

    // Tell the second core we're starting a new block
    MultiCore::SendMessage(MultiCore::MessageType::BEGIN);

    // Envelope indicator and out envelope can be processed in interrupt rate (not audio rate)
    envelope_indicator_.Process();
#ifdef OUT_ENVELOPE_SCALING
    envelope_out_.Process();
#endif

    // Process the chorus
    int32_t lfo_value = chorus_lfo_.Process();
    // Scale down to 10 bits and shift to 0-1023
    lfo_value = (lfo_value >> 22) + 512;
    chorus_lfo_val_ = lfo_value;

    // Go through all buffer
    for (size_t i = 0; i < size; i++)
    {
        q15_t left = 0;
        q15_t right = 0;

        envelope_.Process();
        fadeout_envelope_.Process();

        // Main player
        auto &main_player = players_[active_player_];
        if (main_player.IsPlaying())
        {
            // Read sample
            main_player.Process();
            left = main_player.GetLeft();
            right = main_player.GetRight();

            // Apply envelope
            q15_t env = q31_to_q15(envelope_.GetOutput());
            left = q15_mult(left, env);
            right = q15_mult(right, env);
        }

        // Other player
        auto &other_player = players_[EnumIncrement<PlayerDeck>(active_player_)];
        if (other_player.IsPlaying() && fadeout_envelope_.IsActive())
        {
            // Read sample
            other_player.Process();
            q15_t other_left = other_player.GetLeft();
            q15_t other_right = other_player.GetRight();

            // Apply envelope
            q15_t e = q31_to_q15(fadeout_envelope_.GetOutput());
            other_left = q15_mult(other_left, e);
            other_right = q15_mult(other_right, e);

            // Apply max
            other_left = q15_mult(other_left, fadeout_envelope_max_);
            other_right = q15_mult(other_right, fadeout_envelope_max_);

            // Mix (hopefully won't clip much...)
            left = q15_add(left, other_left);
            right = q15_add(right, other_right);
        }

        // Fill the output buffer
        output[2 * i] = left;
        output[2 * i + 1] = right;

        // Tell the second core it can process this sample
        MultiCore::SendMessage(MultiCore::MessageType::SAMPLE_REQUEST, i);
    }

    // Do time-precise addition for counters
    if (ui_indicate_change_time_ < SIZE_MAX)
    {
        ui_indicate_change_time_++;
    }

    // Wait for samples to finish processing
    MultiCore::WaitForMessage(MultiCore::MessageType::DONE);
}

FASTCODE void AppWaveBard::SecondCoreProcess(size_t index)
{
    bool has_audio_input = Kastle2::hw.IsAudioInJackProbablyPlugged();

    // Clean input
    q15_t left_input = 0;
    q15_t right_input = 0;

    // Read samples from the input
    if (has_audio_input)
    {
        left_input = input_buffer_[2 * index];
        right_input = input_buffer_[2 * index + 1];
    }

    // Read samples from the output
    q15_t left = output_buffer_[2 * index];
    q15_t right = output_buffer_[2 * index + 1];

    // get the loudest sample for the compressor and update it
    q15_t combined_input = q15_mult(left_input, q15(0.7f)) + q15_mult(left, q15(0.3f));
    if (q15_abs(combined_input) > fx_compressor_max_)
    {
        fx_compressor_max_ = combined_input;
    }
    combined_input = q15_mult(right_input, q15(0.7f)) + q15_mult(right, q15(0.3f));
    if (q15_abs(combined_input) > fx_compressor_max_)
    {
        fx_compressor_max_ = combined_input;
    }

    // Compress the input
    compress_amount_ = q15_inv(fx_compressor_.CalculateEnvelope());
    if (compress_amount_ < q15(0.5f))
    {
        compress_amount_ = q15_add(compress_amount_, q15(0.5f));
    }
    else
    {
        compress_amount_ = q15(1.0f);
    }

    // Mix input before other effects if it's set to that
    // the input is too hot for some reason so I have to turn it down first (it's exactly 2 times as loud)
    left_input = q15_mult(left_input, Q15_HALF);
    right_input = q15_mult(right_input, Q15_HALF);

    if (has_audio_input && input_audio_through_fx_)
    {
        // make the sum of signals equal to 1
        left_input = q15_mult(left_input, q15(0.7f));
        right_input = q15_mult(right_input, q15(0.7f));
        left = q15_mult(left, q15(0.3f));
        right = q15_mult(right, q15(0.3f));

        // sum them
        left = q15_add(left, left_input);
        right = q15_add(right, right_input);

        // compress them
        left = q15_mult(left, compress_amount_);
        right = q15_mult(right, compress_amount_);

        // expand them back (the compression is much more aggressive than the previous "above 0.5")
        left = q15_div(left, q15(0.3f));
        right = q15_div(right, q15(0.3f));

        // if the input is routed through the effects, apply distortion here (fx distortion has an envelope which we don't want here)
        left_input = fx_input_clipper_.Process(left_input);
        right_input = fx_input_clipper_.Process(right_input);
        left_input = q15_mult(left_input, fx_volume_compensation_);
        right_input = q15_mult(right_input, fx_volume_compensation_);
    }

#ifdef PLAYBACK_CLIPPER
    // Apply clipping
    left = playback_clipper_.Process(left);
    right = playback_clipper_.Process(right);
#endif
    left = q15_mult(left, fx_volume_compensation_);
    right = q15_mult(right, fx_volume_compensation_);


    // Apply DJ filter
    filter_.Process(left, right);
#ifdef FILTER_VOLUME_COMPENSATION
    // Process filter volume compensation using slewer to avoid zipper noise
    q15_t comp = filter_volume_compensation_slewer_.Process();
    left = q15_div(filter_.GetLeft(), comp);
    right = q15_div(filter_.GetRight(), comp);
#else
    // 0.768016042 is compensation for the filter gain, adjusted for resonance to not clip
    // (calculated by measuring the output of the filter and comparing it to the input)
    left = q15_div(filter_.GetLeft(), q15(0.768016042f));
    right = q15_div(filter_.GetRight(), q15(0.768016042f));
#endif

    // Apply Delay
    // save delayed sample and prevent hard clipping
    q15_t delayed_left = soft_clipper_.Process(q15_div(delay_left_->Read(), Q15_HALF));
    q15_t delayed_right = soft_clipper_.Process(q15_div(delay_right_->Read(), Q15_HALF));
    // update delay with feedback
    delay_left_->Write(q15_mult(left, Q15_HALF) + q15_mult(delayed_left, delay_feedback_ / 2));
    delay_right_->Write(q15_mult(right, Q15_HALF) + q15_mult(delayed_right, delay_feedback_ / 2));
    // add delay to output
    left = q15_mult(left, Q15_MAX - delay_mix_) + q15_mult(delayed_left, delay_mix_);
    right = q15_mult(right, Q15_MAX - delay_mix_) + q15_mult(delayed_right, delay_mix_);

    // Mix input after effects if it's set to that
    if (has_audio_input && !input_audio_through_fx_)
    {

        // make the sum of signals equal to 1
        left_input = q15_mult(left_input, q15(0.7f));
        right_input = q15_mult(right_input, q15(0.7f));
        left = q15_mult(left, q15(0.3f));
        right = q15_mult(right, q15(0.3f));
        // sum them
        left = q15_add(left, left_input);
        right = q15_add(right, right_input);
        // compress them
        left = q15_mult(left, compress_amount_);
        right = q15_mult(right, compress_amount_);
        // expand them back (the compression is much more aggressive than the previous "above 0.5")
        left = q15_div(left, q15(0.3f));
        right = q15_div(right, q15(0.3f));
    }

    // Write the samples back to the buffer
    output_buffer_[2 * index] = left;
    output_buffer_[2 * index + 1] = right;
}

FASTCODE void AppWaveBard::SecondCoreWorker()
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

void AppWaveBard::MidiCallback(midi::Message *msg)
{
    // Pass the message to all pots so they can handle CCs
    for (auto &pot : pots_)
    {
        pot->MidiCallback(msg);
    }

    // Pass the message to the bank selector
    bank_select_.MidiCallback(msg);

    // Note playing
    if (msg->IsNoteOn())
    {
        uint32_t note = msg->GetData1();

        // Kinda ugly since we do the similar thing in the FancyPot but whatever
        if (note < kMidiMinNote && (note % kMidiNoteControlRepeat < samples_.num_samples))
        {
#ifdef LOWEST_TWO_MIDI_OCTAVES_SELECT_ORIGINAL_PITCH
            // If below this note, select the base note
            if (note < kMidiTriggerOriginalPitchBelow)
            {
                midi_note_ = kMidiBaseNote;
                pitch_source_ = PitchSource::MIDI;
            }
#endif

            // Switching samples notes - trigger
            // Sample switching is handled by pot
            Trigger();
        }
        else if (note >= kMidiMinNote)
        {
            // Normal play notes
            midi_note_ = note;
            pitch_source_ = PitchSource::MIDI;
            Trigger();
        }
    }

    // Pitch bend
    if (msg->IsPitchBend())
    {
        midi_pitch_bend_multiplier_ = msg->GetPitchBendAsSemitones(kPitchBendRange);
    }
}

void AppWaveBard::UpdateQuantizedPot()
{
    pots_[Pot::PITCH_QUANTIZED]->ForceValue(pots_[Pot::PITCH]->GetValue(), false);
}

void AppWaveBard::UiLoop()
{
    // mark if the loop started with the triggered flag set
    bool now_triggered = triggered_;
    triggered_ = false;

    for (auto &pot : pots_)
    {
        pot->ReadValue();
    }

    // Read the bank selector, does the button switching etc.
    bank_select_.ReadValue();

    // Do switching magic
    if (Kastle2::hw.GetLayer() != Kastle2::base.GetPrevLayer())
    {
        if (Kastle2::base.GetPrevLayer() == Hardware::Layer::SETTINGS)
        {
            Kastle2::memory.QueueUpdate8(kMemAudioRouting, input_audio_through_fx_);
        }
    }

    // Change from midi to patch pitch source?
    bool change_to_patch_source = false;

    // Knob change detection
    // pitch knob
    if (pots_[Pot::PITCH]->HasChanged())
    {
        quantization_enabled_ = false;
        change_to_patch_source = true;
        UpdateQuantizedPot();
    }
    // octave knob
    if (pots_[Pot::PITCH_QUANTIZED]->HasChanged())
    {
        change_to_patch_source = true;
        quantization_enabled_ = true;
    }
    // scale knob
    if (pots_[Pot::PITCH_SCALE]->HasChanged())
    {
        change_to_patch_source = true;
        if (!quantization_enabled_)
        {
            quantization_enabled_ = true;
            UpdateQuantizedPot();
        }
    }
    // root knob
    if (pots_[Pot::PITCH_ROOT]->HasChanged())
    {
        change_to_patch_source = true;
        if (!quantization_enabled_)
        {
            quantization_enabled_ = true;
            UpdateQuantizedPot();
        }
    }
    // NOTE CV
#ifdef SAMPLE_NOTE_ON_TRIGGER
    if (now_triggered)
    {
        pitch_note_cv_ = Kastle2::hw.GetAnalogValue(CV_PITCH_STEP);
    }
#else
    pitch_note_cv_ = Kastle2::hw.GetAnalogValue(CV_PITCH_STEP);
#endif
    if (diff(pitch_note_cv_, pitch_note_cv_prev_) > kCVChangeDetectThreshold)
    {
        change_to_patch_source = true;
        pitch_note_cv_prev_ = pitch_note_cv_;
        if (!quantization_enabled_)
        {
            quantization_enabled_ = true;
            UpdateQuantizedPot();
        }
    }

    quantizer_.SetEnabled(quantization_enabled_);

    // Quantizer Scale selection
    int32_t quantizer_scale = pots_[Pot::PITCH_SCALE]->GetMappedValue();
    quantizer_.SetScale(quantizer_scale >= 0 ? quantizer_scale : 0);
    // Change indication
    if (quantizer_scale != quantizer_scale_prev_)
    {
        change_to_patch_source = true;
        quantizer_scale_prev_ = quantizer_scale;
        if (pots_[Pot::PITCH_ROOT]->GetSource() == FancyPot::Source::INTERNAL)
        {
            bank_select_.DisableNextChange();
            UiIndicateChange();
        }
    }

    // Quantizer Scale Root selection
    int32_t quantizer_root = pots_[Pot::PITCH_ROOT]->GetMappedValue();

#ifdef QUANTIZER_ROOT_TRANSPOSES
// We don't select the root inside quantizer,
// since we want to quantize it according to C-scale and transpose afterwards
#else
    // Select the root of the scale inside quantizer
    quantizer_.SetRoot(static_cast<Quantizer::ScaleRoot>(quantizer_root));
#endif

    //  Change indication
    if (quantizer_root != quantizer_root_prev_)
    {
        change_to_patch_source = true;
        quantizer_root_prev_ = quantizer_root;
        if (pots_[Pot::PITCH_ROOT]->GetSource() == FancyPot::Source::INTERNAL)
        {
            bank_select_.DisableNextChange();
            UiIndicateChange();
        }
    }

    // Actual pitch calculations
    float base_pitch;
    bool pitch_pot_change_trigger = false;
    if (quantization_enabled_)
    {
#ifdef QUANTIZER_ROOT_TRANSPOSES
        // trigger on octave change
        base_pitch = step_map(pots_[Pot::PITCH_QUANTIZED]->GetValue(), kMapPitchOctaves);
        if (diff(base_pitch, trigger_base_pitch_prev_) > 0.01f)
        {
            trigger_base_pitch_prev_ = base_pitch;
            pitch_pot_change_trigger = true;
        }
#else
        base_pitch = curve_mapf(pitch_quantized_pot_, kMapPitch, K_MAP_PITCH);
        // Trigger on semitone change
        if (diff(pitch_quantized_pot_, trigger_detect_pitch_prev_) > kPitchSemitoneChangeThreshold)
        {
            trigger_detect_pitch_prev_ = pitch_quantized_pot_;
            pitch_pot_change_trigger = true;
        }
#endif
    }
    else
    {
        base_pitch = curve_map(pots_[Pot::PITCH]->GetValue(), kMapPitch);
    }

    int32_t pitch_mod_pot = curve_map(pots_[Pot::PITCH_MOD]->GetValue(), kMapPitchMod);
    float pitch_note_cv_modded = apply_pot_mod_attenuvert(pitch_note_cv_, pitch_mod_pot);
    base_pitch *= std::pow(2.0f, pitch_note_cv_modded / static_cast<float>(ADC_1V)); // proper 1V/octave scaling
    base_pitch = quantizer_.ProcessMultiplier(base_pitch);
#ifdef QUANTIZER_ROOT_TRANSPOSES
    if (quantization_enabled_)
    {
        base_pitch *= kPitchRootTransposeLut[quantizer_root];
    }
#endif
    // Calculate output MIDI note from base_pitch frequency multiplier
    // We do it before the free and fine pitch modulation, so that the MIDI note is not affected by them
    float semitone_offset = 12.0f * std::log2f(base_pitch);
    int32_t midi_note_to_send = kMidiBaseNote + (int32_t)roundf(semitone_offset);
    midi_note_to_send = constrain(midi_note_to_send, 0, 127);
    patch_pitch_midi_note_ = midi_note_to_send;

    // Finish with free and fine pitch modulation
    float pitch_free_cv_modded = apply_pot_mod_attenuvert(Kastle2::hw.GetAnalogValue(CV_PITCH_FREE), pitch_mod_pot);
    float free_pitch_mod = std::pow(2.0f, pitch_free_cv_modded / static_cast<float>(ADC_1V)); // proper 1V/octave scaling
    float fine_pitch_mod = curve_map(pots_[Pot::PITCH_FINE]->GetValue(), kMapPitchFine);
    base_pitch *= free_pitch_mod;
    base_pitch *= fine_pitch_mod;

    if (ui_loop_counter_ == 4) // runs each 8th ui loop (+4) to save some CPU
    {
        // Send the Free Pitch Mod as CV Pitch Bend
        int32_t pitchBendValue = map(12.0f * std::log2f(free_pitch_mod), -24.0f, 24.0f, midi::Message::kPitchBendMin, midi::Message::kPitchBendMax);
        // Min diff 16 is kinda aggressive but we don't want to send it too often
        Kastle2::midi.SendPitchBend(pitchBendValue, false, 16);
    }

    // Prevent trigger at boot
    if (base_pitch_prev_ == 0.f)
    {
        base_pitch_prev_ = base_pitch;
    }
    else if (base_pitch != base_pitch_prev_ && pitch_pot_change_trigger)
    {
        base_pitch_prev_ = base_pitch;
        Trigger();
#ifdef QUANTIZER_ROOT_TRANSPOSES
        UiIndicateChange();
#endif
    }

    // Calculate the MIDI pitch
    float relative_midi_note = static_cast<int32_t>(midi_note_) - static_cast<int32_t>(kMidiBaseNote);
    float midi_pitch = std::pow(2.0f, relative_midi_note / 12.0f);
    midi_pitch *= midi_pitch_bend_multiplier_;
    midi_pitch *= free_pitch_mod;
    midi_pitch *= fine_pitch_mod;

    // Sample playing speed
    switch (pitch_source_)
    {
    case PitchSource::PATCH:
        players_[active_player_].SetSpeed(base_pitch);
        break;
    case PitchSource::MIDI:
        players_[active_player_].SetSpeed(midi_pitch);
        break;
    }

    // Switching back to patch pitch?
    if (pitch_source_ != PitchSource::PATCH && change_to_patch_source)
    {
        pitch_source_ = PitchSource::PATCH;
    }

    // Sample modulation
    int32_t smp = pots_[Pot::SAMPLE_MOD]->GetValue();
    if (smp < POT_HALF)
    {
        sample_play_on_change_ = false;
        sample_cv_ = apply_pot_mod(Kastle2::hw.GetAnalogValue(CV_SAMPLE), (POT_HALF - smp) * 2);
    }
    else
    {
        sample_play_on_change_ = true;
        sample_cv_ = apply_pot_mod(Kastle2::hw.GetAnalogValue(CV_SAMPLE), (smp - POT_HALF) * 2);
    }

// Envelope
#ifdef SAMPLE_DECAY_ON_TRIGGER
    if (now_triggered)
    {
        envelope_cv_ = curve_map(Kastle2::hw.GetAnalogValue(CV_ENVELOPE), kMapEnvelopeCV);
    }
#else
    envelope_cv_ = Kastle2::hw.GetAnalogValue(CV_ENVELOPE);
#endif
    int32_t base_envelope = pots_[Pot::ENVELOPE]->GetValue();
    base_envelope += apply_pot_mod_attenuvert(envelope_cv_, pots_[Pot::ENVELOPE_MOD]->GetValue());
    base_envelope = constrain(base_envelope, POT_MIN, POT_MAX);
    prev_base_envelope_ = base_envelope;
    if (pots_[Pot::ENVELOPE]->HasMoved(FancyPot::Move::TWEAK))
    {
        SendMidiLength(false);
    }

    float decay_time = curve_map(base_envelope, kMapEnvelopeDecay);
    float attack_time = curve_map(base_envelope, kMapEnvelopeAttack);
    float hold_time = curve_map(base_envelope, kMapEnvelopeHold);
    envelope_.SetHoldTime(hold_time);
    envelope_.SetDecayTime(decay_time);
    envelope_.SetAttackTime(attack_time);
    float trigger_spacing = (float)trigger_spacing_us_ / 1000000.f; // convert us to s
    if (base_envelope < POT_HALF)
    {
        // Reverse playback section
        players_[active_player_].SetReverse(true);
        int32_t attack_ticks = players_[active_player_].GetLengthSpeedAdjusted() - (attack_time + hold_time) * SAMPLE_RATE;
        // Stretch out the length to compensate for pitch changes,
        // then adjust the start point with the envelope attack length,
        // and then stretch it back down to samples without pitch change to start playback from the correct point
        players_[active_player_].SetStartSpeedAdjusted(constrain(attack_ticks, 0, players_[active_player_].GetLengthSpeedAdjusted()));
        if (ui_loop_counter_ == 0) // runs each 8th ui loop to save some CPU
        {
            envelope_indicator_.SetAttackTime(attack_time + decay_time < trigger_spacing ? attack_time : trigger_spacing);
            envelope_indicator_.SetDecayTime(attack_time + decay_time < trigger_spacing ? decay_time : 0.f);
        }
#ifdef OUT_ENVELOPE_SCALING
        if (ui_loop_counter_ == 2) // runs each 8th ui loop to save some CPU
        {
            // Set the output envelope to have no hold time,
            // only attack and to be as long as the sample (makes it more usable for pitch sweeps)
            float scaled_attack_time = attack_time + hold_time;
            float sample_length = (float)players_[active_player_].GetLengthSpeedAdjusted() / SAMPLE_RATE;
            scaled_attack_time = scaled_attack_time > sample_length ? sample_length : scaled_attack_time;
            envelope_out_.SetAttackTime(scaled_attack_time);
            envelope_out_.SetDecayTime(0.001f);
        }
#endif
    }
    else
    {
        // Forward playback section
        players_[active_player_].SetReverse(false);
        players_[active_player_].SetStart(0);
        if (ui_loop_counter_ == 0) // runs each 8th ui loop to save some CPU
        {
            envelope_indicator_.SetAttackTime(attack_time + decay_time < trigger_spacing ? attack_time : 0.f);
            envelope_indicator_.SetDecayTime(attack_time + decay_time < trigger_spacing ? decay_time : trigger_spacing);
        }

#ifdef OUT_ENVELOPE_SCALING
        // runs each 8th (+2) ui loop to save some CPU
        if (ui_loop_counter_ == 2)
        {
            // Set the output envelope to have no hold time,
            // Only decay and to be as long as the sample (makes it more usable for pitch sweeps)
            float scaled_decay_time = decay_time + hold_time;
            float sample_length = (float)players_[active_player_].GetLengthSpeedAdjusted() / SAMPLE_RATE;
            scaled_decay_time = scaled_decay_time > sample_length ? sample_length : scaled_decay_time;
            envelope_out_.SetAttackTime(0.001f);
            envelope_out_.SetDecayTime(scaled_decay_time);
        }
#endif
    }
    size_t dur = curve_map(base_envelope, kMapEnvelopeNoteDuration);
    note_sender_.SetDuration(dur);

    // SoftClipper
    int32_t fx_pot = pots_[Pot::FX]->GetValue();
    playback_clipper_.SetDrive(q15_mult(curve_map(fx_pot, kMapDistortionAmount), q31_to_q15(envelope_.GetOutput())));
    fx_input_clipper_.SetDrive(curve_map(fx_pot, kMapInputDistortionAmount));
    fx_volume_compensation_ = curve_map(fx_pot, kMapFXVolumeCompensation);

    // DJ style filter
    int32_t filter_pot = pots_[Pot::FILTER]->GetValue();
    filter_.SetCrossfade(map(filter_pot, POT_MIN, POT_MAX, Q15_MIN, Q15_MAX));

    // because of the resonance we need to make the volume lower at some point
    // set target value for slewer to avoid zipper noise
    filter_volume_compensation_slewer_.SetValue(curve_map(filter_pot, kMapFilterVolumeCompensation));

    // Delay
    delay_mix_ = curve_map(fx_pot, kMapDelayMix);
    delay_feedback_ = curve_map(fx_pot, kMapDelayFeedback);

    int32_t delay_left_val = 0;
    int32_t delay_right_val = 0;

    // Normal delay to the left
    if (fx_pot < pot(0.45f))
    {
        // Possibility of an overflow here, I don't think I need to check for it though
        int32_t delay_ticks_now = ((Kastle2::base.GetClock().GetAverageTargetTicks() * buffer_size_) * kDelayRatio.n) / kDelayRatio.d;

        // Need to filter the delay changes because of possible unstable clock
        // 0.010 = 10 ms min difference between delay time
        constexpr uint32_t min_ticks_change_ext = s2sr(0.010f);
        constexpr uint32_t min_ticks_change_int = s2sr(0.005f);
        uint32_t min_ticks_change = Kastle2::base.GetClock().GetSyncType() == Clock::Sync::INTERNAL ? min_ticks_change_int : min_ticks_change_ext;
        if (diff(delay_ticks_now, delay_ticks_) > min_ticks_change)
        {
            delay_ticks_ = delay_ticks_now;
        }

        // Left and right channel is detuned for a *nice* stereo effect
        delay_left_val = delay_ticks_ + curve_map(fx_pot, kMapDelayStereoLeft);
        delay_right_val = delay_ticks_ + curve_map(fx_pot, kMapDelayStereoRight);
    }
    // Chorus / Flanger to the right, delay modulated by the a LFO triangle (not related to the main LFO)
    else if (fx_pot > pot(0.55f))
    {
        // Chorus
        float chorus_freq = curve_map(fx_pot, kMapChorusFreq);
        chorus_lfo_.SetFrequency(chorus_freq);
        int32_t base_length = curve_map(fx_pot, kMapChorusBaseLength);
        delay_left_val = base_length + curve_map(fx_pot, kMapChorusLfoDepthLeft) * chorus_lfo_val_;
        delay_right_val = base_length + curve_map(fx_pot, kMapChorusLfoDepthRight) * (DAC_MAX - chorus_lfo_val_);
    }

    if (delay_left_val < 0)
    {
        delay_left_val = 0;
    }
    if (delay_right_val < 0)
    {
        delay_right_val = 0;
    }

    // Filter out the delay changes, so the delay doesn't jump
    if (prev_delay_left_val_ != delay_left_val)
    {
        delay_left_->SetDelay(delay_left_val);
        prev_delay_left_val_ = delay_left_val;
    }
    if (prev_delay_right_val_ != delay_right_val)
    {
        delay_right_->SetDelay(delay_right_val);
        prev_delay_right_val_ = delay_right_val;
    }

    // Trigger sample playback if sample mod is set to play and the selected sample changes (either by CV or by POT)
    if (diff(sample_cv_ * sample_play_on_change_, prev_sample_cv_) > kSampleChangeThreshold)
    {
        prev_sample_cv_ = sample_cv_ * sample_play_on_change_;
        // base sample can overflow max pot value, so we need to do modulo on it
        UpdateSelectedSample();
        if (sample_num_selected_ != sample_num_selected_prev_)
        {
            sample_num_selected_prev_ = sample_num_selected_;
            Trigger(false);
        }
    }

    // Writes the envelope state to the ENV output on the patchbay
#ifdef OUT_ENVELOPE_SCALING
    Kastle2::hw.SetEnvOut(envelope_out_.GetOutput() >> (31 - 10));
    // Kastle2::hw.SetEnvOut(compress_amount_ >> (15 - 10));
#else
    Kastle2::hw.SetEnvOut(envelope_.GetOutput() >> (31 - 10));
#endif

    // trigger if the time is short enough
    if (ShiftTrigger())
    {
        Trigger(true);
    }

    // ADVANCED SETTINGS LAYER - set the audio routing (more stuff will be added later)
    if (Kastle2::hw.GetLayer() == Hardware::Layer::SETTINGS)
    {
        if (pots_[Pot::AUDIO_ROUTE]->HasChanged())
        {
            input_audio_through_fx_ = pots_[Pot::AUDIO_ROUTE]->GetValue() < POT_HALF;
        }
    }

    // LEDs
    if (Kastle2::hw.GetLayer() == Hardware::Layer::NORMAL ||
        Kastle2::hw.GetLayer() == Hardware::Layer::MODE)
    {
        uint32_t color = GetColor(bank_select_.GetMode());
        color = WS2812::ApplyBrightness(color, (envelope_indicator_.GetOutput() >> 24) + 127);
        Kastle2::hw.SetLed(Hardware::Led::LED_2, color);
        if (ui_indicate_change_time_ < kUiIndicateChangeTime)
        {
            Kastle2::hw.SetLed(Hardware::Led::LED_1, kUiIndicateChangeColor);
        }
        else
        {
            Kastle2::hw.SetLed(Hardware::Led::LED_1, color);
        }
    }
    else if (Kastle2::hw.GetLayer() == Hardware::Layer::SETTINGS)
    {
        uint8_t brightness = Kastle2::base.GetSettingsLedPulse();
        // show routing of the input signal
        if (input_audio_through_fx_)
        {
            Kastle2::hw.SetLed(Hardware::Led::LED_2, WS2812::ApplyBrightness(WS2812::RED, brightness));
        }
        else
        {
            Kastle2::hw.SetLed(Hardware::Led::LED_2, WS2812::ApplyBrightness(WS2812::BLUE, brightness));
        }
    }

    TriggerCheck();
    now_triggered = false;
    ui_loop_counter_ = (ui_loop_counter_ + 1) % kUiLoopCounterMax;

    // Always send midi sample
    if (!dirty_inputs_handler_.IsBusy())
    {
        SendMidiSample(false);
    }
}
inline void AppWaveBard::UpdateSelectedBank()
{
    sample_bank_selected_ = bank_select_.GetMode();
}

inline void AppWaveBard::UpdateSelectedSample()
{
    sample_num_selected_ = GetPatchedSample();
}

inline size_t AppWaveBard::GetPatchedSample()
{
    int32_t mod_sample = sticky_map(sample_cv_, -SampleCvMaxValue(), SampleCvMaxValue(), -samples_.num_samples, samples_.num_samples, sticky_sample_mod_);
    int32_t max_value = samples_.num_samples - 1;
    mod_sample = constrain(mod_sample, -max_value, max_value);
    return (samples_.num_samples + pots_[Pot::SAMPLE]->GetMappedValue() + mod_sample) % samples_.num_samples;
}

inline size_t AppWaveBard::GetContinousPatchedSample()
{
    // This function is trying to approximate the simple function above using linear values
    // It's really ugly and I'm sorry for that but it somehow works
    int32_t mod_sample = map(sample_cv_, -SampleCvMaxValue(), SampleCvMaxValue(), POT_MIN, POT_MAX, MapClamp::TRUE);
    int32_t continous_value = ((POT_MAX + 1) + mod_sample + pots_[Pot::SAMPLE]->GetValue()) % (POT_MAX + 1);
    continous_value = sticky_map(continous_value, 0, POT_MAX, 0, 127, sticky_sample_continous_mod_);

    // Diff it with patched sample - check whether we are close enough (less than half of the cc range)
    int32_t patched_value = map(GetPatchedSample(), 0, samples_.num_samples - 1, 0, 127);

    // If we are way off, return the stairy patched sample
    if (abs(continous_value - patched_value) > 64)
    {
        return patched_value;
    }
    // Otherwise return the continous value
    return continous_value;
}

void inline AppWaveBard::UiIndicateChange()
{
    ui_indicate_change_time_ = 0;
}

inline void AppWaveBard::Trigger(bool manual)
{
    dirty_inputs_handler_.Trigger();
    trigger_was_manual_ = manual;
}

inline void AppWaveBard::TriggerCheck()
{
#ifdef PASSTHROUGH_MIDI_NOTE
    static constexpr bool passthrough_midi_note = true;
#else
    static constexpr bool passthrough_midi_note = false;
#endif

    if (next_time_send_note_on_)
    {
        next_time_send_note_on_ = false;
        note_sender_.Trigger((pitch_source_ == PitchSource::MIDI &&
                              passthrough_midi_note)
                                 ? midi_note_
                                 : patch_pitch_midi_note_);
    }

    if (dirty_inputs_handler_.GetTrigger())
    {
        ActualTrigger(false);
    }
}

FASTCODE void AppWaveBard::ActualTrigger(bool force)
{
    // Check for last trigger time to prevent double triggering
    absolute_time_t now = get_absolute_time();
    int64_t diff = absolute_time_diff_us(last_trigger_time_, now);
    if (diff < kTimeBetweenTriggers && !force)
    {
        return;
    }
    last_trigger_time_ = now;
    trigger_spacing_us_ = diff;
    note_sender_.Clear();

    // Sample number and bank change with each trigger
    UpdateSelectedSample();
    UpdateSelectedBank();

    // If needed, store the current envelope value and trigger fadeout envelope
    if (players_[active_player_].IsPlaying() && envelope_.IsActive())
    {
        fadeout_envelope_max_ = q31_to_q15(envelope_.GetOutput());
        fadeout_envelope_.Trigger();

        // Disable hifi mode for fadeout
        // Processing two samples using hifi mode is too much for the CPU
        // and it's not really hearable anyway
        players_[active_player_].SetHifi(false);
    }

    // Switch to the other deck before loading the new sample
    active_player_ = EnumIncrement<PlayerDeck>(active_player_);

    // Load the new sample and start playing
    players_[active_player_].SetSample(GetSample());
    players_[active_player_].SetHifi(true);
    players_[active_player_].Play();

    // Trigger envelope
    envelope_.Trigger();
    envelope_indicator_.Trigger();
    envelope_out_.Trigger();
    next_time_send_note_on_ = true;

    // Send CC stuff about samples
    if (!trigger_was_manual_)
    {
        SendMidiCombo(true);
    }
    SendMidiSample(trigger_was_manual_);
    SendMidiBank(false);
    SendMidiLength(false);

    // UI update signal
    triggered_ = true;
}

void AppWaveBard::SendMidiLength(bool force)
{
    if (prev_base_envelope_ < POT_HALF)
    {
        // Send the CCs for the reverse playback
        uint8_t cc_value_ = constrain(sticky_map(prev_base_envelope_, POT_MIN, POT_HALF, 127, 0, prev_backward_length_cc_), 0, 127);
        Kastle2::midi.SendCc(cc::OUT_FORWARD_LENGTH, 0);
        Kastle2::midi.SendCc(cc::OUT_BACKWARD_LENGTH, cc_value_, force);
    }
    else
    {
        // Send the CCs for the forward playback
        uint8_t cc_value_ = constrain(sticky_map(prev_base_envelope_, POT_HALF, POT_MAX, 0, 127, prev_forward_length_cc_), 0, 127);
        Kastle2::midi.SendCc(cc::OUT_FORWARD_LENGTH, cc_value_, force);
        Kastle2::midi.SendCc(cc::OUT_BACKWARD_LENGTH, 0);
    }
}

void AppWaveBard::SendMidiCombo(bool force)
{
    int32_t current = sample_bank_selected_ * samples_.num_samples + sample_num_selected_;
    int32_t max = samples_.num_samples * samples_.num_banks;
    int32_t combo = map(current, 0, max, 0, 128);
    Kastle2::midi.SendCc(cc::OUT_BANK_SAMPLE_COMBO, combo, force);
}

void AppWaveBard::SendMidiBank(bool force)
{
    bank_select_.SendMidi(force);
}

void AppWaveBard::SendMidiSample(bool force)
{
    // Stepped value (just a the sample number staired into 0-127)
    // Kastle2::midi.SendCc(cc::OUT_SAMPLE_SIMPLE, map(GetPatchedSample(), 0, samples_.num_samples - 1, 0, 127), force);

    // Continuous values (improved, linear interpolation of the sample number to 0-127)
    Kastle2::midi.SendCc(cc::OUT_SAMPLE_CONTINOUS, GetContinousPatchedSample(), force);
}

inline SamplePlayer16bit::Sample AppWaveBard::GetSample() const
{
    auto &source_sample = samples_.banks[sample_bank_selected_].samples[sample_num_selected_];
    return SamplePlayer16bit::Sample{
        .data = (int16_t *)source_sample.data,
        .length = source_sample.size / (samples_.bit_depth / 8) / source_sample.channels,
        .channels = source_sample.channels == 2 ? SamplePlayer16bit::STEREO : SamplePlayer16bit::MONO};
}

inline uint32_t AppWaveBard::GetColor(size_t bank) const
{
    return samples_.banks[bank].color.r << 16 | samples_.banks[bank].color.g << 8 | samples_.banks[bank].color.b;
}

void AppWaveBard::MemoryInitialization()
{
    // TODO: Move this to the FancyPot (add MemoryInitialization method)
    Kastle2::memory.Write8(kMemBank, 0);
    Kastle2::memory.Write8(kMemPotFilter, pot_to_mem(POT_HALF));
    Kastle2::memory.Write8(kMemPotFX, pot_to_mem(POT_HALF));
    Kastle2::memory.Write8(kMemPotScale, pot_to_mem(POT_HALF));
    Kastle2::memory.Write8(kMemPotRoot, 0);
    Kastle2::memory.Write8(kMemPotFine, pot_to_mem(POT_HALF));
    Kastle2::memory.Write8(kMemPotLengthMod, pot_to_mem(POT_MAX));
    Kastle2::memory.Write8(kMemPotBankMod, pot_to_mem(POT_MAX));
}

bool AppWaveBard::LoadSamples()
{
    UserDataFile file_reader;

    // Validate file header
    if (!file_reader.Validate("k2wb"))
    {
        return false;
    }

    // Read the header data
    file_reader.Read(&samples_, 18);

    // Validate sequencer length
    if (!between(samples_.sequencer_length, Sequencer::kMinLength, Sequencer::kMaxLength))
    {
        // Fallback to 16 if the loaded length is invalid (compatibility with older files)
        samples_.sequencer_length = 16;
    }

    // Validate all counts (these must be within valid ranges)
    if (
        !between(samples_.num_rhythms, 1, UserDataFile::kMaxRhythms) ||
        !between(samples_.num_scales, 1, UserDataFile::kMaxScales) ||
        !between(samples_.num_banks, 1, 32) ||
        !between(samples_.num_samples, 1, 32))
    {
        return false;
    }

    // Jump to scales section begin
    file_reader.JumpTo(20);

    // Load scales
    samples_.scales = file_reader.LoadScales(samples_.num_scales);
    if (!samples_.scales)
    {
        return false;
    }

    // Load rhythms
    samples_.rhythms = file_reader.LoadRhythms(samples_.num_rhythms);
    if (!samples_.rhythms)
    {
        delete[] samples_.scales;
        return false;
    }

    // Load the banks and samples
    samples_.banks = new WaveBardBank[samples_.num_banks];

    for (size_t i = 0; i < samples_.num_banks; i++)
    {
        WaveBardBank *bank = &samples_.banks[i];

        // Read bank header (11 bytes)
        file_reader.Read(reinterpret_cast<uint8_t *>(bank), 11);
        file_reader.Advance(1); // Skip 1 reserved byte

        // Read samples
        bank->samples = new WaveBardSample[samples_.num_samples];
        for (size_t j = 0; j < samples_.num_samples; j++)
        {
            WaveBardSample *sample = &bank->samples[j];

            // Read sample header (5 bytes: size + channels)
            file_reader.Read(reinterpret_cast<uint8_t *>(sample), 5);
            file_reader.Advance(11); // Skip name (8) + reserved (3) bytes

            // Point the sample data to the correct memory location (don't copy the data since it won't fit)
            sample->data = file_reader.GetCurrentMemoryPointer();
            file_reader.Advance(sample->size);
        }
    }

    return true;
}