:mod:`dis` --- Disassembler for Python bytecode
===============================================

.. module:: dis
   :synopsis: Disassembler for Python bytecode.

**Source code:** :source:`Lib/dis.py`

--------------

The :mod:`dis` module supports the analysis of CPython :term:`bytecode` by
disassembling it. The CPython bytecode which this module takes as an input is
defined in the file :file:`Include/opcode.h` and used by the compiler and the
interpreter.

.. impl-detail::

   Bytecode is an implementation detail of the CPython interpreter.  No
   guarantees are made that bytecode will not be added, removed, or changed
   between versions of Python.  Use of this module should not be considered to
   work across Python VMs or Python releases.

   .. versionchanged:: 3.6
      Use 2 bytes for each instruction. Previously the number of bytes varied
      by instruction.


Example: Given the function :func:`myfunc`::

   def myfunc(alist):
       return len(alist)

the following command can be used to display the disassembly of
:func:`myfunc`::

   >>> dis.dis(myfunc)
     2           0 LOAD_GLOBAL              0 (len)
                 2 LOAD_FAST                0 (alist)
                 4 CALL_FUNCTION            1
                 6 RETURN_VALUE

(The "2" is a line number).

Bytecode analysis
-----------------

.. versionadded:: 3.4

The bytecode analysis API allows pieces of Python code to be wrapped in a
:class:`Bytecode` object that provides easy access to details of the compiled
code.

.. class:: Bytecode(x, *, first_line=None, current_offset=None)


   Analyse the bytecode corresponding to a function, generator, asynchronous
   generator, coroutine, method, string of source code, or a code object (as
   returned by :func:`compile`).

   This is a convenience wrapper around many of the functions listed below, most
   notably :func:`get_instructions`, as iterating over a :class:`Bytecode`
   instance yields the bytecode operations as :class:`Instruction` instances.

   If *first_line* is not ``None``, it indicates the line number that should be
   reported for the first source line in the disassembled code.  Otherwise, the
   source line information (if any) is taken directly from the disassembled code
   object.

   If *current_offset* is not ``None``, it refers to an instruction offset in the
   disassembled code. Setting this means :meth:`.dis` will display a "current
   instruction" marker against the specified opcode.

   .. classmethod:: from_traceback(tb)

      Construct a :class:`Bytecode` instance from the given traceback, setting
      *current_offset* to the instruction responsible for the exception.

   .. data:: codeobj

      The compiled code object.

   .. data:: first_line

      The first source line of the code object (if available)

   .. method:: dis()

      Return a formatted view of the bytecode operations (the same as printed by
      :func:`dis.dis`, but returned as a multi-line string).

   .. method:: info()

      Return a formatted multi-line string with detailed information about the
      code object, like :func:`code_info`.

   .. versionchanged:: 3.7
      This can now handle coroutine and asynchronous generator objects.

Example::

    >>> bytecode = dis.Bytecode(myfunc)
    >>> for instr in bytecode:
    ...     print(instr.opname)
    ...
    LOAD_GLOBAL
    LOAD_FAST
    CALL_FUNCTION
    RETURN_VALUE


Analysis functions
------------------

The :mod:`dis` module also defines the following analysis functions that convert
the input directly to the desired output. They can be useful if only a single
operation is being performed, so the intermediate analysis object isn't useful:

.. function:: code_info(x)

   Return a formatted multi-line string with detailed code object information
   for the supplied function, generator, asynchronous generator, coroutine,
   method, source code string or code object.

   Note that the exact contents of code info strings are highly implementation
   dependent and they may change arbitrarily across Python VMs or Python
   releases.

   .. versionadded:: 3.2

   .. versionchanged:: 3.7
      This can now handle coroutine and asynchronous generator objects.


.. function:: show_code(x, *, file=None)

   Print detailed code object information for the supplied function, method,
   source code string or code object to *file* (or ``sys.stdout`` if *file*
   is not specified).

   This is a convenient shorthand for ``print(code_info(x), file=file)``,
   intended for interactive exploration at the interpreter prompt.

   .. versionadded:: 3.2

   .. versionchanged:: 3.4
      Added *file* parameter.


.. function:: dis(x=None, *, file=None, depth=None)

   Disassemble the *x* object.  *x* can denote either a module, a class, a
   method, a function, a generator, an asynchronous generator, a coroutine,
   a code object, a string of source code or a byte sequence of raw bytecode.
   For a module, it disassembles all functions. For a class, it disassembles
   all methods (including class and static methods). For a code object or
   sequence of raw bytecode, it prints one line per bytecode instruction.
   It also recursively disassembles nested code objects (the code of
   comprehensions, generator expressions and nested functions, and the code
   used for building nested classes).
   Strings are first compiled to code objects with the :func:`compile`
   built-in function before being disassembled.  If no object is provided, this
   function disassembles the last traceback.

   The disassembly is written as text to the supplied *file* argument if
   provided and to ``sys.stdout`` otherwise.

   The maximal depth of recursion is limited by *depth* unless it is ``None``.
   ``depth=0`` means no recursion.

   .. versionchanged:: 3.4
      Added *file* parameter.

   .. versionchanged:: 3.7
      Implemented recursive disassembling and added *depth* parameter.

   .. versionchanged:: 3.7
      This can now handle coroutine and asynchronous generator objects.


.. function:: distb(tb=None, *, file=None)

   Disassemble the top-of-stack function of a traceback, using the last
   traceback if none was passed.  The instruction causing the exception is
   indicated.

   The disassembly is written as text to the supplied *file* argument if
   provided and to ``sys.stdout`` otherwise.

   .. versionchanged:: 3.4
      Added *file* parameter.


.. function:: disassemble(code, lasti=-1, *, file=None)
              disco(code, lasti=-1, *, file=None)

   Disassemble a code object, indicating the last instruction if *lasti* was
   provided.  The output is divided in the following columns:

   #. the line number, for the first instruction of each line
   #. the current instruction, indicated as ``-->``,
   #. a labelled instruction, indicated with ``>>``,
   #. the address of the instruction,
   #. the operation code name,
   #. operation parameters, and
   #. interpretation of the parameters in parentheses.

   The parameter interpretation recognizes local and global variable names,
   constant values, branch targets, and compare operators.

   The disassembly is written as text to the supplied *file* argument if
   provided and to ``sys.stdout`` otherwise.

   .. versionchanged:: 3.4
      Added *file* parameter.


.. function:: get_instructions(x, *, first_line=None)

   Return an iterator over the instructions in the supplied function, method,
   source code string or code object.

   The iterator generates a series of :class:`Instruction` named tuples giving
   the details of each operation in the supplied code.

   If *first_line* is not ``None``, it indicates the line number that should be
   reported for the first source line in the disassembled code.  Otherwise, the
   source line information (if any) is taken directly from the disassembled code
   object.

   .. versionadded:: 3.4


.. function:: findlinestarts(code)

   This generator function uses the ``co_firstlineno`` and ``co_lnotab``
   attributes of the code object *code* to find the offsets which are starts of
   lines in the source code.  They are generated as ``(offset, lineno)`` pairs.
   See :source:`Objects/lnotab_notes.txt` for the ``co_lnotab`` format and
   how to decode it.

   .. versionchanged:: 3.6
      Line numbers can be decreasing. Before, they were always increasing.


.. function:: findlabels(code)

   Detect all offsets in the raw compiled bytecode string *code* which are jump targets, and
   return a list of these offsets.


.. function:: stack_effect(opcode, oparg=None, *, jump=None)

   Compute the stack effect of *opcode* with argument *oparg*.

   If the code has a jump target and *jump* is ``True``, :func:`~stack_effect`
   will return the stack effect of jumping.  If *jump* is ``False``,
   it will return the stack effect of not jumping. And if *jump* is
   ``None`` (default), it will return the maximal stack effect of both cases.

   .. versionadded:: 3.4

   .. versionchanged:: 3.8
      Added *jump* parameter.


.. _bytecodes:

Python Bytecode Instructions
----------------------------

The :func:`get_instructions` function and :class:`Bytecode` class provide
details of bytecode instructions as :class:`Instruction` instances:

.. class:: Instruction

   Details for a bytecode operation

   .. data:: opcode

      numeric code for operation, corresponding to the opcode values listed
      below and the bytecode values in the :ref:`opcode_collections`.


   .. data:: opname

      human readable name for operation


   .. data:: arg

      numeric argument to operation (if any), otherwise ``None``


   .. data:: argval

      resolved arg value (if known), otherwise same as arg


   .. data:: argrepr

      human readable description of operation argument


   .. data:: offset

      start index of operation within bytecode sequence


   .. data:: starts_line

      line started by this opcode (if any), otherwise ``None``


   .. data:: is_jump_target

      ``True`` if other code jumps to here, otherwise ``False``

   .. versionadded:: 3.4


The Python compiler currently generates the following bytecode instructions.


**General instructions**

.. opcode:: NOP

   Do nothing code.  Used as a placeholder by the bytecode optimizer.


.. opcode:: POP_TOP

   Removes the top-of-stack (TOS) item.


.. opcode:: ROT_TWO

   Swaps the two top-most stack items.


.. opcode:: ROT_THREE

   Lifts second and third stack item one position up, moves top down to position
   three.


.. opcode:: ROT_FOUR

   Lifts second, third and fourth stack items one position up, moves top down
   to position four.

   .. versionadded:: 3.8


.. opcode:: DUP_TOP

   Duplicates the reference on top of the stack.

   .. versionadded:: 3.2


.. opcode:: DUP_TOP_TWO

   Duplicates the two references on top of the stack, leaving them in the
   same order.

   .. versionadded:: 3.2


**Unary operations**

Unary operations take the top of the stack, apply the operation, and push the
result back on the stack.

.. opcode:: UNARY_POSITIVE

   Implements ``TOS = +TOS``.


.. opcode:: UNARY_NEGATIVE

   Implements ``TOS = -TOS``.


.. opcode:: UNARY_NOT

   Implements ``TOS = not TOS``.


.. opcode:: UNARY_INVERT

   Implements ``TOS = ~TOS``.


.. opcode:: GET_ITER

   Implements ``TOS = iter(TOS)``.


.. opcode:: GET_YIELD_FROM_ITER

   If ``TOS`` is a :term:`generator iterator` or :term:`coroutine` object
   it is left as is.  Otherwise, implements ``TOS = iter(TOS)``.

   .. versionadded:: 3.5


**Binary operations**

Binary operations remove the top of the stack (TOS) and the second top-most
stack item (TOS1) from the stack.  They perform the operation, and put the
result back on the stack.

.. opcode:: BINARY_POWER

   Implements ``TOS = TOS1 ** TOS``.


.. opcode:: BINARY_MULTIPLY

   Implements ``TOS = TOS1 * TOS``.


.. opcode:: BINARY_MATRIX_MULTIPLY

   Implements ``TOS = TOS1 @ TOS``.

   .. versionadded:: 3.5


.. opcode:: BINARY_FLOOR_DIVIDE

   Implements ``TOS = TOS1 // TOS``.


.. opcode:: BINARY_TRUE_DIVIDE

   Implements ``TOS = TOS1 / TOS``.


.. opcode:: BINARY_MODULO

   Implements ``TOS = TOS1 % TOS``.


.. opcode:: BINARY_ADD

   Implements ``TOS = TOS1 + TOS``.


.. opcode:: BINARY_SUBTRACT

   Implements ``TOS = TOS1 - TOS``.


.. opcode:: BINARY_SUBSCR

   Implements ``TOS = TOS1[TOS]``.


.. opcode:: BINARY_LSHIFT

   Implements ``TOS = TOS1 << TOS``.


.. opcode:: BINARY_RSHIFT

   Implements ``TOS = TOS1 >> TOS``.


.. opcode:: BINARY_AND

   Implements ``TOS = TOS1 & TOS``.


.. opcode:: BINARY_XOR

   Implements ``TOS = TOS1 ^ TOS``.


.. opcode:: BINARY_OR

   Implements ``TOS = TOS1 | TOS``.


**In-place operations**

In-place operations are like binary operations, in that they remove TOS and
TOS1, and push the result back on the stack, but the operation is done in-place
when TOS1 supports it, and the resulting TOS may be (but does not have to be)
the original TOS1.

.. opcode:: INPLACE_POWER

   Implements in-place ``TOS = TOS1 ** TOS``.


.. opcode:: INPLACE_MULTIPLY

   Implements in-place ``TOS = TOS1 * TOS``.


.. opcode:: INPLACE_MATRIX_MULTIPLY

   Implements in-place ``TOS = TOS1 @ TOS``.

   .. versionadded:: 3.5


.. opcode:: INPLACE_FLOOR_DIVIDE

   Implements in-place ``TOS = TOS1 // TOS``.


.. opcode:: INPLACE_TRUE_DIVIDE

   Implements in-place ``TOS = TOS1 / TOS``.


.. opcode:: INPLACE_MODULO

   Implements in-place ``TOS = TOS1 % TOS``.


.. opcode:: INPLACE_ADD

   Implements in-place ``TOS = TOS1 + TOS``.


.. opcode:: INPLACE_SUBTRACT

   Implements in-place ``TOS = TOS1 - TOS``.


.. opcode:: INPLACE_LSHIFT

   Implements in-place ``TOS = TOS1 << TOS``.


.. opcode:: INPLACE_RSHIFT

   Implements in-place ``TOS = TOS1 >> TOS``.


.. opcode:: INPLACE_AND

   Implements in-place ``TOS = TOS1 & TOS``.


.. opcode:: INPLACE_XOR

   Implements in-place ``TOS = TOS1 ^ TOS``.


.. opcode:: INPLACE_OR

   Implements in-place ``TOS = TOS1 | TOS``.


.. opcode:: STORE_SUBSCR

   Implements ``TOS1[TOS] = TOS2``.


.. opcode:: DELETE_SUBSCR

   Implements ``del TOS1[TOS]``.


**Coroutine opcodes**

.. opcode:: GET_AWAITABLE

   Implements ``TOS = get_awaitable(TOS)``, where ``get_awaitable(o)``
   returns ``o`` if ``o`` is a coroutine object or a generator object with
   the CO_ITERABLE_COROUTINE flag, or resolves
   ``o.__await__``.

   .. versionadded:: 3.5


.. opcode:: GET_AITER

   Implements ``TOS = TOS.__aiter__()``.

   .. versionadded:: 3.5
   .. versionchanged:: 3.7
      Returning awaitable objects from ``__aiter__`` is no longer
      supported.


.. opcode:: GET_ANEXT

   Implements ``PUSH(get_awaitable(TOS.__anext__()))``.  See ``GET_AWAITABLE``
   for details about ``get_awaitable``

   .. versionadded:: 3.5


.. opcode:: END_ASYNC_FOR

   Terminates an :keyword:`async for` loop.  Handles an exception raised
   when awaiting a next item.  If TOS is :exc:`StopAsyncIteration` pop 7
   values from the stack and restore the exception state using the second
   three of them.  Otherwise re-raise the exception using the three values
   from the stack.  An exception handler block is removed from the block stack.

   .. versionadded:: 3.8


.. opcode:: BEFORE_ASYNC_WITH

   Resolves ``__aenter__`` and ``__aexit__`` from the object on top of the
   stack.  Pushes ``__aexit__`` and result of ``__aenter__()`` to the stack.

   .. versionadded:: 3.5



**Miscellaneous opcodes**

.. opcode:: PRINT_EXPR

   Implements the expression statement for the interactive mode.  TOS is removed
   from the stack and printed.  In non-interactive mode, an expression statement
   is terminated with :opcode:`POP_TOP`.


.. opcode:: SET_ADD (i)

   Calls ``set.add(TOS1[-i], TOS)``.  Used to implement set comprehensions.


.. opcode:: LIST_APPEND (i)

   Calls ``list.append(TOS1[-i], TOS)``.  Used to implement list comprehensions.


.. opcode:: MAP_ADD (i)

   Calls ``dict.__setitem__(TOS1[-i], TOS1, TOS)``.  Used to implement dict
   comprehensions.

   .. versionadded:: 3.1
   .. versionchanged:: 3.8
      Map value is TOS and map key is TOS1. Before, those were reversed.

For all of the :opcode:`SET_ADD`, :opcode:`LIST_APPEND` and :opcode:`MAP_ADD`
instructions, while the added value or key/value pair is popped off, the
container object remains on the stack so that it is available for further
iterations of the loop.


.. opcode:: RETURN_VALUE

   Returns with TOS to the caller of the function.


.. opcode:: YIELD_VALUE

   Pops TOS and yields it from a :term:`generator`.


.. opcode:: YIELD_FROM

   Pops TOS and delegates to it as a subiterator from a :term:`generator`.

   .. versionadded:: 3.3


.. opcode:: SETUP_ANNOTATIONS

   Checks whether ``__annotations__`` is defined in ``locals()``, if not it is
   set up to an empty ``dict``. This opcode is only emitted if a class
   or module body contains :term:`variable annotations <variable annotation>`
   statically.

   .. versionadded:: 3.6


.. opcode:: IMPORT_STAR

   Loads all symbols not starting with ``'_'`` directly from the module TOS to
   the local namespace. The module is popped after loading all names. This
   opcode implements ``from module import *``.


.. opcode:: POP_EXCEPT

   Pops three values from the stack, which are used to restore the exception state.


.. opcode:: RERAISE

    Re-raises the exception currently on top of the stack. If oparg is non-zero,
    pops an additional value from the stack which is used to set ``f_lasti``
    of the current frame.

    .. versionadded:: 3.9


.. opcode:: PUSH_EXC_INFO

    Pops the three values from the stack. Pushes the current exception to the top of the stack.
    Pushes the three values originally popped back to the stack.
    Used in exception handlers.

    .. versionadded:: 3.11


.. opcode:: WITH_EXCEPT_START

    Calls the function in position 7 on the stack with the top three
    items on the stack as arguments.
    Used to implement the call ``context_manager.__exit__(*exc_info())`` when an exception
    has occurred in a :keyword:`with` statement.

    .. versionadded:: 3.9


.. opcode:: POP_EXCEPT_AND_RERAISE

    Pops the exception currently on top of the stack. Pops the integer value on top
    of the stack and sets the ``f_lasti`` attribute of the frame with that value.
    Then pops the next exception from the stack uses it to restore the current exception.
    Finally it re-raises the originally popped exception.
    Used in excpetion handler cleanup.

    .. versionadded:: 3.11


.. opcode:: LOAD_ASSERTION_ERROR

   Pushes :exc:`AssertionError` onto the stack.  Used by the :keyword:`assert`
   statement.

   .. versionadded:: 3.9


.. opcode:: LOAD_BUILD_CLASS

   Pushes :func:`builtins.__build_class__` onto the stack.  It is later called
   by :opcode:`CALL_FUNCTION` to construct a class.


.. opcode:: BEFORE_WITH (delta)

   This opcode performs several operations before a with block starts.  First,
   it loads :meth:`~object.__exit__` from the context manager and pushes it onto
   the stack for later use by :opcode:`WITH_EXCEPT_START`.  Then,
   :meth:`~object.__enter__` is called. Finally, the result of calling the
   ``__enter__()`` method is pushed onto the stack.

   .. versionadded:: 3.11


.. opcode:: COPY_DICT_WITHOUT_KEYS

   TOS is a tuple of mapping keys, and TOS1 is the match subject.  Replace TOS
   with a :class:`dict` formed from the items of TOS1, but without any of the
   keys in TOS.

   .. versionadded:: 3.10


.. opcode:: GET_LEN

   Push ``len(TOS)`` onto the stack.

   .. versionadded:: 3.10


.. opcode:: MATCH_MAPPING

   If TOS is an instance of :class:`collections.abc.Mapping` (or, more technically: if
   it has the :const:`Py_TPFLAGS_MAPPING` flag set in its
   :c:member:`~PyTypeObject.tp_flags`), push ``True`` onto the stack.  Otherwise, push
   ``False``.

   .. versionadded:: 3.10


.. opcode:: MATCH_SEQUENCE

   If TOS is an instance of :class:`collections.abc.Sequence` and is *not* an instance
   of :class:`str`/:class:`bytes`/:class:`bytearray` (or, more technically: if it has
   the :const:`Py_TPFLAGS_SEQUENCE` flag set in its :c:member:`~PyTypeObject.tp_flags`),
   push ``True`` onto the stack.  Otherwise, push ``False``.

   .. versionadded:: 3.10


.. opcode:: MATCH_KEYS

   TOS is a tuple of mapping keys, and TOS1 is the match subject.  If TOS1
   contains all of the keys in TOS, push a :class:`tuple` containing the
   corresponding values, followed by ``True``. Otherwise, push ``None``,
   followed by ``False``.

   .. versionadded:: 3.10


All of the following opcodes use their arguments.

.. opcode:: STORE_NAME (namei)

   Implements ``name = TOS``. *namei* is the index of *name* in the attribute
   :attr:`co_names` of the code object. The compiler tries to use
   :opcode:`STORE_FAST` or :opcode:`STORE_GLOBAL` if possible.


.. opcode:: DELETE_NAME (namei)

   Implements ``del name``, where *namei* is the index into :attr:`co_names`
   attribute of the code object.


.. opcode:: UNPACK_SEQUENCE (count)

   Unpacks TOS into *count* individual values, which are put onto the stack
   right-to-left.


.. opcode:: UNPACK_EX (counts)

   Implements assignment with a starred target: Unpacks an iterable in TOS into
   individual values, where the total number of values can be smaller than the
   number of items in the iterable: one of the new values will be a list of all
   leftover items.

   The low byte of *counts* is the number of values before the list value, the
   high byte of *counts* the number of values after it.  The resulting values
   are put onto the stack right-to-left.


.. opcode:: STORE_ATTR (namei)

   Implements ``TOS.name = TOS1``, where *namei* is the index of name in
   :attr:`co_names`.


.. opcode:: DELETE_ATTR (namei)

   Implements ``del TOS.name``, using *namei* as index into :attr:`co_names`.


.. opcode:: STORE_GLOBAL (namei)

   Works as :opcode:`STORE_NAME`, but stores the name as a global.


.. opcode:: DELETE_GLOBAL (namei)

   Works as :opcode:`DELETE_NAME`, but deletes a global name.


.. opcode:: LOAD_CONST (consti)

   Pushes ``co_consts[consti]`` onto the stack.


.. opcode:: LOAD_NAME (namei)

   Pushes the value associated with ``co_names[namei]`` onto the stack.


.. opcode:: BUILD_TUPLE (count)

   Creates a tuple consuming *count* items from the stack, and pushes the
   resulting tuple onto the stack.


.. opcode:: BUILD_LIST (count)

   Works as :opcode:`BUILD_TUPLE`, but creates a list.


.. opcode:: BUILD_SET (count)

   Works as :opcode:`BUILD_TUPLE`, but creates a set.


.. opcode:: BUILD_MAP (count)

   Pushes a new dictionary object onto the stack.  Pops ``2 * count`` items
   so that the dictionary holds *count* entries:
   ``{..., TOS3: TOS2, TOS1: TOS}``.

   .. versionchanged:: 3.5
      The dictionary is created from stack items instead of creating an
      empty dictionary pre-sized to hold *count* items.


.. opcode:: BUILD_CONST_KEY_MAP (count)

   The version of :opcode:`BUILD_MAP` specialized for constant keys. Pops the
   top element on the stack which contains a tuple of keys, then starting from
   ``TOS1``, pops *count* values to form values in the built dictionary.

   .. versionadded:: 3.6


.. opcode:: BUILD_STRING (count)

   Concatenates *count* strings from the stack and pushes the resulting string
   onto the stack.

   .. versionadded:: 3.6


.. opcode:: LIST_TO_TUPLE

   Pops a list from the stack and pushes a tuple containing the same values.

   .. versionadded:: 3.9


.. opcode:: LIST_EXTEND (i)

   Calls ``list.extend(TOS1[-i], TOS)``.  Used to build lists.

   .. versionadded:: 3.9


.. opcode:: SET_UPDATE (i)

   Calls ``set.update(TOS1[-i], TOS)``.  Used to build sets.

   .. versionadded:: 3.9


.. opcode:: DICT_UPDATE (i)

   Calls ``dict.update(TOS1[-i], TOS)``.  Used to build dicts.

   .. versionadded:: 3.9


.. opcode:: DICT_MERGE (i)

   Like :opcode:`DICT_UPDATE` but raises an exception for duplicate keys.

   .. versionadded:: 3.9


.. opcode:: LOAD_ATTR (namei)

   Replaces TOS with ``getattr(TOS, co_names[namei])``.


.. opcode:: COMPARE_OP (opname)

   Performs a Boolean operation.  The operation name can be found in
   ``cmp_op[opname]``.


.. opcode:: IS_OP (invert)

   Performs ``is`` comparison, or ``is not`` if ``invert`` is 1.

   .. versionadded:: 3.9


.. opcode:: CONTAINS_OP (invert)

   Performs ``in`` comparison, or ``not in`` if ``invert`` is 1.

   .. versionadded:: 3.9


.. opcode:: IMPORT_NAME (namei)

   Imports the module ``co_names[namei]``.  TOS and TOS1 are popped and provide
   the *fromlist* and *level* arguments of :func:`__import__`.  The module
   object is pushed onto the stack.  The current namespace is not affected: for
   a proper import statement, a subsequent :opcode:`STORE_FAST` instruction
   modifies the namespace.


.. opcode:: IMPORT_FROM (namei)

   Loads the attribute ``co_names[namei]`` from the module found in TOS. The
   resulting object is pushed onto the stack, to be subsequently stored by a
   :opcode:`STORE_FAST` instruction.


.. opcode:: JUMP_FORWARD (delta)

   Increments bytecode counter by *delta*.


.. opcode:: POP_JUMP_IF_TRUE (target)

   If TOS is true, sets the bytecode counter to *target*.  TOS is popped.

   .. versionadded:: 3.1


.. opcode:: POP_JUMP_IF_FALSE (target)

   If TOS is false, sets the bytecode counter to *target*.  TOS is popped.

   .. versionadded:: 3.1

.. opcode:: JUMP_IF_NOT_EXC_MATCH (target)

   Tests whether the second value on the stack is an exception matching TOS,
   and jumps if it is not. Pops two values from the stack.

   .. versionadded:: 3.9


.. opcode:: JUMP_IF_TRUE_OR_POP (target)

   If TOS is true, sets the bytecode counter to *target* and leaves TOS on the
   stack.  Otherwise (TOS is false), TOS is popped.

   .. versionadded:: 3.1


.. opcode:: JUMP_IF_FALSE_OR_POP (target)

   If TOS is false, sets the bytecode counter to *target* and leaves TOS on the
   stack.  Otherwise (TOS is true), TOS is popped.

   .. versionadded:: 3.1


.. opcode:: JUMP_ABSOLUTE (target)

   Set bytecode counter to *target*.


.. opcode:: FOR_ITER (delta)

   TOS is an :term:`iterator`.  Call its :meth:`~iterator.__next__` method.  If
   this yields a new value, push it on the stack (leaving the iterator below
   it).  If the iterator indicates it is exhausted, TOS is popped, and the byte
   code counter is incremented by *delta*.


.. opcode:: LOAD_GLOBAL (namei)

   Loads the global named ``co_names[namei]`` onto the stack.


.. opcode:: LOAD_FAST (var_num)

   Pushes a reference to the local ``co_varnames[var_num]`` onto the stack.


.. opcode:: STORE_FAST (var_num)

   Stores TOS into the local ``co_varnames[var_num]``.


.. opcode:: DELETE_FAST (var_num)

   Deletes local ``co_varnames[var_num]``.


.. opcode:: MAKE_CELL (i)

   Creates a new cell in slot ``i``.  If that slot is empty then
   that value is stored into the new cell.

   .. versionadded:: 3.11


.. opcode:: LOAD_CLOSURE (i)

   Pushes a reference to the cell contained in slot ``i`` of the "fast locals"
   storage.  The name of the variable is ``co_fastlocalnames[i]``.

   Note that ``LOAD_CLOSURE`` is effectively an alias for ``LOAD_FAST``.
   It exists to keep bytecode a little more readable.

   .. versionchanged:: 3.11
      ``i`` is no longer offset by the length of ``co_varnames``.


.. opcode:: LOAD_DEREF (i)

   Loads the cell contained in slot ``i`` of the "fast locals" storage.
   Pushes a reference to the object the cell contains on the stack.

   .. versionchanged:: 3.11
      ``i`` is no longer offset by the length of ``co_varnames``.


.. opcode:: LOAD_CLASSDEREF (i)

   Much like :opcode:`LOAD_DEREF` but first checks the locals dictionary before
   consulting the cell.  This is used for loading free variables in class
   bodies.

   .. versionadded:: 3.4

   .. versionchanged:: 3.11
      ``i`` is no longer offset by the length of ``co_varnames``.


.. opcode:: STORE_DEREF (i)

   Stores TOS into the cell contained in slot ``i`` of the "fast locals"
   storage.

   .. versionchanged:: 3.11
      ``i`` is no longer offset by the length of ``co_varnames``.


.. opcode:: DELETE_DEREF (i)

   Empties the cell contained in slot ``i`` of the "fast locals" storage.
   Used by the :keyword:`del` statement.

   .. versionadded:: 3.2

   .. versionchanged:: 3.11
      ``i`` is no longer offset by the length of ``co_varnames``.


.. opcode:: RAISE_VARARGS (argc)

   Raises an exception using one of the 3 forms of the ``raise`` statement,
   depending on the value of *argc*:

   * 0: ``raise`` (re-raise previous exception)
   * 1: ``raise TOS`` (raise exception instance or type at ``TOS``)
   * 2: ``raise TOS1 from TOS`` (raise exception instance or type at ``TOS1``
     with ``__cause__`` set to ``TOS``)


.. opcode:: CALL_FUNCTION (argc)

   Calls a callable object with positional arguments.
   *argc* indicates the number of positional arguments.
   The top of the stack contains positional arguments, with the right-most
   argument on top.  Below the arguments is a callable object to call.
   ``CALL_FUNCTION`` pops all arguments and the callable object off the stack,
   calls the callable object with those arguments, and pushes the return value
   returned by the callable object.

   .. versionchanged:: 3.6
      This opcode is used only for calls with positional arguments.


.. opcode:: CALL_FUNCTION_KW (argc)

   Calls a callable object with positional (if any) and keyword arguments.
   *argc* indicates the total number of positional and keyword arguments.
   The top element on the stack contains a tuple with the names of the
   keyword arguments, which must be strings.
   Below that are the values for the keyword arguments,
   in the order corresponding to the tuple.
   Below that are positional arguments, with the right-most parameter on
   top.  Below the arguments is a callable object to call.
   ``CALL_FUNCTION_KW`` pops all arguments and the callable object off the stack,
   calls the callable object with those arguments, and pushes the return value
   returned by the callable object.

   .. versionchanged:: 3.6
      Keyword arguments are packed in a tuple instead of a dictionary,
      *argc* indicates the total number of arguments.


.. opcode:: CALL_FUNCTION_EX (flags)

   Calls a callable object with variable set of positional and keyword
   arguments.  If the lowest bit of *flags* is set, the top of the stack
   contains a mapping object containing additional keyword arguments.
   Before the callable is called, the mapping object and iterable object
   are each "unpacked" and their contents passed in as keyword and
   positional arguments respectively.
   ``CALL_FUNCTION_EX`` pops all arguments and the callable object off the stack,
   calls the callable object with those arguments, and pushes the return value
   returned by the callable object.

   .. versionadded:: 3.6


.. opcode:: LOAD_METHOD (namei)

   Loads a method named ``co_names[namei]`` from the TOS object. TOS is popped.
   This bytecode distinguishes two cases: if TOS has a method with the correct
   name, the bytecode pushes the unbound method and TOS. TOS will be used as
   the first argument (``self``) by :opcode:`CALL_METHOD` when calling the
   unbound method. Otherwise, ``NULL`` and the object return by the attribute
   lookup are pushed.

   .. versionadded:: 3.7


.. opcode:: CALL_METHOD (argc)

   Calls a method.  *argc* is the number of positional arguments.
   Keyword arguments are not supported.  This opcode is designed to be used
   with :opcode:`LOAD_METHOD`.  Positional arguments are on top of the stack.
   Below them, the two items described in :opcode:`LOAD_METHOD` are on the
   stack (either ``self`` and an unbound method object or ``NULL`` and an
   arbitrary callable). All of them are popped and the return value is pushed.

   .. versionadded:: 3.7


.. opcode:: CALL_METHOD_KW (argc)

   Calls a method in a similar fashion as :opcode:`CALL_METHOD`, but also supports keyword arguments.
   *argc* is the number of positional and keyword arguments.
   This opcode is designed to be used with :opcode:`LOAD_METHOD`.  TOS is a
   tuple of keyword argument names.  Argument values are below that.
   Below them, the two items described in :opcode:`LOAD_METHOD` are on the
   stack (either ``self`` and an unbound method object or ``NULL`` and an
   arbitrary callable).  All of them are popped from the stack and the return value is pushed.

   .. versionadded:: 3.11

.. opcode:: MAKE_FUNCTION (flags)

   Pushes a new function object on the stack.  From bottom to top, the consumed
   stack must consist of values if the argument carries a specified flag value

   * ``0x01`` a tuple of default values for positional-only and
     positional-or-keyword parameters in positional order
   * ``0x02`` a dictionary of keyword-only parameters' default values
   * ``0x04`` a tuple of strings containing parameters' annotations
   * ``0x08`` a tuple containing cells for free variables, making a closure
   * the code associated with the function (at TOS1)
   * the :term:`qualified name` of the function (at TOS)

   .. versionchanged:: 3.10
      Flag value ``0x04`` is a tuple of strings instead of dictionary

.. opcode:: BUILD_SLICE (argc)

   .. index:: builtin: slice

   Pushes a slice object on the stack.  *argc* must be 2 or 3.  If it is 2,
   ``slice(TOS1, TOS)`` is pushed; if it is 3, ``slice(TOS2, TOS1, TOS)`` is
   pushed. See the :func:`slice` built-in function for more information.


.. opcode:: EXTENDED_ARG (ext)

   Prefixes any opcode which has an argument too big to fit into the default one
   byte. *ext* holds an additional byte which act as higher bits in the argument.
   For each opcode, at most three prefixal ``EXTENDED_ARG`` are allowed, forming
   an argument from two-byte to four-byte.


.. opcode:: FORMAT_VALUE (flags)

   Used for implementing formatted literal strings (f-strings).  Pops
   an optional *fmt_spec* from the stack, then a required *value*.
   *flags* is interpreted as follows:

   * ``(flags & 0x03) == 0x00``: *value* is formatted as-is.
   * ``(flags & 0x03) == 0x01``: call :func:`str` on *value* before
     formatting it.
   * ``(flags & 0x03) == 0x02``: call :func:`repr` on *value* before
     formatting it.
   * ``(flags & 0x03) == 0x03``: call :func:`ascii` on *value* before
     formatting it.
   * ``(flags & 0x04) == 0x04``: pop *fmt_spec* from the stack and use
     it, else use an empty *fmt_spec*.

   Formatting is performed using :c:func:`PyObject_Format`.  The
   result is pushed on the stack.

   .. versionadded:: 3.6


.. opcode:: MATCH_CLASS (count)

   TOS is a tuple of keyword attribute names, TOS1 is the class being matched
   against, and TOS2 is the match subject.  *count* is the number of positional
   sub-patterns.

   Pop TOS.  If TOS2 is an instance of TOS1 and has the positional and keyword
   attributes required by *count* and TOS, set TOS to ``True`` and TOS1 to a
   tuple of extracted attributes.  Otherwise, set TOS to ``False``.

   .. versionadded:: 3.10

.. opcode:: GEN_START (kind)

    Pops TOS. If TOS was not ``None``, raises an exception. The ``kind``
    operand corresponds to the type of generator or coroutine and determines
    the error message. The legal kinds are 0 for generator, 1 for coroutine,
    and 2 for async generator.

   .. versionadded:: 3.10


.. opcode:: ROT_N (count)

   Lift the top *count* stack items one position up, and move TOS down to
   position *count*.

   .. versionadded:: 3.10


.. opcode:: HAVE_ARGUMENT

   This is not really an opcode.  It identifies the dividing line between
   opcodes which don't use their argument and those that do
   (``< HAVE_ARGUMENT`` and ``>= HAVE_ARGUMENT``, respectively).

   .. versionchanged:: 3.6
      Now every instruction has an argument, but opcodes ``< HAVE_ARGUMENT``
      ignore it. Before, only opcodes ``>= HAVE_ARGUMENT`` had an argument.


.. _opcode_collections:

Opcode collections
------------------

These collections are provided for automatic introspection of bytecode
instructions:

.. data:: opname

   Sequence of operation names, indexable using the bytecode.


.. data:: opmap

   Dictionary mapping operation names to bytecodes.


.. data:: cmp_op

   Sequence of all compare operation names.


.. data:: hasconst

   Sequence of bytecodes that access a constant.


.. data:: hasfree

   Sequence of bytecodes that access a free variable (note that 'free' in this
   context refers to names in the current scope that are referenced by inner
   scopes or names in outer scopes that are referenced from this scope.  It does
   *not* include references to global or builtin scopes).


.. data:: hasname

   Sequence of bytecodes that access an attribute by name.


.. data:: hasjrel

   Sequence of bytecodes that have a relative jump target.


.. data:: hasjabs

   Sequence of bytecodes that have an absolute jump target.


.. data:: haslocal

   Sequence of bytecodes that access a local variable.


.. data:: hascompare

   Sequence of bytecodes of Boolean operations.
