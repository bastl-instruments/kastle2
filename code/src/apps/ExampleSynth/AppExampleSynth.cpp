/*
MIT License

Copyright (c) 2026 Vaclav Mach (Bastl Instruments)

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

#include "AppExampleSynth.hpp"
#include "common/core/Kastle2.hpp"
#include "common/utils.hpp"
#include "ExampleSynthParameterMaps.hpp"

using namespace kastle2;

void AppExampleSynth::Init()
{
    inited_ = false;

    // Oscillators
    subtractive_osc_.Init(SAMPLE_RATE);
    subtractive_osc_.SetWaveform(Oscillator::Waveform::SQUARE);
    subtractive_osc_.SetFrequency(110.0f);
    fm_osc_.Init(SAMPLE_RATE);
    fm_osc_.SetFrequency(110.0f);

    // Low-pass filter
    filter_.Init(SAMPLE_RATE);
    filter_.SetFrequency(500.0f);
    filter_.SetResonance(0.0f);
    filter_.SetType(Svf::Type::LOWPASS);

    // Envelope
    env_.Init(SAMPLE_RATE);
    env_.SetAttackTime(0.01f);
    env_.SetDecayTime(1.0f);
    env_.SetNonResetting(AdsrEnv::NonResetting::DECAY); // prevents clicks
    env_value_ = 0;
    env_enabled_ = false;

    // Quantizer
    quantizer_.Init(0.8f);
    quantizer_.SetEnabled(true);
    quantizer_.SetScale(Quantizer::DefaultScale::CHROMATIC);

    // Stereo Delay
    stereo_delay_.Init(SAMPLE_RATE);
    stereo_delay_.SetFeedback(q15(0.15f));
    UpdateDelayTime();

    // Allow ENV OUT to be controlled by this App
    Kastle2::base.SetFeatureEnabled(Base::Feature::ENV_OUT, false);

    // Standard sequencer length
    Kastle2::base.GetSequencer().SetLength(16);

    // Mode selection
    mode_selector_.Init();

    // POTS

    // Normal layer
    pots_[Pot::PITCH_MOD] = FancyPot::Create({
        .pot = Hardware::Pot::POT_1,
        .layer = Hardware::Layer::NORMAL,
        .deadzone = true,
        .freeze = true,
    });

    pots_[Pot::TIMBRE_MOD] = FancyPot::Create({
        .pot = Hardware::Pot::POT_2,
        .layer = Hardware::Layer::NORMAL,
        .deadzone = true,
    });

    pots_[Pot::ENV] = FancyPot::Create({
        .pot = Hardware::Pot::POT_4,
        .layer = Hardware::Layer::NORMAL,
        .deadzone = true,
    });

    pots_[Pot::PITCH] = FancyPot::Create({
        .pot = Hardware::Pot::POT_5,
        .layer = Hardware::Layer::NORMAL,
        .freeze = true,
    });

    pots_[Pot::TIMBRE] = FancyPot::Create({.pot = Hardware::Pot::POT_6,
                                           .layer = Hardware::Layer::NORMAL});

    // Shift layer
    pots_[Pot::ENV_MOD] = FancyPot::Create({.pot = Hardware::Pot::POT_4,
                                            .layer = Hardware::Layer::SHIFT,
                                            .initial_value = kEnvModDefaultValue,
                                            .deadzone = true});

    pots_[Pot::FX] = FancyPot::Create({.pot = Hardware::Pot::POT_2,
                                       .layer = Hardware::Layer::SHIFT,
                                       .initial_value = kFxDefaultValue,
                                       .deadzone = true,
                                       .memory_addr = kMemFx});

    pots_[Pot::RESONANCE] = FancyPot::Create({.pot = Hardware::Pot::POT_6,
                                              .layer = Hardware::Layer::SHIFT,
                                              .initial_value = kResonanceDefaultValue,
                                              .deadzone = true});

    // Mode layer
    pots_[Pot::PITCH_SCALE] = FancyPot::Create({.pot = Hardware::Pot::POT_1,
                                                .layer = Hardware::Layer::MODE,
                                                .initial_value = kPitchScaleDefaultValue,
                                                .map_size = quantizer_.GetScaleTableSize(), // quantizer needs to be initialized before calling this
                                                .memory_addr = kMemPitchScale});

    pots_[Pot::PITCH_ROOT] = FancyPot::Create({.pot = Hardware::Pot::POT_2,
                                               .layer = Hardware::Layer::MODE,
                                               .initial_value = kPitchRootDefaultValue,
                                               .map_size = Quantizer::kMultiplierTable.size(),
                                               .memory_addr = kMemPitchRoot});

    pots_[Pot::PITCH_FINE] = FancyPot::Create({.pot = Hardware::Pot::POT_3,
                                               .layer = Hardware::Layer::MODE,
                                               .initial_value = kPitchFineDefaultValue,
                                               .deadzone = true,
                                               .memory_addr = kMemPitchFine});

    pots_[Pot::MODE_MOD] = FancyPot::Create({.pot = Hardware::Pot::POT_4,
                                             .layer = Hardware::Layer::MODE,
                                             .initial_value = kModeModDefaultValue,
                                             .memory_addr = kMemModeMod});

    // Pots need to be initialized
    for (auto &pot : pots_)
    {
        pot->Init(AUDIO_LOOP_RATE);
    }

    // This disables the next change when the pots are moved
    // or when the current layer time is over the specified number of ticks
    mode_selector_.DisableNextChangeWhen(pots_, kModeShortPressUnder);

    inited_ = true;
}

void AppExampleSynth::DeInit()
{
    inited_ = false;
}

void AppExampleSynth::AudioLoop([[maybe_unused]] q15_t *input, q15_t *output, size_t size)
{
    if (!inited_)
    {
        return;
    }

    // Detect the trigger and do dirty inputs
    if (trigger_.Process(Kastle2::hw.GetTriggerIn()))
    {
        do_trigger_ = true;
    }

    // Calculate all samples
    for (size_t i = 0; i < size; i++)
    {
        q15_t osc_out = 0;

        switch (current_mode_)
        {
        case Mode::FM:
            osc_out = q31_to_q15(fm_osc_.Process());
            break;
        case Mode::SUBTRACTIVE:
            osc_out = q31_to_q15(subtractive_osc_.Process());
            osc_out = filter_.Process(osc_out);
            break;
        }

        // Calculate the envelope
        env_value_ = q31_to_q15(env_.Process());

        // Apply the envelope
        if (env_enabled_)
        {
            osc_out = q15_mult(osc_out, env_value_);
        }

        // Lower the output to prevent clipping
        osc_out /= 2;

        // Apply delay
        auto delay_output = stereo_delay_.Process(osc_out, osc_out);

        // Update outputs
        output[2 * i] = delay_output.left;
        output[2 * i + 1] = delay_output.right;
    }

    // Handle mode audio-rate processing
    mode_selector_.Process();

    // Process pots in audio-rate
    for (auto &pot : pots_)
    {
        pot->Process();
    }
}

void AppExampleSynth::Trigger()
{
    // Store the current note
    // Ideally you'd like to use DirtyInputHandler to make sure you have the latest value
    // But it's OK for this example
    pitch_note_cv_ = Kastle2::hw.GetAnalogValue(CV_PITCH_NOTE);

    // Store current mode
    mode_selector_.TriggerAdcRead();

    // Trigger envelope
    env_.Trigger();
}

void AppExampleSynth::UpdateDelayTime()
{
    size_t stereo_delay_length = ((Kastle2::base.GetClock().GetAverageTargetTicks() * AUDIO_BUFFER_SIZE) * kDelayRatio.n) / kDelayRatio.d;
    // Filter out noise
    if (diff(stereo_delay_length, prev_stereo_delay_length_) < 10)
    {
        return;
    }
    prev_stereo_delay_length_ = stereo_delay_length;
    stereo_delay_.SetDelay(std::max(int(stereo_delay_length) + 80, 5), std::max(int(stereo_delay_length) - 80, 5));
}

void AppExampleSynth::UiLoop()
{
    if (do_trigger_)
    {
        Trigger();
        do_trigger_ = false;
    }

    // Handle mode switching
    mode_selector_.ReadValue();
    current_mode_ = static_cast<Mode>(mode_selector_.GetMode());

    // Update pots
    for (auto &pot : pots_)
    {
        pot->ReadValue();
    }

    // Quantizer Scale selection based on the pot value
    int32_t quantizer_scale = pots_[Pot::PITCH_SCALE]->GetMappedValue();
    quantizer_.SetScale(quantizer_scale >= 0 ? quantizer_scale : 0);

    // Raw pitch value from the pot
    float base_pitch = std::pow(2.0f, curve_map(pots_[Pot::PITCH]->GetValue(), kMapFreePitch));

    // Apply NOTE PITCH mod
    int32_t pitch_mod_pot = pots_[Pot::PITCH_MOD]->GetValue();
    int32_t pitch_note_mod = apply_pot_mod_attenuvert(pitch_note_cv_, pitch_mod_pot);
    base_pitch *= std::pow(2.0f, static_cast<float>(pitch_note_mod) / static_cast<float>(ADC_1V)); // V/Oct

    // Multiply by base frequency
    base_pitch *= kBaseTune;

    // Apply quantization
    base_pitch = quantizer_.Process(base_pitch);

    // Apply quantization root note
    int32_t quantizer_root = pots_[Pot::PITCH_ROOT]->GetMappedValue();
    base_pitch *= Quantizer::kMultiplierTable[quantizer_root];

    // Apply FREE PITCH mod
    int32_t pitch_free_mod = apply_pot_mod_attenuvert(Kastle2::hw.GetAnalogValue(CV_PITCH_FREE), pitch_mod_pot);
    base_pitch *= std::pow(2.0f, static_cast<float>(pitch_free_mod) / static_cast<float>(ADC_1V)); // V/Oct

    // Add fine tuning
    base_pitch *= curve_map(pots_[Pot::PITCH_FINE]->GetValue(), kMapPitchFine);

    // Clamp the frequency
    base_pitch = fmin(base_pitch, kMaxPitchHz);

    // Calculate timbre and resonance settings
    int32_t timbre_val = pots_[Pot::TIMBRE]->GetValue();
    timbre_val += apply_pot_mod_attenuvert(Kastle2::hw.GetAnalogValue(CV_TIMBRE), pots_[Pot::TIMBRE_MOD]->GetValue());
    int32_t resonance_val = pots_[Pot::RESONANCE]->GetValue();

    // Each mode sets different values
    switch (current_mode_)
    {
    case Mode::SUBTRACTIVE:
    {
        subtractive_osc_.SetFrequency(base_pitch);
        float cutoff_frequency = curve_map(timbre_val, kMapFilterFreq, MapClamp::TRUE);
        filter_.SetFrequency(cutoff_frequency);
        float resonance = curve_map(resonance_val, kMapResonance, MapClamp::TRUE);
        filter_.SetResonance(resonance);
        break;
    }
    case Mode::FM:
    {
        fm_osc_.SetFrequency(base_pitch);
        // MapClamp::TRUE and especially MapSafe::TRUE is necessary here so we don't overflow q31 while calculating
        fm_osc_.SetIndex(curve_map(timbre_val, kMapFmIndex, MapClamp::TRUE, MapSafe::TRUE));
        fm_osc_.SetRatio(curve_map(resonance_val, kMapFmRatio, MapClamp::TRUE, MapSafe::TRUE));
        break;
    }
    }

    // Calculate envelope
    int32_t env_val = pots_[Pot::ENV]->GetValue();
    env_val += apply_pot_mod_attenuvert(Kastle2::hw.GetAnalogValue(CV_ENV), pots_[Pot::ENV_MOD]->GetValue());
    env_.SetAttackTime(curve_map(env_val, kMapEnvAttack, MapClamp::TRUE));
    env_.SetDecayTime(curve_map(env_val, kMapEnvDecay, MapClamp::TRUE));
    if (pots_[Pot::ENV]->HasChanged())
    {
        env_enabled_ = (env_val > pot(0.05f)) && (env_val < pot(0.95f));
    }

    // Pass the calculated envelope into ENV output
    Kastle2::hw.SetEnvOut(((uint32_t)env_value_) >> (15 - 10));

    // Update delay
    q15_t delay_wet = curve_map(pots_[Pot::FX]->GetValue(), kMapFxDelay, MapClamp::TRUE, MapSafe::TRUE);
    stereo_delay_.SetWet(delay_wet);
    UpdateDelayTime();

    // LEDs
    if (Kastle2::hw.GetLayer() == Hardware::Layer::NORMAL || Kastle2::hw.GetLayer() == Hardware::Layer::MODE)
    {
        uint32_t color = mode_colors_[current_mode_];
        uint8_t bright = (env_value_ >> (15 - 6)) + 63;
        color = WS2812::ApplyBrightness(color, bright);
        Kastle2::hw.SetLed(Hardware::Led::LED_1, color);
        Kastle2::hw.SetLed(Hardware::Led::LED_2, color);
    }
    else
    {
        Kastle2::hw.SetLed(Hardware::Led::LED_1, 0);
        Kastle2::hw.SetLed(Hardware::Led::LED_2, 0);
    }
}

void AppExampleSynth::MemoryInitialization()
{
    Kastle2::memory.Write8(kMemMode, std::to_underlying(Mode::SUBTRACTIVE));
    Kastle2::memory.Write8(kMemFx, pot_to_mem(kFxDefaultValue));
    Kastle2::memory.Write8(kMemEnvMod, pot_to_mem(kEnvModDefaultValue));
    Kastle2::memory.Write8(kMemPitchScale, pot_to_mem(kPitchScaleDefaultValue));
    Kastle2::memory.Write8(kMemPitchRoot, pot_to_mem(kPitchRootDefaultValue));
    Kastle2::memory.Write8(kMemPitchFine, pot_to_mem(kPitchFineDefaultValue));
    Kastle2::memory.Write8(kMemModeMod, pot_to_mem(kModeModDefaultValue));
};