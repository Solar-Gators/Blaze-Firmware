# BMS Motherboard

Firmware project for Blaze's BMS motherboard.

[Back to the repository overview](../../README.md)

## Current Status

The project contains STM32CubeMX-generated peripheral initialization, bundled
STM32 drivers, an IAR project, and a CMake target definition.

After initialization, `main()` enters an empty loop. Application-level BMS
behavior is not implemented in the checked-in application code.

## Hardware and Software Configuration

These values describe the checked-in project configuration.

| Item | Configuration |
|---|---|
| MCU | STM32G483RET3 |
| Package | LQFP64 |
| STM32 firmware package | STM32Cube FW_G4 V1.6.3 |
| CubeMX project | `Motherboard.ioc` |
| CubeMX output toolchain | EWARM V8.50 |
| CMake target | `BMS-Motherboard.elf` |
| Application execution | Bare-metal main loop; no RTOS startup in `main()` |

Board revision and schematics are not documented.

## Implemented Initialization

`Core/Src/main.c` initializes:

- GPIO
- FDCAN2 and FDCAN3
- I2C3
- SPI1 and SPI3
- TIM8
- USB peripheral controller

The attached devices, communication protocols, and intended use of these
peripherals are not established by the initialization code alone.

## Functional Responsibilities

Not yet documented.

Battery configuration, measurement devices, protection thresholds, contactor
control, and communication responsibilities must be confirmed before they are
documented as supported features.

## Directory Layout

```text
BMS-Motherboard/
├── Core/
│   ├── Inc/
│   └── Src/
├── Drivers/
├── EWARM/
├── CMakeLists.txt
└── Motherboard.ioc
```

## Building

The project is enabled in the repository-level CMake configuration.

GCC build integration is incomplete: startup assembly and linker configuration
are supplied under `EWARM/`, while CMake searches the subsystem root and
`CubeMX/` for startup assembly and GCC `.ld` linker scripts.

The IAR files cannot be used as GCC equivalents without the appropriate
toolchain integration.

After the build configuration is completed, the intended command from the
repository root is:

```sh
cmake --preset debug
cmake --build --preset debug --target BMS-Motherboard.elf
```

A successful build has not been established by this documentation.

See the [environment setup guide](../../docs/guides/environment-setup.md).

## STM32CubeMX Workflow

Open `Motherboard.ioc` to modify peripheral or pin configuration.

Preserve custom edits in regenerated files inside their `USER CODE` sections.
Review toolchain, startup, linker, and driver changes after regeneration.

See the [CubeMX workflow guide](../../docs/guides/cubemx-workflow.md).

## Information Still Needed

- Board revision and schematic reference
- Connected devices and their interfaces
- BMS responsibilities and operating states
- CAN message definitions and bus configuration
- Fault handling and recovery requirements
- Validated build, programming, and bench-test procedure
