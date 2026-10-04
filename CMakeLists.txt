cmake_minimum_required(VERSION 3.20)

# Set cross-compilation toolchain BEFORE defining project()
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/resources/cmake/arm-none-eabi-gcc.cmake)

project(BlazeFirmware C CXX ASM)

# Extract Git Commit Hash
execute_process(
        COMMAND git rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
)

if(NOT GIT_HASH)
    set(GIT_HASH "unknown")
endif()

add_compile_definitions(GIT_COMMIT_HASH="${GIT_HASH}")

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
        -Wshadow
        -Wdouble-promotion
        -Wformat=2
)

add_link_options(
        -mcpu=cortex-m4
        -mthumb
        -mfpu=fpv4-sp-d16
        -mfloat-abi=hard
)

# Clang-Format Target
find_program(CLANG_FORMAT_BIN clang-format)
if(CLANG_FORMAT_BIN)
    file(GLOB_RECURSE ALL_SOURCE_FILES
            "${CMAKE_CURRENT_SOURCE_DIR}/firmware/*.c"
            "${CMAKE_CURRENT_SOURCE_DIR}/firmware/*.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/firmware/*.h"
            "${CMAKE_CURRENT_SOURCE_DIR}/firmware/*.hpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/resources/*.c"
            "${CMAKE_CURRENT_SOURCE_DIR}/resources/*.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/resources/*.h"
            "${CMAKE_CURRENT_SOURCE_DIR}/resources/*.hpp"
    )
    add_custom_target(format
            COMMAND ${CLANG_FORMAT_BIN} -i -style=file ${ALL_SOURCE_FILES}
            COMMENT "Formatting source files with clang-format"
    )
endif()

# Shared centralized resources
add_subdirectory(resources)

# Firmware subsystems
add_subdirectory(firmware/Front-VCU)
#add_subdirectory(firmware/Rear-VCU)
#add_subdirectory(firmware/Steering-Wheel)
#add_subdirectory(firmware/Telemetry)