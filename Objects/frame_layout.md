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

### Linkage section

The linkage section contains pointers, offsets and codes necessary for the
VM to manage and introspect stacks.
The linkage section contains at least the following information:
* Stack size
* Pointer to previous activation record
* Memory management information: a `_PyInterpreterFrame` can be part of a
  thread's stack, or embedded in an object.
* Current instruction offset. Used for return address and debugging.


#### Note:

> In a contiguous stack, we would need to save one fewer registers, as the
> top of the caller's activation record would be the same at the base of the
> callee's. However, since some activation records are kept on the heap we 
> cannot do this.

### Specials section

The specials sections contains the following pointers:

* Globals dict
* Builtins dict
* Locals dict (not the "fast" locals, but the locals for eval and class creation)
* Code object
* Heap allocated `PyFrameObject` for this activation record, if any.
* The function.

The pointer to the function is not strictly required, but it is cheaper to
store a strong reference to the function and borrowed references to the globals
and builtins, that strong references to both globals and builtins.

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


### Field names

Many of the fields in `_PyInterpreterFrame` were copied from the 3.10 `PyFrameObject`.
Consequently, some of the field names may be a bit misleading.

For example the `f_globals` field has a `f_` prefix implying it belongs to the
`PyFrameObject` struct, although it belongs to the `_PyInterpreterFrame` struct.
We may rationale the naming scheme for 3.12.