/*
MIT License

Copyright (c) 2024 Marek Mach (Bastl Instruments)

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

#include "AppTemplate.hpp"
#include "common/core/Kastle2.hpp"
#include "common/utils.hpp"

using namespace kastle2;

void AppTemplate::Init()
{
    inited_ = false;
    mode_ = Mode::FIRST;

    // disable audio chain, we are doing it ourselves
    Kastle2::base.SetFeatureEnabled(Base::Feature::AUDIO_CHAIN, false);

    inited_ = true;
}

void AppTemplate::DeInit()
{
    inited_ = false;
}

FASTCODE void AppTemplate::AudioLoop(q15_t *input, q15_t *output, size_t size)
{
    if (!inited_)
    {
        return;
    }
    for (size_t i = 0; i < size; i++)
    {
        // read
        q15_t left = input[2 * i];
        q15_t right = input[2 * i + 1];

        // code that runs each sample

        // output
        output[2 * i] = left;
        output[2 * i + 1] = right;
    }
}

void AppTemplate::UiLoop()
{
    // Change LED color based on mode
    switch (mode_)
    {
    case Mode::FIRST:
        Kastle2::hw.SetLed(Hardware::Led::LED_2, 255, 255, 255);
        break;

    case Mode::SECOND:
        Kastle2::hw.SetLed(Hardware::Led::LED_2, 0, 255, 0);
        break;

    case Mode::THIRD:
        Kastle2::hw.SetLed(Hardware::Led::LED_2, 255, 0, 0);
        break;
    }

    // Cycle modes
    if (Kastle2::hw.JustReleased(Hardware::Button::MODE))
    {
        mode_ = EnumIncrement(mode_);
    }

    // set kastle LED
    Kastle2::hw.SetLed(Hardware::Led::LED_1, 0xFF8090);
}

void AppTemplate::MidiCallback(midi::Message *msg)
{
    // Do something here...
}
