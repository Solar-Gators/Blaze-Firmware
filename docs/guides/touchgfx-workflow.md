# TouchGFX Workflow

This guide covers the existing Steering-Wheel TouchGFX project.

## Project Files

Open this project in TouchGFX Designer:

```text
firmware/Steering-Wheel/TouchGFX/Steering-Wheel.touchgfx
```

The CubeMX project references TouchGFX Generator 4.26.1.
Use a compatible Designer installation and treat version upgrades as explicit
changes.

The `.touchgfx.part` file is a generated integration/template fragment.
Use the `.touchgfx` file for the existing GUI project.

## Directory Responsibilities

Paths below are relative to `firmware/Steering-Wheel/`.

| Directory | Purpose |
|---|---|
| `TouchGFX/assets/` | Source assets such as fonts and images |
| `TouchGFX/generated/` | Designer-generated GUI code and assets |
| `TouchGFX/gui/` | Application-owned views, presenters, and model |
| `TouchGFX/App/` | Generated application entry points |
| `TouchGFX/target/` | Hardware integration and target overrides |
| `TouchGFX/target/generated/` | Generated target implementation |
| `TouchGFX/simulator/` | Desktop simulator support |
| `user/` | Non-GUI application logic and user tasks |

Check a file's generation notice before editing it.

## Current Integration Status

The firmware target includes TouchGFX application and generated sources,
target integration, and the required graphics libraries.

The current application initializes TouchGFX but does not create a thread
that runs `TouchGFX_Task()`.

The touch-controller implementation is a placeholder.

Display hardware, touch-controller hardware, and external-memory details
are not established by this guide.

## Editing a Screen

1. Preserve existing work.
2. Open `Steering-Wheel.touchgfx` in Designer.
3. Edit the screen layout or interactions.
4. Generate code.
5. Review changes to the project, generated code, and assets.
6. Add custom behavior in the corresponding application-owned GUI classes.
7. Run the simulator when available.
8. Reconfigure and build the firmware.

From the repository root:

```sh
cmake --preset debug
cmake --build --preset debug --target Steering-Wheel.elf
```

Do not hand-edit generated view base classes. Designer can overwrite them.

## Application Logic

Use the existing View, Presenter, and Model structure.

- Views handle screen presentation and UI interactions.
- Presenters connect a view to application-facing behavior.
- The Model provides the GUI-facing data and update path.

Keep peripheral access and lengthy blocking work out of rendering callbacks.

When another RTOS task supplies data, use a defined synchronization or
message-passing mechanism. Update widgets from the GUI execution context
rather than directly from unrelated tasks.

Document ownership and update timing for shared data.

## Rendering Task Integration

The generated entry point is:

```c
void TouchGFX_Task(void* argument);
```

A working RTOS integration needs a thread that reaches this entry point after
the required initialization.

Before adding it:

1. Check whether CubeMX/TouchGFX generation can create the graphics thread
   through the project's middleware configuration.
2. Ensure it will be created exactly once.
3. Choose stack and priority from the integration requirements and validate
   them on the target.
4. Verify graphics synchronization and interrupt configuration.
5. Confirm execution reaches the rendering task.

Do not call the graphics task repeatedly from a periodic application loop.
It is intended to run as a task entry point.

Do not reuse a small placeholder task stack without validating the graphics
workload.

## Hardware Integration

Before claiming working display or input support, verify:

- Display controller and panel configuration
- Display timing
- Framebuffer placement and memory capacity
- Required memory initialization
- Graphics interrupts and synchronization
- Touch-controller interface and coordinate mapping

The current `STM32TouchController::sampleTouch()` returns `false`.
Replace that placeholder only after the actual touch hardware and interface
are known.

## CubeMX Changes

Changes to clocks, display peripherals, memory, RTOS settings, or graphics
middleware may require CubeMX regeneration followed by Designer generation.

Review generated files after each step and confirm application-owned
changes survive.

Keep the TouchGFX framework, generated integration, and GPU library variants
compatible. Do not mix library variants simply to resolve a linker error.

## Simulator Versus Hardware

A simulator run can validate screen behavior and interactions.

It does not verify MCU startup, display timing, framebuffer memory,
graphics interrupts, touch hardware, or communication with other boards.

Record simulator and hardware results separately.

## Files to Commit

Commit the project inputs and generated sources/assets needed for a clean
checkout to build.

Exclude local build products and machine-specific state.

Do not remove the entire `generated/` directory as cleanup: the current
firmware CMake build consumes generated sources directly.

## Troubleshooting

| Symptom | Check |
|---|---|
| Screen changes do not appear in source | Generate code in Designer |
| Custom GUI edits disappear | Move behavior out of generated base classes |
| Firmware builds but display is blank | Rendering thread, initialization, timing, memory, and interrupts |
| Touch never registers | Placeholder touch controller and actual hardware interface |
| New GUI sources are not compiled | Rerun CMake and inspect source patterns |
| Linker reports missing graphics symbols | Compatible libraries, generated sources, and target configuration |
| Simulator works but hardware does not | Target-specific display, memory, input, and RTOS integration |
