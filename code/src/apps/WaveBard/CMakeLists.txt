#
# MIT License
# Copyright (c) 2025 Vaclav Mach (Bastl Instruments)
#

# App definition with included samples
create_kastle2_app(
    APP_NAME "wave-bard"
    APP_NAME_WITH_USER_DATA "wave-bard-with-samples"
    APP_USB_NAME "Kastle 2 Wave Bard"
    APP_USB_PREFIX "K2WB_"
    APP_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/AppWaveBard.cpp
    APP_USER_DATA ${CMAKE_CURRENT_SOURCE_DIR}/SAMPLES.bin
)