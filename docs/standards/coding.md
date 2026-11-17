# Firmware Coding Standard

Status: proposed conventions for project-owned firmware code.

Formatting settings below reflect the existing `.clang-format`. The remaining
rules are proposed development conventions, not a claim that all existing code
already follows them.

## Keep Changes Focused

- Reuse existing code before adding another implementation.
- Prefer straightforward functions and explicit data flow.
- Add abstractions only when current code needs them.
- Avoid unrelated refactoring or formatting in functional changes.
- Follow established naming within the module being changed.

## Formatting

Use the repository's `.clang-format` configuration.

Its principal settings include:

- Four-space indentation
- Spaces instead of tabs
- A 100-column limit
- Allman-style braces
- Left-aligned pointer markers
- Automatic include sorting

Format project-owned files that changed. Preserve vendor notices and avoid
reformatting bundled libraries as part of application work.

A validated clang-format version still needs to be recorded.

## Generated and Application-Owned Code

Keep custom edits to regenerated files inside `USER CODE` sections.

Prefer application-owned files for substantial application logic. Front-VCU
currently uses:

```text
user/Inc/
user/Src/
```

If another subsystem adopts that layout, update its CMake configuration in the
same change.

Document any required manual changes to generated startup or linker files.

## Headers and Language Boundaries

- Use include guards or the module's existing equivalent.
- Include the declarations a file directly depends on.
- Keep internal declarations private where practical.
- Use C linkage for functions called from generated C and implemented in C++.
- Keep declarations and definitions consistent.

The existing `user_threads.h` demonstrates the C/C++ linkage pattern.

## Types, Units, and Protocol Data

- Use explicit-width integer types where width affects hardware or wire formats.
- Name physical values so their units are clear.
- Document scaling, valid ranges, and invalid-value representations.
- Encode and decode wire data explicitly; do not assume a C structure's memory
  layout is the protocol layout.
- Check lengths and ranges before consuming received data.

Do not invent CAN identifiers, timing requirements, or conversion factors.
Document them when they are defined and verified.

## RTOS and Interrupt Code

For each application task, document its responsibility, trigger or period,
priority, and stack allocation.

- Use blocking waits or delays when a task has no work.
- Keep interrupt handlers short.
- Use APIs appropriate to interrupt context.
- Define ownership and synchronization for shared state.
- Check task, queue, and other resource-creation results.
- State the units used for timeouts and stack sizes.

Use the subsystem's configured RTOS interface consistently.

## Errors and Watchdogs

- Handle return values where failure affects behavior.
- Define the response to stale data, failed initialization, and communication loss.
- Make fault recovery explicit.
- Service a watchdog according to demonstrated application health.
- Document enabled failure hooks that remain unimplemented.

Fault thresholds and recovery policies are subsystem requirements; this document
does not assign them.

## Build Integration

When adding or moving files:

1. Update the affected target's sources and include paths.
2. Verify MCU definitions and compiler options.
3. Verify startup, linker, and middleware-port compatibility.
4. Reconfigure and build the affected target.

Do not introduce developer-specific absolute paths into shared build files.

Avoid filenames that differ only by case.

## Documentation

Update the subsystem README when changing hardware configuration, task
structure, build inputs, or implemented behavior.

Distinguish:

- Configured in CubeMX
- Implemented in application code
- Built successfully
- Verified on hardware

These are separate states.
