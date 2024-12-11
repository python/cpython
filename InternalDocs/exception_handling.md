Description of exception handling
---------------------------------

Python uses a technique known as "zero-cost" exception handling, which
minimizes the cost of supporting exceptions. In the common case (where
no exception is raised) the cost is reduced to zero (or close to zero).
The cost of raising an exception is increased, but not by much.

The following code:

```
try:
    g(0)
except:
    res = "fail"

```

compiles into intermediate code like the following:

```
                  RESUME                   0

     1            SETUP_FINALLY            8 (to L1)

     2            LOAD_NAME                0 (g)
                  PUSH_NULL
                  LOAD_CONST               0 (0)
                  CALL                     1
                  POP_TOP
                  POP_BLOCK

    --   L1:      PUSH_EXC_INFO

     3            POP_TOP

     4            LOAD_CONST               1 ('fail')
                  STORE_NAME               1 (res)
```

`SETUP_FINALLY` and `POP_BLOCK` are pseudo-instructions. This means
that they can appear in intermediate code but they are not bytecode
instructions. `SETUP_FINALLY` specifies that henceforth, exceptions
are handled by the code at label L1. The `POP_BLOCK` instruction
reverses the effect of the last `SETUP` instruction, so that the
active exception handler reverts to what it was before.

`SETUP_FINALLY` and `POP_BLOCK` have no effect when no exceptions
are raised. The idea of zero-cost exception handling is to replace
these pseudo-instructions by metadata which is stored alongside the
bytecode, and which is inspected only when an exception occurs.
This metadata is the exception table, and it is stored in the code
object's `co_exceptiontable` field.

When the pseudo-instructions are translated into bytecode,
`SETUP_FINALLY` and `POP_BLOCK` are removed, and the exception
table is constructed, mapping each instruction to the exception
handler that covers it, if any. Instructions which are not
covered by any exception handler within the same code object's
bytecode, do not appear in the exception table at all.

For the code object in our example above, the table has a single
entry specifying that all instructions that were between the
`SETUP_FINALLY` and the `POP_BLOCK` are covered by the exception
handler located at label `L1`.

Handling Exceptions
-------------------

At runtime, when an exception occurs, the interpreter calls
`get_exception_handler()` in [Python/ceval.c](../Python/ceval.c)
to look up the offset of the current instruction in the exception
table. If it finds a handler, control flow transfers to it. Otherwise, the
exception bubbles up to the caller, and the caller's frame is
checked for a handler covering the `CALL` instruction. This
repeats until a handler is found or the topmost frame is reached.
If no handler is found, then the interpreter function
(`_PyEval_EvalFrameDefault()`) returns NULL. During unwinding,
the traceback is constructed as each frame is added to it by
`PyTraceBack_Here()`, which is in [Python/traceback.c](../Python/traceback.c).

Along with the location of an exception handler, each entry of the
exception table also contains the stack depth of the `try` instruction
and a boolean `lasti` value, which indicates whether the instruction
offset of the raising instruction should be pushed to the stack.

Handling an exception, once an exception table entry is found, consists
of the following steps:

1. pop values from the stack until it matches the stack depth for the handler.
2. if `lasti` is true, then push the offset that the exception was raised at.
3. push the exception to the stack.
4. jump to the target offset and resume execution.


Reraising Exceptions and `lasti`
--------------------------------

The purpose of pushing `lasti` to the stack is for cases where an exception
needs to be re-raised, and be associated with the original instruction that
raised it. This happens, for example, at the end of a `finally` block, when
any in-flight exception needs to be propagated on. As the frame's instruction
pointer now points into the finally block, a `RERAISE` instruction
(with `oparg > 0`) sets it to the `lasti` value from the stack.

Format of the exception table
-----------------------------

Conceptually, the exception table consists of a sequence of 5-tuples:

1. `start-offset` (inclusive)
2. `end-offset` (exclusive)
3. `target`
4. `stack-depth`
5. `push-lasti` (boolean)

All offsets and lengths are in code units, not bytes.

We want the format to be compact, but quickly searchable.
For it to be compact, it needs to have variable sized entries so that we can store common (small) offsets compactly, but handle large offsets if needed.
For it to be searchable quickly, we need to support binary search giving us log(n) performance in all cases.
Binary search typically assumes fixed size entries, but that is not necessary, as long as we can identify the start of an entry.

It is worth noting that the size (end-start) is always smaller than the end, so we encode the entries as:
`start, size, target, depth, push-lasti`.

Also, sizes are limited to 2**30 as the code length cannot exceed 2**31 and each code unit takes 2 bytes.
It also happens that depth is generally quite small.

So, we need to encode:

```
start   (up to 30 bits)
size    (up to 30 bits)
target  (up to 30 bits)
depth   (up to ~8 bits)
lasti   (1 bit)
```

We need a marker for the start of the entry, so the first byte of entry will have the most significant bit set.
Since the most significant bit is reserved for marking the start of an entry, we have 7 bits per byte to encode offsets.
Encoding uses a standard varint encoding, but with only 7 bits instead of the usual 8.
The 8 bits of a byte are (msb left) SXdddddd where S is the start bit. X is the extend bit meaning that the next byte is required to extend the offset.

In addition, we combine `depth` and `lasti` into a single value, `((depth<<1)+lasti)`, before encoding.

For example, the exception entry:

```
start:              20
end:                28
target:             100
depth:              3
lasti:              False
```

is encoded by first converting to the more compact four value form:

```
start:              20
size:               8
target:             100
depth<<1+lasti:     6
```

which is then encoded as:

```
148     (MSB + 20 for start)
8       (size)
65      (Extend bit + 1)
36      (Remainder of target, 100 == (1<<6)+36)
6
```

for a total of five bytes.

The code to construct the exception table is in `assemble_exception_table()`
in [Python/assemble.c](../Python/assemble.c).

The interpreter's function to lookup the table by instruction offset is
`get_exception_handler()` in [Python/ceval.c](../Python/ceval.c).
The Python function `_parse_exception_table()` in [Lib/dis.py](../Lib/dis.py)
returns the exception table content as a list of namedtuple instances.

Exception Chaining Implementation
---------------------------------

[Exception chaining](https://docs.python.org/dev/tutorial/errors.html#exception-chaining)
refers to setting the `__context__` and `__cause__` fields of an exception as it is
being raised. The `__context__` field is set by `_PyErr_SetObject()` in
[Python/errors.c](../Python/errors.c) (which is ultimately called by all
`PyErr_Set*()` functions).  The `__cause__` field (explicit chaining) is set by
the `RAISE_VARARGS` bytecode.
