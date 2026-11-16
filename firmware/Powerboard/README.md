# Powerboard

Firmware project for Blaze's powerboard.

[Repository overview](../../README.md)

## Configuration

| Item | Configuration |
|---|---|
| MCU | STM32L433CCT6 |
| CubeMX project | `PowerBoard.ioc` |
| STM32 firmware package | STM32Cube FW_L4 V1.18.2 |
| CubeMX output toolchain | Makefile |
| Repository build | CMake and Arm GCC |
| RTOS interface | CMSIS-RTOS2 over FreeRTOS |
| CMake target | `Powerboard.elf` |

Board revision and schematics are not documented here.

## Current Implementation

The project contains generated peripheral initialization, GCC startup and
linker files, and FreeRTOS support.

The application creates:

| Task | Entry function | Current behavior |
|---|---|---|
| `defaultTask` | `StartDefaultTask()` | Calls `userDefaultTask()` |
| `TempSensing` | `StartTempSensing()` | Loops with `osDelay(1)` |

`userDefaultTask()` is implemented in `user/Src/user_threads.cpp` and
currently loops with `osDelay(1)`.

`TempSensing` does not currently perform temperature acquisition.
Power sequencing, measurement conversion, and fault handling are not
implemented by these placeholder tasks.

## Directory Layout

```text
Powerboard/
├── Core/
├── CubeMX/
├── Drivers/
├── Middlewares/
├── user/
│   ├── Inc/
│   └── Src/
├── CMakeLists.txt
└── PowerBoard.ioc
```

## Building

From the repository root:

```sh
cmake --preset debug
cmake --build --preset debug --target Powerboard.elf
```

For Release, replace `debug` with `release` in both commands.

Outputs are generated under `build/<preset>/firmware/Powerboard/`.

Use the target capitalization exactly as shown.

## Development

- [CubeMX workflow](../../docs/guides/cubemx-workflow.md)
- [Adding threads](../../docs/guides/adding-threads.md)

## Information Still Needed

- Board revision and schematic reference
- Controlled rails and output assignments
- Temperature and other measurement hardware
- Conversion factors and units
- Startup and shutdown sequencing
- CAN messages and fault behavior
- Programming and bench-test procedure
