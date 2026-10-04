cmake_minimum_required(VERSION 3.20)
project(SolarGators_drivers C CXX)

# 1. Collect all driver source files inside resources/
file(GLOB_RECURSE DRIVER_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/Drivers/*.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/Drivers/*.cpp"
)

# 2. Prevent CMake error if resources/src/ is currently empty or contains no sources
if(NOT DRIVER_SOURCES)
    message(STATUS "No driver source files found in resources/src/. Creating dummy source file.")
    set(DUMMY_SRC "${CMAKE_CURRENT_BINARY_DIR}/drivers_placeholder.c")
    if(NOT EXISTS "${DUMMY_SRC}")
        file(WRITE "${DUMMY_SRC}" "/* Placeholder source file for SolarGators_drivers library */\n")
    endif()
    set(DRIVER_SOURCES "${DUMMY_SRC}"
            SolarGators_drivers/Inc/temp.h
            SolarGators_drivers/Src/temp.cpp)
endif()

# 3. Define the Static Central Drivers Library
add_library(SolarGators_drivers STATIC ${DRIVER_SOURCES})

target_compile_options(SolarGators_drivers PRIVATE
        -mcpu=cortex-m4
        -mthumb
        -mfpu=fpv4-sp-d16
        -mfloat-abi=hard
)

# 4. Export Include Directories
# PUBLIC headers are exposed to any node (e.g., Front-VCU) linking against this library
target_include_directories(SolarGators_drivers PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/inc"
        "${CMAKE_CURRENT_SOURCE_DIR}/Drivers"
)