# Blaze Firmware

Embedded firmware for **Blaze**, the UF Solar Gators' single-occupant solar car
for the Formula Sun Grand Prix (FSGP) 2027.

## Project Status

All seven subsystem projects are enabled in the top-level CMake build.
GitHub Actions checks project-owned source formatting and builds the Debug
and Release presets.

Application development is ongoing. Most application tasks currently contain
placeholder loops. A successful build does not establish working vehicle
control, communication, sensing, or display behavior.

## Subsystems

The MCU selections below come from the checked-in CubeMX configurations.
They do not establish the installed board revision.

| Subsystem | Configured MCU | CMake target |
|---|---|---|
| [Front VCU](firmware/Front-VCU/README.md) | STM32G483RET3 | `Front-VCU.elf` |
| [Rear VCU](firmware/Rear-VCU/README.md) | STM32G483RET6 | `Rear-VCU.elf` |
| [Steering wheel](firmware/Steering-Wheel/README.md) | STM32U5G9NJH6Q | `Steering-Wheel.elf` |
| [Telemetry](firmware/Telemetry/README.md) | STM32G483RET3 | `Telemetry.elf` |
| [BMS motherboard](firmware/BMS-Motherboard/README.md) | STM32G483RET3 | `BMS-Motherboard.elf` |
| [BMS daughterboard](firmware/BMS-Daughterboard/README.md) | STM32L433CCT6 | `BMS-Daughterboard.elf` |
| [Powerboard](firmware/Powerboard/README.md) | STM32L433CCT6 | `Powerboard.elf` |

## Repository Layout

```text
Blaze-Firmware/
├── .github/workflows/           # Continuous integration
├── docs/
│   ├── guides/                 # Setup and development procedures
│   ├── hardware/               # Hardware documentation
│   └── standards/              # Coding and validation conventions
├── firmware/                   # Subsystem firmware projects
├── resources/
│   ├── SolarGators_drivers/
│   │   ├── Inc/                # Shared driver headers
│   │   └── Src/                # Shared driver implementations
│   ├── cmake/                  # Arm GCC toolchain configuration
│   └── CMakeLists.txt
├── .clang-format
├── CMakeLists.txt
└── CMakePresets.json
```

Each subsystem has application-owned files under `user/Inc/` and `user/Src/`.
Generated peripheral initialization and RTOS entry points are under `Core/`.

## Development Tools

- Git
- Arm GNU Toolchain for `arm-none-eabi`
- CMake 3.25 or newer
- Ninja
- clang-format 17
- STM32CubeMX for peripheral configuration and code generation
- TouchGFX Designer for steering-wheel GUI development
- A compatible probe and debug server for programming hardware

CLion setup is included in the
[environment setup guide](docs/guides/environment-setup.md).

## Building

Run commands from the repository root.

### Debug

```sh
cmake --preset debug
cmake --build --preset debug --target all
```

### Release

```sh
cmake --preset release
cmake --build --preset release --target all
```

To build one subsystem:

```sh
cmake --build --preset debug --target Front-VCU.elf
```

Replace the target with one from the subsystem table.

Selecting one build target does not prevent CMake from configuring the other
enabled projects.

## Build Outputs

Build outputs are generated locally under:

```text
build/debug/firmware/<subsystem>/
build/release/firmware/<subsystem>/
```

Each firmware target produces an ELF file, a binary image, an Intel HEX image,
and a linker map.

Do not commit local build outputs.

## Application Development

Keep application logic under the subsystem's `user/` directory.

When modifying generated files, place custom code inside the matching
`USER CODE BEGIN` and `USER CODE END` sections.

After adding source files, rerun CMake configuration before building.

See:

- [CubeMX workflow](docs/guides/cubemx-workflow.md)
- [Adding application threads](docs/guides/adding-threads.md)
- [TouchGFX workflow](docs/guides/touchgfx-workflow.md)
- [Coding standard](docs/standards/coding.md)
- [Change validation](docs/standards/firmware-changes-validation.md)

## Formatting and CI

Use clang-format 17 with the repository's `.clang-format`.

CI checks tracked C/C++ files under:

- `firmware/*/user/`
- `resources/SolarGators_drivers/`

CI also configures and builds all enabled firmware targets using both presets.

The existing CMake `format` target has a broader scope and includes generated
and vendor code. Until its scope is narrowed, format individual project-owned
files instead.

## License

See [LICENSE](LICENSE) for the repository license.

Bundled libraries and middleware retain their own license terms and notices.
