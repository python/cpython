.. _interpreter:

===============================
The bytecode interpreter (3.11)
===============================

.. highlight:: c

Preface
=======

The CPython 3.11 bytecode interpreter (a.k.a. virtual machine) has a number of improvements over 3.10.
We describe the inner workings of the 3.11 interpreter here, with an emphasis on understanding not just the code but its design.
While the interpreter is forever evolving, and the 3.12 design will undoubtedly be different again, knowing the 3.11 design will help you understand future improvements to the interpreter.

Other sources
-------------

* Brandt Bucher's talk about the specializing interpreter at PyCon US 2023.
  `Slides <https://github.com/brandtbucher/brandtbucher/blob/master/2023/04/21/inside_cpython_311s_new_specializing_adaptive_interpreter.pdf>`_
  `Video <https://www.youtube.com/watch?v=PGZPSWZSkJI&t=1470s>`_

Introduction
============

The job of the bytecode interpreter, in :cpy-file:`Python/ceval.c`, is to execute Python code.
Its main input is a code object, although this is not a direct argument to the interpreter.
The interpreter is structured as a (recursive) function taking a thread state (``tstate``) and a stack frame (``frame``).
The function also takes an integer ``throwflag``, which is used by the implementation of ``generator.throw``.
It returns a new reference to a Python object (``PyObject *``) or an error indicator, ``NULL``.
Per :pep:`523`, this function is configurable by setting ``interp->eval_frame``; we describe only the default function, ``_PyEval_EvalFrameDefault()``.
(This function's signature has evolved and no longer matches what PEP 523 specifies; the thread state argument is added and the stack frame argument is no longer an object.)

The interpreter finds the code object by looking in the stack frame (``frame->f_code``).
Various other items needed by the interpreter (e.g. globals and builtins) are also accessed via the stack frame.
The thread state stores exception information and a variety of other information, such as the recursion depth.
The thread state is also used to access per-interpreter state (``tstate->interp``) and per-runtime (i.e., truly global) state (``tstate->interp->runtime``).

Note the slightly confusing terminology here.
"Interpreter" refers to the bytecode interpreter, a recursive function.
"Interpreter state" refers to state shared by threads, each of which may be running its own bytecode interpreter.
A single process may even host multiple interpreters, each with their own interpreter state, but sharing runtime state.
The topic of multiple interpreters is covered by several PEPs, notably :pep:`684`, :pep:`630`, and :pep:`554` (with more coming).
The current document focuses on the bytecode interpreter.

Code objects
============

The interpreter uses a code object (``frame->f_code``) as its starting point.
Code objects contain many fields used by the interpreter, as well as some for use by debuggers and other tools.
In 3.11, the final field of a code object is an array of indeterminate length containing the bytecode, ``code->co_code_adaptive``.
(In previous versions the code object was a :class:`bytes` object, ``code->co_code``; it was changed to save an allocation and to allow it to be mutated.)

Code objects are typically produced by the bytecode :ref:`compiler <compiler>`, although they are often written to disk by one process and read back in by another.
The disk version of a code object is serialized using the :mod:`marshal` protocol.
Some code objects are pre-loaded into the interpreter using ``Tools/scripts/deepfreeze.py``, which writes ``Python/deepfreeze/deepfreeze.c``.

Code objects are nominally immutable.
Some fields (including ``co_code_adaptive``) are mutable, but mutable fields are not included when code objects are hashed or compared.

Instruction decoding
====================

The first task of the interpreter is to decode the bytecode instructions.
Bytecode is stored as an array of 16-bit code units (``_Py_CODEUNIT``).
Each code unit contains an 8-bit ``opcode`` and an 8-bit argument (``oparg``), both unsigned.
In order to make the bytecode format independent of the machine byte order when stored on disk, ``opcode`` is always the first byte and ``oparg`` is always the second byte.
Macros are used to extract the ``opcode`` and ``oparg`` from a code unit (``_Py_OPCODE(word)`` and ``_Py_OPARG(word)``).
Some instructions (e.g. ``NOP`` or ``POP_TOP``) have no argument -- in this case we ignore ``oparg``.

A simple instruction decoding loop would look like this:

.. code-block:: c

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

This format supports 256 different opcodes, which is sufficient.
However, it also limits ``oparg`` to 8-bit values, which is not.
To overcome this, the ``EXTENDED_ARG`` opcode allows us to prefix any instruction with one or more additional data bytes.
For example, this sequence of code units::

    EXTENDED_ARG  1
    EXTENDED_ARG  0
    LOAD_CONST    2

would set ``opcode`` to ``LOAD_CONST`` and ``oparg`` to ``65538`` (i.e., ``0x1_00_02``).
The compiler should limit itself to at most three ``EXTENDED_ARG`` prefixes, to allow the resulting ``oparg`` to fit in 32 bits, but the interpreter does not check this.
A series of code units starting with zero to three ``EXTENDED_ARG`` opcodes followed by a primary opcode is called a complete instruction, to distinguish it from a single code unit, which is always two bytes.
The following loop, to be inserted just above the ``switch`` statement, will make the above snippet decode a complete instruction:

.. code-block:: c

    while (opcode == EXTENDED_ARG) {
        word = *next_instr++;
        opcode = _Py_OPCODE(word);
        oparg = (oparg << 8) | _Py_OPARG(word);
    }

For various reasons we'll get to later (mostly efficiency, given that ``EXTENDED_ARG`` is rare) the actual code is different.

Jumps
=====

Note that when the ``switch`` statement is reached, ``next_instr`` (the "instruction offset") already points to the next instruction.
Thus, jump instructions can be implemented by manipulating ``next_instr``:

- An absolute jump (``JUMP_ABSOLUTE``) sets ``next_instr = first_instr + oparg``.
- A relative jump forward (``JUMP_FORWARD``) sets ``next_instr += oparg``.
- A relative jump backward sets ``next_instr -= oparg``.

A relative jump whose ``oparg`` is zero is a no-op.

Inline cache entries
====================

Some (specialized or specializable) instructions have an associated "inline cache".
The inline cache consists of one or more two-byte entries included in the bytecode array as additional words following the ``opcode`` /``oparg`` pair.
The size of the inline cache for a particular instruction is fixed by its ``opcode`` alone.
Moreover, the inline cache size for a family of specialized/specializable instructions (e.g., ``LOAD_ATTR``, ``LOAD_ATTR_SLOT``, ``LOAD_ATTR_MODULE``) must all be the same.
Cache entries are reserved by the compiler and initialized with zeros.
If an instruction has an inline cache, the layout of its cache can be described by a ``struct`` definition and the address of the cache is given by casting ``next_instr`` to a pointer to the cache ``struct``.
The size of such a ``struct`` must be independent of the machine architecture, word size and alignment requirements.
For 32-bit fields, the ``struct`` should use ``_Py_CODEUNIT field[2]``.
Even though inline cache entries are represented by code units, they do not have to conform to the ``opcode`` / ``oparg`` format.

The instruction implementation is responsible for advancing ``next_instr`` past the inline cache.
For example, if an instruction's inline cache is four bytes (i.e., two code units) in size, the code for the instruction must contain ``next_instr += 2;``.
This is equivalent to a relative forward jump by that many code units.
(The proper way to code this is ``JUMPBY(n)``, where ``n`` is the number of code units to jump, typically given as a named constant.)

Serializing non-zero cache entries would present a problem because the serialization (:mod:`marshal`) format must be independent of the machine byte order.

More information about the use of inline caches :pep:`can be found in PEP 659 <659#ancillary-data>`.

The evaluation stack
====================

Apart from unconditional jumps, almost all instructions read or write some data in the form of object references (``PyObject *``).
The CPython 3.11 bytecode interpreter is a stack machine, meaning that it operates by pushing data onto and popping it off the stack.
The stack is a pre-allocated array of object references.
For example, the "add" instruction (which used to be called ``BINARY_ADD`` in 3.10 but is now ``BINARY_OP 0``) pops two objects off the stack and pushes the result back onto the stack.
An interesting property of the CPython bytecode interpreter is that the stack size required to evaluate a given function is known in advance.
The stack size is computed by the bytecode compiler and is stored in ``code->co_stacksize``.
The interpreter uses this information to allocate stack.

The stack grows up in memory; the operation ``PUSH(x)`` is equivalent to ``*stack_pointer++ = x``, whereas ``x = POP()`` means ``x = *--stack_pointer``.
There is no overflow or underflow check (except when compiled in debug mode) -- it would be too expensive, so we really trust the compiler.

At any point during execution, the stack level is knowable based on the instruction pointer alone, and some properties of each item on the stack are also known.
In particular, only a few instructions may push a ``NULL`` onto the stack, and the positions that may be ``NULL`` are known.
A few other instructions (``GET_ITER``, ``FOR_ITER``) push or pop an object that is known to be an iterator.

Instruction sequences that do not allow statically knowing the stack depth are deemed illegal.
The bytecode compiler never generates such sequences.
For example, the following sequence is illegal, because it keeps pushing items on the stack::

    LOAD_FAST 0
    JUMP_BACKWARD 2

Do not confuse the evaluation stack with the call stack, which is used to implement calling and returning from functions.

Error handling
==============

When an instruction like ``BINARY_OP`` encounters an error, an exception is raised.
At this point, a traceback entry is added to the exception (by ``PyTraceBack_Here()``) and cleanup is performed.
In the simplest case (absent any ``try`` blocks), this results in the remaining objects being popped off the evaluation stack and their reference count decremented (if not ``NULL``) .
Then the interpreter function (``_PyEval_EvalFrameDefault()``) returns ``NULL``.

However, if an exception is raised in a ``try`` block, the interpreter must jump to the corresponding ``except`` or ``finally`` block.
In 3.10 and before, there was a separate "block stack" which was used to keep track of nesting ``try`` blocks.
In 3.11, this mechanism has been replaced by a statically generated table, ``code->co_exceptiontable``.
The advantage of this approach is that entering and leaving a ``try`` block normally does not execute any code, making execution faster.
But of course, this table needs to be generated by the compiler, and decoded (by ``get_exception_handler``) when an exception happens.

Exception table format
----------------------

The table is conceptually a list of records, each containing four variable-length integer fields (in a unique format, see below):

- start: start of ``try`` block, in code units from the start of the bytecode
- length: size of the ``try`` block, in code units
- target: start of the first instruction of the ``except`` or ``finally`` block, in code units from the start of the bytecode
- depth_and_lasti: the low bit gives the "lasti" flag, the remaining bits give the stack depth

The stack depth is used to clean up evaluation stack entries above this depth.
The "lasti" flag indicates whether, after stack cleanup, the instruction offset of the raising instruction should be pushed (as a ``PyLongObject *``).
For more information on the design, see :cpy-file:`Objects/exception_handling_notes.txt`.

Each varint is encoded as one or more bytes.
The high bit (bit 7) is reserved for random access -- it is set for the first varint of a record.
The second bit (bit 6) indicates whether this is the last byte or not -- it is set for all but the last bytes of a varint.
The low 6 bits (bits 0-5) are used for the integer value, in big-endian order.

To find the table entry (if any) for a given instruction offset, we can use bisection without decoding the whole table.
We bisect the raw bytes, at each probe finding the start of the record by scanning back for a byte with the high bit set, and then decode the first varint.
See ``get_exception_handler()`` in :cpy-file:`Python/ceval.c` for the exact code (like all bisection algorithms, the code is a bit subtle).

The locations table
-------------------

Whenever an exception is raised, we add a traceback entry to the exception.
The ``tb_lineno`` field of a traceback entry must be set to the line number of the instruction that raised it.
This field is computed from the locations table, ``co_linetable`` (this name is an understatement), using :c:func:`PyCode_Addr2Line`.
This table has an entry for every instruction rather than for every ``try`` block, so a compact format is very important.

The full design of the 3.11 locations table is written up in :cpy-file:`Objects/locations.md`.
While there are rumors that this file is slightly out of date, it is still the best reference we have.
Don't be confused by :cpy-file:`Objects/lnotab_notes.txt`, which describes the 3.10 format.
For backwards compatibility this format is still supported by the ``co_lnotab`` property.

The 3.11 location table format is different because it stores not just the starting line number for each instruction, but also the end line number, *and* the start and end column numbers.
Note that traceback objects don't store all this information -- they store the start line number, for backward compatibility, and the "last instruction" value.
The rest can be computed from the last instruction (``tb_lasti``) with the help of the locations table.
For Python code, a convenient method exists, :meth:`~codeobject.co_positions`, which returns an iterator of :samp:`({line}, {endline}, {column}, {endcolumn})` tuples, one per instruction.
There is also ``co_lines()`` which returns an iterator of :samp:`({start}, {end}, {line})` tuples, where :samp:`{start}` and :samp:`{end}` are bytecode offsets.
The latter is described by :pep:`626`; it is more compact, but doesn't return end line numbers or column offsets.
From C code, you have to call :c:func:`PyCode_Addr2Location`.

Fortunately, the locations table is only consulted by exception handling (to set ``tb_lineno``) and by tracing (to pass the line number to the tracing function).
In order to reduce the overhead during tracing, the mapping from instruction offset to line number is cached in the ``_co_linearray`` field.

Exception chaining
------------------

When an exception is raised during exception handling, the new exception is chained to the old one.
This is done by making the ``__context__`` field of the new exception point to the old one.
This is the responsibility of ``_PyErr_SetObject()`` in :cpy-file:`Python/errors.c` (which is ultimately called by all ``PyErr_Set*()`` functions).
Separately, if a statement of the form :samp:`raise {X} from {Y}` is executed, the ``__cause__`` field of the raised exception (:samp:`{X}`) is set to :samp:`{Y}`.
This is done by :c:func:`PyException_SetCause`, called in response to all ``RAISE_VARARGS`` instructions.
A special case is :samp:`raise {X} from None`, which sets the ``__cause__`` field to ``None`` (at the C level, it sets ``cause`` to ``NULL``).

(TODO: Other exception details.)

Python-to-Python calls
======================

The ``_PyEval_EvalFrameDefault()`` function is recursive, because sometimes the interpreter calls some C function that calls back into the interpreter.
In 3.10 and before, this was the case even when a Python function called another Python function:
The ``CALL`` instruction would call the ``tp_call`` dispatch function of the callee, which would extract the code object, create a new frame for the call stack, and then call back into the interpreter.
This approach is very general but consumes several C stack frames for each nested Python call, thereby increasing the risk of an (unrecoverable) C stack overflow.

In 3.11, the ``CALL`` instruction special-cases function objects to "inline" the call.
When a call gets inlined, a new frame gets pushed onto the call stack and the interpreter "jumps" to the start of the callee's bytecode.
When an inlined callee executes a ``RETURN_VALUE`` instruction, the frame is popped off the call stack and the interpreter returns to its caller,
by popping a frame off the call stack and "jumping" to the return address.
There is a flag in the frame (``frame->is_entry``) that indicates whether the frame was inlined (set if it wasn't).
If ``RETURN_VALUE`` finds this flag set, it performs the usual cleanup and returns from ``_PyEval_EvalFrameDefault()`` altogether, to a C caller.

A similar check is performed when an unhandled exception occurs.

The call stack
==============

Up through 3.10, the call stack used to be implemented as a singly-linked list of :c:type:`PyFrameObject` objects.
This was expensive because each call would require a heap allocation for the stack frame.
(There was some optimization using a free list, but this was not always effective, because frames are variable length.)

In 3.11, frames are no longer fully-fledged objects.
Instead, a leaner internal ``_PyInterpreterFrame`` structure is used, which is allocated using a custom allocator, ``_PyThreadState_BumpFramePointer()``.
Usually a frame allocation is just a pointer bump, which improves memory locality.
The function ``_PyEvalFramePushAndInit()`` allocates and initializes a frame structure.

Sometimes an actual ``PyFrameObject`` is needed, usually because some Python code calls :func:`sys._getframe` or an extension module calls :c:func:`PyEval_GetFrame`.
In this case we allocate a proper ``PyFrameObject`` and initialize it from the ``_PyInterpreterFrame``.
This is a pessimization, but fortunately happens rarely (as introspecting frames is not a common operation).

Things get more complicated when generators are involved, since those don't follow the push/pop model.
(The same applies to async functions, which are implemented using the same infrastructure.)
A generator object has space for a ``_PyInterpreterFrame`` structure, including the variable-size part (used for locals and eval stack).
When a generator (or async) function is first called, a special opcode ``RETURN_GENERATOR`` is executed, which is responsible for creating the generator object.
The generator object's ``_PyInterpreterFrame`` is initialized with a copy of the current stack frame.
The current stack frame is then popped off the stack and the generator object is returned.
(Details differ depending on the ``is_entry`` flag.)
When the generator is resumed, the interpreter pushes the ``_PyInterpreterFrame`` onto the stack and resumes execution.
(There is more hairiness for generators and their ilk; we'll discuss these in a later section in more detail.)

(TODO: Also frame layout and use, and "locals plus".)

All sorts of variables
======================

The bytecode compiler determines the scope in which each variable name is defined, and generates instructions accordingly.
For example, loading a local variable onto the stack is done using ``LOAD_FAST``, while loading a global is done using ``LOAD_GLOBAL``.
The key types of variables are:

- fast locals: used in functions
- (slow or regular) locals: used in classes and at the top level
- globals and builtins: the compiler does not distinguish between globals and builtins (though the specializing interpreter does)
- cells: used for nonlocal references

(TODO: Write the rest of this section. Alas, the author got distracted and won't have time to continue this for a while.)

Other topics
============

(TODO: Each of the following probably deserves its own section.)

- co_consts, co_names, co_varnames, and their ilk
- How calls work (how args are transferred, return, exceptions)
- Generators, async functions, async generators, and ``yield from`` (next, send, throw, close; and await; and how this code breaks the interpreter abstraction)
- Eval breaker (interrupts, GIL)
- Tracing
- Setting the current lineno (debugger-induced jumps)
- Specialization, inline caches etc.


Introducing new bytecode
========================

.. note::

   This section is relevant if you are adding a new bytecode to the interpreter.


Sometimes a new feature requires a new opcode.  But adding new bytecode is
not as simple as just suddenly introducing new bytecode in the AST ->
bytecode step of the compiler.  Several pieces of code throughout Python depend
on having correct information about what bytecode exists.

First, you must choose a name, implement the bytecode in
:cpy-file:`Python/bytecodes.c`, and add a documentation entry in
:cpy-file:`Doc/library/dis.rst`. Then run ``make regen-cases`` to
assign a number for it (see :cpy-file:`Include/opcode_ids.h`) and
regenerate a number of files with the actual implementation of the
bytecodes (:cpy-file:`Python/generated_cases.c.h`) and additional
files with metadata about them.

With a new bytecode you must also change what is called the magic number for
.pyc files.  The variable ``MAGIC_NUMBER`` in
:cpy-file:`Lib/importlib/_bootstrap_external.py` contains the number.
Changing this number will lead to all .pyc files with the old ``MAGIC_NUMBER``
to be recompiled by the interpreter on import.  Whenever ``MAGIC_NUMBER`` is
changed, the ranges in the ``magic_values`` array in :cpy-file:`PC/launcher.c`
must also be updated.  Changes to :cpy-file:`Lib/importlib/_bootstrap_external.py`
will take effect only after running ``make regen-importlib``. Running this
command before adding the new bytecode target to :cpy-file:`Python/bytecodes.c`
(followed by ``make regen-cases``) will result in an error. You should only run
``make regen-importlib`` after the new bytecode target has been added.

.. note:: On Windows, running the ``./build.bat`` script will automatically
   regenerate the required files without requiring additional arguments.

Finally, you need to introduce the use of the new bytecode.  Altering
:cpy-file:`Python/compile.c`, :cpy-file:`Python/bytecodes.c` will be the
primary places to change. Optimizations in :cpy-file:`Python/flowgraph.c`
may also need to be updated.
If the new opcode affects a control flow or the block stack, you may have
to update the ``frame_setlineno()`` function in :cpy-file:`Objects/frameobject.c`.
:cpy-file:`Lib/dis.py` may need an update if the new opcode interprets its
argument in a special way (like ``FORMAT_VALUE`` or ``MAKE_FUNCTION``).

If you make a change here that can affect the output of bytecode that
is already in existence and you do not change the magic number constantly, make
sure to delete your old .py(c|o) files!  Even though you will end up changing
the magic number if you change the bytecode, while you are debugging your work
you will be changing the bytecode output without constantly bumping up the
magic number.  This means you end up with stale .pyc files that will not be
recreated.
Running ``find . -name '*.py[co]' -exec rm -f '{}' +`` should delete all .pyc
files you have, forcing new ones to be created and thus allow you test out your
new bytecode properly.  Run ``make regen-importlib`` for updating the
bytecode of frozen importlib files.  You have to run ``make`` again after this
for recompiling generated C files.
