# Steering Wheel

Firmware project for Blaze's steering wheel.

[Back to the repository overview](../../README.md)

## Current Status

The project contains generated peripheral initialization, a minimal FreeRTOS
application, partial TouchGFX integration, and an IAR project.

The checked-in files do not provide a complete GUI application or a complete
GCC build configuration.

## Hardware and Software Configuration

These values describe the checked-in project configuration.

| Item | Configuration |
|---|---|
| MCU | STM32U5G9NJH6Q |
| Package | TFBGA216 |
| CPU | Arm Cortex-M33 |
| STM32 firmware package | STM32Cube FW_U5 V1.9.0 |
| CubeMX project | `Steering-Wheel.ioc` |
| CubeMX output toolchain | EWARM V9.20 |
| RTOS interface | FreeRTOS with CMSIS-RTOS2 |
| Graphics integration | TouchGFX-generated support files |
| CMake target | `Steering-Wheel.elf` |

Board revision, display part number, input hardware, and schematics are not
documented.

## Implemented Initialization

`Core/Src/main.c` includes initialization for:

- GPIO
- CRC
- DSI
- FDCAN1
- GFXTIM and GPU2D
- LTDC
- FMC SRAM
- DAC1
- TouchGFX and the RTOS kernel

The USB initialization function is present but empty.

The RTOS application creates `defaultTask`, which loops with `osDelay(1)`.
It does not create a TouchGFX rendering task.

`STM32TouchController::init()` is empty, and `sampleTouch()` always returns
`false`. These are placeholders, not a working touch driver.

## Functional Responsibilities

The detailed display content, driver inputs, CAN messages, and fault indications
are not yet documented.

No particular button layout, touch device, or vehicle-control behavior is
assumed here.

## Directory Layout

```text
Steering-Wheel/
├── Core/
│   ├── Inc/
│   └── Src/
├── Drivers/
├── EWARM/
├── Middlewares/
├── TouchGFX/
│   ├── App/
│   ├── target/
│   └── ApplicationTemplate.touchgfx.part
├── CMakeLists.txt
└── Steering-Wheel.ioc
```

## Building

The project is enabled in the repository-level CMake configuration, but
GCC integration is incomplete:

- Startup assembly and linker configuration are supplied for IAR under `EWARM/`.
- The GCC FreeRTOS port path referenced by CMake is absent from the checked-in
  middleware; the supplied port is an IAR port.
- The GUI application directories referenced by CMake are absent.
- Existing `TouchGFX/App/` and `TouchGFX/target/` sources are not included by the
  current source patterns.
- Shared-library and global Cortex-M4 settings need review for this
  Cortex-M33 target.

After the build integration is completed, the intended command from the
repository root is:

```sh
cmake --preset debug
cmake --build --preset debug --target Steering-Wheel.elf
```

A successful build has not been established by this documentation.

See the [environment setup guide](../../docs/guides/environment-setup.md).

## STM32CubeMX Workflow

Open `Steering-Wheel.ioc` to modify peripheral or pin configuration.

Review generated TouchGFX files separately from application-owned files.
The `.touchgfx.part` file is a template fragment, not a complete GUI project.

See the [CubeMX workflow guide](../../docs/guides/cubemx-workflow.md).

## Information Still Needed

- Board revision and schematic reference
- Display, memory, and input-device part numbers
- GUI requirements and vehicle data definitions
- Display and input-driver implementation
- CAN messages and communication behavior
- Validated build, programming, and display-test procedure
