# Front Vehicle Control Unit

Firmware project for Blaze's front VCU.

[Repository overview](../../README.md)

## Configuration

| Item | Configuration |
|---|---|
| MCU | STM32G483RET3 |
| CubeMX project | `Front-VCU.ioc` |
| STM32 firmware package | STM32Cube FW_G4 V1.6.3 |
| CubeMX output toolchain | Makefile |
| Repository build | CMake and Arm GCC |
| RTOS interface | CMSIS-RTOS2 over FreeRTOS |
| CMake target | `Front-VCU.elf` |

Board revision, schematic reference, and final control responsibilities are
not documented here.

## Current Implementation

`Core/Src/main.c` initializes GPIO, FDCAN1, FDCAN2, FDCAN3, the independent
watchdog, and the RTOS kernel.

The application creates `defaultTask`.

`StartDefaultTask()` calls `userDefaultTask()`, declared in
`user/Inc/user_threads.h` and implemented in `user/Src/user_threads.cpp`.
The user function currently loops with `osDelay(1)`.

The current application does not create a separate application thread named
`idleTask`. FreeRTOS manages its own kernel idle task.

Vehicle-control and CAN-processing behavior are not implemented by the
placeholder user task.

## Known Limitations

- The independent watchdog is started but is not currently refreshed.
  Normal execution can therefore end in a watchdog reset.
- Stack-overflow and allocation-failure hooks are placeholders.
- Peripheral initialization alone does not establish working CAN traffic.
- `CubeMX/` contains two identical linker scripts whose filenames differ
  only by case: `STM32G483XX_FLASH.ld` and `STM32G483xx_FLASH.ld`.
  Keep one consistent filename when resolving this issue.

## Directory Layout

```text
Front-VCU/
├── Core/
│   ├── Inc/
│   └── Src/
├── CubeMX/
├── Drivers/
├── Middlewares/
├── user/
│   ├── Inc/
│   └── Src/
├── CMakeLists.txt
└── Front-VCU.ioc
```

## Building

From the repository root:

```sh
cmake --preset debug
cmake --build --preset debug --target Front-VCU.elf
```

For Release, replace `debug` with `release` in both commands.

Outputs are generated under `build/<preset>/firmware/Front-VCU/`.

## Development

Use CubeMX for peripheral and RTOS configuration. Keep application logic in
`user/` and custom generated-file edits inside `USER CODE` sections.

- [CubeMX workflow](../../docs/guides/cubemx-workflow.md)
- [Adding threads](../../docs/guides/adding-threads.md)

## Information Still Needed

- Board revision and schematic reference
- CAN message definitions and bus timing
- Control responsibilities and operating states
- Watchdog health criteria and recovery behavior
- Programming and bench-test procedure
