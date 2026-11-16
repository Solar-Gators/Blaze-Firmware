# Firmware Coding Standard

Conventions for project-owned Blaze firmware code.

## Keep Changes Focused

- Reuse existing code before adding another implementation.
- Prefer straightforward functions and explicit data flow.
- Add abstractions when current requirements justify them.
- Keep unrelated formatting and refactoring out of functional changes.
- Follow the naming conventions of the module being changed.

## Formatting

Use clang-format 17 with the repository's `.clang-format`.

The configuration uses:

- Four-space indentation
- Spaces instead of tabs
- A 100-column limit
- Allman-style braces
- Left-aligned pointer markers
- Automatic include sorting

Format changed project-owned files. Preserve vendor notices and avoid
reformatting bundled libraries.

CI checks C/C++ files under `firmware/*/user/` and
`resources/SolarGators_drivers/`.

The CMake `format` target currently covers additional generated and vendor
files. Its scope does not match CI.

## Generated and Application-Owned Code

Keep application implementations under `user/Src/` and their headers under
`user/Inc/`.

Keep custom generated-file edits inside matching `USER CODE` sections.

For TouchGFX, extend application-owned GUI classes rather than editing
generated base classes.

Document manual changes to generated startup or linker files.

## Headers and C/C++ Boundaries

- Use include guards or the existing module convention.
- Include declarations directly required by a file.
- Keep internal declarations private where practical.
- Use C linkage for functions called from C and implemented in C++.
- Keep declaration and definition signatures identical.
- Do not include a `.cpp` file from another source file.

Use `user_threads.h` for generated task entry points that call user functions.

## RTOS Tasks

For each task, document:

- Responsibility
- Trigger or execution period
- Priority
- Stack size and units
- Allocation method
- Shared resources and synchronization
- Failure behavior

Use blocking waits or delays when a task has no work.

CMSIS-RTOS2 delays use kernel ticks. Do not describe `osDelay(1)` as one
millisecond without checking the configured tick frequency.

Keep one clear owner of a task's repeated-work loop. A user function that
owns an infinite loop should be called once by its generated wrapper.

Check resource-creation results where failure affects startup or behavior.
Use ISR-safe APIs from interrupt context.

## Types and Protocol Data

- Use explicit-width integer types where width affects hardware or protocols.
- Make physical units clear in names or documentation.
- Document scaling, valid ranges, and invalid-value representations.
- Encode and decode messages explicitly.
- Validate lengths and ranges before consuming received data.

Do not invent CAN identifiers, conversion factors, or fault thresholds.

## Errors and Watchdogs

- Handle errors that affect application behavior.
- Define responses to initialization failure, stale data, and communication loss.
- Service watchdogs according to demonstrated application health.
- Implement enabled failure hooks or explicitly document their limitations.
- Keep recovery behavior predictable.

## Build Integration

When adding or moving files:

1. Check target source and include paths.
2. Check MCU definitions and compiler options.
3. Check startup, linker, and middleware-port compatibility.
4. Rerun CMake.
5. Build the affected target.

Do not introduce developer-specific absolute paths.

Avoid filenames that differ only by case.

The shared-driver build configuration currently needs correction. Verify
that a shared source actually enters the intended build before relying on it.

## Documentation

Update the subsystem README when changing configuration, task structure,
build inputs, or implemented behavior.

Distinguish:

- Configured in CubeMX
- Implemented in application code
- Built successfully
- Verified on hardware

Record unknown hardware or protocol details as unknown.
