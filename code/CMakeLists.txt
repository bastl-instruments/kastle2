#
# MIT License
# Copyright (c) 2025 Vaclav Mach (Bastl Instruments)
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#

# Kastle 2 CMake RP2040-based project

cmake_minimum_required(VERSION 3.13)

# Set C/C++ standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 23)

# Custom board definition file for 8 MB flash
set(PICO_BOARD_HEADER_DIRS ${CMAKE_SOURCE_DIR}/board)
set(PICO_BOARD "kastle2_board")
set(PICO_LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/board/memmap_kastle2.ld)
set(PICO_PLATFORM rp2040)

# Optimization level for all builds
# Kastle 2 code relies on aggressive optimizations to work properly (you can try -O2 or -Os, but things may break)
set(KASTLE2_OLEVEL "-O3")  

# RP2040 UF2 family identifier (used when appending user data firmware)
set(RP2040_UF2_FAMILY_ID "0xE48BFF56") 

# User data start address (starts at 512 KB, length 7.5 MB)
set(KASTLE2_USER_DATA_START 0x10080000)

# Set build type (Debug or Release)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# Set compiler flags for Debug and Release configurations
# Note: These will be applied selectively to avoid warnings in pico-sdk
set(KASTLE2_STRICT_DEBUG_FLAGS "${KASTLE2_OLEVEL}" "-g3" "-DDEBUG" "-Wall" "-Wshadow" "-Wdeprecated" "-Wdeprecated-declarations" "-Wpedantic" "-Wextra" "-Wno-switch" "-Wno-system-headers")
set(KASTLE2_STRICT_RELEASE_FLAGS "${KASTLE2_OLEVEL}" "-Wall" "-Wshadow" "-Wdeprecated" "-Wdeprecated-declarations" "-Wpedantic" "-Wextra" "-Wno-switch" "-Wno-system-headers")

# Apply relaxed flags globally (for pico-sdk compatibility)
set(CMAKE_C_FLAGS_DEBUG "${KASTLE2_OLEVEL} -g3 -DDEBUG")
set(CMAKE_C_FLAGS_RELEASE "${KASTLE2_OLEVEL}")
set(CMAKE_CXX_FLAGS_DEBUG "${KASTLE2_OLEVEL} -g3 -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "${KASTLE2_OLEVEL}")

# Add linker flag for memory usage
string(APPEND CMAKE_EXE_LINKER_FLAGS "-Wl,--print-memory-usage")

# Get Raspberry Pi Pico SDK
include(pico_sdk_import.cmake)

# Set project name and languages
project(kastle2 C CXX)

# Executables for ARM objcopy and Python
set(ARM_OBJCOPY_BIN "arm-none-eabi-objcopy")  
set(PYTHON_BIN "python3") 

# Set the Core library name
set(KASTLE2_CORE_LIBRARY "kastle2_core")

# Initialize the Raspberry Pi Pico SDK
pico_sdk_init()

# Pico SDK system includes
# They are specified separately to suppress compiler warnings (Kastle 2 code uses strict flags)
set(PICO_SDK_SYSTEM_INCLUDES
    ${PICO_SDK_PATH}/src/common/pico_stdlib/include
    ${PICO_SDK_PATH}/src/common/pico_base/include
    ${PICO_SDK_PATH}/src/rp2_common
    ${PICO_SDK_PATH}/src/common
    ${PICO_SDK_PATH}/lib/tinyusb/src
)

# Set up directories
SET(SRC ${CMAKE_CURRENT_SOURCE_DIR}/src)
SET(LIBRARIES ${CMAKE_CURRENT_SOURCE_DIR}/lib)
SET(SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/scripts)

# Function to apply Kastle 2 strict compiler flags to source files
function(apply_compiler_flags TARGET_NAME SOURCE_FILES)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set_source_files_properties(${SOURCE_FILES} PROPERTIES COMPILE_OPTIONS "${KASTLE2_STRICT_DEBUG_FLAGS}")
    else()
        set_source_files_properties(${SOURCE_FILES} PROPERTIES COMPILE_OPTIONS "${KASTLE2_STRICT_RELEASE_FLAGS}")
    endif()
endfunction()

# Include directories
include_directories(
    ${SRC}
    ${LIBRARIES}
)

# Source files
# When creating a new cpp file in "src/common", add it here
# Add app-specific sources in each app's CMakeLists.txt
SET(KASTLE2_COMMON_SOURCES 
    ${SRC}/common/core/Base.cpp
    ${SRC}/common/core/Kastle2.cpp
    ${SRC}/common/core/clocks/InternalClockSource.cpp
    ${SRC}/common/core/clocks/ExternalClockSource.cpp
    ${SRC}/common/core/clocks/MidiClockSource.cpp
    ${SRC}/common/core/midi/Handler.cpp
    ${SRC}/common/core/midi/Message.cpp
    ${SRC}/common/core/Clock.cpp
    ${SRC}/common/core/Hardware.cpp
    ${SRC}/common/core/Memory.cpp
    ${SRC}/common/controls/FancyPot.cpp
    ${SRC}/common/controls/FancyMode.cpp
    ${SRC}/common/debug/UsbSerial.cpp
    ${SRC}/common/debug/SEGGER_RTT.c
    ${SRC}/common/fastcode.cpp
    ${SRC}/common/peripherals/NAU88C22.cpp
    ${SRC}/common/peripherals/AT24C.cpp
    ${SRC}/common/peripherals/WS2812.cpp
    ${SRC}/common/testmode/TestMode.cpp
    ${SRC}/common/testmode/TestEntry.cpp
    ${SRC}/common/testmode/version_samples.cpp
    ${SRC}/common/dsp/synthesis/Oscillator.cpp
    ${SRC}/common/dsp/synthesis/OscillatorQ15.cpp
    ${SRC}/common/dsp/synthesis/MultiOscillator.cpp
    ${SRC}/common/dsp/synthesis/Fm2.cpp
    ${SRC}/common/dsp/control/AdsrEnv.cpp
    ${SRC}/common/dsp/control/Lfo.cpp
    ${SRC}/common/dsp/control/EnvelopeFollower.cpp
    ${SRC}/common/dsp/filters/Svf.cpp
    ${SRC}/common/dsp/filters/SvfStereo.cpp
    ${SRC}/common/dsp/filters/DjFilter.cpp
    ${SRC}/common/dsp/filters/DjFilterStereo.cpp
    ${SRC}/common/dsp/math/Fft.cpp
    ${SRC}/common/dsp/utility/Quantizer.cpp
    ${SRC}/common/dsp/utility/SignalCorrelator.cpp
    ${SRC}/common/dsp/utility/Sequencer.cpp
    ${SRC}/common/dsp/utility/Portamento.cpp
    ${SRC}/common/dsp/utility/KastleRungler.cpp
    ${SRC}/common/dsp/effects/HardClipper.cpp
    ${SRC}/common/dsp/effects/SoftClipper.cpp
    ${SRC}/common/dsp/effects/StereoDelay.cpp
    ${SRC}/common/dsp/effects/CorrectingTrackAndHold.cpp
    ${SRC}/common/dsp/control/BeatDetector.cpp
    ${SRC}/usb_descriptors.c
    ${LIBRARIES}/I2S.cpp
)

# Libraries
SET(KASTLE2_COMMON_LIBRARIES
    pico_stdlib
    pico_multicore
    pico_unique_id
    hardware_adc
    hardware_flash
    hardware_pio
    hardware_dma
    hardware_pwm
    hardware_i2c
    hardware_clocks
    hardware_watchdog
    hardware_interp
    tinyusb_device
)

# Kastle 2 Core Library
add_library(${KASTLE2_CORE_LIBRARY} ${KASTLE2_COMMON_SOURCES})

# Make PIO headers
pico_generate_pio_header(${KASTLE2_CORE_LIBRARY} ${LIBRARIES}/I2S.pio)
pico_generate_pio_header(${KASTLE2_CORE_LIBRARY} ${SRC}/common/peripherals/WS2812.pio)

# Include directories for core
target_include_directories(${KASTLE2_CORE_LIBRARY} PUBLIC ${SRC}/common)

# Apply strict compiler flags to Kastle 2 common source files
apply_compiler_flags(${KASTLE2_CORE_LIBRARY} "${KASTLE2_COMMON_SOURCES}")

# Mark pico-sdk includes as system includes to suppress warnings (especially -Wpedantic)
target_include_directories(${KASTLE2_CORE_LIBRARY} SYSTEM PRIVATE ${PICO_SDK_SYSTEM_INCLUDES})

# Link libraries
target_link_libraries(${KASTLE2_CORE_LIBRARY} PRIVATE 
    ${KASTLE2_COMMON_LIBRARIES}
)

# Specify the output directory for all executables
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/output)

# Function to create JLink script from hex file
function(create_jlink_script_from_hex HEX_FILE_PATH SCRIPT_PATH)
    # Create a custom command to generate the .jlink file
    add_custom_command(
        OUTPUT ${SCRIPT_PATH}
        COMMAND ${CMAKE_COMMAND} -E echo "loadfile \"${HEX_FILE_PATH}\"" > ${SCRIPT_PATH}
        COMMAND ${CMAKE_COMMAND} -E echo "r" >> ${SCRIPT_PATH}
        COMMAND ${CMAKE_COMMAND} -E echo "g" >> ${SCRIPT_PATH}
        COMMAND ${CMAKE_COMMAND} -E echo "RSetType 2" >> ${SCRIPT_PATH}
        COMMAND ${CMAKE_COMMAND} -E echo "R" >> ${SCRIPT_PATH}
        COMMAND ${CMAKE_COMMAND} -E echo "exit" >> ${SCRIPT_PATH}
        COMMENT "Generating JLink script ${SCRIPT_PATH}"
    )
endfunction()

# Function for generating output files (UF2, HEX, BIN, JLink)
function(generate_output_files APP_NAME)
    # Set the linker script
    set_target_properties(${APP_NAME} PROPERTIES PICO_TARGET_LINKER_SCRIPT ${PICO_LINKER_SCRIPT})

    # Set the filename
    set(FILENAME "${PROJECT_NAME}-${APP_NAME}")

    # Set the output name
    set_target_properties(${APP_NAME} PROPERTIES OUTPUT_NAME ${FILENAME})
    
    # Generate extra outputs (UF2, HEX, BIN files)
    pico_add_extra_outputs(${APP_NAME})

    # JLink script for flashing
    set(SCRIPT_PATH "${CMAKE_BINARY_DIR}/${PROJECT_NAME}-${APP_NAME}-upload.jlink")
    set(HEX_PATH "${CMAKE_BINARY_DIR}/output/${PROJECT_NAME}-${APP_NAME}.hex")
    
    # Use the helper function to create the JLink script
    create_jlink_script_from_hex(${HEX_PATH} ${SCRIPT_PATH})

    # Create a custom target to trigger the custom command
    add_custom_target(${APP_NAME}_upload_script DEPENDS ${SCRIPT_PATH})
    
    # Make sure the target depends on the generated script
    add_dependencies(${APP_NAME} ${APP_NAME}_upload_script)

    # Manually move the generated UF2, BIN, HEX files to the output directory
    if(${PICO_SDK_VERSION_STRING} VERSION_LESS "2")
        # For Pico SDK version < 2, add all copy commands
        add_custom_command(TARGET ${APP_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy ${FILENAME}.bin ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/
            COMMAND ${CMAKE_COMMAND} -E copy ${FILENAME}.hex ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/
            COMMAND ${CMAKE_COMMAND} -E copy ${FILENAME}.uf2 ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/
        )
    else()
        # For Pico SDK version >= 2, only copy UF2 file
        add_custom_command(TARGET ${APP_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy ${FILENAME}.uf2 ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/
        )
    endif()
endfunction()

# Function for creating data-merged variant of an app
function(create_data_merged_target APP_NAME APP_NAME_WITH_USER_DATA USER_DATA_SRC_FILE)
    set(APP_FILENAME "${PROJECT_NAME}-${APP_NAME}")
    set(APP_WITH_DATA_FILENAME "${PROJECT_NAME}-${APP_NAME_WITH_USER_DATA}")
    
    set(USER_DATA_FILENAME ${USER_DATA_SRC_FILE})
    set(FW_HEX_FILE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${APP_FILENAME}.hex)
    set(USER_DATA_HEX_FILE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${APP_WITH_DATA_FILENAME}-only-user-data.hex)
    set(MERGED_HEX_FILE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${APP_WITH_DATA_FILENAME}.hex)
    set(MERGED_UF2_FILE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${APP_WITH_DATA_FILENAME}.uf2)
    
if(EXISTS ${USER_DATA_SRC_FILE})
        # Generate JLink script for the merged target
        set(SCRIPT_PATH "${CMAKE_BINARY_DIR}/${PROJECT_NAME}-${APP_NAME_WITH_USER_DATA}-upload.jlink")
        create_jlink_script_from_hex(${MERGED_HEX_FILE} ${SCRIPT_PATH})
        
        add_custom_target(${APP_NAME_WITH_USER_DATA}
            COMMENT "Merging ${USER_DATA_FILENAME} with ${APP_NAME}"
            # Make HEX from the user data
            COMMAND ${ARM_OBJCOPY_BIN} -I binary -O ihex --change-addresses ${KASTLE2_USER_DATA_START} ${USER_DATA_SRC_FILE} ${USER_DATA_HEX_FILE}
            # Custom merge script which fills the gap between the firmware and user data with 0x00
            COMMAND ${PYTHON_BIN} ${SCRIPTS}/srec_cat_fill.py ${FW_HEX_FILE} ${USER_DATA_HEX_FILE} -o ${MERGED_HEX_FILE}
            # Converting merged hex file to UF2
            COMMAND ${CMAKE_COMMAND} -E echo "Converting merged HEX to UF2..."
            COMMAND ${PYTHON_BIN} ${SCRIPTS}/uf2/uf2conv.py -f ${RP2040_UF2_FAMILY_ID} -c -o ${MERGED_UF2_FILE} ${MERGED_HEX_FILE}
            # Remove temporary user data hex file
            COMMAND ${CMAKE_COMMAND} -E remove ${USER_DATA_HEX_FILE}
            DEPENDS ${APP_NAME} ${SCRIPT_PATH}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        )
    else()
        add_custom_target(${APP_NAME_WITH_USER_DATA}
            COMMENT "${USER_DATA_FILENAME} not found, skipping data merge for ${APP_NAME}"
            DEPENDS ${APP_NAME}
        )
    endif()
endfunction()

# Function to create a Kastle 2 app with common boilerplate
function(create_kastle2_app)
    # Parse function arguments
    set(options )
    set(oneValueArgs APP_NAME APP_USB_NAME APP_USB_PREFIX APP_NAME_WITH_USER_DATA APP_USER_DATA)
    set(multiValueArgs APP_SOURCES)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # Validate required arguments
    if(NOT ARG_APP_NAME)
        message(FATAL_ERROR "APP_NAME is required")
    endif()
    if(NOT ARG_APP_SOURCES)
        message(FATAL_ERROR "APP_SOURCES is required")
    endif()

    # Auto-generate WITH_DATA variant name if APP_USER_DATA is specified but name not provided
    if(ARG_APP_USER_DATA AND NOT ARG_APP_NAME_WITH_USER_DATA)
        set(ARG_APP_NAME_WITH_USER_DATA "${ARG_APP_NAME}-with-data")
    endif()

    # Define the main executable
    add_executable(${ARG_APP_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)

    # Add the sources
    target_sources(${ARG_APP_NAME} PRIVATE
        ${KASTLE2_COMMON_SOURCES}
        ${ARG_APP_SOURCES}
    )

    # Apply strict compiler flags only to app source files
    apply_compiler_flags(${ARG_APP_NAME} "${ARG_APP_SOURCES};${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")

    # Mark pico-sdk includes as system includes to suppress warnings
    target_include_directories(${ARG_APP_NAME} SYSTEM PRIVATE ${PICO_SDK_SYSTEM_INCLUDES})

    # Add product name define if provided
    if(ARG_APP_USB_NAME)
        target_compile_definitions(${ARG_APP_NAME} PRIVATE "USB_PRODUCT_NAME=\"${ARG_APP_USB_NAME}\"")
    endif()
    if(ARG_APP_USB_PREFIX)
        target_compile_definitions(${ARG_APP_NAME} PRIVATE "USB_SERIAL_PREFIX=\"${ARG_APP_USB_PREFIX}\"")
    endif()

    # Link the libraries
    target_link_libraries(${ARG_APP_NAME} PRIVATE
        ${KASTLE2_COMMON_LIBRARIES}
        ${KASTLE2_CORE_LIBRARY}
    )

    # Disable stdio through UART and USB - we need those for other things
    pico_enable_stdio_uart(${ARG_APP_NAME} 0)
    pico_enable_stdio_usb(${ARG_APP_NAME} 0)

    # Set custom properties for the executable
    pico_set_program_name(${ARG_APP_NAME} ${ARG_APP_NAME})

    # Generate standard output files (UF2, HEX, BIN, J-Link script)
    generate_output_files(${ARG_APP_NAME})

    # Create data-merged variant if user data file is specified
    if(ARG_APP_USER_DATA)
        create_data_merged_target(${ARG_APP_NAME} ${ARG_APP_NAME_WITH_USER_DATA} ${ARG_APP_USER_DATA})
    endif()
endfunction()

# Get all subdirectories within SRC/apps
file(GLOB APP_DIRS "${SRC}/apps/*")

# Loop over each subdirectory
foreach(APP_DIR ${APP_DIRS})
    # Check if the directory contains a CMakeLists.txt file
    if(EXISTS "${APP_DIR}/CMakeLists.txt")
        # Add the subdirectory
        message(STATUS "Adding subdirectory: ${APP_DIR}")
        add_subdirectory(${APP_DIR})
    endif()
endforeach()