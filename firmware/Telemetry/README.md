# Telemetry

Firmware project for Blaze's telemetry subsystem.

[Repository overview](../../README.md)

## Configuration

| Item | Configuration |
|---|---|
| MCU | STM32G483RET3 |
| CubeMX project | `Telemetry.ioc` |
| STM32 firmware package | STM32Cube FW_G4 V1.6.3 |
| CubeMX output toolchain | Makefile |
| Repository build | CMake and Arm GCC |
| RTOS interface | CMSIS-RTOS2 over FreeRTOS |
| CMake target | `Telemetry.elf` |

The configured MCU does not establish the final board revision or telemetry
hardware selection.

## Current Implementation

The project contains generated initialization code, GCC startup and linker
files, FreeRTOS support, and application-owned source directories.

The application creates `defaultTask`.

`StartDefaultTask()` calls `userDefaultTask()`, which currently loops with
`osDelay(1)`.

Radio communication, logging, data transport, and destination-service behavior
are not implemented in the placeholder user task.

## Directory Layout

```text
Telemetry/
├── Core/
├── CubeMX/
├── Drivers/
├── Middlewares/
├── user/
│   ├── Inc/
│   └── Src/
├── CMakeLists.txt
└── Telemetry.ioc
```

## Building

The project is enabled in the top-level build.

From the repository root:

```sh
cmake --preset debug
cmake --build --preset debug --target Telemetry.elf
```

For Release, replace `debug` with `release` in both commands.

Outputs are generated under `build/<preset>/firmware/Telemetry/`.

## Development

- [CubeMX workflow](../../docs/guides/cubemx-workflow.md)
- [Adding threads](../../docs/guides/adding-threads.md)

## Information Still Needed

- Board revision and schematic reference
- Radio, modem, or other communication hardware
- Data sources and message definitions
- Transport and destination service
- Update rates, buffering, and logging requirements
- Connection-loss and recovery behavior
- Programming and bench-test procedure
