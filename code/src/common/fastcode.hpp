/*
MIT License

Copyright (c) 2025 Vaclav Mach (Bastl Instruments)

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

#pragma once

/**
 * @file fastcode.hpp
 * @ingroup core
 * @author Vaclav Mach (Bastl Instruments)
 * @date 2025-06-25
 * @brief Since QSPI code execution is slow, we need to store performance critical code in RAM.
 *
 * Code marked with FASTCODE is placed in a separate section in the compiled binary.
 * That section is copied to RAM at startup by calling copy_fastcode_to_ram() function and executed from there.
 */

/**
 * @brief Enables "fast code" functionality.
 */
#define FASTCODE_ENABLED

// Actual implementation

#ifdef FASTCODE_ENABLED

/**
 * @brief Marks a function to be placed in the .fastcode section.
 * This code is copied to RAM on startup, so it's faster to execute.
 */
#define FASTCODE __attribute__((section(".fastcode"))) __attribute__((noinline))

/**
 * @brief Copies the .fastcode section to RAM. Call this right at the beginning of the app.
 */
void copy_fastcode_to_ram(void);

#else

/**
 * @brief Placeholder if fastcode is disabled.
 */
#define FASTCODE

#endif
