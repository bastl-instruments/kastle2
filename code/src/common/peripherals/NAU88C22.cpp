/*
MIT License

Copyright (c) 2023 Vaclav Mach (Bastl Instruments)

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

#include "NAU88C22.hpp"
#include "pico/stdlib.h"
#include "hardware/i2c.h"

using namespace kastle2;

bool NAU88C22::Init(i2c_inst_t *i2c_inst)
{
    i2c_inst_ = i2c_inst;
    next_time_finish_special_registers_ = get_absolute_time();
    zero_cross_enabled_ = false;

    // Init itself
    WriteRegister(RESET, 0x000); // Reset all
    sleep_ms(100);

    uint16_t device_id = ReadRegister(DEVICE_ID);
    if (device_id != DEVICE_ID_SHOULD_BE)
    {
        return false;
    }

    WriteRegister(POWER_MANAGEMENT_1, 0b011011111); // Enable AUX mixer, PLL and MIC bias is disabled, internal bias 80kOhm
    WriteRegister(POWER_MANAGEMENT_2, 0b110111111); // Enable L/R Headphone, ADC Mix/Boost, ADC
    WriteRegister(POWER_MANAGEMENT_3, 0b110001111); // Enable AUX out, disable speaker, L/R main mixer, DAC
    WriteRegister(POWER_MANAGEMENT_4, 0b000000000); // Disable power saving

    // WriteRegister(AUDIO_INTERFACE, 0b000010000); // 16-bit word length, I2S format, Stereo

    // I don't know why, but L+R must be reversed in ADC stage so the channels are OK.
    WriteRegister(AUDIO_INTERFACE, 0b001010010); // 24-bit word length, I2S format, Stereo, ADC L<->R reversed

    WriteRegister(COMPANDING_CONTROL, 0b000000000); // Companding control and loop back mode (all disable)

    WriteRegister(CLOCK_CONTROL_1, 0b0); // External MCLK etc
    WriteRegister(CLOCK_CONTROL_2, 0b000000000);

    WriteRegister(DAC_CONTROL, 0b000001000);        // DAC soft mute is disabled, DAC oversampling rate is 128x
    WriteRegister(LEFT_DAC_DIGITAL_VOLUME, 0x0FF);  // DAC left digital volume control, max volume is FF
    WriteRegister(RIGHT_DAC_DIGITAL_VOLUME, 0x1FF); // DAC right digital volume control, max volume is FF
    WriteRegister(DAC_LIMITER_1, 0b000000000);      // Disable DAC limiter
    WriteRegister(DAC_LIMITER_2, 0b000000000);      // Disable DAC limiter

    // WriteRegister(DAC_DITHER, 0b0);              // Disable DAC Dither - doesn't help
    // WriteRegister(MISC_CONTROLS, 0b000000001);   // Oversampling to 256 - no hearable change

    WriteRegister(INPUT_CONTROL, 0b001000100); // Enable line inputs

    WriteRegister(ADC_CONTROL, 0b000001000);             // ADC HP filter is disabled, ADC oversampling rate is 128x
    WriteRegister(LEFT_ADC_BOOST_CONTROL, 0b000000000);  // Direct ADC: Line disconnected, Aux disconnected
    WriteRegister(RIGHT_ADC_BOOST_CONTROL, 0b000000000); //  Direct ADC: Line disconnected, Aux disconnected
                                                         // WriteRegister(LEFT_ADC_BOOST_CONTROL, 0b001010000);  // Line connected at 0db, Aux disconnected
                                                         // WriteRegister(RIGHT_ADC_BOOST_CONTROL, 0b001010000); // Line connected at 0db, Aux disconnected
    WriteRegister(LEFT_ADC_DIGITAL_VOLUME, 0x0F0);       // ADC left digital volume control (FF is max)
    WriteRegister(RIGHT_ADC_DIGITAL_VOLUME, 0x1F0);      // ADC right digital volume control (FF is max)

    WriteRegister(OUTPUT_CONTROL, 0b000000010);
    WriteRegister(LEFT_MIXER_CONTROL, 0b101010101);  // Left DAC connected to LMIX
    WriteRegister(RIGHT_MIXER_CONTROL, 0b101010101); // Right DAC connected to RMIX

    // WriteRegister(LEFT_HP_VOLUME, 0b100111001);  //  0 dB
    // WriteRegister(RIGHT_HP_VOLUME, 0b100111001); //  0 dB
    Update();

    // WriteRegister(AUX2_MIXER_CONTROL, 0b000000010); // Left main mixer to AUX2
    WriteRegister(AUX2_MIXER_CONTROL, 0b000000010); // Left main mixer to AUX2
    WriteRegister(AUX1_MIXER_CONTROL, 0b000000010); // Right main mixer to AUX1

    // reduce highs a bit to lower noise
    // frequency 11.7kHz, -2dB
    SetEqPath(EqPath::INPUT);
    SetEqBand(EqBand::BAND_5, EqCutoff::CUTOFF_4, -2, EqWidth::WIDE);

    return true;
}

void NAU88C22::WriteRegister(uint8_t addr, uint16_t value)
{
    // the value can be 9-bit, so the MSB is part of the register address
    uint8_t data[2] = {
        (uint8_t)(addr << 1 | ((value >> 8) & 1)),
        (uint8_t)(value & 0xFF)};

    i2c_write_blocking_until(i2c_inst_, I2C_ADDRESS, data, 2, false, make_timeout_time_ms(20));
}

uint16_t NAU88C22::ReadRegister(uint8_t addr)
{
    uint8_t data[1] = {(uint8_t)(addr << 1)};
    i2c_write_blocking_until(i2c_inst_, I2C_ADDRESS, data, 1, true, make_timeout_time_ms(20));
    uint8_t received_bytes[2];
    uint8_t num_bytes_read;
    num_bytes_read = i2c_read_blocking_until(i2c_inst_, I2C_ADDRESS, received_bytes, 2, false, make_timeout_time_ms(20));
    if (num_bytes_read != 2)
    {
        return INVALID;
    }
    return (received_bytes[0] << 8) | received_bytes[1];
}

void NAU88C22::SetEqPath(EqPath path)
{
    uint16_t reg = ReadRegister(EQ1) & 0b011111111;
    reg |= (static_cast<uint16_t>(path) << 8);
    WriteRegister(EQ1, reg);
}

void NAU88C22::SetEqBand(EqBand band, EqCutoff cutoff, int8_t gain, EqWidth width)
{
    if (gain > 12)
    {
        gain = 12;
    }
    if (gain < -12)
    {
        gain = -12;
    }

    uint8_t gainu = 12 - gain;
    uint16_t reg = 0;

    if (band == EqBand::BAND_1)
    {
        reg = ReadRegister(EQ1) & 0b100000000;
    }
    else if (band != EqBand::BAND_5)
    {
        reg = static_cast<uint16_t>(width) << 8;
    }
    reg |= (static_cast<uint16_t>(cutoff) << 5) | gainu;
    WriteRegister(EQ1 + static_cast<uint8_t>(band), reg);
}

void NAU88C22::SetHpVolume(uint8_t volume)
{
    volume &= MAX_HP_VOLUME;
    SendSpecialRegister(SpecialRegister::HP_VOLUME, volume);
}

void NAU88C22::SetInputGain(uint8_t gain)
{
    gain &= MAX_INPUT_GAIN;
    SendSpecialRegister(SpecialRegister::INPUT_GAIN, gain);
}

void NAU88C22::Set3dEffect(uint8_t effect)
{
    effect &= MAX_3D_EFFECT;
    SendSpecialRegister(SpecialRegister::REG_3D, effect);
}

void NAU88C22::SendSpecialRegister(SpecialRegister reg, uint8_t value)
{
    if (value == special_registers_values_[reg])
    {
        return;
    }

    special_registers_set_[reg] = true;
    special_registers_values_[reg] = value;
}

void NAU88C22::Update()
{
    if (absolute_time_diff_us(next_time_finish_special_registers_, get_absolute_time()) < 0)
    {
        return;
    }

    uint32_t zero_cross_flag = zero_cross_enabled_ << 7;

    for (SpecialRegister reg : EnumRange<SpecialRegister>())
    {
        if (special_registers_set_[reg])
        {
            uint8_t val = special_registers_values_[reg];
            special_registers_set_[reg] = false;
            switch (reg)
            {
            case SpecialRegister::HP_VOLUME:
                WriteRegister(LEFT_HP_VOLUME, 0b100000000 | val | zero_cross_flag);  //  set and update immediatelly
                WriteRegister(RIGHT_HP_VOLUME, 0b100000000 | val | zero_cross_flag); //  set and update immediatelly
                break;
            case SpecialRegister::INPUT_GAIN:
                WriteRegister(LEFT_INP_PGA_CONTROL, 0b100000000 | val | zero_cross_flag);  //  set and update immediatelly
                WriteRegister(RIGHT_INP_PGA_CONTROL, 0b100000000 | val | zero_cross_flag); //  set and update immediatelly
                break;
            case SpecialRegister::REG_3D:
                WriteRegister(_3D_CONTROL, val);
                break;
            }
        }
    }

    // Update codec each few ms, not immediatelly
    // Prevents codec sound glitches
    next_time_finish_special_registers_ = make_timeout_time_ms(20);
}

void NAU88C22::SetZeroCrossUpdate(bool enabled)
{
    zero_cross_enabled_ = enabled;
}