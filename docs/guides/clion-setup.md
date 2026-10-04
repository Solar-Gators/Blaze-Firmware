# CLion Configuration Guide (FOLLOW THIS AT YOUR OWN PERIL, I HAVENT CHECKED IT. THIS IS MORE A PLACEHOLDER FILE)

CLion natively supports CMake and embedded debugging via ST-Link/OpenOCD.

## 1. Configure Toolchain
1. Open **Settings / Preferences** $\rightarrow$ **Build, Execution, Deployment** $\rightarrow$ **Toolchains**.
2. Click **+** to add a new **System** toolchain.
3. Set the C Compiler: `/opt/arm-gnu-toolchain-14.2.rel1-x86_64-arm-none-eabi/bin/arm-none-eabi-gcc`
4. Set the C++ Compiler: `/opt/arm-gnu-toolchain-14.2.rel1-x86_64-arm-none-eabi/bin/arm-none-eabi-g++`
5. Set the Debugger: `/opt/arm-gnu-toolchain-14.2.rel1-x86_64-arm-none-eabi/bin/arm-none-eabi-gdb`

## 2. Configure CMake Profile
1. Go to **Settings** $\rightarrow$ **Build, Execution, Deployment** $\rightarrow$ **CMake**.
2. Set **Build type** to `Debug`.
3. Set **Generator** to `Ninja`.
4. Verify CMake correctly detects `resources/cmake/arm-none-eabi-gcc.cmake`.

## 3. Configure OpenOCD Flashing & Debugging
1. Go to **Run** $\rightarrow$ **Edit Configurations...**
2. Click **+** and select **OpenOCD Download & Run**.
3. Target: `Front-VCU.elf` (or equivalent subsystem).
4. Board Config file: Select or create an OpenOCD board file:
   ```text
   interface/stlink.cfg
   target/stm32g4x.cfg