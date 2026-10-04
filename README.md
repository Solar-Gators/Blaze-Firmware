# Blaze-Firmware

> Firmware for **Blaze**, the UF Solar Gators' single-occupant solar car competing at the Formula Sun Grand Prix (FSGP) 2027.

---

## Overview

This repository houses the embedded firmware for all on-vehicle electronic control units (ECUs) on **Blaze**.

* **Microcontroller:** STM32G483xx (ARM Cortex-M4 with FPU @ 170 MHz)
* **Real-Time Operating System:** FreeRTOS (CMSIS-RTOS V2 API)
* **Toolchain:** `arm-none-eabi-gcc` + `CMake` + `Ninja`
* **Hardware Abstraction:** STM32G4 HAL / LL

---

## Directory Layout

```text
Blaze-Firmware/
├── docs/               # Technical standards, guides, pinouts, and maps
├── firmware/           # Subsystem application binaries
│   ├── Front-VCU/      # Front Vehicle Control Unit (Pedals, Front CAN, Lights)
│   ├── Rear-VCU/       # Rear Vehicle Control Unit (Motor controller, Battery)
│   ├── Steering-Wheel/ # Driver display & steering controls
│   └── Telemetry/      # Cellular/RF telemetry node
├── resources/          # Shared driver libraries and toolchain definitions
│   ├── cmake/          # Cross-compilation toolchain configurations
│   ├── inc/            # Common firmware headers
│   └── src/            # Central static driver implementations (SolarGators_drivers)
└── CMakeLists.txt      # Top-level build configuration
