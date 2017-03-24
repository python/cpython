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


   Analyse the bytecode corresponding to a function, generator, method, string
   of source code, or a code object (as returned by :func:`compile`).

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
   for the supplied function, generator, method, source code string or code object.

   Note that the exact contents of code info strings are highly implementation
   dependent and they may change arbitrarily across Python VMs or Python
   releases.

   .. versionadded:: 3.2


.. function:: show_code(x, *, file=None)

   Print detailed code object information for the supplied function, method,
   source code string or code object to *file* (or ``sys.stdout`` if *file*
   is not specified).

   This is a convenient shorthand for ``print(code_info(x), file=file)``,
   intended for interactive exploration at the interpreter prompt.

   .. versionadded:: 3.2

   .. versionchanged:: 3.4
      Added *file* parameter.


.. function:: dis(x=None, *, file=None)

   Disassemble the *x* object.  *x* can denote either a module, a class, a
   method, a function, a generator, a code object, a string of source code or
   a byte sequence of raw bytecode.  For a module, it disassembles all functions.
   For a class, it disassembles all methods (including class and static methods).
   For a code object or sequence of raw bytecode, it prints one line per bytecode
   instruction.  Strings are first compiled to code objects with the :func:`compile`
   built-in function before being disassembled.  If no object is provided, this
   function disassembles the last traceback.

   The disassembly is written as text to the supplied *file* argument if
   provided and to ``sys.stdout`` otherwise.

   .. versionchanged:: 3.4
      Added *file* parameter.


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

   Detect all offsets in the code object *code* which are jump targets, and
   return a list of these offsets.


.. function:: stack_effect(opcode, [oparg])

   Compute the stack effect of *opcode* with argument *oparg*.

   .. versionadded:: 3.4

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


.. opcode:: DUP_TOP

   Duplicates the reference on top of the stack.


.. opcode:: DUP_TOP_TWO

   Duplicates the two references on top of the stack, leaving them in the
   same order.


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


.. opcode:: GET_AITER

   Implements ``TOS = get_awaitable(TOS.__aiter__())``.  See ``GET_AWAITABLE``
   for details about ``get_awaitable``


.. opcode:: GET_ANEXT

   Implements ``PUSH(get_awaitable(TOS.__anext__()))``.  See ``GET_AWAITABLE``
   for details about ``get_awaitable``


.. opcode:: BEFORE_ASYNC_WITH

   Resolves ``__aenter__`` and ``__aexit__`` from the object on top of the
   stack.  Pushes ``__aexit__`` and result of ``__aenter__()`` to the stack.


.. opcode:: SETUP_ASYNC_WITH

   Creates a new frame object.



**Miscellaneous opcodes**

.. opcode:: PRINT_EXPR

   Implements the expression statement for the interactive mode.  TOS is removed
   from the stack and printed.  In non-interactive mode, an expression statement
   is terminated with :opcode:`POP_TOP`.


.. opcode:: BREAK_LOOP

   Terminates a loop due to a :keyword:`break` statement.


.. opcode:: CONTINUE_LOOP (target)

   Continues a loop due to a :keyword:`continue` statement.  *target* is the
   address to jump to (which should be a :opcode:`FOR_ITER` instruction).


.. opcode:: SET_ADD (i)

   Calls ``set.add(TOS1[-i], TOS)``.  Used to implement set comprehensions.


.. opcode:: LIST_APPEND (i)

   Calls ``list.append(TOS[-i], TOS)``.  Used to implement list comprehensions.


.. opcode:: MAP_ADD (i)

   Calls ``dict.setitem(TOS1[-i], TOS, TOS1)``.  Used to implement dict
   comprehensions.

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


.. opcode:: POP_BLOCK

   Removes one block from the block stack.  Per frame, there is a stack of
   blocks, denoting nested loops, try statements, and such.


.. opcode:: POP_EXCEPT

   Removes one block from the block stack. The popped block must be an exception
   handler block, as implicitly created when entering an except handler.  In
   addition to popping extraneous values from the frame stack, the last three
   popped values are used to restore the exception state.


.. opcode:: END_FINALLY

   Terminates a :keyword:`finally` clause.  The interpreter recalls whether the
   exception has to be re-raised, or whether the function returns, and continues
   with the outer-next block.


.. opcode:: LOAD_BUILD_CLASS

   Pushes :func:`builtins.__build_class__` onto the stack.  It is later called
   by :opcode:`CALL_FUNCTION` to construct a class.


.. opcode:: SETUP_WITH (delta)

   This opcode performs several operations before a with block starts.  First,
   it loads :meth:`~object.__exit__` from the context manager and pushes it onto
   the stack for later use by :opcode:`WITH_CLEANUP`.  Then,
   :meth:`~object.__enter__` is called, and a finally block pointing to *delta*
   is pushed.  Finally, the result of calling the enter method is pushed onto
   the stack.  The next opcode will either ignore it (:opcode:`POP_TOP`), or
   store it in (a) variable(s) (:opcode:`STORE_FAST`, :opcode:`STORE_NAME`, or
   :opcode:`UNPACK_SEQUENCE`).


.. opcode:: WITH_CLEANUP_START

   Cleans up the stack when a :keyword:`with` statement block exits.  TOS is the
   context manager's :meth:`__exit__` bound method. Below TOS are 1--3 values
   indicating how/why the finally clause was entered:

   * SECOND = ``None``
   * (SECOND, THIRD) = (``WHY_{RETURN,CONTINUE}``), retval
   * SECOND = ``WHY_*``; no retval below it
   * (SECOND, THIRD, FOURTH) = exc_info()

   In the last case, ``TOS(SECOND, THIRD, FOURTH)`` is called, otherwise
   ``TOS(None, None, None)``.  Pushes SECOND and result of the call
   to the stack.


.. opcode:: WITH_CLEANUP_FINISH

   Pops exception type and result of 'exit' function call from the stack.

   If the stack represents an exception, *and* the function call returns a
   'true' value, this information is "zapped" and replaced with a single
   ``WHY_SILENCED`` to prevent :opcode:`END_FINALLY` from re-raising the
   exception.  (But non-local gotos will still be resumed.)

   .. XXX explain the WHY stuff!


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

   The version of :opcode:`BUILD_MAP` specialized for constant keys.  *count*
   values are consumed from the stack.  The top element on the stack contains
   a tuple of keys.

   .. versionadded:: 3.6


.. opcode:: BUILD_STRING (count)

   Concatenates *count* strings from the stack and pushes the resulting string
   onto the stack.

   .. versionadded:: 3.6


.. opcode:: BUILD_TUPLE_UNPACK (count)

   Pops *count* iterables from the stack, joins them in a single tuple,
   and pushes the result.  Implements iterable unpacking in tuple
   displays ``(*x, *y, *z)``.

   .. versionadded:: 3.5


.. opcode:: BUILD_TUPLE_UNPACK_WITH_CALL (count)

   This is similar to :opcode:`BUILD_TUPLE_UNPACK`,
   but is used for ``f(*x, *y, *z)`` call syntax. The stack item at position
   ``count + 1`` should be the corresponding callable ``f``.

   .. versionadded:: 3.6


.. opcode:: BUILD_LIST_UNPACK (count)

   This is similar to :opcode:`BUILD_TUPLE_UNPACK`, but pushes a list
   instead of tuple.  Implements iterable unpacking in list
   displays ``[*x, *y, *z]``.

   .. versionadded:: 3.5


.. opcode:: BUILD_SET_UNPACK (count)

   This is similar to :opcode:`BUILD_TUPLE_UNPACK`, but pushes a set
   instead of tuple.  Implements iterable unpacking in set
   displays ``{*x, *y, *z}``.

   .. versionadded:: 3.5


.. opcode:: BUILD_MAP_UNPACK (count)

   Pops *count* mappings from the stack, merges them into a single dictionary,
   and pushes the result.  Implements dictionary unpacking in dictionary
   displays ``{**x, **y, **z}``.

   .. versionadded:: 3.5


.. opcode:: BUILD_MAP_UNPACK_WITH_CALL (count)

   This is similar to :opcode:`BUILD_MAP_UNPACK`,
   but is used for ``f(**x, **y, **z)`` call syntax.  The stack item at
   position ``count + 2`` should be the corresponding callable ``f``.

   .. versionadded:: 3.5
   .. versionchanged:: 3.6
      The position of the callable is determined by adding 2 to the opcode
      argument instead of encoding it in the second byte of the argument.


.. opcode:: LOAD_ATTR (namei)

   Replaces TOS with ``getattr(TOS, co_names[namei])``.


.. opcode:: COMPARE_OP (opname)

   Performs a Boolean operation.  The operation name can be found in
   ``cmp_op[opname]``.


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


.. opcode:: POP_JUMP_IF_FALSE (target)

   If TOS is false, sets the bytecode counter to *target*.  TOS is popped.


.. opcode:: JUMP_IF_TRUE_OR_POP (target)

   If TOS is true, sets the bytecode counter to *target* and leaves TOS on the
   stack.  Otherwise (TOS is false), TOS is popped.


.. opcode:: JUMP_IF_FALSE_OR_POP (target)

   If TOS is false, sets the bytecode counter to *target* and leaves TOS on the
   stack.  Otherwise (TOS is true), TOS is popped.


.. opcode:: JUMP_ABSOLUTE (target)

   Set bytecode counter to *target*.


.. opcode:: FOR_ITER (delta)

   TOS is an :term:`iterator`.  Call its :meth:`~iterator.__next__` method.  If
   this yields a new value, push it on the stack (leaving the iterator below
   it).  If the iterator indicates it is exhausted TOS is popped, and the byte
   code counter is incremented by *delta*.


.. opcode:: LOAD_GLOBAL (namei)

   Loads the global named ``co_names[namei]`` onto the stack.


.. opcode:: SETUP_LOOP (delta)

   Pushes a block for a loop onto the block stack.  The block spans from the
   current instruction with a size of *delta* bytes.


.. opcode:: SETUP_EXCEPT (delta)

   Pushes a try block from a try-except clause onto the block stack. *delta*
   points to the first except block.


.. opcode:: SETUP_FINALLY (delta)

   Pushes a try block from a try-except clause onto the block stack. *delta*
   points to the finally block.


.. opcode:: LOAD_FAST (var_num)

   Pushes a reference to the local ``co_varnames[var_num]`` onto the stack.


.. opcode:: STORE_FAST (var_num)

   Stores TOS into the local ``co_varnames[var_num]``.


.. opcode:: DELETE_FAST (var_num)

   Deletes local ``co_varnames[var_num]``.


.. opcode:: STORE_ANNOTATION (namei)

   Stores TOS as ``locals()['__annotations__'][co_names[namei]] = TOS``.

   .. versionadded:: 3.6


.. opcode:: LOAD_CLOSURE (i)

   Pushes a reference to the cell contained in slot *i* of the cell and free
   variable storage.  The name of the variable is ``co_cellvars[i]`` if *i* is
   less than the length of *co_cellvars*.  Otherwise it is ``co_freevars[i -
   len(co_cellvars)]``.


.. opcode:: LOAD_DEREF (i)

   Loads the cell contained in slot *i* of the cell and free variable storage.
   Pushes a reference to the object the cell contains on the stack.


.. opcode:: LOAD_CLASSDEREF (i)

   Much like :opcode:`LOAD_DEREF` but first checks the locals dictionary before
   consulting the cell.  This is used for loading free variables in class
   bodies.


.. opcode:: STORE_DEREF (i)

   Stores TOS into the cell contained in slot *i* of the cell and free variable
   storage.


.. opcode:: DELETE_DEREF (i)

   Empties the cell contained in slot *i* of the cell and free variable storage.
   Used by the :keyword:`del` statement.


.. opcode:: RAISE_VARARGS (argc)

   Raises an exception. *argc* indicates the number of parameters to the raise
   statement, ranging from 0 to 3.  The handler will find the traceback as TOS2,
   the parameter as TOS1, and the exception as TOS.


.. opcode:: CALL_FUNCTION (argc)

   Calls a function.  *argc* indicates the number of positional arguments.
   The positional arguments are on the stack, with the right-most argument
   on top.  Below the arguments, the function object to call is on the stack.
   Pops all function arguments, and the function itself off the stack, and
   pushes the return value.

   .. versionchanged:: 3.6
      This opcode is used only for calls with positional arguments.


.. opcode:: CALL_FUNCTION_KW (argc)

   Calls a function.  *argc* indicates the number of arguments (positional
   and keyword).  The top element on the stack contains a tuple of keyword
   argument names.  Below the tuple, keyword arguments are on the stack, in
   the order corresponding to the tuple.  Below the keyword arguments, the
   positional arguments are on the stack, with the right-most parameter on
   top.  Below the arguments, the function object to call is on the stack.
   Pops all function arguments, and the function itself off the stack, and
   pushes the return value.

   .. versionchanged:: 3.6
      Keyword arguments are packed in a tuple instead of a dictionary,
      *argc* indicates the total number of arguments


.. opcode:: CALL_FUNCTION_EX (flags)

   Calls a function. The lowest bit of *flags* indicates whether the
   var-keyword argument is placed at the top of the stack.  Below the
   var-keyword argument, the var-positional argument is on the stack.
   Below the arguments, the function object to call is placed.
   Pops all function arguments, and the function itself off the stack, and
   pushes the return value. Note that this opcode pops at most three items
   from the stack. Var-positional and var-keyword arguments are packed
   by :opcode:`BUILD_MAP_UNPACK_WITH_CALL` and
   :opcode:`BUILD_MAP_UNPACK_WITH_CALL`.

   .. versionadded:: 3.6


.. opcode:: MAKE_FUNCTION (argc)

   Pushes a new function object on the stack.  From bottom to top, the consumed
   stack must consist of values if the argument carries a specified flag value

   * ``0x01`` a tuple of default argument objects in positional order
   * ``0x02`` a dictionary of keyword-only parameters' default values
   * ``0x04`` an annotation dictionary
   * ``0x08`` a tuple containing cells for free variables, making a closure
   * the code associated with the function (at TOS1)
   * the :term:`qualified name` of the function (at TOS)


.. opcode:: BUILD_SLICE (argc)

   .. index:: builtin: slice

   Pushes a slice object on the stack.  *argc* must be 2 or 3.  If it is 2,
   ``slice(TOS1, TOS)`` is pushed; if it is 3, ``slice(TOS2, TOS1, TOS)`` is
   pushed. See the :func:`slice` built-in function for more information.


.. opcode:: EXTENDED_ARG (ext)

   Prefixes any opcode which has an argument too big to fit into the default two
   bytes.  *ext* holds two additional bytes which, taken together with the
   subsequent opcode's argument, comprise a four-byte argument, *ext* being the
   two most-significant bytes.


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

   Sequence of bytecodes that have a constant parameter.


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
