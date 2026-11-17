# BMS Daughterboard

Firmware project for Blaze's BMS daughterboard.

[Back to the repository overview](../../README.md)

## Current Status

The project contains STM32CubeMX-generated peripheral initialization, bundled
STM32 drivers, an IAR project, and a CMake target definition.

After initialization, `main()` enters an empty loop. Application-level
measurement, communication, and fault-handling behavior are not implemented
in the checked-in application code.

## Hardware and Software Configuration

These values describe the checked-in project configuration.

| Item | Configuration |
|---|---|
| MCU | STM32L433CCT6 |
| Package | LQFP48 |
| STM32 firmware package | STM32Cube FW_L4 V1.18.2 |
| CubeMX project | `Daughterboard 2.ioc` |
| CubeMX output toolchain | EWARM V8.32 |
| CMake target | `BMS-Daughterboard.elf` |
| Application execution | Bare-metal main loop; no RTOS startup in `main()` |

Board revision and schematics are not documented. The filename
`Daughterboard 2.ioc` does not establish a hardware revision.

## Implemented Initialization

`Core/Src/main.c` initializes:

- GPIO
- ADC1
- CAN1
- I2C2
- SPI3

GPIO definitions include names such as `ALERT`, `ADDR0`, `ADDR1`, `FAULT`,
`ERROR`, `OK`, `CS1`, and `CS2`. Their electrical meaning and application
behavior require confirmation against the hardware documentation.

## Functional Responsibilities

Not yet documented.

The number of monitored cells, measurement hardware, balancing capability,
addressing scheme, and fault behavior are unknown.

## Directory Layout

```text
BMS-Daughterboard/
├── Core/
│   ├── Inc/
│   └── Src/
├── Drivers/
├── EWARM/
├── CMakeLists.txt
└── Daughterboard 2.ioc
```

## Building

The project is enabled in the repository-level CMake configuration.

GCC build integration is incomplete: the supplied startup assembly and linker
configuration are under `EWARM/`, but CMake searches the subsystem root and
`CubeMX/` for startup assembly and GCC `.ld` linker scripts.

After the build configuration is completed, the intended command from the
repository root is:

```sh
cmake --preset debug
cmake --build --preset debug --target BMS-Daughterboard.elf
```

A successful build has not been established by this documentation.

See the [environment setup guide](../../docs/guides/environment-setup.md).

## STM32CubeMX Workflow

Open `Daughterboard 2.ioc` to modify peripheral or pin configuration.

Preserve custom edits in regenerated files inside their `USER CODE` sections.
Review the generated files and corresponding CMake inputs after regeneration.

See the [CubeMX workflow guide](../../docs/guides/cubemx-workflow.md).

## Information Still Needed

- Board revision and schematic reference
- Measurement devices and channel assignments
- Address selection behavior
- CAN message definitions and bus configuration
- Fault indications, thresholds, and recovery behavior
- Validated build, programming, and bench-test procedure
