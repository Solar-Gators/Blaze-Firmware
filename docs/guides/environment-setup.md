# Environment Setup

This guide covers the CMake and Arm GCC development environment for Blaze.

All seven firmware projects are enabled in the repository build. Programming
and hardware validation require additional board-specific setup.

## Required Tools

Install:

- Git
- Arm GNU Toolchain targeting `arm-none-eabi`
- CMake 3.25 or newer
- Ninja
- clang-format 17

Optional development tools:

- CLion
- STM32CubeMX for peripheral and RTOS configuration
- TouchGFX Designer for steering-wheel GUI development
- A compatible debug probe and debug server

Use the STM32 firmware package recorded in the subsystem's `.ioc` file.
The steering-wheel project currently references TouchGFX Generator 4.26.1.

## Verify the Command-Line Tools

Make the tools available on `PATH`, then open a new terminal:

```sh
git --version
cmake --version
ninja --version
arm-none-eabi-gcc --version
arm-none-eabi-g++ --version
arm-none-eabi-objcopy --version
arm-none-eabi-size --version
clang-format --version
```

Check that clang-format reports major version 17.

On systems that install a versioned executable, use `clang-format-17`
instead of `clang-format`.

CMake presets use schema version 6, which requires CMake 3.25 or newer.

## Get the Repository

```sh
git clone https://github.com/Solar-Gators/Blaze-Firmware.git
cd Blaze-Firmware
git switch -c your-name/your-feature
```

If you already have a checkout, use it and select the team's intended base
branch before creating your feature branch.

## Configure CLion

1. Open the repository root containing `CMakePresets.json`.
2. Open CLion's toolchain settings under Build, Execution, Deployment.
3. Select available CMake and Ninja executables.
4. Ensure the Arm GNU Toolchain is available in CLion's environment.
5. Use the profiles imported from `CMakePresets.json`.
6. Reload CMake.

The repository toolchain file selects `arm-none-eabi-gcc` and
`arm-none-eabi-g++`. Ensure the IDE resolves those programs to the intended
Arm toolchain.

Expected preset settings:

| Preset | Generator | Build type | Build directory |
|---|---|---|---|
| `debug` | Ninja | Debug | `build/debug/` |
| `release` | Ninja | Release | `build/release/` |

Both presets use:

```text
resources/cmake/arm-none-eabi-gcc.cmake
```

Select a firmware target such as `Front-VCU.elf` and build it.

Do not open a subsystem directory as a standalone CMake project. Its build
depends on the repository's root configuration.

## Configure Formatting in CLion

Enable ClangFormat integration in CLion's C/C++ code-style settings and use
the repository's `.clang-format`.

Check the actual formatter executable/version used by your CLion installation.
Installing clang-format 17 on PATH does not necessarily change an IDE's
bundled formatter.

Use the Reformat Code action from the Code menu. Its shortcut depends on
your operating system and keymap.

To verify editor output against the CI formatter:

```sh
clang-format --style=file --dry-run --Werror firmware/Front-VCU/user/Src/user_threads.cpp
```

To format that file:

```sh
clang-format --style=file -i firmware/Front-VCU/user/Src/user_threads.cpp
```

Format project-owned files. The current CMake `format` target also includes
generated and vendor files, so avoid it for routine application changes.

## Terminal Builds

Run from the repository root:

```sh
cmake --preset debug
cmake --build --preset debug --target all
```

For Release:

```sh
cmake --preset release
cmake --build --preset release --target all
```

To build one target:

```sh
cmake --build --preset debug --target Front-VCU.elf
```

After adding source files or regenerating code, rerun configuration:

```sh
cmake --preset debug
```

Then rebuild the affected target.

## Build Outputs

Outputs are generated locally under:

```text
build/<preset>/firmware/<subsystem>/
```

For example:

```text
build/debug/firmware/Front-VCU/Front-VCU.elf
build/debug/firmware/Front-VCU/Front-VCU.bin
build/debug/firmware/Front-VCU/Front-VCU.hex
build/debug/firmware/Front-VCU/Front-VCU.map
```

Do not commit generated build directories.

## Flashing and Debugging

Use a probe and debug server that support the board's MCU.

Configure:

- Exact target device
- Debug interface and connection speed
- Reset behavior
- Matching firmware ELF
- Required probe/server software

These settings are board-specific. Do not copy another subsystem's device
selection without checking it.

In CLion, use the embedded debug-server/profile configuration supported by
your installed version.

A validated programming procedure is not yet documented for every subsystem.
Record a working setup with the board revision, probe, server version,
connection settings, and firmware commit.

## Troubleshooting

| Symptom | Check |
|---|---|
| Arm compiler not found | Tool installation, PATH, and IDE environment |
| Preset rejected | CMake is at least 3.25 |
| Ninja not found | Installation and selected build tool |
| Terminal builds but IDE does not | Tool versions, environment, and selected preset |
| New source not compiled | Rerun CMake and check source patterns |
| Undefined user-thread function | Declaration, definition, C linkage, and source inclusion |
| clang-format fails to run | Executable path, version, and `.clang-format` parsing |
| Firmware builds but does not flash | Probe, MCU, connection, and image selection |

When requesting help, include the branch, commit, target, preset, tool
versions, exact command, and first meaningful error.

See the root README for known repository integration issues.
