# STM32CubeMX Workflow

Use this workflow when changing peripheral configuration or regenerating code.

## Existing Projects

| Subsystem | CubeMX file | Configured output toolchain |
|---|---|---|
| Front-VCU | `firmware/Front-VCU/Front-VCU.ioc` | Makefile |
| BMS-Motherboard | `firmware/BMS-Motherboard/Motherboard.ioc` | EWARM V8.50 |
| BMS-Daughterboard | `firmware/BMS-Daughterboard/Daughterboard 2.ioc` | EWARM V8.32 |
| Powerboard | `firmware/Powerboard/PowerBoard.ioc` | EWARM V8.32 |
| Steering-Wheel | `firmware/Steering-Wheel/Steering-Wheel.ioc` | EWARM V9.20 |

Rear-VCU and Telemetry do not currently contain `.ioc` files.

## Before Regenerating

1. Check `git status` and preserve any existing work.
2. Open the correct subsystem's `.ioc` file.
3. Verify the MCU, firmware package, output toolchain, and output directory.
4. Review any migration prompt before accepting it.
5. Identify custom edits outside generated user-code sections.

A firmware-package or CubeMX upgrade may change more than the peripheral
configuration you intended to edit. Record intentional upgrades explicitly.

## Application Code Placement

For edits inside regenerated files, use the matching `USER CODE` sections.

Front-VCU keeps application-owned code under `user/Inc/` and `user/Src/`.
Its generated `StartDefaultTask()` calls `userDefaultTask()` through a
C-compatible declaration in `user_threads.h`.

When adopting this pattern in another subsystem:

- Add the directories to that target's CMake include and source paths.
- Keep generated entry points small.
- Verify C/C++ linkage when generated C calls C++ functions.

Creating a `user/` directory alone does not add its files to the build.

## Regenerate and Review

After generation:

1. Review the `.ioc` diff.
2. Review generated source and header changes.
3. Verify that custom code survived.
4. Check startup assembly and linker files. (probably dont have to do this)
5. Check middleware paths and compiler-specific ports. (probably dont have to do this)
6. Reload CMake and build the affected target when its integration is complete.

Useful checks from the repository root:

```sh
git status --short
git diff --stat
git diff --check
```

Inspect the full diff before committing.

## Toolchain Compatibility

IAR and GCC use different startup, linker, and middleware-port inputs.

Changing the CubeMX output toolchain requires corresponding build changes.
Do not rename an IAR linker configuration to `.ld` or assume an IAR RTOS port
can be compiled by GCC.

## Front-VCU Linker Filename

The reviewed branch contains:

```text
STM32G483XX_FLASH.ld
STM32G483xx_FLASH.ld
```

These names differ only by case. The CMake configuration selects the first.

Resolve the duplicate naming in the repository before relying on consistent
behavior across case-sensitive and case-insensitive filesystems.

## TouchGFX

For Steering-Wheel, review both CubeMX and TouchGFX changes.

Verify that application sources, generated assets, target integration, and
required libraries are present and included in the build. Peripheral
initialization alone does not complete the graphics application.
