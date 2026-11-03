# Change Validation and Review (I wouldnt follow this just yet)

Use this guide to document what changed and what was verified.

## Change Description

Describe:

- Problem or requirement
- Affected subsystem
- Resulting behavior
- Validation performed
- Known limitations

Keep the description proportional to the change.

## Build Validation

For firmware or build changes, record:

- Commit and local modifications
- Target and preset
- Compiler and CMake versions
- Build command
- Result and relevant warnings

Example:

```sh
cmake --preset debug
cmake --build --preset debug --target Front-VCU.elf
```

Use Release validation when optimization may affect behavior.

For shared build changes, build all targets:

```sh
cmake --preset debug
cmake --build --preset debug --target all
cmake --preset release
cmake --build --preset release --target all
```

CI checks project-owned formatting and both complete builds. These checks
do not replace functional or hardware tests.

## Formatting Validation

Use clang-format 17.

For each changed project-owned file:

```sh
clang-format --style=file --dry-run --Werror path/to/file.cpp
```

Inspect the diff after applying formatting.

## Functional Validation

Choose checks that exercise the changed behavior.

Examples:

- Known inputs and expected conversion results
- Boundary and invalid-input cases
- CAN transmit and receive behavior
- Task startup, timing, and blocking
- Fault detection and recovery
- Reset and restart behavior
- GUI updates and input handling

Use host-side tests for logic that does not require hardware.
Use board tests for peripheral and electrical behavior.

## New or Changed Threads

Verify:

- The generated thread exists and is created successfully.
- Its entry function reaches the intended user implementation.
- Its stack and priority are suitable for the work.
- It blocks or delays appropriately.
- Shared data access is synchronized.
- Regeneration preserves the connection to user code.

## TouchGFX Changes

For GUI-only changes, test the relevant screens and interactions in the
simulator when available.

For target changes, also verify on hardware:

- Rendering task startup
- Display output
- Memory configuration
- Input handling
- Data transfer between application tasks and the GUI

Simulator success does not verify the MCU's display or touch integration.

## Hardware Test Record

| Field | Result |
|---|---|
| Date and tester | |
| Subsystem | |
| Board revision | |
| Firmware commit | |
| Local modifications | |
| Compiler and preset | |
| Probe and debug server | |
| Test equipment and connections | |
| Procedure | |
| Expected behavior | |
| Observed behavior | |
| Evidence or log location | |
| Remaining issues | |

Write `Unknown` for unavailable information.

## Generated-Code Review

Check changes to:

- MCU and pin configuration
- Clocks and peripheral timing
- Interrupt priorities
- RTOS configuration
- Compiler-specific middleware ports
- Startup and linker files
- Application-owned code preservation
- Vendor files and middleware versions

Record intentional package migrations explicitly.

## Communication Changes

Document:

- Bus or interface
- Identifier and frame format
- Payload length and field layout
- Byte order
- Units and scaling
- Transmission trigger or period
- Timeout and invalid-data behavior
- Compatibility with existing receivers

## Documentation-Only Changes

Check paths, commands, links, and consistency with the source.

Do not describe source inspection as a successful build or hardware test.

## Review Checklist

- [ ] The change addresses the stated requirement.
- [ ] Application and generated changes are identifiable.
- [ ] New source files enter the intended target.
- [ ] Relevant checks have recorded results.
- [ ] Unperformed checks are listed.
- [ ] Documentation matches the implementation.
- [ ] No unrelated build outputs or local configuration were added.
