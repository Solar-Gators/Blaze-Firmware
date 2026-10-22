cmake_minimum_required(VERSION 3.20)
project(Steering-Wheel C CXX ASM)

# 1. Local Header Directories
set(NODE_INCLUDES
        Core/Inc
        Drivers/STM32U5xx_HAL_Driver/Inc
        Drivers/STM32U5xx_HAL_Driver/Inc/Legacy
        Drivers/CMSIS/Device/ST/STM32U5xx/Include
        Drivers/CMSIS/Include
        Middlewares/Third_Party/FreeRTOS/Source/include
        Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM33_NT/non_secure
        Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2
        TouchGFX/gui/include
        TouchGFX/generated/gui_generated/include
        TouchGFX/generated/fonts/include
        TouchGFX/generated/images/include
        Middlewares/ST/TouchGFX/touchgfx/framework/include
)

# 2. Collect CubeMX Generated Sources
file(GLOB_RECURSE CUBEMX_SOURCES
        "Core/Src/*.c"
        "Core/Src/*.cpp"
        "Drivers/STM32U5xx_HAL_Driver/Src/*.c"
        "Middlewares/Third_Party/FreeRTOS/Source/*.c"
        "Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM33_NT/non_secure/*.c"
        "Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2/*.c"
        "TouchGFX/gui/src/*.cpp"
        "TouchGFX/generated/*.cpp"
)

# Exclude unused FreeRTOS heap allocators (keep only heap_1.c or heap_4.c per IOC settings)
list(FILTER CUBEMX_SOURCES EXCLUDE REGEX "Middlewares/Third_Party/FreeRTOS/Source/portable/MemMang/heap_[2345]\\.c")

# Exclude HAL template sources to resolve duplicate symbol errors
list(FILTER CUBEMX_SOURCES EXCLUDE REGEX ".*_template\\.c$")

# 3. Locate Startup Assembly File
file(GLOB STARTUP_SRC
        "${CMAKE_CURRENT_SOURCE_DIR}/startup_*.s"
        "${CMAKE_CURRENT_SOURCE_DIR}/startup_*.S"
        "${CMAKE_CURRENT_SOURCE_DIR}/CubeMX/startup_*.s"
        "${CMAKE_CURRENT_SOURCE_DIR}/CubeMX/startup_*.S"
)

if(NOT CUBEMX_SOURCES AND NOT STARTUP_SRC)
    message(FATAL_ERROR "No source files or startup script found for ${PROJECT_NAME}!")
endif()

# 4. Create Executable Target
add_executable(${PROJECT_NAME}.elf ${CUBEMX_SOURCES} ${STARTUP_SRC})

target_include_directories(${PROJECT_NAME}.elf PRIVATE ${NODE_INCLUDES})

# 5. Compile Definitions
target_compile_definitions(${PROJECT_NAME}.elf PRIVATE
        STM32U5G9xx
        USE_HAL_DRIVER
)

# 6. Linker Configuration
file(GLOB LINKER_SCRIPT
        "${CMAKE_CURRENT_SOURCE_DIR}/*.ld"
        "${CMAKE_CURRENT_SOURCE_DIR}/CubeMX/*.ld"
)

target_compile_options(${PROJECT_NAME}.elf PRIVATE
        -mcpu=cortex-m33
        -mthumb
        -mfpu=fpv5-sp-d16
        -mfloat-abi=hard
)

target_link_options(${PROJECT_NAME}.elf PRIVATE
        -mcpu=cortex-m33
        -mthumb
        -mfpu=fpv5-sp-d16
        -mfloat-abi=hard
        -T${LINKER_SCRIPT}
        -Wl,-Map=${PROJECT_NAME}.map
        -Wl,--gc-sections
        -Wl,--print-memory-usage
        --specs=nano.specs
)

# 7. Link Central Drivers Library & Share Includes
target_link_libraries(${PROJECT_NAME}.elf PRIVATE SolarGators_drivers)
target_include_directories(SolarGators_drivers PRIVATE ${NODE_INCLUDES})

# 8. Post-Build Commands (.bin, .hex, size display)
add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O binary ${PROJECT_NAME}.elf ${PROJECT_NAME}.bin
        COMMAND ${CMAKE_OBJCOPY} -O ihex ${PROJECT_NAME}.elf ${PROJECT_NAME}.hex
        COMMAND ${CMAKE_SIZE} ${PROJECT_NAME}.elf
)