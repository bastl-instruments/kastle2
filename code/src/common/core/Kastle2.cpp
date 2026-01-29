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

#include "Kastle2.hpp"
#include "common/debug.hpp"
#include "common/fastcode.hpp"
#include "tusb.h"

using namespace kastle2;

void Kastle2::StartAudio(I2S::AudioCallback callback)
{
    audio_callback_ = callback;
    hw.GetI2S().StartAudio(AudioCallback);
}

void Kastle2::StartSecondCore(MultiCore::Worker second_core_worker)
{
    MultiCore::StartSecondCore(second_core_worker);
}

void Kastle2::SetAppMidiCallback(midi::Handler::Callback callback)
{
    midi.SetAppCallback(callback);
}

void Kastle2::BaseMidiCallback(midi::Message *msg)
{
    base.MidiCallback(msg);
}

void Kastle2::TestModeMidiCallback(midi::Message *msg)
{
    if (test_mode_enabled_ && test_mode_ != nullptr)
    {
        test_mode_->MidiCallback(msg);
    }
}

void Kastle2::Init(std::span<const SamplePlayer16bit::Sample> version_chain)
{
    // Fast code lives in RAM and is much faster than executing from the QSPI flash
    copy_fastcode_to_ram();

    // Fix for Rpi Debug Probe
    fix_pi_probe_debugging();

    test_mode_enabled_ = false;

// Binary info
#ifdef USB_PRODUCT_NAME
    bi_decl(bi_program_description(USB_PRODUCT_NAME));
#else
    bi_decl(bi_program_description("Kastle 2"));
#endif

// Set a custom RP2040 frequency (necessary for proper I2S clock)
#if defined(PICO_SDK_VERSION_MAJOR) && PICO_SDK_VERSION_MAJOR >= 2
    set_sys_clock_hz(SYSTEM_CLOCK_KHZ * 1000, true);
#else
    set_sys_clock_khz(SYSTEM_CLOCK_KHZ, true);
#endif

    // Initialize Tiny USB for both MIDI and CDC
    tusb_init();

    // Init Hardware
    hw.Init();

    // Init Codec
    if (!codec.Init(Hardware::I2C_INSTANCE))
    {
        while (1)
        {
            hw.ShowStartupMessage(Hardware::StartupMessage::CODEC_FAIL);
        }
    }

    // Init memory
    Memory::State state = memory.Init(Hardware::I2C_INSTANCE);
    switch (state)
    {
    case Memory::State::OK:
        break;
    case Memory::State::INIT_OK:
        hw.ShowStartupMessage(Hardware::StartupMessage::EEPROM_INIT_OK);
        break;
    case Memory::State::INIT_FAIL:
        hw.ShowStartupMessage(Hardware::StartupMessage::EEPROM_INIT_FAIL);
        break;
    case Memory::State::MISSING:
        hw.ShowStartupMessage(Hardware::StartupMessage::EEPROM_MISSING);
        break;
    }

    // Read & set calibrations
    Hardware::CalibrationsType calibrations;
    if (memory.ReadCalibrations(calibrations))
    {
        hw.SetCustomCalibrations(calibrations);
    }

    // Do some readings to make sure everything is working and srand is activated etc.
    ReadInputs();

    // Allow some time for ADC to take readings
    sleep_ms(100);

    // Button reading timeout setup
    timeout_read_buttons_ = get_absolute_time();

    // Init Midi
    midi.Init();

    // If MODE button is pressed, enter test mode (hijacks the whole system)
    if (ALWAYS_GO_TO_TEST_MODE || hw.Pressed(Hardware::Button::MODE))
    {
        test_mode_enabled_ = true;
        test_mode_ = std::make_unique<TestMode>();
        test_mode_->Init(SAMPLE_RATE);
        test_mode_->SetVersionChain(version_chain);
        StartAudio(nullptr);

        // Set the MIDI callback for the TestMode
        midi.SetBaseCallback(TestModeMidiCallback);

        while (true)
        {
            ReadInputs();
            test_mode_->UiLoop();
        }
        return;
    }

    // Set the MIDI callback for the Base
    midi.SetBaseCallback(BaseMidiCallback);

    // Init Base
    base.Init();
}

void Kastle2::ReadInputs()
{
#if MEASURE_UI_LOOP
    Kastle2::hw.SetDebugPin(0, 1);
#endif

    // Weird to have it at beginning of the function, but we are in while(true) loop...
    base.AfterUiLoop();
    hw.LatchLeds();

    // Process TinyUSB tasks (including MIDI and CDC Serial)
    tud_task();

    // Each loop clear "JustPressed" and "JustReleased"
    hw.ClearButtonJusts();

    // Prevents buttons bouncing
    if (absolute_time_diff_us(timeout_read_buttons_, get_absolute_time()) > 0)
    {
        hw.ReadButtons();
        // Wait kUiRefreshWaitMs before next button read
        timeout_read_buttons_ = make_timeout_time_ms(Hardware::kUiRefreshWaitMs);
    }

    codec.Update();
    midi.Process();
    base.BeforeUiLoop();
    memory.ProcessQueue();
    debug.Process();
#if MEASURE_UI_LOOP
    Kastle2::hw.SetDebugPin(0, 0);
#endif
}

void Kastle2::AudioCallback(q15_t *input, q15_t *output, size_t size)
{
#if MEASURE_AUDIO_LOOP
    Kastle2::hw.SetDebugPin(0, 1);
#endif

    // Citadel DC offset removal
    if constexpr (kCitadelInputDcOffsetRemove)
    {
        if (Kastle2::hw.GetVersion() == Hardware::Version::CITADEL)
        {
            for (size_t i = 0; i < size * 2; i++)
            {
                input[i] = q15_add(input[i], kCitadelInputDcOffset);
            }
        }
    }

    if (!test_mode_enabled_)
    {
        base.BeforeAudioLoop(input, size);
        audio_callback_(input, output, size);
        base.AfterAudioLoop(input, output, size);
    }
    else
    {
        test_mode_->AudioLoop(input, output, size);
    }

    // Citadel DC offset removal
    if constexpr (kCitadelOutputDcOffsetRemove)
    {
        if (Kastle2::hw.GetVersion() == Hardware::Version::CITADEL)
        {
            for (size_t i = 0; i < size * 2; i++)
            {
                output[i] = q15_add(output[i], kCitadelOutputDcOffset);
            }
        }
    }

#if MEASURE_AUDIO_LOOP
    Kastle2::hw.SetDebugPin(0, 0);
#endif
}

bool Kastle2::RegisterApp(App *app_to_register)
{
    app = app_to_register;
    uint8_t app_id = app->GetId();
    uint8_t stored_id;
    if (!memory.Read8(Memory::ADDR_APP_ID, &stored_id))
    {
        return false;
    }

    // If the stored ID is different, init the EEPROM
    if (stored_id != app_id && app_id != kDefaultAppId)
    {
        hw.ShowStartupMessage(Hardware::StartupMessage::EEPROM_WILL_CLEAR);
        app->MemoryInitialization();
        memory.Write8(Memory::ADDR_APP_ID, app_id);
        return true;
    }

    return false;
}