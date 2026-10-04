# Environment Setup Guide

This guide details setting up your development environment for compiling and flashing **Blaze-Firmware**.

## Toolchain Requirements
* **Toolchain:** `arm-none-eabi-gcc` (v14.0+)
* **Build System:** `CMake` (v3.20+) and `Ninja`
* **Linter/Formatter:** `clang-format`
* **Debugger/Programmer:** `OpenOCD` (with ST-Link support, lowkey dont know if this is true) 

---

## Linux (Ubuntu / Debian) (ts is skeleton, i havent filled it out yet)

1. **Install Build Tools & Dependencies:**
   ```bash
   sudo apt update
   sudo apt install -y cmake ninja-build clang-format openocd git