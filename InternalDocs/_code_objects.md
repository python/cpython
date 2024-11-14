
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

The locations table
-------------------

Whenever an exception is raised, we add a traceback entry to the exception.
The ``tb_lineno`` field of a traceback entry is (lazily) set to the line number of the instruction that raised it.
This field is computed from the locations table, ``co_linetable`` (this name is an understatement), using :c:func:`PyCode_Addr2Line`.
This table has an entry for every instruction rather than for every ``try`` block, so a compact format is very important.

The full design of the 3.11 locations table is written up in :cpy-file:`InternalDocs/locations.md`.
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


TODO:
- co_consts, co_names, co_varnames, and their ilk
