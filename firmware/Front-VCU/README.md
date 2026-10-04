# Front Vehicle Control Unit (Front-VCU)

## Hardware Overview
* **Target MCU:** STM32G483RET6
* **Board Revision:** Rev 1.0
* **Schematics:** [Link to Schematics / Altium Repository]

---

## Functional Responsibilities
* Dual-channel accelerator pedal position sensor (APPS) sampling and plausibility checks.
* Brake pressure sensor readings & brake light actuation.
* Front lights, turn signals, and horn driver controls.
* Primary CAN bus frame transmission and routing.

---

## STM32CubeMX Rules & Workflow
When updating MCU peripherals or pin definitions in CubeMX:
* **DO NOT** edit files inside `CubeMX/` manually (e.g., `startup_stm32g483xx.s` or `STM32G483XX_FLASH.ld`).
* Edit code only between CubeMX user code tags inside `Core/Src/` and `Core/Inc/`:
  ```c
  /* USER CODE BEGIN 0 */

  /* USER CODE END 0 */