cmake_minimum_required(VERSION 3.20)
project(Front-VCU C CXX ASM)

set(NODE_INCLUDES
        Core/Inc
        user/Inc
        Drivers/STM32G4xx_HAL_Driver/Inc
        Drivers/STM32G4xx_HAL_Driver/Inc/Legacy
        Drivers/CMSIS/Device/ST/STM32G4xx/Include
        Drivers/CMSIS/Include
        Middlewares/Third_Party/FreeRTOS/Source/include
        Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F
        Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2
)

file(GLOB_RECURSE CUBEMX_SOURCES CONFIGURE_DEPENDS
        "Core/Src/*.c"
        "Core/Src/*.cpp"
        "user/Src/*.c"
        "user/Src/*.cpp"
        "Drivers/STM32G4xx_HAL_Driver/Src/*.c"
        "Middlewares/Third_Party/FreeRTOS/Source/*.c"
)

list(FILTER CUBEMX_SOURCES EXCLUDE REGEX
        "Middlewares/Third_Party/FreeRTOS/Source/portable/MemMang/heap_[1235]\\.c"
)
list(FILTER CUBEMX_SOURCES EXCLUDE REGEX ".*_template\\.c$")

set(STARTUP_SRC
        "${CMAKE_CURRENT_SOURCE_DIR}/CubeMX/startup_stm32g483xx.s"
)

if(NOT EXISTS "${STARTUP_SRC}")
    file(GLOB STARTUP_SRC
            "${CMAKE_CURRENT_SOURCE_DIR}/CubeMX/startup_*.s"
            "${CMAKE_CURRENT_SOURCE_DIR}/CubeMX/startup_*.S"
    )
endif()

if(NOT CUBEMX_SOURCES OR NOT STARTUP_SRC)
    message(FATAL_ERROR "Missing application sources or startup assembly for ${PROJECT_NAME}")
endif()

add_executable(${PROJECT_NAME}.elf ${CUBEMX_SOURCES} ${STARTUP_SRC})

target_include_directories(${PROJECT_NAME}.elf PRIVATE ${NODE_INCLUDES})

target_compile_definitions(${PROJECT_NAME}.elf PRIVATE
        STM32G483xx
        USE_HAL_DRIVER
)

target_compile_options(${PROJECT_NAME}.elf PRIVATE
        -mcpu=cortex-m4
        -mthumb
        -mfpu=fpv4-sp-d16
        -mfloat-abi=hard
)

# Preserve the currently selected filename.
set(LINKER_SCRIPT
        "${CMAKE_CURRENT_SOURCE_DIR}/CubeMX/STM32G483XX_FLASH.ld"
)

if(NOT EXISTS "${LINKER_SCRIPT}")
    file(GLOB LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/CubeMX/*.ld")
endif()

list(LENGTH LINKER_SCRIPT LINKER_SCRIPT_COUNT)
if(NOT LINKER_SCRIPT_COUNT EQUAL 1)
    message(FATAL_ERROR "Expected one linker script for ${PROJECT_NAME}")
endif()

target_link_options(${PROJECT_NAME}.elf PRIVATE
        -mcpu=cortex-m4
        -mthumb
        -mfpu=fpv4-sp-d16
        -mfloat-abi=hard
        "-T${LINKER_SCRIPT}"
        "-Wl,-Map=${PROJECT_NAME}.map"
        -Wl,--gc-sections
        -Wl,--print-memory-usage
        --specs=nano.specs
)

add_solargators_drivers(${PROJECT_NAME}.elf)

add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O binary
        "$<TARGET_FILE:${PROJECT_NAME}.elf>"
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.bin"
        COMMAND ${CMAKE_OBJCOPY} -O ihex
        "$<TARGET_FILE:${PROJECT_NAME}.elf>"
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.hex"
        COMMAND ${CMAKE_SIZE} "$<TARGET_FILE:${PROJECT_NAME}.elf>"
        VERBATIM
)