# Rear Vehicle Control Unit

Firmware project for Blaze's rear VCU.

[Repository overview](../../README.md)

## Configuration

| Item | Configuration |
|---|---|
| MCU | STM32G483RET6 |
| CubeMX project | `Rear-VCU.ioc` |
| STM32 firmware package | STM32Cube FW_G4 V1.6.3 |
| CubeMX output toolchain | Makefile |
| Repository build | CMake and Arm GCC |
| RTOS interface | CMSIS-RTOS2 over FreeRTOS |
| CMake target | `Rear-VCU.elf` |

Board revision, schematic reference, and final control responsibilities are
not documented here.

## Current Implementation

The project contains generated peripheral initialization, GCC startup and
linker files, FreeRTOS support, and USB device middleware.

The application creates `defaultTask`.

`StartDefaultTask()` initializes the USB device stack and calls
`userDefaultTask()`. The user function currently loops with `osDelay(1)`.

The USB device sources include CDC support. This does not establish a tested
USB connection or an application-level USB protocol.

Vehicle-control behavior is not implemented in the placeholder user task.

## Directory Layout

```text
Rear-VCU/
├── Core/
├── CubeMX/
├── Drivers/
├── Middlewares/
├── USB_Device/
│   ├── App/
│   └── Target/
├── user/
│   ├── Inc/
│   └── Src/
├── CMakeLists.txt
└── Rear-VCU.ioc
```

## Building

The project is enabled in the top-level build.

From the repository root:

```sh
cmake --preset debug
cmake --build --preset debug --target Rear-VCU.elf
```

For Release, replace `debug` with `release` in both commands.

Outputs are generated under `build/<preset>/firmware/Rear-VCU/`.

## Development

Keep application logic under `user/`. Preserve generated-file changes inside
the appropriate `USER CODE` sections.

- [CubeMX workflow](../../docs/guides/cubemx-workflow.md)
- [Adding threads](../../docs/guides/adding-threads.md)

## Information Still Needed

- Board revision and schematic reference
- Peripheral assignments and attached devices
- Vehicle-control responsibilities
- Communication protocols and timeout behavior
- Validated USB, programming, and bench-test procedures
