/*
 * I2S.h
 *
 * Lightweight I2S driver for the Raspberry Pi Pico (RP2040)
 * Author: Vaclav Mach (Bastl Instruments)
 * Date: July 2025
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * Inspiration and Derivation:
 *
 * - Portions of this code were derived from the I2S implementation in
 *   the Arduino-Pico core by Earle F. Philhower, III
 *   Copyright (c) 2022 Earle F. Philhower, III <earlephilhower@yahoo.com>
 *   https://github.com/earlephilhower/arduino-pico
 *   Licensed under the GNU Lesser General Public License v2.1
 *
 * - This implementation was also inspired by the structure and behavior of
 *   "rp2040_i2s_example" by Daniel Collins (GPL-3.0 licensed)
 *   https://github.com/malacalypse/rp2040_i2s_example
 *   No code from that project was copied; this implementation was written
 *   independently from scratch using original techniques.
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "hardware/pio.h"

#include "I2S.pio.h"

/**
 * @class I2S
 * @brief I2S audio input/output driver for the Raspberry Pi Pico (RP2040)
 * @ingroup peripherals
 * @author Vaclav Mach (Bastl Instruments)
 * @date 2025-07-08
 */
class I2S
{
public:
    /**
     * @brief Audio buffer size per channel (in frames)
     */
    static constexpr size_t kAudioBufferSize = 48;

    /**
     * @brief Total audio buffer frames (both channels)
     */
    static constexpr size_t kAudioBufferFrames = kAudioBufferSize * 2;

    /**
     * @brief Audio callback function type
     * @param input 16-bit signed integer inside 32-bit container pointer to input audio buffer (interleaved stereo)
     * @param output 16-bit signed integer inside 32-bit container pointer to output audio buffer (interleaved stereo)
     * @param size Number of frames to process (each frame contains samples for both channels)
     */
    typedef void (*AudioCallback)(int32_t *input, int32_t *output, size_t size);

    /**
     * @brief I2S pins
     */
    struct Pins
    {
        size_t mclk; ///< Master clock
        size_t dout; ///< Data output
        size_t din;  ///< Data input
        size_t bclk; ///< Base pin for clocks (LRCLK is always BCLK + 1)
    };

    /**
     * @brief Initializes the I2S driver.
     * @param sample_rate Sample rate of the audio.
     * @param pins I2S pin configuration.
     */
    void Init(const float sample_rate, const Pins &pins)
    {
        sample_rate_ = sample_rate;
        pins_ = pins;
        instance_ = this; // Set the singleton instance for the callback
    }

    /**
     * @brief Starts the audio playback.
     * @param callback Audio callback function.
     */
    void StartAudio(const AudioCallback callback);

private:
    static constexpr float kMclkMult = 256.0f; ///< MCLK multiplier for I2S (typically 256)
    static constexpr float kBitDepth = 32.0f;  ///< We scale it down for processing but we use 32-bits for the communication

    /**
     * @brief Fractional divider structure for PIO clocking.
     */
    struct FractionalDivider
    {
        uint16_t div; ///< Integer part of the divider
        uint8_t frac; ///< Fractional part of the divider (8-bit)
    };

    /**
     * @brief Calculates the fractional divider for a given target frequency.
     * @param target_frequency_hz Target frequency in Hz.
     * @return FractionalDivider containing the integer and fractional parts of the divider.
     */
    FractionalDivider CalculateDivider(float target_frequency_hz);

    /**
     * @brief Gets the MCLK divider based on the sample rate.
     * @return FractionalDivider for MCLK.
     */
    FractionalDivider GetMclkDivider();

    /**
     * @brief Gets the BCLK divider based on the sample rate and bit depth.
     * @return FractionalDivider for BCLK.
     */
    FractionalDivider GetBclkDivider();

    /**
     * @brief Initialize double-buffered DMA for I2S
     */
    void DmaInit();

    /**
     * @brief Handles the DMA interrupt for I2S.
     * @details Needs to be static to be used as an interrupt handler.
     *          Uses the singleton instance to access the I2S object.
     */
    static void DmaHandler();

    /**
     * @brief Singleton instance necessary for the DMA interrupt handler
     */
    static inline I2S *instance_;

    /**
     * @brief Audio callback function
     */
    AudioCallback callback_ = nullptr;

    /**
     * @brief I2S pins configuration
     */
    Pins pins_ = {0, 0, 0, 0};

    /**
     * @brief Sample rate of the audio
     */
    float sample_rate_ = 0.0f;

    /**
     * @brief Converts a q15_t to a q31_t.
     */
    static inline constexpr int32_t upscale_to_32bits(const int32_t x)
    {
        return x << 16;
    }

    /**
     * @brief Converts a q31_t to a q15_t.
     */
    static inline constexpr int32_t downscale_to_16bits(const int32_t x)
    {
        return x >> 16;
    }

    PIO pio_;
    uint32_t sm_mclk_;
    uint32_t sm_dout_;
    uint32_t sm_din_;

    size_t dma_din_ctrl_;
    size_t dma_din_data_;
    size_t dma_dout_ctrl_;
    size_t dma_dout_data_;
    int32_t input_buffers_[2][kAudioBufferFrames];                                             ///< Input buffers for double buffering
    int32_t output_buffers_[2][kAudioBufferFrames];                                            ///< Output buffers for double buffering
    __attribute__((aligned(8))) int32_t *din_ptr_[2]{input_buffers_[0], input_buffers_[1]};    ///< Needs to be aligned to 8 bytes for DMA
    __attribute__((aligned(8))) int32_t *dout_ptr_[2]{output_buffers_[0], output_buffers_[1]}; ///< Needs to be aligned to 8 bytes for DMA
};