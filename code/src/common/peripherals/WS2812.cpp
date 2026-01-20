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

/*
 * This implementation incorporates code derived from:
 *
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Original library by ForsakenNGS: https://github.com/ForsakenNGS/Pico_WS2812
 */

#include "WS2812.hpp"
#include "WS2812.pio.h"

using namespace kastle2;

WS2812::WS2812(size_t pin, size_t length, PIO pio, size_t sm)
{
    Initialize(pin, length, pio, sm, DataByte::GREEN, DataByte::RED, DataByte::BLUE);
}

WS2812::WS2812(size_t pin, size_t length, PIO pio, size_t sm, DataFormat format)
{
    switch (format)
    {
    case DataFormat::RGB:
        Initialize(pin, length, pio, sm, DataByte::RED, DataByte::GREEN, DataByte::BLUE);
        break;
    case DataFormat::GRB:
        Initialize(pin, length, pio, sm, DataByte::GREEN, DataByte::RED, DataByte::BLUE);
        break;
    }
}

WS2812::WS2812(size_t pin, size_t length, PIO pio, size_t sm, DataByte b1, DataByte b2, DataByte b3)
{
    Initialize(pin, length, pio, sm, b1, b2, b3);
}

void WS2812::Initialize(size_t pin, size_t length, PIO pio, size_t sm, DataByte b1, DataByte b2, DataByte b3)
{
    pin_ = pin;
    length_ = length;
    pio_ = pio;
    sm_ = sm;
    data_ = std::make_unique<uint32_t[]>(length);
    bytes_[0] = b1;
    bytes_[1] = b2;
    bytes_[2] = b3;
    size_t offset = pio_add_program(pio, &ws2812_program);
    size_t bits = 24;
    ws2812_program_init(pio, sm, offset, pin, 800000, bits);
}

uint32_t WS2812::ConvertData(uint32_t rgb)
{
    uint32_t result = 0;
    for (size_t b = 0; b < 3; b++)
    {
        switch (bytes_[b])
        {
        case DataByte::RED:
            result |= (rgb & 0xFF0000) >> 16;
            break;
        case DataByte::GREEN:
            result |= (rgb & 0xFF00) >> 8;
            break;
        case DataByte::BLUE:
            result |= (rgb & 0xFF);
            break;
        case DataByte::NONE:
        default:
            result = 0;
            break;
        }
        result <<= 8;
    }
    return result;
}

void WS2812::SetPixelColor(size_t index, uint32_t color)
{
    if (index < length_)
    {
        data_[index] = ConvertData(color);
    }
}

void WS2812::SetPixelColor(size_t index, uint8_t red, uint8_t green, uint8_t blue)
{
    SetPixelColor(index, RGB(red, green, blue));
}

void WS2812::Fill(uint32_t color)
{
    Fill(color, 0, length_);
}

void WS2812::Fill(uint32_t color, size_t first)
{
    Fill(color, first, length_ - first);
}

void WS2812::Fill(uint32_t color, size_t first, size_t count)
{
    size_t last = (first + count);
    if (last > length_)
    {
        last = length_;
    }
    color = ConvertData(color);
    for (size_t i = first; i < last; i++)
    {
        data_[i] = color;
    }
}

void WS2812::Show()
{
    for (size_t i = 0; i < length_; i++)
    {
        pio_sm_put_blocking(pio_, sm_, data_[i]);
    }
}

uint32_t WS2812::ApplyBrightness(uint32_t color, uint8_t brightness)
{
    return RGB(
        brightness * GetColorByte(color, 2) >> 8,
        brightness * GetColorByte(color, 1) >> 8,
        brightness * GetColorByte(color, 0) >> 8);
}

uint32_t WS2812::MixColors(uint32_t color1, uint32_t color2)
{
    return RGB(
        SaturateByte(GetColorByte(color1, 2) + GetColorByte(color2, 2)),
        SaturateByte(GetColorByte(color1, 1) + GetColorByte(color2, 1)),
        SaturateByte(GetColorByte(color1, 0) + GetColorByte(color2, 0)));
}

uint32_t WS2812::CrossfadeColors(uint32_t color1, uint32_t color2, uint8_t ratio)
{
    if (ratio == 0)
    {
        return color1;
    }
    else if (ratio == 255)
    {
        return color2;
    }
    else
    {
        return RGB(
            (((uint16_t)GetColorByte(color1, 2) * (255 - ratio)) + ((uint16_t)GetColorByte(color2, 2) * ratio)) / 255,
            (((uint16_t)GetColorByte(color1, 1) * (255 - ratio)) + ((uint16_t)GetColorByte(color2, 1) * ratio)) / 255,
            (((uint16_t)GetColorByte(color1, 0) * (255 - ratio)) + ((uint16_t)GetColorByte(color2, 0) * ratio)) / 255);
    }
}

uint8_t WS2812::GetColorByte(uint32_t color, uint32_t index)
{
    if (index > 2)
    {
        return 0;
    }
    return (color >> (8 * index)) & 0xFF;
}

uint8_t WS2812::SaturateByte(uint32_t color)
{
    if (color > 0xFF)
    {
        return 0xFF;
    }
    return color;
}