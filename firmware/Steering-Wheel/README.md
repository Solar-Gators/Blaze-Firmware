# Steering Wheel

Firmware project for Blaze's steering wheel.

[Repository overview](../../README.md)

## Configuration

| Item | Configuration |
|---|---|
| MCU | STM32U5G9NJH6Q |
| CPU | Arm Cortex-M33 |
| CubeMX project | `Steering-Wheel.ioc` |
| STM32 firmware package | STM32Cube FW_U5 V1.9.0 |
| CubeMX output toolchain | Makefile |
| Repository build | CMake and Arm GCC |
| RTOS interface | CMSIS-RTOS2 over FreeRTOS |
| TouchGFX generator version | 4.26.1 |
| TouchGFX project | `TouchGFX/Steering-Wheel.touchgfx` |
| CMake target | `Steering-Wheel.elf` |

Board revision, display part number, touch hardware, and schematics are not
documented here.

## Current Implementation

The project includes generated peripheral initialization, GCC startup and
linker files, FreeRTOS, a TouchGFX project, generated GUI sources, and
TouchGFX/GPU libraries.

The CMake target uses the GCC Cortex-M33 non-TrustZone FreeRTOS port.

`main()` initializes TouchGFX. `MX_FREERTOS_Init()` creates `defaultTask`,
which calls `userDefaultTask()`. That user function currently loops with
`osDelay(1)`.

## Remaining Graphics Integration

The generated `TouchGFX_Task()` entry function exists, but the current RTOS
initialization does not create a thread that runs it.

`STM32TouchController::init()` is empty, and `sampleTouch()` returns `false`.
Touch input is therefore not implemented.

The USB initialization function is also a placeholder.

A successful build confirms compilation and linking. It does not establish
working display output, touch input, external memory, or vehicle-data updates.

## Directory Layout

```text
Steering-Wheel/
├── Core/
├── CubeMX/
├── Drivers/
├── Middlewares/
├── TouchGFX/
│   ├── App/
│   ├── assets/
│   ├── generated/
│   ├── gui/
│   ├── simulator/
│   ├── target/
│   └── Steering-Wheel.touchgfx
├── user/
│   ├── Inc/
│   └── Src/
├── CMakeLists.txt
└── Steering-Wheel.ioc
```

## Building

From the repository root:

```sh
cmake --preset debug
cmake --build --preset debug --target Steering-Wheel.elf
```

For Release, replace `debug` with `release` in both commands.

Outputs are generated under `build/<preset>/firmware/Steering-Wheel/`.

## Development

Use CubeMX for hardware and RTOS configuration. Use TouchGFX Designer for
screen layout and generated GUI code.

Keep custom GUI behavior in application-owned GUI classes. Keep non-GUI
application logic under `user/`.

- [TouchGFX workflow](../../docs/guides/touchgfx-workflow.md)
- [CubeMX workflow](../../docs/guides/cubemx-workflow.md)
- [Adding threads](../../docs/guides/adding-threads.md)

## Information Still Needed

- Board revision and schematic reference
- Display, touch-controller, and memory part numbers
- Display timing and memory configuration
- GUI requirements and vehicle-data definitions
- Input handling and CAN messages
- Rendering-task stack and priority validation
- Programming, display, and touch-test procedures
