
Generators and Coroutines
=========================

Generators
----------

The implementation of generators in CPython consists of the builtin object type
`PyGenObject` and bytecode instructions that operate on instances of this type.

A generator object executes in its own [`frame`](frames.md), like a function.
The difference is that a function returns only once, while a generator
"returns" to the caller every time it emits a new item with a
[`yield` expression](https://docs.python.org/dev/reference/expressions.html#yield-expressions).
This is implemented by the
[`YIELD_VALUE`](https://docs.python.org/dev/library/dis.html#opcode-YIELD_VALUE)
bytecode, which is similar to
[`RETURN_VALUE`](https://docs.python.org/dev/library/dis.html#opcode-RETURN_VALUE)
in the sense that it puts a value on the stack and returns execution to the
calling frame, but it also needs to perform additional work to leave the generator
frame in a state that allows it to be resumed.  In particular, it updates the frame's
instruction pointer and stores the interpreter's exception state on the generator
object. When the generator it resumed, this exception state is copied back to the
interpreter state.

The `frame` of a generator is embedded in the generator object struct (see
`_PyGenObject_HEAD` in [`pycore_genobject.h`](../Include/internal/pycore_genobject.h)).
This means that we can get the frame from the generator or the generator
from the frame (see `_PyGen_GetGeneratorFromFrame` in the same file).
Other fields of the generator struct include metadata (such as the name of
the generator function) and runtime state information (such as whether its
frame is executing, suspended, cleared, etc.).

Chained Generators
------------------

A `yield from` expression creates a generator that efficiently yields the
sequence created by another generator. This is implemented with the
[`SEND` instruction](https://docs.python.org/dev/library/dis.html#opcode-SEND),
which pushes the value of its arg to the stack of the generator's frame, sets
the exception state on this frame, and resumes execution of the chained generator. 

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
