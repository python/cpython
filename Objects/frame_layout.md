# The Frame Stack

Each call to a Python function has an activation record,
commonly known as a "frame".
Python semantics allows frames to outlive the activation,
so they have (before 3.11) been allocated on the heap.
This is expensive as it requires many allocations and
results in poor locality of reference.

In 3.11, rather than have these frames scattered about memory,
as happens for heap-allocated objects, frames are allocated
contiguously in a per-thread stack.
This improves performance significantly for two reasons:
* It reduces allocation overhead to a pointer comparison and increment.
* Stack allocated data has the best possible locality and will always be in
  CPU cache.

Generator and coroutines still need heap allocated activation records, but
can be linked into the per-thread stack so as to not impact performance too much.

## Layout

Each activation record consists of four conceptual sections:

* Local variables (including arguments, cells and free variables)
* Evaluation stack
* Specials: The per-frame object references needed by the VM: globals dict,
  code object, etc.
* Linkage: Pointer to the previous activation record, stack depth, etc.

### Layout

The specials and linkage sections are a fixed size, so are grouped together.

Each activation record is laid out as:
* Specials and linkage
* Locals
* Stack

This seems to provide the best performance without excessive complexity.
It needs the interpreter to hold two pointers, a frame pointer and a stack pointer.

#### Alternative layout

An alternative layout that was used for part of 3.11 alpha was:

* Locals
* Specials and linkage
* Stack

This has the advantage that no copying is required when making a call,
as the arguments on the stack are (usually) already in the correct
location for the parameters. However, it requires the VM to maintain
an extra pointer for the locals, which can hurt performance.

A variant that only needs the need two pointers is to reverse the numbering
of the locals, so that the last one is numbered `0`, and the first in memory
is numbered `N-1`.
This allows the locals, specials and linkage to accessed from the frame pointer.
We may implement this in the future.

#### Note:

> In a contiguous stack, we would need to save one fewer registers, as the
> top of the caller's activation record would be the same at the base of the
> callee's. However, since some activation records are kept on the heap we
> cannot do this.

### Generators and Coroutines

Generators and coroutines contain a `_PyInterpreterFrame`
The specials sections contains the following pointers:

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
and a strong reference to it placed in the `frame_obj` field of the specials
section. The `frame_obj` field is initially `NULL`.

The `PyFrameObject` may outlive a stack-allocated `_PyInterpreterFrame`.
If it does then `_PyInterpreterFrame` is copied into the `PyFrameObject`,
except the evaluation stack which must be empty at this point.
The linkage section is updated to reflect the new location of the frame.

This mechanism provides the appearance of persistent, heap-allocated
frames for each activation, but with low runtime overhead.

### Generators and Coroutines


Generator objects have a `_PyInterpreterFrame` embedded in them.
This means that creating a generator requires only a single allocation,
reducing allocation overhead and improving locality of reference.
The embedded frame is linked into the per-thread frame when iterated or
awaited.

If a frame object associated with a generator outlives the generator, then
the embedded `_PyInterpreterFrame` is copied into the frame object.


All the above applies to coroutines and async generators as well.

### Field names

Many of the fields in `_PyInterpreterFrame` were copied from the 3.10 `PyFrameObject`.
Thus, some of the field names may be a bit misleading.

For example the `f_globals` field has a `f_` prefix implying it belongs to the
`PyFrameObject` struct, although it belongs to the `_PyInterpreterFrame` struct.
We may rationalize this naming scheme for 3.12.


### Shim frames

On entry to `_PyEval_EvalFrameDefault()` a shim `_PyInterpreterFrame` is pushed.
This frame is stored on the C stack, and popped when `_PyEval_EvalFrameDefault()`
returns. This extra frame is inserted so that `RETURN_VALUE`, `YIELD_VALUE`, and
`RETURN_GENERATOR` do not need to check whether the current frame is the entry frame.
The shim frame points to a special code object containing the `INTERPRETER_EXIT`
instruction which cleans up the shim frame and returns.
