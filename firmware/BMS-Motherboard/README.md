# BMS Motherboard

Firmware project for Blaze's BMS motherboard.

[Repository overview](../../README.md)

## Configuration

| Item | Configuration |
|---|---|
| MCU | STM32G483RET3 |
| CubeMX project | `Motherboard.ioc` |
| STM32 firmware package | STM32Cube FW_G4 V1.6.3 |
| CubeMX output toolchain | Makefile |
| Repository build | CMake and Arm GCC |
| RTOS interface | CMSIS-RTOS2 over FreeRTOS |
| CMake target | `BMS-Motherboard.elf` |

Board revision and schematics are not documented here.

## Current Implementation

The project contains generated peripheral initialization, GCC startup and
linker files, and FreeRTOS support.

The application creates `defaultTask`.

`StartDefaultTask()` calls `userDefaultTask()`, which currently loops with
`osDelay(1)`.

The task does not currently implement battery monitoring, protection logic,
contactor control, or communication behavior. Peripheral configuration alone
does not establish these capabilities.

## Directory Layout

```text
BMS-Motherboard/
├── Core/
├── CubeMX/
├── Drivers/
├── Middlewares/
├── user/
│   ├── Inc/
│   └── Src/
├── CMakeLists.txt
└── Motherboard.ioc
```

## Building

From the repository root:

```sh
cmake --preset debug
cmake --build --preset debug --target BMS-Motherboard.elf
```

For Release, replace `debug` with `release` in both commands.

Outputs are generated under `build/<preset>/firmware/BMS-Motherboard/`.

## Development

- [CubeMX workflow](../../docs/guides/cubemx-workflow.md)
- [Adding threads](../../docs/guides/adding-threads.md)

## Information Still Needed

- Board revision and schematic reference
- Connected devices and interfaces
- Battery configuration and measurement responsibilities
- Protection thresholds and recovery requirements
- Contactor-control responsibilities, if applicable
- CAN messages and timeout behavior
- Programming and bench-test procedure
