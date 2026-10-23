# Blaze Firmware

Embedded firmware for **Blaze**, the UF Solar Gators' single-occupant solar car
for the Formula Sun Grand Prix (FSGP) 2027.

## Project Status

This repository contains subsystem firmware projects, STM32CubeMX
configurations, bundled vendor libraries, and a shared CMake build structure.

Development is in progress. Most application code currently consists of
generated peripheral initialization and placeholder application loops or RTOS
tasks. The repository does not yet provide a complete, buildable vehicle
firmware release.

## Subsystems

Hardware listed below comes from the checked-in CubeMX configurations. It does
not establish the installed board revision.

| Subsystem | Configured MCU | Current contents |
|---|---|---|
| [Front VCU](firmware/Front-VCU/README.md) | STM32G483RET3 | Peripheral initialization, FreeRTOS/CMSIS-RTOS2 setup, and GCC startup/linker files |
| Rear VCU | Not yet defined by a checked-in CubeMX project | CMake scaffold only; disabled in the top-level build |
| [Steering wheel](firmware/Steering-Wheel/README.md) | STM32U5G9NJH6Q | Peripheral initialization, FreeRTOS setup, partial TouchGFX integration, and an IAR project |
| Telemetry | Not yet defined by a checked-in CubeMX project | CMake scaffold only; disabled in the top-level build |
| BMS motherboard | STM32G483RET3 | Peripheral initialization and an IAR project |
| BMS daughterboard | STM32L433CCT6 | Peripheral initialization and an IAR project |
| Powerboard | STM32L433CCT6 | Peripheral initialization and an IAR project |

The firmware spans STM32G4, STM32L4, and STM32U5 devices. MCU architecture, clock
configuration, and middleware vary by subsystem.

## Repository Layout

```text
Blaze-Firmware/
├── docs/
│   ├── guides/                   # Development environment and workflow guides
│   ├── hardware/                 # Placeholder for hardware documentation
│   └── standards/                # Development standards
├── firmware/
│   ├── BMS-Daughterboard/
│   ├── BMS-Motherboard/
│   ├── Front-VCU/
│   ├── Powerboard/
│   ├── Rear-VCU/
│   ├── Steering-Wheel/
│   └── Telemetry/
├── resources/
│   ├── SolarGators_drivers/
│   │   ├── Inc/                  # Shared driver headers
│   │   └── Src/                  # Shared driver sources
│   ├── cmake/                    # Arm GCC toolchain configuration
│   └── CMakeLists.txt
├── .clang-format
├── CMakeLists.txt
├── CMakePresets.json
└── LICENSE
```

Subsystem directories contain source code and project configuration.

CMake creates build outputs locally under `build/debug/` or `build/release/`
at the repository root. These directories are generated during configuration
and building.

## Development Tools

For the intended CMake workflow:

- Git
- Arm GNU Toolchain providing `arm-none-eabi-gcc`, `arm-none-eabi-g++`,
  `arm-none-eabi-objcopy`, and `arm-none-eabi-size`
- CMake **3.25 or newer**
- Ninja
- STM32CubeMX when modifying peripheral configuration or regenerating code
- `clang-format` when formatting source code

The checked-in presets use schema version 6, which requires CMake 3.25 or newer.
See the [CMake preset format documentation](https://cmake.org/cmake/help/v3.25/manual/cmake-presets.7.html#format).
The lower version declarations in the repository do not override this requirement.

The toolchain file resolves Arm tools by name, so their installation directory
must be available on `PATH`.

Setup guides:

- [Environment setup](docs/guides/environment-setup.md)
- [CLion setup](docs/guides/clion-setup.md)

Programming and debugging also require a compatible probe and a configuration
matching the target board.

## Build Configuration

Run CMake from the repository root.

The repository defines `debug` and `release` configure/build presets, with
output directories `build/debug/` and `build/release/`.

The following commands describe the intended workflow. Build integration remains
incomplete; consult the known blockers below and the relevant subsystem README.

### Debug

```sh
cmake --preset debug
cmake --build --preset debug --target Front-VCU.elf
```

### Release

```sh
cmake --preset release
cmake --build --preset release --target Front-VCU.elf
```

The target definitions include post-build commands to produce `.bin` and `.hex`
files alongside each `.elf` file.

For example, a successful debug build of the front VCU would place these files
under:

```text
build/debug/firmware/Front-VCU/
```

Selecting a specific build target does not prevent CMake from processing other
enabled subsystems during configuration.

### Known Build Blockers

- **BMS and powerboard:** Their startup assembly and linker configurations are
  supplied under `EWARM/`. The CMake targets instead search for GCC startup files
  and `.ld` linker scripts in the subsystem root or `CubeMX/`.
- **Steering wheel:** GCC startup/linker files and parts of the TouchGFX application
  are missing. The CMake FreeRTOS port path also differs from the supplied IAR port.
- **Shared drivers:** `resources/CMakeLists.txt` searches `src/` and `Drivers/`,
  while the checked-in files are under `SolarGators_drivers/Src/` and
  `SolarGators_drivers/Inc/`. The current build creates a dummy library source.
- **Mixed MCU architectures:** Global and shared-library options select Cortex-M4,
  while the steering-wheel target selects Cortex-M33. Build settings and shared
  code need to account for the different targets.

Rear VCU and Telemetry are currently excluded from the top-level build.
Each directory contains only a CMake scaffold. Enabling either project before
adding its required sources and startup files will trigger a configuration error.

## STM32CubeMX Workflow

1. Open the relevant subsystem's `.ioc` file.
2. Check its MCU, firmware package, and toolchain selection before generating code.
3. Make peripheral and pin changes in CubeMX.
4. Keep custom edits to regenerated files inside `USER CODE` sections.
5. Review generated changes, including startup files, linker scripts, and middleware.
6. Check that the subsystem's CMake configuration matches the generated files.

Several projects currently generate IAR files, while the shared build uses GCC.
Those project formats are not interchangeable.

Front-VCU keeps application-owned headers and implementation under `user/Inc/`
and `user/Src/`. Other subsystems may require CMake updates before adopting
the same layout.

## Formatting

The repository provides a `.clang-format` configuration.

When CMake finds `clang-format`, it defines a `format` target. That target formats
C/C++ sources and headers recursively under both `firmware/` and `resources/`,
including bundled vendor code.

For focused changes, format only the files you edited and review the resulting
diff.

## License

See [LICENSE](LICENSE) for the repository's MIT license.

Bundled vendor libraries and middleware retain their own license terms and
notices.
