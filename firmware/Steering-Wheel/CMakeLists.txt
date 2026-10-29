cmake_minimum_required(VERSION 3.20)
project(Steering-Wheel LANGUAGES C CXX ASM)

set(NODE_INCLUDES
        Core/Inc
        user/Inc

        Drivers/STM32U5xx_HAL_Driver/Inc
        Drivers/STM32U5xx_HAL_Driver/Inc/Legacy
        Drivers/CMSIS/Device/ST/STM32U5xx/Include
        Drivers/CMSIS/Include
        Drivers/CMSIS/RTOS2/Include

        Middlewares/Third_Party/FreeRTOS/Source/include
        Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM33_NTZ/non_secure
        Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2

        TouchGFX/App
        TouchGFX/gui/include
        TouchGFX/generated/gui_generated/include
        TouchGFX/generated/fonts/include
        TouchGFX/generated/images/include
        TouchGFX/generated/texts/include
        TouchGFX/target
        TouchGFX/target/generated

        Middlewares/ST/touchgfx/framework/include
        Middlewares/ST/touchgfx_components/gpu2d/TouchGFXNema_NemaPVG_m33_r01/include
        Middlewares/ST/touchgfx_components/gpu2d/NemaGFX_NemaPVG_m33_r01/include
)

file(GLOB_RECURSE CUBEMX_SOURCES CONFIGURE_DEPENDS
        "Core/Src/*.c"
        "Core/Src/*.cpp"
        "user/Src/*.c"
        "user/Src/*.cpp"
        "Drivers/STM32U5xx_HAL_Driver/Src/*.c"
        "Middlewares/Third_Party/FreeRTOS/Source/*.c"
        "TouchGFX/App/*.c"
        "TouchGFX/App/*.cpp"
        "TouchGFX/gui/src/*.cpp"
        "TouchGFX/generated/*.c"
        "TouchGFX/generated/*.cpp"
        "TouchGFX/target/*.c"
        "TouchGFX/target/*.cpp"
)

# Preserve the selected Cortex-M33 NTZ port and heap_1 allocator.
set(FILTERED_SOURCES "")

foreach(source IN LISTS CUBEMX_SOURCES)
    if(source MATCHES "FreeRTOS/Source/portable/")
        if(source MATCHES "portable/GCC/ARM_CM33_NTZ/non_secure/"
                OR source MATCHES "portable/MemMang/heap_1\\.c$")
            list(APPEND FILTERED_SOURCES "${source}")
        endif()
    else()
        list(APPEND FILTERED_SOURCES "${source}")
    endif()
endforeach()

set(CUBEMX_SOURCES ${FILTERED_SOURCES})

list(FILTER CUBEMX_SOURCES EXCLUDE REGEX "TouchGFX/generated/simulator/")
list(FILTER CUBEMX_SOURCES EXCLUDE REGEX "TouchGFX/simulator/")
list(FILTER CUBEMX_SOURCES EXCLUDE REGEX ".*_template\\.c$")
list(REMOVE_DUPLICATES CUBEMX_SOURCES)

set(STARTUP_SRC
        "${CMAKE_CURRENT_SOURCE_DIR}/CubeMX/startup_stm32u5g9xx.s"
        ../../resources/SolarGators_drivers/Inc/BQ76952_Registers.h
)

if(NOT EXISTS "${STARTUP_SRC}")
    file(GLOB STARTUP_CANDIDATES CONFIGURE_DEPENDS
            "${CMAKE_CURRENT_SOURCE_DIR}/CubeMX/startup_*.s"
            "${CMAKE_CURRENT_SOURCE_DIR}/CubeMX/startup_*.S"
            "${CMAKE_CURRENT_SOURCE_DIR}/startup_*.s"
            "${CMAKE_CURRENT_SOURCE_DIR}/startup_*.S"
    )

    list(LENGTH STARTUP_CANDIDATES STARTUP_COUNT)

    if(NOT STARTUP_COUNT EQUAL 1)
        message(FATAL_ERROR "Expected one startup assembly file for ${PROJECT_NAME}")
    endif()

    list(GET STARTUP_CANDIDATES 0 STARTUP_SRC)
endif()

if(NOT CUBEMX_SOURCES)
    message(FATAL_ERROR "No application sources found for ${PROJECT_NAME}")
endif()

# Matching TouchGFX / NemaPVG libraries.
set(ST_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/Middlewares/ST")

set(TOUCHGFX_LIB
        "${ST_ROOT}/touchgfx/lib/core/cortex_m33/gcc/libtouchgfx-float-abi-hard.a"
)

set(TOUCHGFXNEMA_LIB
        "${ST_ROOT}/touchgfx_components/gpu2d/TouchGFXNema_NemaPVG_m33_r01/lib/cortex_m33/gcc/libtouchgfxnema-float-abi-hard.a"
)

set(NEMAGFX_LIB
        "${ST_ROOT}/touchgfx_components/gpu2d/NemaGFX_NemaPVG_m33_r01/lib/core/cortex_m33/gcc/libnemagfx-float-abi-hard.a"
)

foreach(library IN ITEMS TOUCHGFX_LIB TOUCHGFXNEMA_LIB NEMAGFX_LIB)
    if(NOT EXISTS "${${library}}")
        message(FATAL_ERROR "Missing library: ${${library}}")
    endif()
endforeach()

add_executable(${PROJECT_NAME}.elf
        ${CUBEMX_SOURCES}
        ${STARTUP_SRC}
)

target_include_directories(${PROJECT_NAME}.elf PRIVATE ${NODE_INCLUDES})

target_compile_definitions(${PROJECT_NAME}.elf PRIVATE
        STM32U5G9xx
        USE_HAL_DRIVER
)

target_compile_options(${PROJECT_NAME}.elf PRIVATE
        -mcpu=cortex-m33
        -mthumb
        -mfpu=fpv5-sp-d16
        -mfloat-abi=hard
        $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>
        $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>
        $<$<COMPILE_LANGUAGE:CXX>:-fno-threadsafe-statics>
)

# Creates a separate library with this board's U5/M33 settings.
add_solargators_drivers(${PROJECT_NAME}.elf)

target_link_libraries(${PROJECT_NAME}.elf PRIVATE
        -Wl,--start-group
        "${TOUCHGFX_LIB}"
        "${TOUCHGFXNEMA_LIB}"
        "${NEMAGFX_LIB}"
        -Wl,--end-group
)

set(LINKER_SCRIPT
        "${CMAKE_CURRENT_SOURCE_DIR}/CubeMX/STM32U5G9xx_FLASH.ld"
)

if(NOT EXISTS "${LINKER_SCRIPT}")
    file(GLOB LINKER_CANDIDATES CONFIGURE_DEPENDS
            "${CMAKE_CURRENT_SOURCE_DIR}/CubeMX/*FLASH*.ld"
            "${CMAKE_CURRENT_SOURCE_DIR}/*FLASH*.ld"
    )

    list(LENGTH LINKER_CANDIDATES LINKER_COUNT)

    if(NOT LINKER_COUNT EQUAL 1)
        message(FATAL_ERROR "Expected one FLASH linker script for ${PROJECT_NAME}")
    endif()

    list(GET LINKER_CANDIDATES 0 LINKER_SCRIPT)
endif()

target_link_options(${PROJECT_NAME}.elf PRIVATE
        -mcpu=cortex-m33
        -mthumb
        -mfpu=fpv5-sp-d16
        -mfloat-abi=hard
        "-T${LINKER_SCRIPT}"
        "-Wl,-Map=${PROJECT_NAME}.map"
        -Wl,--gc-sections
        -Wl,--print-memory-usage
        --specs=nano.specs
)

# nosys.specs remains supplied by the existing toolchain file.
add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O binary
        "$<TARGET_FILE:${PROJECT_NAME}.elf>"
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.bin"
        COMMAND ${CMAKE_OBJCOPY} -O ihex
        "$<TARGET_FILE:${PROJECT_NAME}.elf>"
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.hex"
        COMMAND ${CMAKE_SIZE} "$<TARGET_FILE:${PROJECT_NAME}.elf>"
        COMMENT "Generating BIN, HEX, and memory usage report"
        VERBATIM
)