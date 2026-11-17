# Change Validation and Review

Status: proposed conventions for documenting and reviewing firmware changes.

## Minimum Change Description

Describe:

- The problem or requirement
- The affected subsystem
- The resulting behavior
- Validation performed
- Known limitations or untested behavior

Keep the description proportional to the change.

## Build Validation

For executable-code or build-configuration changes, record:

- Commit and local modifications
- Target and build preset
- Compiler and CMake versions
- Exact build command
- Result and relevant warnings

If existing repository issues prevent a build, name the blocker and state that
the change remains unbuilt. Do not describe source inspection as a passing build.

For documentation-only changes, check paths, commands, links, and consistency
with the source.

## Functional Validation

Choose checks that exercise the changed behavior.

Examples include:

- Known inputs and expected outputs for conversion or parsing logic
- Boundary and invalid-input cases
- CAN transmit/receive observations
- Task startup and timing behavior
- Fault detection and recovery
- Persistence across reset, when relevant

Use host-side tests for logic that does not require hardware. Use board tests
for behavior that depends on peripherals or electrical interfaces.

Passing a host test does not establish correct hardware behavior.

## Hardware Test Record

Use this template when reporting a board test:

| Field | Result |
|---|---|
| Date and tester | |
| Subsystem | |
| Board revision | |
| Firmware commit | |
| Local modifications | |
| Compiler and build preset | |
| Test equipment and connections | |
| Procedure | |
| Expected behavior | |
| Observed behavior | |
| Evidence or log location | |
| Remaining issues | |

If a value is unknown, write `Unknown`.

## Generated-Code Review

For CubeMX or middleware changes, check:

- MCU and pin configuration
- Clock sources and derived timing
- Interrupt configuration
- RTOS configuration
- Compiler-specific middleware ports
- Startup and linker files
- Preservation of application-owned code
- Unintended vendor-file changes

Record firmware-package migrations separately from application changes.

## Communication Changes

When adding or changing a message, document:

- Bus or interface
- Identifier and frame format
- Payload length
- Field positions and byte order
- Units, scaling, and valid ranges
- Transmission trigger or period
- Timeout and invalid-data behavior
- Compatibility with existing senders and receivers

Values remain unspecified until the protocol is defined.

## Review Checklist

- [ ] The change addresses the stated problem.
- [ ] Implementation and documentation agree.
- [ ] Generated and application-owned changes are identifiable.
- [ ] Relevant checks have recorded results.
- [ ] Unperformed checks are explicitly listed.
- [ ] No unrelated build products or local configuration were added.
- [ ] New files are included in the intended build target.
- [ ] Known failures are not presented as verified behavior.
