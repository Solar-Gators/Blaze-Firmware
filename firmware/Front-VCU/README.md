# Front Vehicle Control Unit

Firmware project for Blaze's front vehicle control unit.

[Back to the repository overview](../../README.md)

## Current Status

The project currently provides generated STM32 peripheral initialization and a minimal FreeRTOS application.

The following are intended responsibilities, not implemented features in the current application code:

- Throttle
- Brake light control
- Front lighting, turn signal, and horn control
- CAN message transmission and routing

The checked-in configuration does not yet include the ADC and application I/O setup needed to support all of these functions.

## Hardware and Software Configuration

| Item | Checked-in configuration |
|---|---|
| MCU | STM32G483RET3, as selected in `Front-VCU.ioc` |
| Package | LQFP64 |
| CPU | Arm Cortex-M4 |
| System clock | 170 MHz |
| STM32 firmware package | STM32Cube FW_G4 V1.6.3 |
| RTOS interface | FreeRTOS with CMSIS-RTOS2 |
| CubeMX output toolchain | Makefile |
| CMake target | `Front-VCU.elf` |

Board revision and schematic links are not documented in this directory.

## Implemented Initialization

`Core/Src/main.c` initializes:

- GPIO support
- FDCAN1, FDCAN2, and FDCAN3
- The independent watchdog
- The CMSIS-RTOS2 kernel and an application thread named `idleTask`

`StartIdleTask()` currently loops with `osDelay(1)`. It does not perform vehicle control or CAN processing.

The stack-overflow and allocation-failure hooks in `Core/Src/app_freertos.c` are placeholders. The application also does not currently refresh the initialized watchdog.

Peripheral initialization alone does not establish working CAN communication, sensor acquisition, or control behavior.

## Directory Layout

```text
Front-VCU/
├── Core/
│   ├── Inc/                      # Application and HAL configuration headers
│   └── Src/                      # Startup flow, peripheral setup, and RTOS hooks
├── CubeMX/
│   ├── Makefile
│   ├── startup_stm32g483xx.s
│   └── STM32G483XX_FLASH.ld
├── Drivers/                      # STM32 HAL and CMSIS
├── Middlewares/                  # FreeRTOS and its CMSIS-RTOS2 adapter
├── CMakeLists.txt
├── Front-VCU.ioc
└── README.md
```

## Building

This target is part of the repository-level CMake project. Use the repository root rather than configuring this directory independently.

See the root README for [tool requirements and build blockers](../../README.md#build-configuration).

After those blockers are resolved, the intended debug build is:

```sh
cmake --preset debug
cmake --build --preset debug --target Front-VCU.elf
```

Selecting this target does not bypass configuration errors in other enabled subsystems.

## STM32CubeMX Workflow

1. Open `Front-VCU.ioc`.
2. Update peripheral, clock, or pin configuration in CubeMX.
3. Preserve the Makefile output configuration and `CubeMX/` toolchain location
   unless intentionally updating the build integration.
4. Regenerate the project.
5. Review the generated diff and confirm that CMake still references the correct
   startup assembly, linker script, sources, and headers.

Keep custom edits in regenerated `Core/` files inside the corresponding user-code sections:

```c
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */
```

Treat files under `CubeMX/` as generated artifacts. Make configuration changes through CubeMX where possible, and document any necessary manual startup or linker changes so they can be preserved during regeneration.
