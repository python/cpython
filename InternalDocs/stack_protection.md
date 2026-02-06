# Stack Protection

CPython protects against stack overflow in the form of runaway, or just very deep, recursion by raising a `RecursionError` instead of just crashing.
Protection against pure Python stack recursion has existed since very early, but in 3.12 we added protection against stack overflow
in C code. This was initially implemented using a counter and later improved in 3.14 to use the actual stack depth.
For those platforms that support it (Windows, Mac, and most Linuxes) we query the operating system to find the stack bounds.
For other platforms we use conservative estimates.


The C stack looks like this:

```
         +-------+  <--- Top of machine stack
         |       |
         |       |

            ~~

         |       |
         |       |
         +-------+  <--- Soft limit
         |       |
         |       |     _PyOS_STACK_MARGIN_BYTES
         |       |
         +-------+  <---  Hard limit
         |       |
         |       |     _PyOS_STACK_MARGIN_BYTES
         |       |
         +-------+  <--- Bottom of machine stack
```


We get the current stack pointer using compiler intrinsics where available, or by taking the address of a C local variable. See `_Py_get_machine_stack_pointer()`.

The soft and hard limits pointers are set by calling `_Py_InitializeRecursionLimits()` during thread initialization.

Recursion checks are performed by `_Py_EnterRecursiveCall()` or `_Py_EnterRecursiveCallTstate()` which compare the stack pointer to the soft limit. If the stack pointer is lower than the soft limit, then `_Py_CheckRecursiveCall()` is called which checks against both the hard and soft limits:

```python
kb_used = (stack_top - stack_pointer)>>10
if stack_pointer < bottom_of_machine_stack:
    pass # Our stack limits could be wrong so it is safest to do nothing.
elif stack_pointer < hard_limit:
    FatalError(f"Unrecoverable stack overflow (used {kb_used} kB)")
elif stack_pointer < soft_limit:
    raise RecursionError(f"Stack overflow (used {kb_used} kB)")
```

### User space threads and other oddities

Some libraries provide user-space threads. These will change the C stack at runtime.
To guard against this we only raise if the stack pointer is in the window between the expected stack base and the soft limit.

### Diagnosing and fixing stack overflows

For stack protection to work correctly the amount of stack consumed between calls to `_Py_EnterRecursiveCall()` must be less than `_PyOS_STACK_MARGIN_BYTES`.

If you see a traceback ending in: `RecursionError: Stack overflow (used ... kB)` then the stack protection is working as intended. If you don't expect to see the error, then check the amount of stack used. If it seems low then CPython may not be configured properly.

However, if you see a fatal error or crash, then something is not right.
Either a recursive call is not checking `_Py_EnterRecursiveCall()`, or the amount of C stack consumed by a single call exceeds `_PyOS_STACK_MARGIN_BYTES`. If a hard crash occurs, it probably means that the amount of C stack consumed is more than double `_PyOS_STACK_MARGIN_BYTES`.

Likely causes:
* Recursive code is not calling `_Py_EnterRecursiveCall()`
* `-O0` compilation flags, especially for Clang. With no optimization, C calls can consume a lot of stack space
* Giant, complex functions in third-party C extensions. This is unlikely as the function in question would need to be more complicated than the bytecode interpreter.
* `_PyOS_STACK_MARGIN_BYTES` is just too low.
* `_Py_InitializeRecursionLimits()` is not setting the soft and hard limits correctly for that platform.
