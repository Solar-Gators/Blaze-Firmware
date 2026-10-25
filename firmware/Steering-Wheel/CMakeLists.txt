cmake_minimum_required(VERSION 3.20)
project(Steering-Wheel C CXX ASM)

# 1. Local Header Directories
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
        Middlewares/ST/TouchGFX/touchgfx/framework/include
        Middlewares/ST/TouchGFX/driver/include
)

# 2. Collect CubeMX Generated, User & TouchGFX Sources
file(GLOB_RECURSE CUBEMX_SOURCES
        "Core/Src/*.c"
        "Core/Src/*.cpp"
        "user/Src/*.c"
        "user/Src/*.cpp"
        "Drivers/STM32U5xx_HAL_Driver/Src/*.c"
        "Middlewares/Third_Party/FreeRTOS/Source/*.c"
        "Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM33_NTZ/non_secure/*.c"
        "Middlewares/Third_Party/FreeRTOS/Source/portable/MemMang/*.c"
        "Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2/*.c"
        "TouchGFX/App/*.c"
        "TouchGFX/App/*.cpp"
        "TouchGFX/gui/src/*.cpp"
        "TouchGFX/generated/*.cpp"
        "TouchGFX/target/*.c"
        "TouchGFX/target/*.cpp"
)

# Exclude unused FreeRTOS heap allocators (keep only heap_1.c per X-CUBE-FREERTOS settings)
list(FILTER CUBEMX_SOURCES EXCLUDE REGEX "Middlewares/Third_Party/FreeRTOS/Source/portable/MemMang/heap_[2345]\\.c")

# Exclude HAL template sources to resolve duplicate symbol errors
list(FILTER CUBEMX_SOURCES EXCLUDE REGEX ".*_template\\.c$")

# 3. Locate Startup Assembly File
set(STARTUP_SRC "${CMAKE_CURRENT_SOURCE_DIR}/CubeMX/startup_stm32u5g9xx.s")

if(NOT EXISTS "${STARTUP_SRC}")
    file(GLOB STARTUP_SRC
            "${CMAKE_CURRENT_SOURCE_DIR}/CubeMX/startup_*.s"
            "${CMAKE_CURRENT_SOURCE_DIR}/CubeMX/startup_*.S"
            "${CMAKE_CURRENT_SOURCE_DIR}/startup_*.s"
            "${CMAKE_CURRENT_SOURCE_DIR}/startup_*.S"
    )
endif()

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

target_compile_definitions(SolarGators_drivers PRIVATE
        STM32U5G9xx
        USE_HAL_DRIVER
)

# 6. Linker Configuration & Target Link Options
set(LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/CubeMX/STM32U5G9NJHXQ_FLASH.ld")

if(NOT EXISTS "${LINKER_SCRIPT}")
    file(GLOB LINKER_SCRIPT
            "${CMAKE_CURRENT_SOURCE_DIR}/CubeMX/*.ld"
            "${CMAKE_CURRENT_SOURCE_DIR}/*.ld"
    )
endif()

target_compile_options(${PROJECT_NAME}.elf PRIVATE
        $<$<COMPILE_LANGUAGE:C,CXX,ASM>:-mcpu=cortex-m33>
        $<$<COMPILE_LANGUAGE:C,CXX,ASM>:-mthumb>
        $<$<COMPILE_LANGUAGE:C,CXX,ASM>:-mfpu=fpv5-sp-d16>
        $<$<COMPILE_LANGUAGE:C,CXX,ASM>:-mfloat-abi=hard>
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

# 7. Link Central Drivers Library & Share CubeMX Includes
target_link_libraries(${PROJECT_NAME}.elf PRIVATE SolarGators_drivers)
target_include_directories(SolarGators_drivers PRIVATE ${NODE_INCLUDES})

# 8. Post-Build Commands (.bin, .hex, size display)
add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O binary ${PROJECT_NAME}.elf ${PROJECT_NAME}.bin
        COMMAND ${CMAKE_OBJCOPY} -O ihex ${PROJECT_Name}.elf ${PROJECT_Name}.hex
        COMMAND ${CMAKE_SIZE} ${PROJECT_NAME}.elf
)