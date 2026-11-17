# Environment Setup Guide

This guide covers setting up **Blaze-Firmware** for development.

CLion is the recommended starting point for this guide. VS Code and terminal
builds are also supported by the repository's CMake configuration.

You need:

- **Arm GNU Toolchain** providing `arm-none-eabi-gcc` and `arm-none-eabi-g++`
- **CMake 3.25 or newer**
- **Ninja**
- **Git**
- A compatible debug probe and its software for flashing and debugging

Install **STM32CubeMX** if you will change MCU peripherals, clocks, or pin assignments.

> Some Blaze projects still have incomplete build integration. A correctly
> installed environment does not mean every firmware target will build.
> Check the relevant subsystem README for its current status.

---

## 1. Install the Development Tools

### CLion

Download and install [JetBrains CLion](https://www.jetbrains.com/clion/).

CLion includes CMake and Ninja options, so separate installations are not
necessary if you use the bundled tools through CLion. Verify that the selected
CMake version is at least 3.25.

### Arm GNU Toolchain

Download the [Arm GNU Toolchain](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads).

Choose the package for your computer's operating system and architecture,
with **`arm-none-eabi`** as the target.

During installation, enable the option to add the toolchain to `PATH` if offered.
Otherwise, add its `bin` directory manually.

Restart your terminal and CLion after changing `PATH`.

Check that the compiler is available:

```sh
arm-none-eabi-gcc --version
arm-none-eabi-g++ --version
arm-none-eabi-objcopy --version
arm-none-eabi-size --version
```

The repository's toolchain file finds these programs by name.

### Git

Install [Git](https://git-scm.com/downloads), then verify:

```sh
git --version
```

### STM32CubeMX

Install [STM32CubeMX](https://www.st.com/en/development-tools/stm32cubemx.html)
when you need to edit a project's `.ioc` configuration.

Use the firmware package recorded in that project's `.ioc` file. Review
migration prompts before upgrading a project to another package version.

---

## 2. Clone the Repository

Open a terminal and run:

```sh
git clone https://github.com/Solar-Gators/Blaze-Firmware.git
cd Blaze-Firmware
```

To use the repository-setup branch:

```sh
git switch repo-setup
```

Before making changes, create your own branch:

```sh
git switch -c your-name/your-feature
```

If you already have a checkout, use it instead of cloning another copy.

---

## 3. Open and Configure CLion

1. Open the **Blaze-Firmware repository root** in CLion.
2. Go to **Settings > Build, Execution, Deployment > Toolchains**.
3. Verify that CMake and Ninja are available.
4. Set the C compiler to `arm-none-eabi-gcc`.
5. Set the C++ compiler to `arm-none-eabi-g++`.
6. Ensure the Arm toolchain's `bin` directory is available in CLion's environment.

Use your own installation paths if CLion does not find the programs automatically.

See the [CLion toolchain documentation](https://www.jetbrains.com/help/clion/how-to-create-toolchain-in-clion.html)
for platform-specific setup.

### Select the Repository Presets

Go to **Settings > Build, Execution, Deployment > CMake** and use the profiles
imported from `CMakePresets.json`.

The repository defines:

| Preset name | Display name | Output directory |
|---|---|---|
| `debug` | GCC ARM Embedded (Debug) | `build/debug/` |
| `release` | GCC ARM Embedded (Release) | `build/release/` |

CLion's displayed profile labels may vary by version. Identify the profiles
by their preset settings rather than relying on labels such as
`debug - debug`.

Verify that the profile uses:

- **Generator:** Ninja
- **Toolchain file:** `resources/cmake/arm-none-eabi-gcc.cmake`
- **Build type:** Debug or Release
- **Build directory:** The matching directory from the table above

See the [CLion CMake profile documentation](https://www.jetbrains.com/help/clion/cmake-profile.html).

### Build a Firmware Target

1. Reload CMake.
2. Select an enabled target, such as `Front-VCU.elf`.
3. Click the build button.
4. Read the first meaningful error if the build fails.

Open the repository root, not an individual firmware folder. The subsystem
targets depend on the root CMake configuration and shared resources.

---

## 4. Flashing and Debugging in CLion

Flashing requires a debug probe connected to the target board and software
that supports both the probe and MCU.

Blaze does not yet document a validated probe configuration for every board.
The following describes the setup process; the device and connection settings
must match your hardware.

### If You Use J-Link

1. Install the [SEGGER J-Link Software and Documentation Pack](https://www.segger.com/downloads/jlink/).
2. Connect the J-Link probe to the board's documented debug connector.
3. Configure a SEGGER J-Link debug profile/server in CLion.
4. Select the exact target MCU from the supported device list.
5. Set the interface, connection speed, and reset behavior for the board.
6. Select the firmware target and its matching ELF file.

Do not copy Flare's `STM32U575RG` device setting into a Blaze configuration.
Blaze projects target different MCUs.

### CLion Version Differences

In **CLion 2026.2**, open:

**Settings > Build, Execution, Deployment > Debugger > Debug Profiles**

Add the profile matching your probe and configure its server, device, and
debugger settings.

Earlier versions use a different Debug Servers interface. Follow the
[JetBrains debug-server instructions](https://www.jetbrains.com/help/clion/debug-servers.html)
for your installed version.

### Other Probes

If using ST-LINK or another probe, use its compatible debug server and the
correct MCU configuration.

The repository does not establish that OpenOCD is required, or that one
OpenOCD configuration works for every Blaze board.

### Start a Session

After obtaining a successful build and configuring the probe:

1. Select the firmware run/debug configuration.
2. Select the matching embedded debug profile.
3. Set a breakpoint.
4. Start debugging.
5. Confirm that programming completes and execution reaches the expected code.

Record the working probe, server version, MCU selection, connection settings,
and board revision in the subsystem documentation.

---

## Alternative: VS Code

You can use the same Arm compiler and CMake presets with VS Code.

### Install the Tools

Install:

- [Visual Studio Code](https://code.visualstudio.com/)
- Microsoft's **C/C++** extension
- Microsoft's **CMake Tools** extension
- [CMake](https://cmake.org/download/) 3.25 or newer
- [Ninja](https://ninja-build.org/)
- Git and the Arm GNU Toolchain described above

Make sure these tools are available in the environment used by VS Code:

```sh
cmake --version
ninja --version
arm-none-eabi-gcc --version
arm-none-eabi-g++ --version
```

### Configure and Build

1. Open the **repository root** in VS Code.
2. Use CMake Tools to select the existing `debug` configure preset.
3. Configure the project.
4. Select the matching build preset and an enabled target.
5. Build the target.

Use the existing `CMakeLists.txt` and `CMakePresets.json`; do not generate a
replacement project with CMake Quick Start.

For IntelliSense, select CMake Tools as the C/C++ configuration provider when
prompted. This lets editor configuration follow the build's include paths and
compiler settings.

Flashing and debugging require a separately configured embedded debugger.
A Blaze-specific VS Code debug configuration is not supplied by this guide.

---

## Alternative: Terminal Builds

Run all commands below from the repository root.

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

Replace `Front-VCU.elf` with the enabled target you want to build.

Selecting one target does not prevent CMake from processing the other enabled
projects during configuration.

### Clean and Rebuild a Target

```sh
cmake --build --preset debug --target Front-VCU.elf --clean-first
```

After adding source files or regenerating CubeMX code, rerun configuration:

```sh
cmake --preset debug
```

Then rebuild the affected target.

Blaze's documented workflow uses root-level CMake presets. Do not assume the
per-project `make` commands from Flare apply here.

---

## Where Build Outputs Go

The presets create build directories inside your local repository:

```text
Blaze-Firmware/
├── firmware/
├── resources/
├── build/
│   ├── debug/
│   └── release/
├── CMakeLists.txt
└── CMakePresets.json
```

For example, a successful Front-VCU debug build produces files under:

```text
build/debug/firmware/Front-VCU/
```

The target defines these outputs:

```text
Front-VCU.elf
Front-VCU.bin
Front-VCU.hex
Front-VCU.map
```

The build directory is generated locally; it does not need to exist on GitHub.

---

## Working on Application Code

Front-VCU currently separates generated and application-owned code:

```text
Front-VCU/
├── Core/              # Generated startup, peripheral setup, and RTOS entry points
├── CubeMX/            # Generated build support, startup assembly, and linker files
├── Drivers/           # Bundled STM32 and CMSIS code
├── Middlewares/       # Bundled middleware
├── user/
│   ├── Inc/           # Application-owned headers
│   └── Src/           # Application-owned implementation
├── CMakeLists.txt
└── Front-VCU.ioc
```

Keep substantial application logic in application-owned files.

When editing regenerated `Core/` files, place custom changes inside the matching
`USER CODE` sections.

Other subsystems do not necessarily have a `user/` directory yet. Adding one
also requires updating that subsystem's CMake sources and include paths.

---

## Formatting

The repository's `.clang-format` defines the formatting style.

Install a compatible `clang-format` version and configure your editor to use
the repository configuration.

The CMake `format` target recursively includes generated and vendor sources.
For ordinary changes, format only the project-owned files you edited.

Blaze does not currently include Flare's pre-commit configuration. Do not
assume formatting runs automatically when you commit.

---

## Common Problems

| Problem | What to check |
|---|---|
| `arm-none-eabi-gcc` is not found | Install the Arm toolchain, update `PATH`, and restart the IDE |
| CMake rejects the presets | Use CMake 3.25 or newer |
| Ninja is not found | Check the selected build tool and its installation |
| Terminal builds work but IDE builds fail | Compare tool paths and environments |
| Startup assembly or linker script is missing | Check the subsystem's generated files and toolchain |
| New source file is not compiled | Reload or rerun CMake |
| Firmware builds but will not flash | Verify the probe, MCU selection, connection, and image |
| A target is missing | Check whether it is enabled in the top-level CMake file |

Some projects still contain IAR-generated files while their CMake targets expect
GCC inputs. Steering-Wheel also has incomplete middleware and graphics build
integration. These are repository issues, not necessarily installation errors.

The reviewed branch also contains two Front-VCU linker filenames differing only
by case. This needs resolving for consistent behavior on case-insensitive
filesystems.

When asking for help, include:

- Operating system
- Branch and commit
- Firmware target
- Tool versions
- Command or IDE action
- First meaningful error message
