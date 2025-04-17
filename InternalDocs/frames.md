# Frames

Each call to a Python function has an activation record, commonly known as a
"frame". It contains information about the function being executed, consisting
of three conceptual sections:

* Local variables (including arguments, cells and free variables)
* Evaluation stack
* Specials: The per-frame object references needed by the VM, including
  globals dict, code object, instruction pointer, stack depth, the
  previous frame, etc.

The definition of the `_PyInterpreterFrame` struct is in
[Include/internal/pycore_frame.h](../Include/internal/pycore_frame.h).

# Allocation

Python semantics allows frames to outlive the activation, so they need to
be allocated outside the C call stack. To reduce overhead and improve locality
of reference, most frames are allocated contiguously in a per-thread stack
(see `_PyThreadState_PushFrame` in [Python/pystate.c](../Python/pystate.c)).

Frames of generators and coroutines are embedded in the generator and coroutine
objects, so are not allocated in the per-thread stack. See `PyGenObject` in
[Include/internal/pycore_genobject.h](../Include/internal/pycore_genobject.h).

## Layout

Each activation record is laid out as:

* Specials
* Locals
* Stack

This seems to provide the best performance without excessive complexity.
The specials have a fixed size, so the offset of the locals is known. The
interpreter needs to hold two pointers, a frame pointer and a stack pointer.

#### Alternative layout

An alternative layout that was used for part of 3.11 alpha was:

* Locals
* Specials
* Stack

This has the advantage that no copying is required when making a call,
as the arguments on the stack are (usually) already in the correct
location for the parameters. However, it requires the VM to maintain
an extra pointer for the locals, which can hurt performance.

### Specials

The specials section contains the following pointers:

* Globals dict
* Builtins dict
* Locals dict (not the "fast" locals, but the locals for eval and class creation)
* Code object
* Heap allocated `PyFrameObject` for this activation record, if any.
* The function.

The pointer to the function is not strictly required, but it is cheaper to
store a strong reference to the function and borrowed references to the globals
and builtins, than strong references to both globals and builtins.

### Frame objects

When creating a backtrace or when calling `sys._getframe()` the frame becomes
visible to Python code. When this happens a new `PyFrameObject` is created
and a strong reference to it is placed in the `frame_obj` field of the specials
section. The `frame_obj` field is initially `NULL`.

The `PyFrameObject` may outlive a stack-allocated `_PyInterpreterFrame`.
If it does then `_PyInterpreterFrame` is copied into the `PyFrameObject`,
except the evaluation stack which must be empty at this point.
The previous frame link is updated to reflect the new location of the frame.

This mechanism provides the appearance of persistent, heap-allocated
frames for each activation, but with low runtime overhead.

### Generators and Coroutines

Generators (objects of type `PyGen_Type`, `PyCoro_Type` or
`PyAsyncGen_Type`) have a `_PyInterpreterFrame` embedded in them, so
that they can be created with a single memory allocation.
When such an embedded frame is iterated or awaited, it can be linked with
frames on the per-thread stack via the linkage fields.

If a frame object associated with a generator outlives the generator, then
the embedded `_PyInterpreterFrame` is copied into the frame object (see
`take_ownership()` in [Python/frame.c](../Python/frame.c)).

### Field names

Many of the fields in `_PyInterpreterFrame` were copied from the 3.10 `PyFrameObject`.
Thus, some of the field names may be a bit misleading.

For example the `f_globals` field has a `f_` prefix implying it belongs to the
`PyFrameObject` struct, although it belongs to the `_PyInterpreterFrame` struct.
We may rationalize this naming scheme for a later version.


### Shim frames

On entry to `_PyEval_EvalFrameDefault()` a shim `_PyInterpreterFrame` is pushed.
This frame is stored on the C stack, and popped when `_PyEval_EvalFrameDefault()`
returns. This extra frame is inserted so that `RETURN_VALUE`, `YIELD_VALUE`, and
`RETURN_GENERATOR` do not need to check whether the current frame is the entry frame.
The shim frame points to a special code object containing the `INTERPRETER_EXIT`
instruction which cleans up the shim frame and returns.


### The Instruction Pointer

`_PyInterpreterFrame` has two fields which are used to maintain the instruction
pointer: `instr_ptr` and `return_offset`.

When a frame is executing, `instr_ptr` points to the instruction currently being
executed. In a suspended frame, it points to the instruction that would execute
if the frame were to resume. After `frame.f_lineno` is set, `instr_ptr` points to
the next instruction to be executed. During a call to a python function,
`instr_ptr` points to the call instruction, because this is what we would expect
to see in an exception traceback.

The `return_offset` field determines where a `RETURN` should go in the caller,
relative to `instr_ptr`.  It is only meaningful to the callee, so it needs to
be set in any instruction that implements a call (to a Python function),
including CALL, SEND and BINARY_OP_SUBSCR_GETITEM, among others. If there is no
callee, then return_offset is meaningless. It is necessary to have a separate
field for the return offset because (1) if we apply this offset to `instr_ptr`
while executing the `RETURN`, this is too early and would lose us information
about the previous instruction which we could need for introspecting and
debugging. (2) `SEND` needs to pass two offsets to the generator: one for
`RETURN` and one for `YIELD`. It uses the `oparg` for one, and the
`return_offset` for the other.
