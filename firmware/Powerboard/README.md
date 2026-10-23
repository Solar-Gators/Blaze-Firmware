# Powerboard

Firmware project for Blaze's powerboard.

[Back to the repository overview](../../README.md)

## Current Status

The project contains STM32CubeMX-generated peripheral initialization, bundled
STM32 drivers, an IAR project, and a CMake target definition.

After initialization, `main()` enters an empty loop. Power-control behavior
is not implemented in the checked-in application code.

## Hardware and Software Configuration

These values describe the checked-in project configuration.

| Item | Configuration |
|---|---|
| MCU | STM32L433CCT6 |
| Package | LQFP48 |
| STM32 firmware package | STM32Cube FW_L4 V1.18.2 |
| CubeMX project | `PowerBoard.ioc` |
| CubeMX output toolchain | EWARM V8.32 |
| CMake target | `Powerboard.elf` |
| Application execution | Bare-metal main loop; no RTOS startup in `main()` |

Board revision and schematics are not documented.

## Implemented Initialization

`Core/Src/main.c` initializes:

- GPIO
- ADC1
- CAN1
- I2C2
- TIM1 and TIM16

The loads, sensors, and external devices associated with these peripherals
are not established by the initialization code alone.

## Functional Responsibilities

Not yet documented.

Controlled power rails, output channels, sequencing requirements, measurement
scaling, and fault behavior are unknown.

## Directory Layout

```text
Powerboard/
├── Core/
│   ├── Inc/
│   └── Src/
├── Drivers/
├── EWARM/
├── CMakeLists.txt
└── PowerBoard.ioc
```

## Building

The project is enabled in the repository-level CMake configuration.

GCC build integration is incomplete: startup assembly and linker configuration
are supplied under `EWARM/`, while CMake searches the subsystem root and
`CubeMX/` for startup assembly and GCC `.ld` linker scripts.

After the build configuration is completed, the intended command from the
repository root is:

```sh
cmake --preset debug
cmake --build --preset debug --target Powerboard.elf
```

Target and directory names are case-sensitive on some systems. Use
`Powerboard.elf` exactly as shown.

A successful build has not been established by this documentation.

See the [environment setup guide](../../docs/guides/environment-setup.md).

## STM32CubeMX Workflow

Open `PowerBoard.ioc` to modify peripheral or pin configuration.

Preserve custom edits in regenerated files inside their `USER CODE` sections.
Review the generated files and corresponding CMake inputs after regeneration.

See the [CubeMX workflow guide](../../docs/guides/cubemx-workflow.md).

## Information Still Needed

- Board revision and schematic reference
- Controlled rails and output assignments
- Sensor interfaces and conversion factors
- Startup and shutdown sequencing
- CAN message definitions and bus configuration
- Fault handling and validated bench-test procedure
