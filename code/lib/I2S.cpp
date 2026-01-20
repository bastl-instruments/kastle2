/*
 * I2S.cpp
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

#include "I2S.hpp"

I2S::FractionalDivider I2S::CalculateDivider(float target_frequency)
{
    float sys_clk = static_cast<float>(clock_get_hz(clk_sys));
    float divider = sys_clk / target_frequency;
    uint16_t int_div = static_cast<uint16_t>(divider);
    uint8_t frac_div = static_cast<uint8_t>((divider - int_div) * 256.0f + 0.5f);
    return FractionalDivider{int_div, frac_div};
}

I2S::FractionalDivider I2S::GetMclkDivider()
{
    float mclk_freq = kMclkMult * sample_rate_;
    float pio_freq = 2.0f * mclk_freq; // Toggle loop = 2 instructions per cycle
    return CalculateDivider(pio_freq);
}

I2S::FractionalDivider I2S::GetBclkDivider()
{
    float bclk_freq = sample_rate_ * kBitDepth * 2.0f; // sample rate * bit depth * 2 (for stereo)
    float pio_freq = 2.0f * bclk_freq;
    return CalculateDivider(pio_freq);
}

void I2S::StartAudio(AudioCallback callback)
{
    callback_ = callback;
    pio_ = pio0; // Using PIO0 for I2S

    // Initialize MCLK (Master Clock)
    auto mclk_clock = GetMclkDivider();
    sm_mclk_ = pio_claim_unused_sm(pio_, true);
    uint32_t mclk_offset = pio_add_program(pio_, &i2s_mclk_program);
    i2s_mclk_program_init(pio_, sm_mclk_, mclk_offset, pins_.mclk);
    pio_sm_set_clkdiv_int_frac(pio_, sm_mclk_, mclk_clock.div, mclk_clock.frac);

    // Initialize DIN (Data Input)
    sm_din_ = pio_claim_unused_sm(pio_, true);
    uint32_t din_offset = pio_add_program(pio_, &i2s_din_program);
    i2s_din_program_init(pio_, sm_din_, din_offset, pins_.din, kBitDepth);
    pio_sm_set_clkdiv_int_frac(pio_, sm_din_, mclk_clock.div, mclk_clock.frac);

    // Initialize DOUT (Data Output)
    auto bclk_clock = GetBclkDivider();
    sm_dout_ = pio_claim_unused_sm(pio_, true);
    uint32_t dout_offset = pio_add_program(pio_, &i2s_dout_program);
    i2s_dout_program_init(pio_, sm_dout_, dout_offset, pins_.dout, pins_.bclk, kBitDepth);
    pio_sm_set_clkdiv_int_frac(pio_, sm_dout_, bclk_clock.div, bclk_clock.frac);

    // Initialize DMA and start state machines
    DmaInit();
    pio_enable_sm_mask_in_sync(pio_, (1u << sm_mclk_) | (1u << sm_din_) | (1u << sm_dout_));
}

void I2S::DmaInit()
{
    // Claim DMA channels for double-buffered I2S transfer
    dma_din_ctrl_ = dma_claim_unused_channel(true);  // Controls input buffer swapping
    dma_din_data_ = dma_claim_unused_channel(true);  // Transfers audio data from I2S to memory
    dma_dout_ctrl_ = dma_claim_unused_channel(true); // Controls output buffer swapping
    dma_dout_data_ = dma_claim_unused_channel(true); // Transfers audio data from memory to I2S

    // Configure input data DMA - This channel moves data from PIO RX FIFO to memory
    dma_channel_config din_data_cfg = dma_channel_get_default_config(dma_din_data_);
    channel_config_set_transfer_data_size(&din_data_cfg, DMA_SIZE_32);          // I2S data is 32-bit
    channel_config_set_read_increment(&din_data_cfg, false);                    // Read from same FIFO address
    channel_config_set_write_increment(&din_data_cfg, true);                    // Write sequentially to buffer
    channel_config_set_dreq(&din_data_cfg, pio_get_dreq(pio_, sm_din_, false)); // Wait for PIO RX FIFO
    channel_config_set_chain_to(&din_data_cfg, dma_din_ctrl_);                  // Chain to control DMA after completion
    channel_config_set_irq_quiet(&din_data_cfg, false);                         // Generate interrupt for audio processing

    // Configure input control DMA - This channel updates the data DMA's write address for double buffering
    dma_channel_config din_ctrl_cfg = dma_channel_get_default_config(dma_din_ctrl_);
    channel_config_set_transfer_data_size(&din_ctrl_cfg, DMA_SIZE_32); // Transferring 32-bit addresses
    channel_config_set_read_increment(&din_ctrl_cfg, true);            // Read from sequential buffer pointers
    channel_config_set_write_increment(&din_ctrl_cfg, false);          // Write to same config register
    channel_config_set_ring(&din_ctrl_cfg, false, 3);                  // Ring size of 8 (2^3) for buffer pointer array

    // Configure output data DMA - This channel moves data from memory to PIO TX FIFO
    dma_channel_config dout_data_cfg = dma_channel_get_default_config(dma_dout_data_);
    channel_config_set_transfer_data_size(&dout_data_cfg, DMA_SIZE_32);          // I2S data is 32-bit
    channel_config_set_read_increment(&dout_data_cfg, true);                     // Read sequentially from buffer
    channel_config_set_write_increment(&dout_data_cfg, false);                   // Write to same FIFO address
    channel_config_set_dreq(&dout_data_cfg, pio_get_dreq(pio_, sm_dout_, true)); // Wait for PIO TX FIFO
    channel_config_set_chain_to(&dout_data_cfg, dma_dout_ctrl_);                 // Chain to control DMA after completion

    // Configure output control DMA - This channel updates the data DMA's read address for double buffering
    dma_channel_config dout_ctrl_cfg = dma_channel_get_default_config(dma_dout_ctrl_);
    channel_config_set_transfer_data_size(&dout_ctrl_cfg, DMA_SIZE_32); // Transferring 32-bit addresses
    channel_config_set_read_increment(&dout_ctrl_cfg, true);            // Read from sequential buffer pointers
    channel_config_set_write_increment(&dout_ctrl_cfg, false);          // Write to same config register
    channel_config_set_ring(&dout_ctrl_cfg, false, 3);                  // Ring size of 8 (2^3) for buffer pointer array

    // Set up input data DMA - Moves audio data from PIO to memory buffer
    dma_channel_configure(
        dma_din_data_,       // The data DMA channel
        &din_data_cfg,       // Configuration we just created
        NULL,                // Initial write address (set by control DMA)
        &pio_->rxf[sm_din_], // Read from PIO RX FIFO
        kAudioBufferFrames,  // Transfer this many words
        false                // Don't start yet
    );

    // Set up input control DMA - Controls double buffering for input
    dma_channel_configure(
        dma_din_ctrl_,                                  // The control DMA channel
        &din_ctrl_cfg,                                  // Configuration we just created
        &dma_hw->ch[dma_din_data_].al2_write_addr_trig, // Write to data DMA's write address and trigger
        din_ptr_,                                       // Read addresses from our buffer pointer array
        1,                                              // Transfer one address per buffer
        false                                           // Don't start yet
    );

    // Set up output data DMA - Moves audio data from memory buffer to PIO
    dma_channel_configure(
        dma_dout_data_,       // The data DMA channel
        &dout_data_cfg,       // Configuration we just created
        &pio_->txf[sm_dout_], // Write to PIO TX FIFO
        output_buffers_[0],   // Initial read address (first buffer)
        kAudioBufferFrames,   // Transfer this many words
        false                 // Don't start yet
    );

    // Set up output control DMA - Controls double buffering for output
    dma_channel_configure(
        dma_dout_ctrl_,                                 // The control DMA channel
        &dout_ctrl_cfg,                                 // Configuration we just created
        &dma_hw->ch[dma_dout_data_].al3_read_addr_trig, // Write to data DMA's read address and trigger
        dout_ptr_,                                      // Read addresses from our buffer pointer array
        1,                                              // Transfer one address per buffer
        false                                           // Don't start yet
    );

    // Set up interrupt handling - We use input completion to trigger audio processing
    dma_channel_set_irq0_enabled(dma_din_data_, true); // Enable interrupt on input data completion
    irq_set_exclusive_handler(DMA_IRQ_0, DmaHandler);  // Set our handler
    irq_set_enabled(DMA_IRQ_0, true);                  // Enable the interrupt

    // Start the DMA transfers - Only start control channels, they will trigger data channels
    dma_start_channel_mask((1u << dma_din_ctrl_) | (1u << dma_dout_ctrl_));
}

void I2S::DmaHandler()
{
    // No instance available, nothing to do
    if (instance_ == nullptr)
    {
        return;
    }

    // Determine which buffer the DMA is currently reading to by checking the read address of the control channel
    size_t buffer_idx = 0;
    if (dma_hw->ch[instance_->dma_din_ctrl_].read_addr == (size_t)&instance_->din_ptr_[0])
    {
        buffer_idx = 0;
    }
    else
    {
        buffer_idx = 1;
    }

    // Scale down input to 16 bits
    for (size_t i = 0; i < kAudioBufferFrames; i++)
    {
        instance_->input_buffers_[buffer_idx][i] = downscale_to_16bits(instance_->input_buffers_[buffer_idx][i]);
    }

    // Process audio in 16 bits
    instance_->callback_(instance_->input_buffers_[buffer_idx],
                         instance_->output_buffers_[buffer_idx],
                         kAudioBufferSize);

    // Scale back audio to 32 bits
    for (size_t i = 0; i < kAudioBufferFrames; i++)
    {
        instance_->output_buffers_[buffer_idx][i] = upscale_to_32bits(instance_->output_buffers_[buffer_idx][i]);
    }

    // Clear the interrupt
    dma_hw->ints0 = 1u << instance_->dma_din_data_;
}