cmake_minimum_required(VERSION 3.20)

# Set cross-compilation toolchain BEFORE defining project()
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/resources/cmake/arm-none-eabi-gcc.cmake)

project(BlazeFirmware C CXX ASM)

# Global compiler flags for ARM Cortex-M
add_compile_options(
        -mcpu=cortex-m4
        -mthumb
        -mfpu=fpv4-sp-d16
        -mfloat-abi=hard
        -fdata-sections
        -ffunction-sections
        -Wall
        -Wextra
)

# Shared centralized resources
add_subdirectory(resources)

# Firmware subsystems
add_subdirectory(firmware/Front-VCU)
add_subdirectory(firmware/Rear-VCU)
add_subdirectory(firmware/Steering-Wheel)
add_subdirectory(firmware/Telemetry)