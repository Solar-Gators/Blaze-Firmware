cmake_minimum_required(VERSION 3.20)
project(Front-VCU C CXX ASM)

# 1. Local Header Directories
set(NODE_INCLUDES
        Core/Inc
        Drivers/STM32G4xx_HAL_Driver/Inc
        Drivers/STM32G4xx_HAL_Driver/Inc/Legacy
        Drivers/CMSIS/Device/ST/STM32G4xx/Include
        Drivers/CMSIS/Include
        Middlewares/Third_Party/FreeRTOS/Source/include
        Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F
        Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2
)

# 2. Collect CubeMX Generated Sources
file(GLOB_RECURSE CUBEMX_SOURCES
        "Core/Src/*.c"
        "Core/Src/*.cpp"
        "Drivers/STM32G4xx_HAL_Driver/Src/*.c"
        "Middlewares/Third_Party/FreeRTOS/Source/*.c"
        "Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F/*.c"
        "Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2/*.c"
)

# Exclude unused FreeRTOS heap allocators (keep only heap_4.c)
list(FILTER CUBEMX_SOURCES EXCLUDE REGEX "Middlewares/Third_Party/FreeRTOS/Source/portable/MemMang/heap_[1235]\\.c")

# Exclude HAL template sources to resolve duplicate symbol errors
list(FILTER CUBEMX_SOURCES EXCLUDE REGEX ".*_template\\.c$")

# 3. Locate Startup Assembly File (in CubeMX folder)
set(STARTUP_SRC "${CMAKE_CURRENT_SOURCE_DIR}/CubeMX/startup_stm32g483xx.s")

if(NOT EXISTS "${STARTUP_SRC}")
    file(GLOB STARTUP_SRC
            "${CMAKE_CURRENT_SOURCE_DIR}/CubeMX/startup_*.s"
            "${CMAKE_CURRENT_SOURCE_DIR}/CubeMX/startup_*.S"
    )
endif()

if(NOT CUBEMX_SOURCES AND NOT STARTUP_SRC)
    message(FATAL_ERROR "No source files or startup script found!")
endif()

# 4. Create Executable Target
add_executable(${PROJECT_NAME}.elf ${CUBEMX_SOURCES} ${STARTUP_SRC})

target_include_directories(${PROJECT_NAME}.elf PRIVATE ${NODE_INCLUDES})

# 5. Compile Definitions
target_compile_definitions(${PROJECT_NAME}.elf PRIVATE
        STM32G483xx
        USE_HAL_DRIVER
)

target_compile_definitions(SolarGators_drivers PRIVATE
        STM32G483xx
        USE_HAL_DRIVER
)

# 6. Linker Configuration & Target Link Options (in CubeMX folder)
set(LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/CubeMX/STM32G483XX_FLASH.ld")

if(NOT EXISTS "${LINKER_SCRIPT}")
    file(GLOB LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/CubeMX/*.ld")
endif()

target_compile_options(${PROJECT_NAME}.elf PRIVATE
        -mcpu=cortex-m4
        -mthumb
        -mfpu=fpv4-sp-d16
        -mfloat-abi=hard
)

target_link_options(${PROJECT_NAME}.elf PRIVATE
        -mcpu=cortex-m4
        -mthumb
        -mfpu=fpv4-sp-d16
        -mfloat-abi=hard
        -T${LINKER_SCRIPT}
        -Wl,-Map=${PROJECT_NAME}.map
        -Wl,--gc-sections
        --specs=nano.specs
)

# 7. Link Central Drivers Library & Share CubeMX Includes
target_link_libraries(${PROJECT_NAME}.elf PRIVATE SolarGators_drivers)
target_include_directories(SolarGators_drivers PRIVATE ${NODE_INCLUDES})

# 8. Post-Build Commands (.bin, .hex, size display)
add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O binary ${PROJECT_NAME}.elf ${PROJECT_NAME}.bin
        COMMAND ${CMAKE_OBJCOPY} -O ihex ${PROJECT_NAME}.elf ${PROJECT_NAME}.hex
        COMMAND ${CMAKE_SIZE} ${PROJECT_NAME}.elf
)