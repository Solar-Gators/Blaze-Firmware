# STM32CubeMX Workflow

Use CubeMX to configure peripherals, clocks, pins, and RTOS objects.

The repository builds firmware with CMake and Arm GCC. The checked-in
CubeMX projects select Makefile output.

## Projects

Paths below are relative to `firmware/`.

| Subsystem | CubeMX file | STM32 firmware package |
|---|---|---|
| Front VCU | `Front-VCU/Front-VCU.ioc` | FW_G4 V1.6.3 |
| Rear VCU | `Rear-VCU/Rear-VCU.ioc` | FW_G4 V1.6.3 |
| Steering wheel | `Steering-Wheel/Steering-Wheel.ioc` | FW_U5 V1.9.0 |
| Telemetry | `Telemetry/Telemetry.ioc` | FW_G4 V1.6.3 |
| BMS motherboard | `BMS-Motherboard/Motherboard.ioc` | FW_G4 V1.6.3 |
| BMS daughterboard | `BMS-Daughterboard/Daughterboard.ioc` | FW_L4 V1.18.2 |
| Powerboard | `Powerboard/PowerBoard.ioc` | FW_L4 V1.18.2 |

## Before Generating

1. Check `git status` and preserve existing work.
2. Open the correct `.ioc`.
3. Verify the MCU and firmware package.
4. Check the output location and toolchain.
5. Enable preservation of user code during regeneration.
6. Review migration prompts before accepting upgrades.

Do not change firmware packages or middleware versions incidentally while
making an unrelated pin or task change.

## Application Code Placement

All subsystem projects have:

```text
user/Inc/
user/Src/
```

Keep substantial application logic there.

Generated entry functions should call application-owned functions through
headers such as `user_threads.h`.

Custom edits in generated files must stay inside the corresponding
`USER CODE BEGIN` and `USER CODE END` sections.

See [Adding application threads](adding-threads.md).

## Generate and Review

After changing configuration:

1. Generate code.
2. Review the `.ioc` diff.
3. Review generated source and header changes.
4. Confirm custom code survived.
5. Check startup, linker, middleware-port, and include paths.
6. Rerun CMake.
7. Build the affected target.

From the repository root:

```sh
git diff --stat
git diff --check
cmake --preset debug
cmake --build --preset debug --target Front-VCU.elf
```

Replace the target as appropriate.

CubeMX generation does not automatically maintain every custom CMake input.
Check CMake when generation adds files, moves files, or enables middleware.

## Startup and Linker Files

The CMake projects use GCC startup assembly and linker scripts under
`CubeMX/`.

Do not substitute IAR startup code or linker configuration files for GCC
inputs.

Use exact, consistent filenames. Avoid files whose names differ only by case.

Front-VCU currently has a case-only duplicate linker script. Resolve that
duplicate together with its CMake reference.

## RTOS Configuration

Review:

- Task entry function and name
- Priority
- Stack allocation and its units
- Static or dynamic allocation
- Required queues, semaphores, and mutexes
- Generated interrupt priorities and RTOS port

The `.ioc` and generated C code should agree. Verify generated output rather
than assuming a saved CubeMX setting has already been regenerated.

## TouchGFX

Steering-Wheel changes may require both CubeMX and TouchGFX Designer generation.

Review hardware configuration, generated graphics integration, GUI code,
and library versions together.

See [TouchGFX workflow](touchgfx-workflow.md).
