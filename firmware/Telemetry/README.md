# Telemetry

Project directory for Blaze's telemetry firmware.

[Back to the repository overview](../../README.md)

## Current Status

This directory currently contains only `CMakeLists.txt`.

It does not contain application sources, a CubeMX project, startup assembly,
or a linker script. The project is disabled in the top-level CMake configuration.

## Hardware and Software Configuration

| Item | Status |
|---|---|
| MCU | Not confirmed |
| Board revision | Not documented |
| Schematics | Not documented |
| Communication hardware | Not documented |
| CubeMX project | Not present |
| RTOS | Not confirmed |
| Declared CMake target | `Telemetry.elf` |

The CMake scaffold references STM32G4, `STM32G483xx`, and FreeRTOS paths.
These references do not establish the final hardware or software selection.

## Functional Responsibilities

Not yet documented.

The repository does not establish the telemetry transport, radio or modem,
data format, logging behavior, or destination service.

## Directory Layout

```text
Telemetry/
└── CMakeLists.txt
```

## Building

There is no complete firmware project to build.

The target is not available through the current top-level configuration.
Enabling its `add_subdirectory()` entry without adding the required sources
and startup files will trigger the scaffold's missing-source error.

Before enabling this project:

1. Confirm the MCU and communication hardware.
2. Add the matching project configuration and application sources.
3. Supply startup assembly and a linker script for the selected compiler.
4. Update source paths, include paths, MCU definitions, and compiler options.
5. Establish a successful build and document the result.

See the [environment setup guide](../../docs/guides/environment-setup.md).

## Information Still Needed

- Hardware selection and schematic reference
- Data sources and communication interfaces
- Transport and message format
- Update rates and buffering requirements
- Connection-loss and recovery behavior
- Build, programming, and test procedure
