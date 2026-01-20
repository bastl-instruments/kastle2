/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
 * Copyright (c) 2025 Vaclav Mach (Bastl Instruments)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include "pico/unique_id.h"
#include "hardware/flash.h"
#include "tusb.h"

#define USB_VENDOR_ID 0x2E8A  // Raspberry Pi
#define USB_PRODUCT_ID 0x10D5 // Kastle 2 (assigned here https://github.com/raspberrypi/usb-pid)

//--------------------------------------------------------------------+
// Device Descriptors
//--------------------------------------------------------------------+
tusb_desc_device_t const desc_device =
    {
        .bLength = sizeof(tusb_desc_device_t),
        .bDescriptorType = TUSB_DESC_DEVICE,
        .bcdUSB = 0x0200,
        .bDeviceClass = 0xEF,
        .bDeviceSubClass = 0x02,
        .bDeviceProtocol = 0x01,
        .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,

        .idVendor = USB_VENDOR_ID,
        .idProduct = USB_PRODUCT_ID,
        .bcdDevice = 0x0100,

        .iManufacturer = 0x01,
        .iProduct = 0x02,
        .iSerialNumber = 0x03,

        .bNumConfigurations = 0x01};

// Invoked when received GET DEVICE DESCRIPTOR
// Application return pointer to descriptor
uint8_t const *tud_descriptor_device_cb(void)
{
    return (uint8_t const *)&desc_device;
}

//--------------------------------------------------------------------+
// Configuration Descriptor
//--------------------------------------------------------------------+

enum
{
    ITF_NUM_CDC = 0,
    ITF_NUM_CDC_DATA,
    ITF_NUM_MIDI,
    ITF_NUM_MIDI_STREAMING,
    ITF_NUM_TOTAL
};

#define CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN + TUD_MIDI_DESC_LEN)

#if CFG_TUSB_MCU == OPT_MCU_LPC175X_6X || CFG_TUSB_MCU == OPT_MCU_LPC177X_8X || CFG_TUSB_MCU == OPT_MCU_LPC40XX
// LPC 17xx and 40xx endpoint type (bulk/interrupt/iso) are fixed by its number
// 0 control, 1 In, 2 Bulk, 3 Iso, 4 In etc ...
#define EPNUM_CDC_NOTIF 0x01
#define EPNUM_CDC_OUT 0x02
#define EPNUM_CDC_IN 0x02
#define EPNUM_MIDI 0x03
#else
#define EPNUM_CDC_NOTIF 0x01
#define EPNUM_CDC_OUT 0x02
#define EPNUM_CDC_IN 0x03
#define EPNUM_MIDI 0x04
#endif

uint8_t const desc_fs_configuration[] = {
    // Config number, interface count, string index, total length, attribute, power in mA
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 200),

    // CDC: Interface number, string index, EP notification address and size, EP data address (out, in) and size.
    TUD_CDC_DESCRIPTOR(ITF_NUM_CDC, 4, 0x80 | EPNUM_CDC_NOTIF, 8, EPNUM_CDC_OUT, 0x80 | EPNUM_CDC_IN, 64),

    // Interface number, string index, EP Out & EP In address, EP size
    TUD_MIDI_DESCRIPTOR(ITF_NUM_MIDI, 5, EPNUM_MIDI, 0x80 | EPNUM_MIDI, 64)};

#if TUD_OPT_HIGH_SPEED
uint8_t const desc_hs_configuration[] =
    {
        // Config number, interface count, string index, total length, attribute, power in mA
        TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),

        // CDC: Interface number, string index, EP notification address and size, EP data address (out, in) and size.
        TUD_CDC_DESCRIPTOR(ITF_NUM_CDC, 4, 0x80 | EPNUM_CDC_NOTIF, 8, EPNUM_CDC_OUT, 0x80 | EPNUM_CDC_IN, 512),

        // Interface number, string index, EP Out & EP In address, EP size
        TUD_MIDI_DESCRIPTOR(ITF_NUM_MIDI, 5, EPNUM_MIDI, 0x80 | EPNUM_MIDI, 512)};
#endif

// Invoked when received GET CONFIGURATION DESCRIPTOR
// Application return pointer to descriptor
// Descriptor contents must exist long enough for transfer to complete
uint8_t const *tud_descriptor_configuration_cb(uint8_t index)
{
    (void)index; // for multiple configurations

#if TUD_OPT_HIGH_SPEED
    // Although we are highspeed, host may be fullspeed.
    return (tud_speed_get() == TUSB_SPEED_HIGH) ? desc_hs_configuration : desc_fs_configuration;
#else
    return desc_fs_configuration;
#endif
}

//--------------------------------------------------------------------+
// String Descriptors
//--------------------------------------------------------------------+

// array of pointer to string descriptors
char const *string_desc_arr[] = {
    (const char[]){0x09, 0x04}, // 0: is supported language is English (0x0409)
    "Bastl Instruments",        // 1: Manufacturer
#ifdef USB_PRODUCT_NAME
    USB_PRODUCT_NAME,          // 2: Product name
#else
    "Kastle 2",                // 2: Product name
#endif
    "UNIQUEID",                // 3: Serial, overriden with chip ID below
    "CDC Serial",              // 4: CDC Interface
#ifdef USB_PRODUCT_NAME
    USB_PRODUCT_NAME,          // 5: MIDI Interface - use same name as product
#else
    "Kastle 2",                // 5: MIDI Interface
#endif
};

static uint16_t _desc_str[32];

// Invoked when received GET STRING DESCRIPTOR request
// Application return pointer to descriptor, whose contents must exist long enough for transfer to complete
uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
    (void)langid;

    uint8_t chr_count;

    if (index == 0)
    {
        memcpy(&_desc_str[1], string_desc_arr[0], 2);
        chr_count = 1;
    }
    else
    {
        // Note: the 0xEE index string is a Microsoft OS 1.0 Descriptors.
        // https://docs.microsoft.com/en-us/windows-hardware/drivers/usbcon/microsoft-defined-usb-descriptors

        if (!(index < sizeof(string_desc_arr) / sizeof(string_desc_arr[0])))
        {
            return NULL;
        }

        // Serial Number? Use chip ID
        if (index == 3)
        {
            char unique_str[2 * PICO_UNIQUE_BOARD_ID_SIZE_BYTES + 1];
            pico_get_unique_board_id_string(unique_str, sizeof(unique_str));
            chr_count = 0;

#ifdef USB_SERIAL_PREFIX
            // Add prefix if defined (K2WB, K2FXW etc)
            // Necessary so we have unique serial if people swap firmwares (computers remeber the USB name)
            const char* prefix = USB_SERIAL_PREFIX;
            size_t prefix_len = strlen(prefix);
            if (prefix_len > 31)
            {
                prefix_len = 31;
            }
            // Convert prefix to UTF-16
            for (uint8_t i = 0; i < prefix_len; i++)
            {
                _desc_str[1 + i] = prefix[i];
            }
            chr_count = prefix_len;
#endif

            // Add unique board ID
            size_t id_len = strlen(unique_str);
            if (chr_count + id_len > 31)
            {
                id_len = 31 - chr_count;
            }
            // Convert board ID to UTF-16
            for (uint8_t i = 0; i < id_len; i++)
            {
                _desc_str[1 + chr_count + i] = unique_str[i];
            }
            chr_count += id_len;
        }
        else
        {
            // For other strings, use the predefined string array
            const char *str = string_desc_arr[index];
            // Cap at max char
            chr_count = strlen(str);
            if (chr_count > 31)
            {
                chr_count = 31;
            }
            // Convert ASCII string into UTF-16
            for (uint8_t i = 0; i < chr_count; i++)
            {
                _desc_str[1 + i] = str[i];
            }
        }
    }

    // first byte is length (including header), second byte is string type
    _desc_str[0] = (TUSB_DESC_STRING << 8) | (2 * chr_count + 2);

    return _desc_str;
}

//--------------------------------------------------------------------+
// TinyUSB Callbacks
//--------------------------------------------------------------------+

// Invoked when USB device is mounted (ready to use)
void tud_mount_cb(void)
{
    // USB device is ready
}

// Invoked when USB device is unmounted
void tud_umount_cb(void)
{
    // USB device is disconnected
}

// Invoked when CDC line state change e.g. connected/disconnected
void tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts)
{
    (void)itf; // unused
    (void)rts; // unused
    (void)dtr; // unused
    // DTR = true means USB host is connected
    // Can be used to reset device when USB is plugged in
}

// Invoked when there are MIDI data available to read
void tud_midi_rx_cb(uint8_t itf)
{
    (void)itf; // Unused parameter
    // This callback is called when MIDI data is available
    // The actual reading is handled in midi::Handler::Process()
}

// Add CDC receive function to forward characters to stdio
void tud_cdc_rx_cb(uint8_t itf)
{
    (void)itf; // unused
}