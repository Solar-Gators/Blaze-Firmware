# BMS Daughterboard

Firmware project for Blaze's BMS daughterboard.

[Repository overview](../../README.md)

## Configuration

| Item | Configuration |
|---|---|
| MCU | STM32L433CCT6 |
| CubeMX project | `Daughterboard.ioc` |
| STM32 firmware package | STM32Cube FW_L4 V1.18.2 |
| CubeMX output toolchain | Makefile |
| Repository build | CMake and Arm GCC |
| RTOS interface | CMSIS-RTOS2 over FreeRTOS |
| CMake target | `BMS-Daughterboard.elf` |

Board revision and schematics are not documented here.

## Current Implementation

The project contains generated peripheral initialization, GCC startup and
linker files, and FreeRTOS support.

The application creates:

| Task | Entry function | Current behavior |
|---|---|---|
| `defaultTask` | `StartDefaultTask()` | Loops with `osDelay(1)` |
| `CellCheck` | `StartCellCheck()` | Calls `userStartCellCheck()` |

`userStartCellCheck()` is implemented in `user/Src/user_threads.cpp`.
It currently loops with `osDelay(1)`.

The task name does not establish implemented cell measurement or protection.
Measurement, balancing, communication, and fault-handling behavior remain
to be defined and implemented.

## Directory Layout

```text
BMS-Daughterboard/
├── Core/
├── CubeMX/
├── Drivers/
├── Middlewares/
├── user/
│   ├── Inc/
│   └── Src/
├── CMakeLists.txt
└── Daughterboard.ioc
```

## Building

From the repository root:

```sh
cmake --preset debug
cmake --build --preset debug --target BMS-Daughterboard.elf
```

For Release, replace `debug` with `release` in both commands.

Outputs are generated under `build/<preset>/firmware/BMS-Daughterboard/`.

## Development

Open `Daughterboard.ioc` for CubeMX changes. Older documentation used the
filename `Daughterboard 2.ioc`; that is not the current project filename.

- [CubeMX workflow](../../docs/guides/cubemx-workflow.md)
- [Adding threads](../../docs/guides/adding-threads.md)

## Information Still Needed

- Board revision and schematic reference
- Measurement hardware and number of channels
- Address-selection behavior
- Balancing capability and requirements
- CAN message definitions
- Fault thresholds and recovery behavior
- Programming and bench-test procedure
