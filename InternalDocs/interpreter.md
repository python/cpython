# The bytecode interpreter

This document describes the workings and implementation of the bytecode
interpreter, the part of python that executes compiled Python code. Its
entry point is in [Python/ceval.c](../Python/ceval.c).

At a high level, the interpreter consists of a loop that iterates over the
bytecode instructions, executing each of them via a switch statement that
has a case implementing each opcode. This switch statement is generated
from the instruction definitions in [Python/bytecodes.c](../Python/bytecodes.c)
which are written in [a DSL](../Tools/cases_generator/interpreter_definition.md)
developed for this purpose.

Recall that the [Python Compiler](compiler.md) produces a [`CodeObject`](code_objects.md),
which contains the bytecode instructions along with static data that is required to execute them,
such as the consts list, variable names,
[exception table](exception_handling.md#format-of-the-exception-table), and so on.

When the interpreter's
[`PyEval_EvalCode()`](https://docs.python.org/3.14/c-api/veryhigh.html#c.PyEval_EvalCode)
function is called to execute a `CodeObject`, it constructs a [`Frame`](frames.md) and calls
[`_PyEval_EvalFrame()`](https://docs.python.org/3.14/c-api/veryhigh.html#c.PyEval_EvalCode)
to execute the code object in this frame. The frame holds the dynamic state of the
`CodeObject`'s execution, including the instruction pointer, the globals and builtins.
It also has a reference to the `CodeObject` itself.

In addition to the frame, `_PyEval_EvalFrame()` also receives a
[`Thread State`](https://docs.python.org/3/c-api/init.html#c.PyThreadState)
object, `tstate`, which includes things like the exception state and the
recursion depth.  The thread state also provides access to the per-interpreter
state (`tstate->interp`), which has a pointer to the per-runtime (that is,
truly global) state (`tstate->interp->runtime`).

Finally, `_PyEval_EvalFrame()` receives an integer argument `throwflag`
which, when nonzero, indicates that the interpreter should just raise the current exception
(this is used in the implementation of
[`gen.throw`](https://docs.python.org/3.14/reference/expressions.html#generator.throw).

By default, [`_PyEval_EvalFrame()`](https://docs.python.org/3.14/c-api/veryhigh.html#c.PyEval_EvalCode)
simply calls [`_PyEval_EvalFrameDefault()`] to execute the frame. However, as per
[`PEP 523`](https://peps.python.org/pep-0523/) this is configurable by setting
`interp->eval_frame`. In the following, we describe the default function,
`_PyEval_EvalFrameDefault()`.


## Instruction decoding

The first task of the interpreter is to decode the bytecode instructions.
Bytecode is stored as an array of 16-bit code units (`_Py_CODEUNIT`).
Each code unit contains an 8-bit `opcode` and an 8-bit argument (`oparg`), both unsigned.
In order to make the bytecode format independent of the machine byte order when stored on disk,
`opcode` is always the first byte and `oparg` is always the second byte.
Macros are used to extract the `opcode` and `oparg` from a code unit
(`_Py_OPCODE(word)` and `_Py_OPARG(word)`).
Some instructions (for example, `NOP` or `POP_TOP`) have no argument -- in this case
we ignore `oparg`.

A simplified version of the interpreter's main loop looks like this:

```c
    _Py_CODEUNIT *first_instr = code->co_code_adaptive;
    _Py_CODEUNIT *next_instr = first_instr;
    while (1) {
        _Py_CODEUNIT word = *next_instr++;
        unsigned char opcode = _Py_OPCODE(word);
        unsigned int oparg = _Py_OPARG(word);
        switch (opcode) {
        // ... A case for each opcode ...
        }
    }
```

This loop iterates over the instructions, decoding each into its `opcode`
and `oparg`, and then executes the switch case that implements this `opcode`.

The instruction format supports 256 different opcodes, which is sufficient.
However, it also limits `oparg` to 8-bit values, which is too restrictive.
To overcome this, the `EXTENDED_ARG` opcode allows us to prefix any instruction
with one or more additional data bytes, which combine into a larger oparg.
For example, this sequence of code units:

    EXTENDED_ARG  1
    EXTENDED_ARG  0
    LOAD_CONST    2

would set `opcode` to `LOAD_CONST` and `oparg` to `65538` (that is, `0x1_00_02`).
The compiler should limit itself to at most three `EXTENDED_ARG` prefixes, to allow the
resulting `oparg` to fit in 32 bits, but the interpreter does not check this.

In the following, a `code unit` is always two bytes, while an `instruction` is a
sequence of code units consisting of zero to three `EXTENDED_ARG` opcodes followed by
a primary opcode.

The following loop, to be inserted just above the `switch` statement, will make the above
snippet decode a complete instruction:

```c
    while (opcode == EXTENDED_ARG) {
        word = *next_instr++;
        opcode = _Py_OPCODE(word);
        oparg = (oparg << 8) | _Py_OPARG(word);
    }
```

For various reasons we'll get to later (mostly efficiency, given that `EXTENDED_ARG`
is rare) the actual code is different.

## Jumps

Note that when the `switch` statement is reached, `next_instr` (the "instruction offset")
already points to the next instruction.
Thus, jump instructions can be implemented by manipulating `next_instr`:

- A jump forward (`JUMP_FORWARD`) sets `next_instr += oparg`.
- A jump backward (`JUMP_BACKWARD`) sets `next_instr -= oparg`.

## Inline cache entries

Some (specialized or specializable) instructions have an associated "inline cache".
The inline cache consists of one or more two-byte entries included in the bytecode
array as additional words following the `opcode`/`oparg` pair.
The size of the inline cache for a particular instruction is fixed by its `opcode`.
Moreover, the inline cache size for all instructions in a
[family of specialized/specializable instructions](#Specialization)
(for example, `LOAD_ATTR`, `LOAD_ATTR_SLOT`, `LOAD_ATTR_MODULE`) must all be
the same.  Cache entries are reserved by the compiler and initialized with zeros.
Although they are represented by code units, cache entries do not conform to the
`opcode` / `oparg` format.

If an instruction has an inline cache, the layout of its cache is described in
the instruction's definition in [`Python/bytecodes.c`](../Python/bytecodes.c).
The structs defined in [`pycore_code.h`](../Include/internal/pycore_code.h)
allow us to access the cache by casting `next_instr` to a pointer to the relevant
`struct`.  The size of such a `struct` must be independent of the machine
architecture, word size and alignment requirements.  For a 32-bit field, the
`struct` should use `_Py_CODEUNIT field[2]`.

The instruction implementation is responsible for advancing `next_instr` past the inline cache.
For example, if an instruction's inline cache is four bytes (that is, two code units) in size,
the code for the instruction must contain `next_instr += 2;`.
This is equivalent to a relative forward jump by that many code units.
(In the interpreter definition DSL, this is coded as `JUMPBY(n)`, where `n` is the number
of code units to jump, typically given as a named constant.)

Serializing non-zero cache entries would present a problem because the serialization
(:mod:`marshal`) format must be independent of the machine byte order.

More information about the use of inline caches can be found in
[PEP 659](https://peps.python.org/pep-0659/#ancillary-data).

## The evaluation stack

Most instructions read or write some data in the form of object references (`PyObject *`).
The CPython bytecode interpreter is a stack machine, meaning that its instructions operate
by pushing data onto and popping it off the stack.
The stack forms part of the frame for the code object. Its maximum depth is calculated
by the compiler and stored in the `co_stacksize` field of the code object, so that the
stack can be pre-allocated as a contiguous array of `PyObject*` pointers, when the frame
is created.

The stack effects of each instruction are also exposed through the
[opcode metadata](../Include/internal/pycore_opcode_metadata.h) through two
functions that report how many stack elements the instructions consumes,
and how many it produces (`_PyOpcode_num_popped` and `_PyOpcode_num_pushed`).
For example, the `BINARY_OP` instruction pops two objects from the stack and pushes the
result back onto the stack.

The stack grows up in memory; the operation `PUSH(x)` is equivalent to `*stack_pointer++ = x`,
whereas `x = POP()` means `x = *--stack_pointer`.
Overflow and underflow checks are active in debug mode, but are otherwise optimized away.

At any point during execution, the stack level is knowable based on the instruction pointer
alone, and some properties of each item on the stack are also known.
In particular, only a few instructions may push a `NULL` onto the stack, and the positions
that may be `NULL` are known.
A few other instructions (`GET_ITER`, `FOR_ITER`) push or pop an object that is known to
be an iterator.

Instruction sequences that do not allow statically knowing the stack depth are deemed illegal;
the bytecode compiler never generates such sequences.
For example, the following sequence is illegal, because it keeps pushing items on the stack:

    LOAD_FAST 0
    JUMP_BACKWARD 2

> [!NOTE]
> Do not confuse the evaluation stack with the call stack, which is used to implement calling
> and returning from functions.

## Error handling

When the implementation of an opcode raises an exception, it jumps to the
`exception_unwind` label in [Python/ceval.c](../Python/ceval.c).
The exception is then handled as described in the
[`exception handling documentation`](exception_handling.md#handling-exceptions).

## Python-to-Python calls

The `_PyEval_EvalFrameDefault()` function is recursive, because sometimes
the interpreter calls some C function that calls back into the interpreter.
In 3.10 and before, this was the case even when a Python function called
another Python function:
The `CALL` opcode would call the `tp_call` dispatch function of the
callee, which would extract the code object, create a new frame for the call
stack, and then call back into the interpreter.  This approach is very general
but consumes several C stack frames for each nested Python call, thereby
increasing the risk of an (unrecoverable) C stack overflow.

Since 3.11, the `CALL` instruction special-cases function objects to "inline"
the call.  When a call gets inlined, a new frame gets pushed onto the call
stack and the interpreter "jumps" to the start of the callee's bytecode.
When an inlined callee executes a `RETURN_VALUE` instruction, the frame is
popped off the call stack and the interpreter returns to its caller,
by popping a frame off the call stack and "jumping" to the return address.
There is a flag in the frame (`frame->is_entry`) that indicates whether
the frame was inlined (set if it wasn't).
If `RETURN_VALUE` finds this flag set, it performs the usual cleanup and
returns from `_PyEval_EvalFrameDefault()` altogether, to a C caller.

A similar check is performed when an unhandled exception occurs.

## The call stack

Up through 3.10, the call stack was implemented as a singly-linked list of
[frame objects](frames.md). This was expensive because each call would require a
heap allocation for the stack frame.

Since 3.11, frames are no longer fully-fledged objects. Instead, a leaner internal
`_PyInterpreterFrame` structure is used, which is allocated using a custom allocator
function (`_PyThreadState_BumpFramePointer()`), which allocates and initializes a
frame structure. Usually a frame allocation is just a pointer bump, which improves
memory locality.

Sometimes an actual `PyFrameObject` is needed, such as when Python code calls
`sys._getframe()` or an extension module calls
[`PyEval_GetFrame()`](https://docs.python.org/3/c-api/reflection.html#c.PyEval_GetFrame).
In this case we allocate a proper `PyFrameObject` and initialize it from the
`_PyInterpreterFrame`.

Things get more complicated when generators are involved, since those do not
follow the push/pop model. This includes async functions, which are based on
the same mechanism.  A generator object has space for a `_PyInterpreterFrame`
structure, including the variable-size part (used for locals and the eval stack).
When a generator (or async) function is first called, a special opcode
`RETURN_GENERATOR` is executed, which is responsible for creating the
generator object.  The generator object's `_PyInterpreterFrame` is initialized
with a copy of the current stack frame.  The current stack frame is then popped
off the frame stack and the generator object is returned.
(Details differ depending on the `is_entry` flag.)
When the generator is resumed, the interpreter pushes its `_PyInterpreterFrame`
onto the frame stack and resumes execution.
See also the [generators](generators.md) section.

<!--

## All sorts of variables

The bytecode compiler determines the scope in which each variable name is defined,
and generates instructions accordingly.  For example, loading a local variable
onto the stack is done using `LOAD_FAST`, while loading a global is done using
`LOAD_GLOBAL`.
The key types of variables are:

- fast locals: used in functions
- (slow or regular) locals: used in classes and at the top level
- globals and builtins: the compiler cannot distinguish between globals and
builtins (though at runtime, the specializing interpreter can)
- cells: used for nonlocal references

(TODO: Write the rest of this section. Alas, the author got distracted and won't have time to continue this for a while.)

-->

<!--

Other topics
------------

(TODO: Each of the following probably deserves its own section.)

- co_consts, co_names, co_varnames, and their ilk
- How calls work (how args are transferred, return, exceptions)
- Eval breaker (interrupts, GIL)
- Tracing
- Setting the current lineno (debugger-induced jumps)
- Specialization, inline caches etc.

-->

## Introducing a new bytecode instruction

It is occasionally necessary to add a new opcode in order to implement
a new feature or change the way that existing features are compiled.
This section describes the changes required to do this.

First, you must choose a name for the bytecode, implement it in
[`Python/bytecodes.c`](../Python/bytecodes.c) and add a documentation
entry in [`Doc/library/dis.rst`](../Doc/library/dis.rst).
Then run `make regen-cases` to assign a number for it (see
[`Include/opcode_ids.h`](../Include/opcode_ids.h)) and regenerate a
number of files with the actual implementation of the bytecode in
[`Python/generated_cases.c.h`](../Python/generated_cases.c.h) and
metadata about it in additional files.

With a new bytecode you must also change what is called the "magic number" for
.pyc files: bump the value of the variable `MAGIC_NUMBER` in
[`Lib/importlib/_bootstrap_external.py`](../Lib/importlib/_bootstrap_external.py).
Changing this number will lead to all .pyc files with the old `MAGIC_NUMBER`
to be recompiled by the interpreter on import.  Whenever `MAGIC_NUMBER` is
changed, the ranges in the `magic_values` array in
[`PC/launcher.c`](../PC/launcher.c) may also need to be updated.  Changes to
[`Lib/importlib/_bootstrap_external.py`](../Lib/importlib/_bootstrap_external.py)
will take effect only after running `make regen-importlib`.

> [!NOTE]
> Running `make regen-importlib` before adding the new bytecode target to
> [`Python/bytecodes.c`](../Python/bytecodes.c)
> (followed by `make regen-cases`) will result in an error. You should only run
> `make regen-importlib` after the new bytecode target has been added.

> [!NOTE]
> On Windows, running the `./build.bat` script will automatically
> regenerate the required files without requiring additional arguments.

Finally, you need to introduce the use of the new bytecode.  Update
[`Python/codegen.c`](../Python/codegen.c) to emit code with this bytecode.
Optimizations in [`Python/flowgraph.c`](../Python/flowgraph.c) may also
need to be updated.  If the new opcode affects a control flow or the block
stack, you may have to update the `frame_setlineno()` function in
[`Objects/frameobject.c`](../Objects/frameobject.c).  It may also be necessary
to update [`Lib/dis.py`](../Lib/dis.py) if the new opcode interprets its
argument in a special way (like `FORMAT_VALUE` or `MAKE_FUNCTION`).

If you make a change here that can affect the output of bytecode that
is already in existence and you do not change the magic number, make
sure to delete your old .py(c|o) files!  Even though you will end up changing
the magic number if you change the bytecode, while you are debugging your work
you may be changing the bytecode output without constantly bumping up the
magic number.  This can leave you with stale .pyc files that will not be
recreated.
Running `find . -name '*.py[co]' -exec rm -f '{}' +` should delete all .pyc
files you have, forcing new ones to be created and thus allow you test out your
new bytecode properly.  Run `make regen-importlib` for updating the
bytecode of frozen importlib files.  You have to run `make` again after this
to recompile the generated C files.

## Specialization

Bytecode specialization, which was introduced in
[PEP 659](https://peps.python.org/pep-0659/), speeds up program execution by
rewriting instructions based on runtime information. This is done by replacing
a generic instruction with a faster version that works for the case that this
program encounters. Each specializable instruction is responsible for rewriting
itself, using its [inline caches](#inline-cache-entries) for
bookkeeping.

When an adaptive instruction executes, it may attempt to specialize itself,
depending on the argument and the contents of its cache. This is done
by calling one of the `_Py_Specialize_XXX` functions in
[`Python/specialize.c`](../Python/specialize.c).


The specialized instructions are responsible for checking that the special-case
assumptions still apply, and de-optimizing back to the generic version if not.

## Families of instructions

A *family* of instructions consists of an adaptive instruction along with the
specialized instructions that it can be replaced by.
It has the following fundamental properties:

* It corresponds to a single instruction in the code
  generated by the bytecode compiler.
* It has a single adaptive instruction that records an execution count and,
  at regular intervals, attempts to specialize itself. If not specializing,
  it executes the base implementation.
* It has at least one specialized form of the instruction that is tailored
  for a particular value or set of values at runtime.
* All members of the family must have the same number of inline cache entries,
  to ensure correct execution.
  Individual family members do not need to use all of the entries,
  but must skip over any unused entries when executing.

The current implementation also requires the following,
although these are not fundamental and may change:

* All families use one or more inline cache entries,
  the first entry is always the counter.
* All instruction names should start with the name of the adaptive
  instruction.
* Specialized forms should have names describing their specialization.

## Example family

The `LOAD_GLOBAL` instruction (in [Python/bytecodes.c](../Python/bytecodes.c))
already has an adaptive family that serves as a relatively simple example.

The `LOAD_GLOBAL` instruction performs adaptive specialization,
calling `_Py_Specialize_LoadGlobal()` when the counter reaches zero.

There are two specialized instructions in the family, `LOAD_GLOBAL_MODULE`
which is specialized for global variables in the module, and
`LOAD_GLOBAL_BUILTIN` which is specialized for builtin variables.

## Performance analysis

The benefit of a specialization can be assessed with the following formula:
`Tbase/Tadaptive`.

Where `Tbase` is the mean time to execute the base instruction,
and `Tadaptive` is the mean time to execute the specialized and adaptive forms.

`Tadaptive = (sum(Ti*Ni) + Tmiss*Nmiss)/(sum(Ni)+Nmiss)`

`Ti` is the time to execute the `i`th instruction in the family and `Ni` is
the number of times that instruction is executed.
`Tmiss` is the time to process a miss, including de-optimzation
and the time to execute the base instruction.

The ideal situation is where misses are rare and the specialized
forms are much faster than the base instruction.
`LOAD_GLOBAL` is near ideal, `Nmiss/sum(Ni) ≈ 0`.
In which case we have `Tadaptive ≈ sum(Ti*Ni)`.
Since we can expect the specialized forms `LOAD_GLOBAL_MODULE` and
`LOAD_GLOBAL_BUILTIN` to be much faster than the adaptive base instruction,
we would expect the specialization of `LOAD_GLOBAL` to be profitable.

## Design considerations

While `LOAD_GLOBAL` may be ideal, instructions like `LOAD_ATTR` and
`CALL_FUNCTION` are not. For maximum performance we want to keep `Ti`
low for all specialized instructions and `Nmiss` as low as possible.

Keeping `Nmiss` low means that there should be specializations for almost
all values seen by the base instruction. Keeping `sum(Ti*Ni)` low means
keeping `Ti` low which means minimizing branches and dependent memory
accesses (pointer chasing). These two objectives may be in conflict,
requiring judgement and experimentation to design the family of instructions.

The size of the inline cache should as small as possible,
without impairing performance, to reduce the number of
`EXTENDED_ARG` jumps, and to reduce pressure on the CPU's data cache.

### Gathering data

Before choosing how to specialize an instruction, it is important to gather
some data. What are the patterns of usage of the base instruction?
Data can best be gathered by instrumenting the interpreter. Since a
specialization function and adaptive instruction are going to be required,
instrumentation can most easily be added in the specialization function.

### Choice of specializations

The performance of the specializing adaptive interpreter relies on the
quality of specialization and keeping the overhead of specialization low.

Specialized instructions must be fast. In order to be fast,
specialized instructions should be tailored for a particular
set of values that allows them to:

1. Verify that incoming value is part of that set with low overhead.
2. Perform the operation quickly.

This requires that the set of values is chosen such that membership can be
tested quickly and that membership is sufficient to allow the operation to be
performed quickly.

For example, `LOAD_GLOBAL_MODULE` is specialized for `globals()`
dictionaries that have a keys with the expected version.

This can be tested quickly:

* `globals->keys->dk_version == expected_version`

and the operation can be performed quickly:

* `value = entries[cache->index].me_value;`.

Because it is impossible to measure the performance of an instruction without
also measuring unrelated factors, the assessment of the quality of a
specialization will require some judgement.

As a general rule, specialized instructions should be much faster than the
base instruction.

### Implementation of specialized instructions

In general, specialized instructions should be implemented in two parts:

1. A sequence of guards, each of the form
  `DEOPT_IF(guard-condition-is-false, BASE_NAME)`.
2. The operation, which should ideally have no branches and
  a minimum number of dependent memory accesses.

In practice, the parts may overlap, as data required for guards
can be re-used in the operation.

If there are branches in the operation, then consider further specialization
to eliminate the branches.

### Maintaining stats

Finally, take care that stats are gathered correctly.
After the last `DEOPT_IF` has passed, a hit should be recorded with
`STAT_INC(BASE_INSTRUCTION, hit)`.
After an optimization has been deferred in the adaptive instruction,
that should be recorded with `STAT_INC(BASE_INSTRUCTION, deferred)`.


Additional resources
--------------------

* Brandt Bucher's talk about the specializing interpreter at PyCon US 2023.
  [Slides](https://github.com/brandtbucher/brandtbucher/blob/master/2023/04/21/inside_cpython_311s_new_specializing_adaptive_interpreter.pdf)
  [Video](https://www.youtube.com/watch?v=PGZPSWZSkJI&t=1470s)
