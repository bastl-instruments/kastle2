/*
MIT License

Copyright (c) 2023 Vaclav Mach (Bastl Instruments)
Copyright (c) 2023 Marek Mach (Bastl Instruments)

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

// Main stuff
#include "common/core/Kastle2.hpp"
#include "common/testmode/VersionChainGenerator.hpp"
#include "apps/FxWizard/AppFxWizard.hpp"
#include "version_samples_app.hpp"

using namespace kastle2;

/**
 * @file main.cpp
 * @brief Main entry point for Kastle 2 firmware.
 * @author Vaclav Mach (Bastl Instruments), Marek Mach (Bastl Instruments)
 * @date 2023-11-28
 */

#define APP_VERSION "1.6"
AppFxWizard app;

static void process_audio(q15_t *input, q15_t *output, size_t size)
{
    app.AudioLoop(input, output, size);
}

static void second_core()
{
    app.SecondCoreWorker();
}

static void midi_callback(midi::Message *msg)
{
    app.MidiCallback(msg);
}

int main()
{
    // Initializes the hardware, passing the version chain
    Kastle2::Init(VersionChainGenerator::Generate(version::fx_wizard, APP_VERSION));

    // Register it with the Kastle 2
    // Clears the EEPROM app space if the ID is different
    Kastle2::RegisterApp(&app);

    // Initialize the app
    app.Init();

    // Start second core
    Kastle2::StartSecondCore(second_core);

    // Start I2S
    Kastle2::StartAudio(process_audio);

    // Set the MIDI callback
    Kastle2::SetAppMidiCallback(midi_callback);

    // Infinite program loop
    while (true)
    {
        Kastle2::ReadInputs();
        app.UiLoop();
    }

    return 0;
}
