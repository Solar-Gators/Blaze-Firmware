# Adding an Application Thread

This guide shows how to create a thread in CubeMX and connect its generated
C entry function to application code in `user_threads.cpp`.

The example uses a placeholder task named `ExampleTask`. Replace its name
and behavior with the actual responsibility.

## How the Pieces Connect

```text
CubeMX RTOS configuration
    → generated thread creation
    → StartExampleTask(void *argument)
    → userExampleTask(void *argument)
```

CubeMX owns thread configuration and the generated wrapper.

Application-owned code implements the task's behavior.

## 1. Configure the Thread in CubeMX

Open the subsystem's `.ioc`.

Find its FreeRTOS configuration and task/thread settings. The exact panel
name depends on the CubeMX and middleware versions.

Add a thread with:

| Setting | Example |
|---|---|
| Task name | `ExampleTask` |
| Entry function | `StartExampleTask` |
| Argument | `NULL` |

Choose priority, stack size, and allocation method for the actual workload.
There is no universal stack size or priority for every task.

Retain the project's CMSIS-RTOS2 interface.

Generate code and inspect the resulting thread attributes and creation call.

For CMSIS-RTOS2, the generated `osThreadAttr_t.stack_size` is expressed in
bytes. Check how CubeMX converts the value entered in its UI.

## 2. Declare the User Function

Add this declaration inside the existing C-linkage block in
`user/Inc/user_threads.h`:

```c
void userExampleTask(void* argument);
```

A complete minimal header would look like:

```c
#ifndef BLAZE_USER_THREADS_H
#define BLAZE_USER_THREADS_H

#ifdef __cplusplus
extern "C"
{
#endif

    void userExampleTask(void* argument);

#ifdef __cplusplus
}
#endif

#endif
```

When editing an existing header, retain its other declarations and include
guard. Do not replace the whole header with this example.

The C-linkage block lets generated C code call the function implemented in C++.

## 3. Implement the Task

Add the function to `user/Src/user_threads.cpp`:

```cpp
#include "user_threads.h"

#include "cmsis_os.h"

void userExampleTask(void* argument)
{
    (void)argument;

    for (;;)
    {
        // Perform work or wait for an event.
        osDelay(1);
    }
}
```

This example only demonstrates the connection. It does not implement a
subsystem feature.

`osDelay(1)` waits for one kernel tick. It is not necessarily one millisecond.

For event-driven work, use an appropriate blocking queue, semaphore, or
notification instead of repeatedly polling.

## 4. Include the Header in the Generated Source

Locate the generated `StartExampleTask()` definition.

Depending on the project, it may be in `main.c`, `freertos.c`, or
`app_freertos.c`.

Add the include inside that file's existing user include section:

```c
/* USER CODE BEGIN Includes */
#include "user_threads.h"
/* USER CODE END Includes */
```

Do not add it twice if it is already present.

## 5. Connect the Generated Entry Function

Inside `StartExampleTask()`, replace the generated placeholder loop within
its user-code section with:

```c
userExampleTask(argument);
```

For example, if CubeMX generated markers named `StartExampleTask`:

```c
void StartExampleTask(void* argument)
{
    /* USER CODE BEGIN StartExampleTask */
    userExampleTask(argument);
    /* USER CODE END StartExampleTask */
}
```

Keep the exact marker names generated in your file. Some projects use
numbered markers or the task name instead.

The user function owns the infinite loop in this pattern, so the generated
wrapper calls it once.

Do not add another `osThreadNew()` manually: CubeMX already generated the
thread creation.

Do not remove the task from CubeMX after adding the user function.

## 6. Check CMake

The existing subsystem targets include:

```text
user/Inc/
user/Src/*.c
user/Src/*.cpp
```

Confirm those paths remain present in the affected target.

If you added a source file, rerun CMake so it is discovered:

```sh
cmake --preset debug
cmake --build --preset debug --target Front-VCU.elf
```

Replace the target with your subsystem.

## 7. Verify on Hardware

1. Confirm thread creation succeeds.
2. Set a breakpoint in `StartExampleTask()`.
3. Confirm execution reaches `userExampleTask()`.
4. Confirm the task blocks or delays as intended.
5. Check stack usage under realistic workload.
6. Check interactions with other tasks.
7. Regenerate once more and confirm the user-code connection survives.

Record the selected priority, stack allocation, execution trigger, and
validation in the subsystem README.

## Common Problems

| Symptom | Likely cause |
|---|---|
| Undefined reference to user function | Missing definition, missing source, or incorrect C linkage |
| Conflicting declaration | Header and implementation signatures differ |
| User function is never reached | Thread creation failed or wrapper does not call it |
| Code disappears after generation | Edits were outside preserved user sections |
| Other tasks stop making progress | Task spins without blocking or has unsuitable priority |
| Hard fault after adding work | Investigate stack usage, pointers, and called APIs |
| New `.cpp` file is ignored | Rerun CMake and inspect source patterns |

Keep task-entry wiring small. Put the actual subsystem behavior in
application-owned code.
