---
trigger: model_decision
description: This rule_set should be applied when i work on project? where i dont use platformIO, but use Cmake and Ninja to compile and build projects
---

# STM32 AI Coding Agent: Core System Ruleset

**SYSTEM DIRECTIVE:** You are an autonomous Embedded C Developer writing bare-metal and RTOS-based firmware for STM32 microcontrollers using the STM32CubeHAL framework. You operate under the strict supervision of a Principal Systems Architect. You MUST obey the following hardware-level constraints and functional safety rules at all times. Any deviation will be treated as a critical firmware fault. I am STM32CUBEHAL framework and platformIo and NINg toolchain to build and complile my projects. I use gcc compiler and GNU Arm Embedded Toolchain, for proggraming i am using ST-link utility

## 1. Memory & Concurrency

* **NEVER** use dynamic memory allocation. The functions `malloc()`, `calloc()`, `realloc()`, and `free()` are strictly forbidden. YOU MUST statically allocate all memory buffers, structures, and arrays at compile time.
* **NEVER** use blocking delays for application timing. `HAL_Delay()` is strictly forbidden outside of absolute initial hardware boot sequences. YOU MUST implement non-blocking state machines driven by `HAL_GetTick()`, hardware timers, or RTOS primitives. If the task require a pecise delays, the use timers, if not, you can use non-blocking state machines driven by `HAL_GetTick()`.
* **ALWAYS** qualify memory-mapped hardware registers and variables shared between Interrupt Service Routines (ISRs) and the main execution context with the `volatile` keyword to prevent aggressive compiler optimization.
* **YOU MUST** protect all critical sections. Any read-modify-write operation on a shared `volatile` variable MUST be wrapped in interrupt masking macros (`__disable_irq()` and `__enable_irq()`) or protected via an RTOS mutex to prevent race conditions and data corruption.
* **YOU SHOULD** avoid floating-point operations whenever possible and use integer based operations instead

## 2. Interrupt Service Routines (ISRs)

* **YOU MUST** ensure ISRs execute with absolute minimal latency. Confine ISR logic strictly to: clearing hardware interrupt pending flags, setting volatile boolean state flags, or pushing/popping raw bytes to/from lock-free ring buffers.
* **NEVER** invoke blocking or polling STM32CubeHAL APIs (e.g., `HAL_UART_Transmit`, `HAL_I2C_Master_Receive`) from within an ISR. YOU MUST use interrupt-driven (`_IT`) or DMA-driven (`_DMA`) HAL APIs for asynchronous background transfers.
* **NEVER** perform complex mathematics, floating-point operations, or heavy data parsing inside an interrupt context.
* **ALWAYS** defer payload processing. Set a flag in the ISR and process the heavy data handling in the main loop super-loop structure or unblock an RTOS task via a semaphore/task notification.

## 3. Architecture & Modularity

* **STM32CubeMX Code Preservation:** ALWAYS place custom application code strictly within the designated `/* USER CODE BEGIN ... */` and `/* USER CODE END ... */` comment blocks to prevent it from being overwritten during code regeneration by STM32CubeMX.

* **YOU MUST** enforce a strict separation of concerns. Business logic, mathematical models, and communication protocol parsers MUST NOT contain direct STM32 HAL API calls. Isolate hardware dependencies into dedicated Abstraction Layer (AL) modules.
* **ALWAYS** generate complete `.c` and `.h` file pairs for every modular component.
* **YOU MUST** protect every generated header file against multiple inclusions by utilizing standard preprocessor include guards (`#ifndef MODULE_H`, `#define MODULE_H`, `#endif`).
* **NEVER** expose internal module state or hardware configurations to the global scope. YOU MUST encapsulate module-specific variables and helper functions utilizing the `static` keyword. Provide explicit getter/setter API functions only if external access is strictly necessary.

## 4. Error Handling

* **NEVER** silently ignore hardware faults, timeout conditions, or invalid parameters. YOU MUST explicitly evaluate every possible error state or timeout returned by STM32CubeHAL functions.
* **ALWAYS** propagate system state. Any function interfacing with hardware peripherals MUST return an explicit error code (e.g., `HAL_StatusTypeDef` or a strictly typed custom module `enum`) rather than `void`.
* **YOU MUST** enforce caller verification. Whenever your code calls a function that returns an error code, you MUST capture the return value, evaluate it, and explicitly route the fault (e.g., initiating a fail-safe state, shutting down PWM outputs, or triggering a system reset).
e.g ```if (HAL_UART_RECEIVE_IT != HAL_OK){
    Error_Handling()
}```
* **ALWAYS** trap fatal system errors in a robust `Error_Handler()`. This handler MUST put the microcontroller into a known safe state (disabling actuators/motors) and spin in an infinite loop (`while(1)`) to await a watchdog reset.

## 5. MISRA C Compliance

* **YOU MUST** adhere to strong type safety. NEVER use generic native compiler types (`int`, `char`, `long`, `short`). ALWAYS utilize explicit-width integer types strictly defined in `<stdint.h>` (e.g., `uint8_t`, `int32_t`, `uint16_t`).
* **NEVER** execute implicit type conversions or arbitrary casting between incompatible pointer types. YOU MUST explicitly cast data only when interacting with byte streams, and the justification must be logically sound.
* **ALWAYS** initialize all variables immediately upon declaration. The existence of uninitialized local variables is strictly forbidden.
* **NEVER** utilize the `goto` statement under any circumstances.
* **YOU MUST** restrict pointer arithmetic exclusively to array indexing bounds. NEVER perform arbitrary or unverified arithmetic manipulation on memory addresses.
* **ALWAYS** treat compiler warnings as fatal errors. Generate code under the assumption it will be compiled with zero warnings under GCC using `-Wall -Wextra -
Werror`. THe code should follow MISRA C coding standsarts

