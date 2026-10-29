
Generators and Coroutines
=========================

Generators
----------

Generators in CPython are implemented with the struct `PyGenObject`.
They consist of a [`frame`](frames.md) and metadata about the generator's
execution state.

A generator object resumes execution in its frame when its `send()`
method is called. This is analogous to a function executing in its own
frame when it is called, but a function returns to the calling frame only once,
while a generator "returns" execution to the caller's frame every time
it emits a new item with a
[`yield` expression](https://docs.python.org/dev/reference/expressions.html#yield-expressions).
This is implemented by the
[`YIELD_VALUE`](https://docs.python.org/dev/library/dis.html#opcode-YIELD_VALUE)
bytecode, which is similar to
[`RETURN_VALUE`](https://docs.python.org/dev/library/dis.html#opcode-RETURN_VALUE)
in the sense that it puts a value on the stack and returns execution to the
calling frame, but it also needs to perform additional work to leave the generator
frame in a state that allows it to be resumed.  In particular, it updates the frame's
instruction pointer and stores the interpreter's exception state on the generator
object. When the generator is resumed, this exception state is copied back to the
interpreter state.

The `frame` of a generator is embedded in the generator object struct as a
[`_PyInterpreterFrame`](frames.md) (see `_PyGenObject_HEAD` in
[`pycore_interpframe_structs.h`](../Include/internal/pycore_interpframe_structs.h)).
This means that we can get the frame from the generator or the generator
from the frame (see `_PyGen_GetGeneratorFromFrame` in [`pycore_genobject.h`](../Include/internal/pycore_genobject.h)).
Other fields of the generator struct include metadata (such as the name of
the generator function) and runtime state information (such as whether its
frame is executing, suspended, cleared, etc.).

Generator Object Creation and Destruction
-----------------------------------------

The bytecode of a generator function begins with a
[`RETURN_GENERATOR`](https://docs.python.org/dev/library/dis.html#opcode-RETURN_GENERATOR)
instruction, which creates a generator object, including its embedded frame.
The generator's frame is initialized as a copy of the frame in which
`RETURN_GENERATOR` is executing, but its `owner` field is overwritten to indicate
that it is owned by a generator. Finally, `RETURN_GENERATOR` pushes the new generator
object to the stack and returns to the caller of the generator function (at
which time its frame is destroyed). When the generator is next resumed by
[`gen_send_ex2()`](../Objects/genobject.c), `_PyEval_EvalFrame()` is called
to continue executing the generator function, in the frame that is embedded in
the generator object.

When a generator object is destroyed in [`gen_dealloc`](../Objects/genobject.c),
its embedded `_PyInterpreterFrame` field may need to be preserved, if it is exposed
to Python as part of a [`PyFrameObject`](frames.md#frame-objects). This is detected
in [`_PyFrame_ClearExceptCode`](../Python/frame.c) by the fact that the interpreter
frame's `frame_obj` field is set, and the frame object it points to has refcount
greater than 1. If so, the `take_ownership()` function is called to create a new
copy of the interpreter frame and transfer ownership of it from the generator to
the frame object.

Iteration
---------

The [`FOR_ITER`](https://docs.python.org/dev/library/dis.html#opcode-FOR_ITER)
instruction calls `__next__` on the iterator which is on the top of the stack,
and pushes the result to the stack. It has [`specializations`](interpreter.md)
for a few common iterator types, including `FOR_ITER_GEN`, for iterating over
a generator. `FOR_ITER_GEN` bypasses the call to `__next__`, and instead
directly pushes the generator stack and resumes its execution from the
instruction that follows the last yield.

Chained Generators
------------------

A `yield from` expression creates a generator that efficiently yields the
sequence created by another generator. This is implemented with the
[`SEND` instruction](https://docs.python.org/dev/library/dis.html#opcode-SEND),
which pushes the value of its arg to the stack of the generator's frame, sets
the exception state on this frame, and resumes execution of the chained generator.
On return from `SEND`, the value at the top of the stack is sent back up
the generator chain with a `YIELD_VALUE`. This sequence of `SEND` followed by
`YIELD_VALUE` is repeated in a loop, until a `StopIteration` exception is
raised to indicate that the generator has no more values to emit.

The [`CLEANUP_THROW`](https://docs.python.org/dev/library/dis.html#opcode-CLEANUP_THROW)
instruction is used to handle exceptions raised from the send-yield loop.
Exceptions of type `StopIteration` is handled, their `value` field hold the
value to be returned by the generator's `close()` function. Any other
exception is re-raised by `CLEANUP_THROW`.

Coroutines
----------

Coroutines are generators that use the value returned from a `yield` expression,
i.e., the argument that was passed to the `.send()` call that resumed it after
it yielded. This makes it possible for data to flow in both directions: from
the generator to the caller via the argument of the `yield` expression, and
from the caller to the generator via the send argument to the `send()` call.
A `yield from` expression passes the `send` argument to the chained generator,
so this data flow works along the chain (see `gen_send_ex2()` in
[`genobject.c`](../Objects/genobject.c)).

Recall that a generator's `__next__` function simply calls `self.send(None)`,
so all this works the same in generators and coroutines, but only coroutines
use the value of the argument to `send`.
