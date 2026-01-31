:mod:`!dis` --- Disassembler for Python bytecode
================================================

.. module:: dis
   :synopsis: Disassembler for Python bytecode.

**Source code:** :source:`Lib/dis.py`

.. testsetup::

   import dis
   def myfunc(alist):
       return len(alist)

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

   .. versionchanged:: 3.10
      The argument of jump, exception handling and loop instructions is now
      the instruction offset rather than the byte offset.

   .. versionchanged:: 3.11
      Some instructions are accompanied by one or more inline cache entries,
      which take the form of :opcode:`CACHE` instructions. These instructions
      are hidden by default, but can be shown by passing ``show_caches=True`` to
      any :mod:`dis` utility. Furthermore, the interpreter now adapts the
      bytecode to specialize it for different runtime conditions. The
      adaptive bytecode can be shown by passing ``adaptive=True``.

   .. versionchanged:: 3.12
      The argument of a jump is the offset of the target instruction relative
      to the instruction that appears immediately after the jump instruction's
      :opcode:`CACHE` entries.

      As a consequence, the presence of the :opcode:`CACHE` instructions is
      transparent for forward jumps but needs to be taken into account when
      reasoning about backward jumps.

   .. versionchanged:: 3.13
      The output shows logical labels rather than instruction offsets
      for jump targets and exception handlers. The ``-O`` command line
      option and the ``show_offsets`` argument were added.

   .. versionchanged:: 3.14
      The :option:`-P <dis --show-positions>` command-line option
      and the ``show_positions`` argument were added.

      The :option:`-S <dis --specialized>` command-line option is added.

Example: Given the function :func:`!myfunc`::

   def myfunc(alist):
       return len(alist)

the following command can be used to display the disassembly of
:func:`!myfunc`:

.. doctest::

   >>> dis.dis(myfunc)
     2           RESUME                   0
   <BLANKLINE>
     3           LOAD_GLOBAL              1 (len + NULL)
                 LOAD_FAST_BORROW         0 (alist)
                 CALL                     1
                 RETURN_VALUE

(The "2" is a line number).

.. _dis-cli:

Command-line interface
----------------------

The :mod:`dis` module can be invoked as a script from the command line:

.. code-block:: sh

   python -m dis [-h] [-C] [-O] [-P] [-S] [infile]

The following options are accepted:

.. program:: dis

.. option:: -h, --help

   Display usage and exit.

.. option:: -C, --show-caches

   Show inline caches.

   .. versionadded:: 3.13

.. option:: -O, --show-offsets

   Show offsets of instructions.

   .. versionadded:: 3.13

.. option:: -P, --show-positions

   Show positions of instructions in the source code.

   .. versionadded:: 3.14

.. option:: -S, --specialized

   Show specialized bytecode.

   .. versionadded:: 3.14

If :file:`infile` is specified, its disassembled code will be written to stdout.
Otherwise, disassembly is performed on compiled source code received from stdin.

Bytecode analysis
-----------------

.. versionadded:: 3.4

The bytecode analysis API allows pieces of Python code to be wrapped in a
:class:`Bytecode` object that provides easy access to details of the compiled
code.

.. class:: Bytecode(x, *, first_line=None, current_offset=None,\
                    show_caches=False, adaptive=False, show_offsets=False,\
                    show_positions=False)

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

   If *show_caches* is ``True``, :meth:`.dis` will display inline cache
   entries used by the interpreter to specialize the bytecode.

   If *adaptive* is ``True``, :meth:`.dis` will display specialized bytecode
   that may be different from the original bytecode.

   If *show_offsets* is ``True``, :meth:`.dis` will include instruction
   offsets in the output.

   If *show_positions* is ``True``, :meth:`.dis` will include instruction
   source code positions in the output.

   .. classmethod:: from_traceback(tb, *, show_caches=False)

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

   .. versionchanged:: 3.11
      Added the *show_caches* and *adaptive* parameters.

   .. versionchanged:: 3.13
      Added the *show_offsets* parameter

   .. versionchanged:: 3.14
      Added the *show_positions* parameter.

Example:

.. doctest::

    >>> bytecode = dis.Bytecode(myfunc)
    >>> for instr in bytecode:
    ...     print(instr.opname)
    ...
    RESUME
    LOAD_GLOBAL
    LOAD_FAST_BORROW
    CALL
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


.. function:: dis(x=None, *, file=None, depth=None, show_caches=False,\
                  adaptive=False, show_offsets=False, show_positions=False)

   Disassemble the *x* object.  *x* can denote either a module, a class, a
   method, a function, a generator, an asynchronous generator, a coroutine,
   a code object, a string of source code or a byte sequence of raw bytecode.
   For a module, it disassembles all functions. For a class, it disassembles
   all methods (including class and static methods). For a code object or
   sequence of raw bytecode, it prints one line per bytecode instruction.
   It also recursively disassembles nested code objects. These can include
   generator expressions, nested functions, the bodies of nested classes,
   and the code objects used for :ref:`annotation scopes <annotation-scopes>`.
   Strings are first compiled to code objects with the :func:`compile`
   built-in function before being disassembled.  If no object is provided, this
   function disassembles the last traceback.

   The disassembly is written as text to the supplied *file* argument if
   provided and to ``sys.stdout`` otherwise.

   The maximal depth of recursion is limited by *depth* unless it is ``None``.
   ``depth=0`` means no recursion.

   If *show_caches* is ``True``, this function will display inline cache
   entries used by the interpreter to specialize the bytecode.

   If *adaptive* is ``True``, this function will display specialized bytecode
   that may be different from the original bytecode.

   .. versionchanged:: 3.4
      Added *file* parameter.

   .. versionchanged:: 3.7
      Implemented recursive disassembling and added *depth* parameter.

   .. versionchanged:: 3.7
      This can now handle coroutine and asynchronous generator objects.

   .. versionchanged:: 3.11
      Added the *show_caches* and *adaptive* parameters.

   .. versionchanged:: 3.13
      Added the *show_offsets* parameter.

   .. versionchanged:: 3.14
      Added the *show_positions* parameter.

.. function:: distb(tb=None, *, file=None, show_caches=False, adaptive=False,\
                    show_offset=False, show_positions=False)

   Disassemble the top-of-stack function of a traceback, using the last
   traceback if none was passed.  The instruction causing the exception is
   indicated.

   The disassembly is written as text to the supplied *file* argument if
   provided and to ``sys.stdout`` otherwise.

   .. versionchanged:: 3.4
      Added *file* parameter.

   .. versionchanged:: 3.11
      Added the *show_caches* and *adaptive* parameters.

   .. versionchanged:: 3.13
      Added the *show_offsets* parameter.

   .. versionchanged:: 3.14
      Added the *show_positions* parameter.

.. function:: disassemble(code, lasti=-1, *, file=None, show_caches=False,\
                          adaptive=False, show_offsets=False, show_positions=False)
              disco(code, lasti=-1, *, file=None, show_caches=False, adaptive=False,\
                    show_offsets=False, show_positions=False)

   Disassemble a code object, indicating the last instruction if *lasti* was
   provided.  The output is divided in the following columns:

   #. the source code location of the instruction. Complete location information
      is shown if *show_positions* is true. Otherwise (the default) only the
      line number is displayed.
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

   .. versionchanged:: 3.11
      Added the *show_caches* and *adaptive* parameters.

   .. versionchanged:: 3.13
      Added the *show_offsets* parameter.

   .. versionchanged:: 3.14
      Added the *show_positions* parameter.

.. function:: get_instructions(x, *, first_line=None, show_caches=False, adaptive=False)

   Return an iterator over the instructions in the supplied function, method,
   source code string or code object.

   The iterator generates a series of :class:`Instruction` named tuples giving
   the details of each operation in the supplied code.

   If *first_line* is not ``None``, it indicates the line number that should be
   reported for the first source line in the disassembled code.  Otherwise, the
   source line information (if any) is taken directly from the disassembled code
   object.

   The *adaptive* parameter works as it does in :func:`dis`.

   .. versionadded:: 3.4

   .. versionchanged:: 3.11
      Added the *show_caches* and *adaptive* parameters.

   .. versionchanged:: 3.13
      The *show_caches* parameter is deprecated and has no effect. The iterator
      generates the :class:`Instruction` instances with the *cache_info*
      field populated (regardless of the value of *show_caches*) and it no longer
      generates separate items for the cache entries.

.. function:: findlinestarts(code)

   This generator function uses the :meth:`~codeobject.co_lines` method
   of the :ref:`code object <code-objects>` *code* to find the offsets which
   are starts of
   lines in the source code.  They are generated as ``(offset, lineno)`` pairs.

   .. versionchanged:: 3.6
      Line numbers can be decreasing. Before, they were always increasing.

   .. versionchanged:: 3.10
      The :pep:`626` :meth:`~codeobject.co_lines` method is used instead of the
      :attr:`~codeobject.co_firstlineno` and :attr:`~codeobject.co_lnotab`
      attributes of the :ref:`code object <code-objects>`.

   .. versionchanged:: 3.13
      Line numbers can be ``None`` for bytecode that does not map to source lines.


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

   .. versionchanged:: 3.13
      If ``oparg`` is omitted (or ``None``), the stack effect is now returned
      for ``oparg=0``. Previously this was an error for opcodes that use their
      arg. It is also no longer an error to pass an integer ``oparg`` when
      the ``opcode`` does not use it; the ``oparg`` in this case is ignored.


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


   .. data:: baseopcode

      numeric code for the base operation if operation is specialized;
      otherwise equal to :data:`opcode`


   .. data:: baseopname

      human readable name for the base operation if operation is specialized;
      otherwise equal to :data:`opname`


   .. data:: arg

      numeric argument to operation (if any), otherwise ``None``

   .. data:: oparg

      alias for :data:`arg`

   .. data:: argval

      resolved arg value (if any), otherwise ``None``


   .. data:: argrepr

      human readable description of operation argument (if any),
      otherwise an empty string.


   .. data:: offset

      start index of operation within bytecode sequence


   .. data:: start_offset

      start index of operation within bytecode sequence, including prefixed
      ``EXTENDED_ARG`` operations if present; otherwise equal to :data:`offset`


   .. data:: cache_offset

      start index of the cache entries following the operation


   .. data:: end_offset

      end index of the cache entries following the operation


   .. data:: starts_line

      ``True`` if this opcode starts a source line, otherwise ``False``


   .. data:: line_number

      source line number associated with this opcode (if any), otherwise ``None``


   .. data:: is_jump_target

      ``True`` if other code jumps to here, otherwise ``False``


   .. data:: jump_target

      bytecode index of the jump target if this is a jump operation,
      otherwise ``None``


   .. data:: positions

      :class:`dis.Positions` object holding the
      start and end locations that are covered by this instruction.

   .. data:: cache_info

      Information about the cache entries of this instruction, as
      triplets of the form ``(name, size, data)``, where the ``name``
      and ``size`` describe the cache format and data is the contents
      of the cache. ``cache_info`` is ``None`` if the instruction does not have
      caches.

   .. versionadded:: 3.4

   .. versionchanged:: 3.11

      Field ``positions`` is added.

   .. versionchanged:: 3.13

      Changed field ``starts_line``.

      Added fields ``start_offset``, ``cache_offset``, ``end_offset``,
      ``baseopname``, ``baseopcode``, ``jump_target``, ``oparg``,
      ``line_number`` and ``cache_info``.


.. class:: Positions

   In case the information is not available, some fields might be ``None``.

   .. data:: lineno
   .. data:: end_lineno
   .. data:: col_offset
   .. data:: end_col_offset

   .. versionadded:: 3.11


The Python compiler currently generates the following bytecode instructions.


**General instructions**

In the following, We will refer to the interpreter stack as ``STACK`` and describe
operations on it as if it was a Python list. The top of the stack corresponds to
``STACK[-1]`` in this language.

.. opcode:: NOP

   Do nothing code.  Used as a placeholder by the bytecode optimizer, and to
   generate line tracing events.


.. opcode:: NOT_TAKEN

   Do nothing code.
   Used by the interpreter to record :monitoring-event:`BRANCH_LEFT`
   and :monitoring-event:`BRANCH_RIGHT` events for :mod:`sys.monitoring`.

   .. versionadded:: 3.14


.. opcode:: POP_ITER

   Removes the iterator from the top of the stack.

   .. versionadded:: 3.14


.. opcode:: POP_TOP

   Removes the top-of-stack item::

      STACK.pop()


.. opcode:: END_FOR

   Removes the top-of-stack item.
   Equivalent to ``POP_TOP``.
   Used to clean up at the end of loops, hence the name.

   .. versionadded:: 3.12


.. opcode:: END_SEND

   Implements ``del STACK[-2]``.
   Used to clean up when a generator exits.

   .. versionadded:: 3.12


.. opcode:: COPY (i)

   Push the i-th item to the top of the stack without removing it from its original
   location::

      assert i > 0
      STACK.append(STACK[-i])

   .. versionadded:: 3.11


.. opcode:: SWAP (i)

   Swap the top of the stack with the i-th element::

      STACK[-i], STACK[-1] = STACK[-1], STACK[-i]

   .. versionadded:: 3.11


.. opcode:: CACHE

   Rather than being an actual instruction, this opcode is used to mark extra
   space for the interpreter to cache useful data directly in the bytecode
   itself. It is automatically hidden by all ``dis`` utilities, but can be
   viewed with ``show_caches=True``.

   Logically, this space is part of the preceding instruction. Many opcodes
   expect to be followed by an exact number of caches, and will instruct the
   interpreter to skip over them at runtime.

   Populated caches can look like arbitrary instructions, so great care should
   be taken when reading or modifying raw, adaptive bytecode containing
   quickened data.

   .. versionadded:: 3.11


**Unary operations**

Unary operations take the top of the stack, apply the operation, and push the
result back on the stack.


.. opcode:: UNARY_NEGATIVE

   Implements ``STACK[-1] = -STACK[-1]``.


.. opcode:: UNARY_NOT

   Implements ``STACK[-1] = not STACK[-1]``.

   .. versionchanged:: 3.13
      This instruction now requires an exact :class:`bool` operand.


.. opcode:: UNARY_INVERT

   Implements ``STACK[-1] = ~STACK[-1]``.


.. opcode:: GET_ITER

   Implements ``STACK[-1] = iter(STACK[-1])``.


.. opcode:: GET_YIELD_FROM_ITER

   If ``STACK[-1]`` is a :term:`generator iterator` or :term:`coroutine` object
   it is left as is.  Otherwise, implements ``STACK[-1] = iter(STACK[-1])``.

   .. versionadded:: 3.5


.. opcode:: TO_BOOL

   Implements ``STACK[-1] = bool(STACK[-1])``.

   .. versionadded:: 3.13


**Binary and in-place operations**

Binary operations remove the top two items from the stack (``STACK[-1]`` and
``STACK[-2]``). They perform the operation, then put the result back on the stack.

In-place operations are like binary operations, but the operation is done in-place
when ``STACK[-2]`` supports it, and the resulting ``STACK[-1]`` may be (but does
not have to be) the original ``STACK[-2]``.


.. opcode:: BINARY_OP (op)

   Implements the binary and in-place operators (depending on the value of
   *op*)::

      rhs = STACK.pop()
      lhs = STACK.pop()
      STACK.append(lhs op rhs)

   .. versionadded:: 3.11
   .. versionchanged:: 3.14
      With oparg :``NB_SUBSCR``, implements binary subscript (replaces opcode ``BINARY_SUBSCR``)


.. opcode:: STORE_SUBSCR

   Implements::

      key = STACK.pop()
      container = STACK.pop()
      value = STACK.pop()
      container[key] = value


.. opcode:: DELETE_SUBSCR

   Implements::

      key = STACK.pop()
      container = STACK.pop()
      del container[key]

.. opcode:: BINARY_SLICE

   Implements::

      end = STACK.pop()
      start = STACK.pop()
      container = STACK.pop()
      STACK.append(container[start:end])

   .. versionadded:: 3.12


.. opcode:: STORE_SLICE

   Implements::

      end = STACK.pop()
      start = STACK.pop()
      container = STACK.pop()
      value = STACK.pop()
      container[start:end] = value

   .. versionadded:: 3.12


**Coroutine opcodes**

.. opcode:: GET_AWAITABLE (where)

   Implements ``STACK[-1] = get_awaitable(STACK[-1])``, where ``get_awaitable(o)``
   returns ``o`` if ``o`` is a coroutine object or a generator object with
   the :data:`~inspect.CO_ITERABLE_COROUTINE` flag, or resolves
   ``o.__await__``.

    If the ``where`` operand is nonzero, it indicates where the instruction
    occurs:

    * ``1``: After a call to ``__aenter__``
    * ``2``: After a call to ``__aexit__``

   .. versionadded:: 3.5

   .. versionchanged:: 3.11
      Previously, this instruction did not have an oparg.


.. opcode:: GET_AITER

   Implements ``STACK[-1] = STACK[-1].__aiter__()``.

   .. versionadded:: 3.5
   .. versionchanged:: 3.7
      Returning awaitable objects from ``__aiter__`` is no longer
      supported.


.. opcode:: GET_ANEXT

   Implement ``STACK.append(get_awaitable(STACK[-1].__anext__()))`` to the stack.
   See ``GET_AWAITABLE`` for details about ``get_awaitable``.

   .. versionadded:: 3.5


.. opcode:: END_ASYNC_FOR

   Terminates an :keyword:`async for` loop.  Handles an exception raised
   when awaiting a next item. The stack contains the async iterable in
   ``STACK[-2]`` and the raised exception in ``STACK[-1]``. Both are popped.
   If the exception is not :exc:`StopAsyncIteration`, it is re-raised.

   .. versionadded:: 3.8

   .. versionchanged:: 3.11
      Exception representation on the stack now consist of one, not three, items.


.. opcode:: CLEANUP_THROW

   Handles an exception raised during a :meth:`~generator.throw` or
   :meth:`~generator.close` call through the current frame.  If ``STACK[-1]`` is an
   instance of :exc:`StopIteration`, pop three values from the stack and push
   its ``value`` member.  Otherwise, re-raise ``STACK[-1]``.

   .. versionadded:: 3.12



**Miscellaneous opcodes**

.. opcode:: SET_ADD (i)

   Implements::

      item = STACK.pop()
      set.add(STACK[-i], item)

   Used to implement set comprehensions.


.. opcode:: LIST_APPEND (i)

   Implements::

      item = STACK.pop()
      list.append(STACK[-i], item)

   Used to implement list comprehensions.


.. opcode:: MAP_ADD (i)

   Implements::

      value = STACK.pop()
      key = STACK.pop()
      dict.__setitem__(STACK[-i], key, value)

   Used to implement dict comprehensions.

   .. versionadded:: 3.1
   .. versionchanged:: 3.8
      Map value is ``STACK[-1]`` and map key is ``STACK[-2]``. Before, those
      were reversed.

For all of the :opcode:`SET_ADD`, :opcode:`LIST_APPEND` and :opcode:`MAP_ADD`
instructions, while the added value or key/value pair is popped off, the
container object remains on the stack so that it is available for further
iterations of the loop.


.. opcode:: RETURN_VALUE

   Returns with ``STACK[-1]`` to the caller of the function.


.. opcode:: YIELD_VALUE

   Yields ``STACK.pop()`` from a :term:`generator`.

   .. versionchanged:: 3.11
      oparg set to be the stack depth.

   .. versionchanged:: 3.12
      oparg set to be the exception block depth, for efficient closing of generators.

   .. versionchanged:: 3.13
      oparg is ``1`` if this instruction is part of a yield-from or await, and ``0``
      otherwise.

.. opcode:: SETUP_ANNOTATIONS

   Checks whether ``__annotations__`` is defined in ``locals()``, if not it is
   set up to an empty ``dict``. This opcode is only emitted if a class
   or module body contains :term:`variable annotations <variable annotation>`
   statically.

   .. versionadded:: 3.6


.. opcode:: POP_EXCEPT

   Pops a value from the stack, which is used to restore the exception state.

   .. versionchanged:: 3.11
      Exception representation on the stack now consist of one, not three, items.

.. opcode:: RERAISE

   Re-raises the exception currently on top of the stack. If oparg is non-zero,
   pops an additional value from the stack which is used to set
   :attr:`~frame.f_lasti` of the current frame.

   .. versionadded:: 3.9

   .. versionchanged:: 3.11
      Exception representation on the stack now consist of one, not three, items.

.. opcode:: PUSH_EXC_INFO

   Pops a value from the stack. Pushes the current exception to the top of the stack.
   Pushes the value originally popped back to the stack.
   Used in exception handlers.

   .. versionadded:: 3.11

.. opcode:: CHECK_EXC_MATCH

   Performs exception matching for ``except``. Tests whether the ``STACK[-2]``
   is an exception matching ``STACK[-1]``. Pops ``STACK[-1]`` and pushes the boolean
   result of the test.

   .. versionadded:: 3.11

.. opcode:: CHECK_EG_MATCH

   Performs exception matching for ``except*``. Applies ``split(STACK[-1])`` on
   the exception group representing ``STACK[-2]``.

   In case of a match, pops two items from the stack and pushes the
   non-matching subgroup (``None`` in case of full match) followed by the
   matching subgroup. When there is no match, pops one item (the match
   type) and pushes ``None``.

   .. versionadded:: 3.11

.. opcode:: WITH_EXCEPT_START

   Calls the function in position 4 on the stack with arguments (type, val, tb)
   representing the exception at the top of the stack.
   Used to implement the call ``context_manager.__exit__(*exc_info())`` when an exception
   has occurred in a :keyword:`with` statement.

   .. versionadded:: 3.9

   .. versionchanged:: 3.11
      The ``__exit__`` function is in position 4 of the stack rather than 7.
      Exception representation on the stack now consist of one, not three, items.


.. opcode:: LOAD_COMMON_CONSTANT

   Pushes a common constant onto the stack. The interpreter contains a hardcoded
   list of constants supported by this instruction.  Used by the :keyword:`assert`
   statement to load :exc:`AssertionError`.

   .. versionadded:: 3.14


.. opcode:: LOAD_BUILD_CLASS

   Pushes :func:`!builtins.__build_class__` onto the stack.  It is later called
   to construct a class.

.. opcode:: GET_LEN

   Perform ``STACK.append(len(STACK[-1]))``. Used in :keyword:`match` statements where
   comparison with structure of pattern is needed.

   .. versionadded:: 3.10


.. opcode:: MATCH_MAPPING

   If ``STACK[-1]`` is an instance of :class:`collections.abc.Mapping` (or, more
   technically: if it has the :c:macro:`Py_TPFLAGS_MAPPING` flag set in its
   :c:member:`~PyTypeObject.tp_flags`), push ``True`` onto the stack.  Otherwise,
   push ``False``.

   .. versionadded:: 3.10


.. opcode:: MATCH_SEQUENCE

   If ``STACK[-1]`` is an instance of :class:`collections.abc.Sequence` and is *not* an instance
   of :class:`str`/:class:`bytes`/:class:`bytearray` (or, more technically: if it has
   the :c:macro:`Py_TPFLAGS_SEQUENCE` flag set in its :c:member:`~PyTypeObject.tp_flags`),
   push ``True`` onto the stack.  Otherwise, push ``False``.

   .. versionadded:: 3.10


.. opcode:: MATCH_KEYS

   ``STACK[-1]`` is a tuple of mapping keys, and ``STACK[-2]`` is the match subject.
   If ``STACK[-2]`` contains all of the keys in ``STACK[-1]``, push a :class:`tuple`
   containing the corresponding values. Otherwise, push ``None``.

   .. versionadded:: 3.10

   .. versionchanged:: 3.11
      Previously, this instruction also pushed a boolean value indicating
      success (``True``) or failure (``False``).


.. opcode:: STORE_NAME (namei)

   Implements ``name = STACK.pop()``. *namei* is the index of *name* in the attribute
   :attr:`~codeobject.co_names` of the :ref:`code object <code-objects>`.
   The compiler tries to use :opcode:`STORE_FAST` or :opcode:`STORE_GLOBAL` if possible.


.. opcode:: DELETE_NAME (namei)

   Implements ``del name``, where *namei* is the index into :attr:`~codeobject.co_names`
   attribute of the :ref:`code object <code-objects>`.


.. opcode:: UNPACK_SEQUENCE (count)

   Unpacks ``STACK[-1]`` into *count* individual values, which are put onto the stack
   right-to-left. Require there to be exactly *count* values.::

      assert(len(STACK[-1]) == count)
      STACK.extend(STACK.pop()[:-count-1:-1])


.. opcode:: UNPACK_EX (counts)

   Implements assignment with a starred target: Unpacks an iterable in ``STACK[-1]``
   into individual values, where the total number of values can be smaller than the
   number of items in the iterable: one of the new values will be a list of all
   leftover items.

   The number of values before and after the list value is limited to 255.

   The number of values before the list value is encoded in the argument of the
   opcode. The number of values after the list if any is encoded using an
   ``EXTENDED_ARG``. As a consequence, the argument can be seen as a two bytes values
   where the low byte of *counts* is the number of values before the list value, the
   high byte of *counts* the number of values after it.

   The extracted values are put onto the stack right-to-left, i.e. ``a, *b, c = d``
   will be stored after execution as ``STACK.extend((a, b, c))``.


.. opcode:: STORE_ATTR (namei)

   Implements::

      obj = STACK.pop()
      value = STACK.pop()
      obj.name = value

   where *namei* is the index of name in :attr:`~codeobject.co_names` of the
   :ref:`code object <code-objects>`.

.. opcode:: DELETE_ATTR (namei)

   Implements::

      obj = STACK.pop()
      del obj.name

   where *namei* is the index of name into :attr:`~codeobject.co_names` of the
   :ref:`code object <code-objects>`.


.. opcode:: STORE_GLOBAL (namei)

   Works as :opcode:`STORE_NAME`, but stores the name as a global.


.. opcode:: DELETE_GLOBAL (namei)

   Works as :opcode:`DELETE_NAME`, but deletes a global name.


.. opcode:: LOAD_CONST (consti)

   Pushes ``co_consts[consti]`` onto the stack.


.. opcode:: LOAD_SMALL_INT (i)

   Pushes the integer ``i`` onto the stack.
   ``i`` must be in ``range(256)``

   .. versionadded:: 3.14


.. opcode:: LOAD_NAME (namei)

   Pushes the value associated with ``co_names[namei]`` onto the stack.
   The name is looked up within the locals, then the globals, then the builtins.


.. opcode:: LOAD_LOCALS

   Pushes a reference to the locals dictionary onto the stack.  This is used
   to prepare namespace dictionaries for :opcode:`LOAD_FROM_DICT_OR_DEREF`
   and :opcode:`LOAD_FROM_DICT_OR_GLOBALS`.

   .. versionadded:: 3.12


.. opcode:: LOAD_FROM_DICT_OR_GLOBALS (i)

   Pops a mapping off the stack and looks up the value for ``co_names[namei]``.
   If the name is not found there, looks it up in the globals and then the builtins,
   similar to :opcode:`LOAD_GLOBAL`.
   This is used for loading global variables in
   :ref:`annotation scopes <annotation-scopes>` within class bodies.

   .. versionadded:: 3.12


.. opcode:: BUILD_TEMPLATE

   Constructs a new :class:`~string.templatelib.Template` instance from a tuple
   of strings and a tuple of interpolations and pushes the resulting object
   onto the stack::

      interpolations = STACK.pop()
      strings = STACK.pop()
      STACK.append(_build_template(strings, interpolations))

   .. versionadded:: 3.14


.. opcode:: BUILD_INTERPOLATION (format)

   Constructs a new :class:`~string.templatelib.Interpolation` instance from a
   value and its source expression and pushes the resulting object onto the
   stack.

   If no conversion or format specification is present, ``format`` is set to
   ``2``.

   If the low bit of ``format`` is set, it indicates that the interpolation
   contains a format specification.

   If ``format >> 2`` is non-zero, it indicates that the interpolation
   contains a conversion. The value of ``format >> 2`` is the conversion type
   (``0`` for no conversion, ``1`` for ``!s``, ``2`` for ``!r``, and
   ``3`` for ``!a``)::

      conversion = format >> 2
      if format & 1:
          format_spec = STACK.pop()
      else:
          format_spec = None
      expression = STACK.pop()
      value = STACK.pop()
      STACK.append(_build_interpolation(value, expression, conversion, format_spec))

   .. versionadded:: 3.14


.. opcode:: BUILD_TUPLE (count)

   Creates a tuple consuming *count* items from the stack, and pushes the
   resulting tuple onto the stack::

      if count == 0:
          value = ()
      else:
          value = tuple(STACK[-count:])
          STACK = STACK[:-count]

      STACK.append(value)


.. opcode:: BUILD_LIST (count)

   Works as :opcode:`BUILD_TUPLE`, but creates a list.


.. opcode:: BUILD_SET (count)

   Works as :opcode:`BUILD_TUPLE`, but creates a set.


.. opcode:: BUILD_MAP (count)

   Pushes a new dictionary object onto the stack.  Pops ``2 * count`` items
   so that the dictionary holds *count* entries:
   ``{..., STACK[-4]: STACK[-3], STACK[-2]: STACK[-1]}``.

   .. versionchanged:: 3.5
      The dictionary is created from stack items instead of creating an
      empty dictionary pre-sized to hold *count* items.


.. opcode:: BUILD_STRING (count)

   Concatenates *count* strings from the stack and pushes the resulting string
   onto the stack.

   .. versionadded:: 3.6


.. opcode:: LIST_EXTEND (i)

   Implements::

      seq = STACK.pop()
      list.extend(STACK[-i], seq)

   Used to build lists.

   .. versionadded:: 3.9


.. opcode:: SET_UPDATE (i)

   Implements::

      seq = STACK.pop()
      set.update(STACK[-i], seq)

   Used to build sets.

   .. versionadded:: 3.9


.. opcode:: DICT_UPDATE (i)

   Implements::

      map = STACK.pop()
      dict.update(STACK[-i], map)

   Used to build dicts.

   .. versionadded:: 3.9


.. opcode:: DICT_MERGE (i)

   Like :opcode:`DICT_UPDATE` but raises an exception for duplicate keys.

   .. versionadded:: 3.9


.. opcode:: LOAD_ATTR (namei)

   If the low bit of ``namei`` is not set, this replaces ``STACK[-1]`` with
   ``getattr(STACK[-1], co_names[namei>>1])``.

   If the low bit of ``namei`` is set, this will attempt to load a method named
   ``co_names[namei>>1]`` from the ``STACK[-1]`` object. ``STACK[-1]`` is popped.
   This bytecode distinguishes two cases: if ``STACK[-1]`` has a method with the
   correct name, the bytecode pushes the unbound method and ``STACK[-1]``.
   ``STACK[-1]`` will be used as the first argument (``self``) by :opcode:`CALL`
   or :opcode:`CALL_KW` when calling the unbound method.
   Otherwise, ``NULL`` and the object returned by
   the attribute lookup are pushed.

   .. versionchanged:: 3.12
      If the low bit of ``namei`` is set, then a ``NULL`` or ``self`` is
      pushed to the stack before the attribute or unbound method respectively.


.. opcode:: LOAD_SUPER_ATTR (namei)

   This opcode implements :func:`super`, both in its zero-argument and
   two-argument forms (e.g. ``super().method()``, ``super().attr`` and
   ``super(cls, self).method()``, ``super(cls, self).attr``).

   It pops three values from the stack (from top of stack down):

   * ``self``: the first argument to the current method
   * ``cls``: the class within which the current method was defined
   * the global ``super``

   With respect to its argument, it works similarly to :opcode:`LOAD_ATTR`,
   except that ``namei`` is shifted left by 2 bits instead of 1.

   The low bit of ``namei`` signals to attempt a method load, as with
   :opcode:`LOAD_ATTR`, which results in pushing ``NULL`` and the loaded method.
   When it is unset a single value is pushed to the stack.

   The second-low bit of ``namei``, if set, means that this was a two-argument
   call to :func:`super` (unset means zero-argument).

   .. versionadded:: 3.12


.. opcode:: COMPARE_OP (opname)

   Performs a Boolean operation.  The operation name can be found in
   ``cmp_op[opname >> 5]``. If the fifth-lowest bit of ``opname`` is set
   (``opname & 16``), the result should be coerced to ``bool``.

   .. versionchanged:: 3.13
      The fifth-lowest bit of the oparg now indicates a forced conversion to
      :class:`bool`.


.. opcode:: IS_OP (invert)

   Performs ``is`` comparison, or ``is not`` if ``invert`` is 1.

   .. versionadded:: 3.9


.. opcode:: CONTAINS_OP (invert)

   Performs ``in`` comparison, or ``not in`` if ``invert`` is 1.

   .. versionadded:: 3.9


.. opcode:: IMPORT_NAME (namei)

   Imports the module ``co_names[namei]``.  ``STACK[-1]`` and ``STACK[-2]`` are
   popped and provide the *fromlist* and *level* arguments of :func:`__import__`.
   The module object is pushed onto the stack.  The current namespace is not affected: for a proper import statement, a subsequent :opcode:`STORE_FAST` instruction
   modifies the namespace.


.. opcode:: IMPORT_FROM (namei)

   Loads the attribute ``co_names[namei]`` from the module found in ``STACK[-1]``.
   The resulting object is pushed onto the stack, to be subsequently stored by a
   :opcode:`STORE_FAST` instruction.


.. opcode:: JUMP_FORWARD (delta)

   Increments bytecode counter by *delta*.


.. opcode:: JUMP_BACKWARD (delta)

   Decrements bytecode counter by *delta*. Checks for interrupts.

   .. versionadded:: 3.11


.. opcode:: JUMP_BACKWARD_NO_INTERRUPT (delta)

   Decrements bytecode counter by *delta*. Does not check for interrupts.

   .. versionadded:: 3.11


.. opcode:: POP_JUMP_IF_TRUE (delta)

   If ``STACK[-1]`` is true, increments the bytecode counter by *delta*.
   ``STACK[-1]`` is popped.

   .. versionchanged:: 3.11
      The oparg is now a relative delta rather than an absolute target.
      This opcode is a pseudo-instruction, replaced in final bytecode by
      the directed versions (forward/backward).

   .. versionchanged:: 3.12
      This is no longer a pseudo-instruction.

   .. versionchanged:: 3.13
      This instruction now requires an exact :class:`bool` operand.

.. opcode:: POP_JUMP_IF_FALSE (delta)

   If ``STACK[-1]`` is false, increments the bytecode counter by *delta*.
   ``STACK[-1]`` is popped.

   .. versionchanged:: 3.11
      The oparg is now a relative delta rather than an absolute target.
      This opcode is a pseudo-instruction, replaced in final bytecode by
      the directed versions (forward/backward).

   .. versionchanged:: 3.12
      This is no longer a pseudo-instruction.

   .. versionchanged:: 3.13
      This instruction now requires an exact :class:`bool` operand.

.. opcode:: POP_JUMP_IF_NOT_NONE (delta)

   If ``STACK[-1]`` is not ``None``, increments the bytecode counter by *delta*.
   ``STACK[-1]`` is popped.

   .. versionadded:: 3.11

   .. versionchanged:: 3.12
      This is no longer a pseudo-instruction.


.. opcode:: POP_JUMP_IF_NONE (delta)

   If ``STACK[-1]`` is ``None``, increments the bytecode counter by *delta*.
   ``STACK[-1]`` is popped.

   .. versionadded:: 3.11

   .. versionchanged:: 3.12
      This is no longer a pseudo-instruction.

.. opcode:: FOR_ITER (delta)

   ``STACK[-1]`` is an :term:`iterator`.  Call its :meth:`~iterator.__next__` method.
   If this yields a new value, push it on the stack (leaving the iterator below
   it).  If the iterator indicates it is exhausted then the byte code counter is
   incremented by *delta*.

   .. versionchanged:: 3.12
      Up until 3.11 the iterator was popped when it was exhausted.

.. opcode:: LOAD_GLOBAL (namei)

   Loads the global named ``co_names[namei>>1]`` onto the stack.

   .. versionchanged:: 3.11
      If the low bit of ``namei`` is set, then a ``NULL`` is pushed to the
      stack before the global variable.

.. opcode:: LOAD_FAST (var_num)

   Pushes a reference to the local ``co_varnames[var_num]`` onto the stack.

   .. versionchanged:: 3.12
      This opcode is now only used in situations where the local variable is
      guaranteed to be initialized. It cannot raise :exc:`UnboundLocalError`.

.. opcode:: LOAD_FAST_BORROW (var_num)

   Pushes a borrowed reference to the local ``co_varnames[var_num]`` onto the
   stack.

   .. versionadded:: 3.14

.. opcode:: LOAD_FAST_LOAD_FAST (var_nums)

   Pushes references to ``co_varnames[var_nums >> 4]`` and
   ``co_varnames[var_nums & 15]`` onto the stack.

   .. versionadded:: 3.13


.. opcode:: LOAD_FAST_BORROW_LOAD_FAST_BORROW (var_nums)

   Pushes borrowed references to ``co_varnames[var_nums >> 4]`` and
   ``co_varnames[var_nums & 15]`` onto the stack.

   .. versionadded:: 3.14

.. opcode:: LOAD_FAST_CHECK (var_num)

   Pushes a reference to the local ``co_varnames[var_num]`` onto the stack,
   raising an :exc:`UnboundLocalError` if the local variable has not been
   initialized.

   .. versionadded:: 3.12

.. opcode:: LOAD_FAST_AND_CLEAR (var_num)

   Pushes a reference to the local ``co_varnames[var_num]`` onto the stack (or
   pushes ``NULL`` onto the stack if the local variable has not been
   initialized) and sets ``co_varnames[var_num]`` to ``NULL``.

   .. versionadded:: 3.12

.. opcode:: STORE_FAST (var_num)

   Stores ``STACK.pop()`` into the local ``co_varnames[var_num]``.

.. opcode:: STORE_FAST_STORE_FAST (var_nums)

   Stores ``STACK[-1]`` into ``co_varnames[var_nums >> 4]``
   and ``STACK[-2]`` into ``co_varnames[var_nums & 15]``.

   .. versionadded:: 3.13

.. opcode:: STORE_FAST_LOAD_FAST (var_nums)

   Stores ``STACK.pop()`` into the local ``co_varnames[var_nums >> 4]``
   and pushes a reference to the local ``co_varnames[var_nums & 15]``
   onto the stack.

   .. versionadded:: 3.13

.. opcode:: DELETE_FAST (var_num)

   Deletes local ``co_varnames[var_num]``.


.. opcode:: MAKE_CELL (i)

   Creates a new cell in slot ``i``.  If that slot is nonempty then
   that value is stored into the new cell.

   .. versionadded:: 3.11


.. opcode:: LOAD_DEREF (i)

   Loads the cell contained in slot ``i`` of the "fast locals" storage.
   Pushes a reference to the object the cell contains on the stack.

   .. versionchanged:: 3.11
      ``i`` is no longer offset by the length of :attr:`~codeobject.co_varnames`.


.. opcode:: LOAD_FROM_DICT_OR_DEREF (i)

   Pops a mapping off the stack and looks up the name associated with
   slot ``i`` of the "fast locals" storage in this mapping.
   If the name is not found there, loads it from the cell contained in
   slot ``i``, similar to :opcode:`LOAD_DEREF`. This is used for loading
   :term:`closure variables <closure variable>` in class bodies (which previously used
   :opcode:`!LOAD_CLASSDEREF`) and in
   :ref:`annotation scopes <annotation-scopes>` within class bodies.

   .. versionadded:: 3.12


.. opcode:: STORE_DEREF (i)

   Stores ``STACK.pop()`` into the cell contained in slot ``i`` of the "fast locals"
   storage.

   .. versionchanged:: 3.11
      ``i`` is no longer offset by the length of :attr:`~codeobject.co_varnames`.


.. opcode:: DELETE_DEREF (i)

   Empties the cell contained in slot ``i`` of the "fast locals" storage.
   Used by the :keyword:`del` statement.

   .. versionadded:: 3.2

   .. versionchanged:: 3.11
      ``i`` is no longer offset by the length of :attr:`~codeobject.co_varnames`.


.. opcode:: COPY_FREE_VARS (n)

   Copies the ``n`` :term:`free (closure) variables <closure variable>` from the closure
   into the frame. Removes the need for special code on the caller's side when calling
   closures.

   .. versionadded:: 3.11


.. opcode:: RAISE_VARARGS (argc)

   Raises an exception using one of the 3 forms of the ``raise`` statement,
   depending on the value of *argc*:

   * 0: ``raise`` (re-raise previous exception)
   * 1: ``raise STACK[-1]`` (raise exception instance or type at ``STACK[-1]``)
   * 2: ``raise STACK[-2] from STACK[-1]`` (raise exception instance or type at
     ``STACK[-2]`` with ``__cause__`` set to ``STACK[-1]``)


.. opcode:: CALL (argc)

   Calls a callable object with the number of arguments specified by ``argc``.
   On the stack are (in ascending order):

   * The callable
   * ``self`` or ``NULL``
   * The remaining positional arguments

   ``argc`` is the total of the positional arguments, excluding ``self``.

   ``CALL`` pops all arguments and the callable object off the stack,
   calls the callable object with those arguments, and pushes the return value
   returned by the callable object.

   .. versionadded:: 3.11

   .. versionchanged:: 3.13
      The callable now always appears at the same position on the stack.

   .. versionchanged:: 3.13
      Calls with keyword arguments are now handled by :opcode:`CALL_KW`.


.. opcode:: CALL_KW (argc)

   Calls a callable object with the number of arguments specified by ``argc``,
   including one or more named arguments. On the stack are (in ascending order):

   * The callable
   * ``self`` or ``NULL``
   * The remaining positional arguments
   * The named arguments
   * A :class:`tuple` of keyword argument names

   ``argc`` is the total of the positional and named arguments, excluding ``self``.
   The length of the tuple of keyword argument names is the number of named arguments.

   ``CALL_KW`` pops all arguments, the keyword names, and the callable object
   off the stack, calls the callable object with those arguments, and pushes the
   return value returned by the callable object.

   .. versionadded:: 3.13


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


.. opcode:: PUSH_NULL

   Pushes a ``NULL`` to the stack.
   Used in the call sequence to match the ``NULL`` pushed by
   :opcode:`!LOAD_METHOD` for non-method calls.

   .. versionadded:: 3.11


.. opcode:: MAKE_FUNCTION

   Pushes a new function object on the stack built from the code object at ``STACK[-1]``.

   .. versionchanged:: 3.10
      Flag value ``0x04`` is a tuple of strings instead of dictionary

   .. versionchanged:: 3.11
      Qualified name at ``STACK[-1]`` was removed.

   .. versionchanged:: 3.13
      Extra function attributes on the stack, signaled by oparg flags, were
      removed. They now use :opcode:`SET_FUNCTION_ATTRIBUTE`.


.. opcode:: SET_FUNCTION_ATTRIBUTE (flag)

   Sets an attribute on a function object. Expects the function at ``STACK[-1]``
   and the attribute value to set at ``STACK[-2]``; consumes both and leaves the
   function at ``STACK[-1]``. The flag determines which attribute to set:

   * ``0x01`` a tuple of default values for positional-only and
     positional-or-keyword parameters in positional order
   * ``0x02`` a dictionary of keyword-only parameters' default values
   * ``0x04`` a tuple of strings containing parameters' annotations
   * ``0x08`` a tuple containing cells for free variables, making a closure
   * ``0x10`` the :term:`annotate function` for the function object

   .. versionadded:: 3.13

   .. versionchanged:: 3.14
      Added ``0x10`` to indicate the annotate function for the function object.


.. opcode:: BUILD_SLICE (argc)

   .. index:: pair: built-in function; slice

   Pushes a slice object on the stack.  *argc* must be 2 or 3.  If it is 2, implements::

      end = STACK.pop()
      start = STACK.pop()
      STACK.append(slice(start, end))

   if it is 3, implements::

      step = STACK.pop()
      end = STACK.pop()
      start = STACK.pop()
      STACK.append(slice(start, end, step))

   See the :func:`slice` built-in function for more information.


.. opcode:: EXTENDED_ARG (ext)

   Prefixes any opcode which has an argument too big to fit into the default one
   byte. *ext* holds an additional byte which act as higher bits in the argument.
   For each opcode, at most three prefixal ``EXTENDED_ARG`` are allowed, forming
   an argument from two-byte to four-byte.


.. opcode:: CONVERT_VALUE (oparg)

   Convert value to a string, depending on ``oparg``::

      value = STACK.pop()
      result = func(value)
      STACK.append(result)

   * ``oparg == 1``: call :func:`str` on *value*
   * ``oparg == 2``: call :func:`repr` on *value*
   * ``oparg == 3``: call :func:`ascii` on *value*

   Used for implementing formatted string literals (f-strings).

   .. versionadded:: 3.13


.. opcode:: FORMAT_SIMPLE

   Formats the value on top of stack::

      value = STACK.pop()
      result = value.__format__("")
      STACK.append(result)

   Used for implementing formatted string literals (f-strings).

   .. versionadded:: 3.13

.. opcode:: FORMAT_WITH_SPEC

   Formats the given value with the given format spec::

      spec = STACK.pop()
      value = STACK.pop()
      result = value.__format__(spec)
      STACK.append(result)

   Used for implementing formatted string literals (f-strings).

   .. versionadded:: 3.13


.. opcode:: MATCH_CLASS (count)

   ``STACK[-1]`` is a tuple of keyword attribute names, ``STACK[-2]`` is the class
   being matched against, and ``STACK[-3]`` is the match subject.  *count* is the
   number of positional sub-patterns.

   Pop ``STACK[-1]``, ``STACK[-2]``, and ``STACK[-3]``. If ``STACK[-3]`` is an
   instance of ``STACK[-2]`` and has the positional and keyword attributes
   required by *count* and ``STACK[-1]``, push a tuple of extracted attributes.
   Otherwise, push ``None``.

   .. versionadded:: 3.10

   .. versionchanged:: 3.11
      Previously, this instruction also pushed a boolean value indicating
      success (``True``) or failure (``False``).


.. opcode:: RESUME (context)

   A no-op. Performs internal tracing, debugging and optimization checks.

   The ``context`` operand consists of two parts. The lowest two bits
   indicate where the ``RESUME`` occurs:

   * ``0`` The start of a function, which is neither a generator, coroutine
     nor an async generator
   * ``1`` After a ``yield`` expression
   * ``2`` After a ``yield from`` expression
   * ``3`` After an ``await`` expression

   The next bit is ``1`` if the RESUME is at except-depth ``1``, and ``0``
   otherwise.

   .. versionadded:: 3.11

   .. versionchanged:: 3.13
      The oparg value changed to include information about except-depth


.. opcode:: RETURN_GENERATOR

   Create a generator, coroutine, or async generator from the current frame.
   Used as first opcode of in code object for the above mentioned callables.
   Clear the current frame and return the newly created generator.

   .. versionadded:: 3.11


.. opcode:: SEND (delta)

   Equivalent to ``STACK[-1] = STACK[-2].send(STACK[-1])``. Used in ``yield from``
   and ``await`` statements.

   If the call raises :exc:`StopIteration`, pop the top value from the stack,
   push the exception's ``value`` attribute, and increment the bytecode counter
   by *delta*.

   .. versionadded:: 3.11


.. opcode:: HAVE_ARGUMENT

   This is not really an opcode.  It identifies the dividing line between
   opcodes in the range [0,255] which don't use their argument and those
   that do (``< HAVE_ARGUMENT`` and ``>= HAVE_ARGUMENT``, respectively).

   If your application uses pseudo instructions or specialized instructions,
   use the :data:`hasarg` collection instead.

   .. versionchanged:: 3.6
      Now every instruction has an argument, but opcodes ``< HAVE_ARGUMENT``
      ignore it. Before, only opcodes ``>= HAVE_ARGUMENT`` had an argument.

   .. versionchanged:: 3.12
      Pseudo instructions were added to the :mod:`dis` module, and for them
      it is not true that comparison with ``HAVE_ARGUMENT`` indicates whether
      they use their arg.

   .. deprecated:: 3.13
      Use :data:`hasarg` instead.

.. opcode:: CALL_INTRINSIC_1

   Calls an intrinsic function with one argument. Passes ``STACK[-1]`` as the
   argument and sets ``STACK[-1]`` to the result. Used to implement
   functionality that is not performance critical.

   The operand determines which intrinsic function is called:

   +-----------------------------------+-----------------------------------+
   | Operand                           | Description                       |
   +===================================+===================================+
   | ``INTRINSIC_1_INVALID``           | Not valid                         |
   +-----------------------------------+-----------------------------------+
   | ``INTRINSIC_PRINT``               | Prints the argument to standard   |
   |                                   | out. Used in the REPL.            |
   +-----------------------------------+-----------------------------------+
   | ``INTRINSIC_IMPORT_STAR``         | Performs ``import *`` for the     |
   |                                   | named module.                     |
   +-----------------------------------+-----------------------------------+
   | ``INTRINSIC_STOPITERATION_ERROR`` | Extracts the return value from a  |
   |                                   | ``StopIteration`` exception.      |
   +-----------------------------------+-----------------------------------+
   | ``INTRINSIC_ASYNC_GEN_WRAP``      | Wraps an async generator value    |
   +-----------------------------------+-----------------------------------+
   | ``INTRINSIC_UNARY_POSITIVE``      | Performs the unary ``+``          |
   |                                   | operation                         |
   +-----------------------------------+-----------------------------------+
   | ``INTRINSIC_LIST_TO_TUPLE``       | Converts a list to a tuple        |
   +-----------------------------------+-----------------------------------+
   | ``INTRINSIC_TYPEVAR``             | Creates a :class:`typing.TypeVar` |
   +-----------------------------------+-----------------------------------+
   | ``INTRINSIC_PARAMSPEC``           | Creates a                         |
   |                                   | :class:`typing.ParamSpec`         |
   +-----------------------------------+-----------------------------------+
   | ``INTRINSIC_TYPEVARTUPLE``        | Creates a                         |
   |                                   | :class:`typing.TypeVarTuple`      |
   +-----------------------------------+-----------------------------------+
   | ``INTRINSIC_SUBSCRIPT_GENERIC``   | Returns :class:`typing.Generic`   |
   |                                   | subscripted with the argument     |
   +-----------------------------------+-----------------------------------+
   | ``INTRINSIC_TYPEALIAS``           | Creates a                         |
   |                                   | :class:`typing.TypeAliasType`;    |
   |                                   | used in the :keyword:`type`       |
   |                                   | statement. The argument is a tuple|
   |                                   | of the type alias's name,         |
   |                                   | type parameters, and value.       |
   +-----------------------------------+-----------------------------------+

   .. versionadded:: 3.12

.. opcode:: CALL_INTRINSIC_2

   Calls an intrinsic function with two arguments. Used to implement functionality
   that is not performance critical::

      arg2 = STACK.pop()
      arg1 = STACK.pop()
      result = intrinsic2(arg1, arg2)
      STACK.append(result)

   The operand determines which intrinsic function is called:

   +----------------------------------------+-----------------------------------+
   | Operand                                | Description                       |
   +========================================+===================================+
   | ``INTRINSIC_2_INVALID``                | Not valid                         |
   +----------------------------------------+-----------------------------------+
   | ``INTRINSIC_PREP_RERAISE_STAR``        | Calculates the                    |
   |                                        | :exc:`ExceptionGroup` to raise    |
   |                                        | from a ``try-except*``.           |
   +----------------------------------------+-----------------------------------+
   | ``INTRINSIC_TYPEVAR_WITH_BOUND``       | Creates a :class:`typing.TypeVar` |
   |                                        | with a bound.                     |
   +----------------------------------------+-----------------------------------+
   | ``INTRINSIC_TYPEVAR_WITH_CONSTRAINTS`` | Creates a                         |
   |                                        | :class:`typing.TypeVar` with      |
   |                                        | constraints.                      |
   +----------------------------------------+-----------------------------------+
   | ``INTRINSIC_SET_FUNCTION_TYPE_PARAMS`` | Sets the ``__type_params__``      |
   |                                        | attribute of a function.          |
   +----------------------------------------+-----------------------------------+

   .. versionadded:: 3.12


.. opcode:: LOAD_SPECIAL

   Performs special method lookup on ``STACK[-1]``.
   If ``type(STACK[-1]).__xxx__`` is a method, leave
   ``type(STACK[-1]).__xxx__; STACK[-1]`` on the stack.
   If ``type(STACK[-1]).__xxx__`` is not a method, leave
   ``STACK[-1].__xxx__; NULL`` on the stack.

   .. versionadded:: 3.14


**Pseudo-instructions**

These opcodes do not appear in Python bytecode. They are used by the compiler
but are replaced by real opcodes or removed before bytecode is generated.

.. opcode:: SETUP_FINALLY (target)

   Set up an exception handler for the following code block. If an exception
   occurs, the value stack level is restored to its current state and control
   is transferred to the exception handler at ``target``.


.. opcode:: SETUP_CLEANUP (target)

   Like ``SETUP_FINALLY``, but in case of an exception also pushes the last
   instruction (``lasti``) to the stack so that ``RERAISE`` can restore it.
   If an exception occurs, the value stack level and the last instruction on
   the frame are restored to their current state, and control is transferred
   to the exception handler at ``target``.


.. opcode:: SETUP_WITH (target)

   Like ``SETUP_CLEANUP``, but in case of an exception one more item is popped
   from the stack before control is transferred to the exception handler at
   ``target``.

   This variant is used in :keyword:`with` and :keyword:`async with`
   constructs, which push the return value of the context manager's
   :meth:`~object.__enter__` or :meth:`~object.__aenter__` to the stack.


.. opcode:: POP_BLOCK

   Marks the end of the code block associated with the last ``SETUP_FINALLY``,
   ``SETUP_CLEANUP`` or ``SETUP_WITH``.


.. opcode:: JUMP
            JUMP_NO_INTERRUPT

   Undirected relative jump instructions which are replaced by their
   directed (forward/backward) counterparts by the assembler.

.. opcode:: JUMP_IF_TRUE
            JUMP_IF_FALSE

   Conditional jumps which do not impact the stack. Replaced by the sequence
   ``COPY 1``, ``TO_BOOL``, ``POP_JUMP_IF_TRUE/FALSE``.

.. opcode:: LOAD_CLOSURE (i)

   Pushes a reference to the cell contained in slot ``i`` of the "fast locals"
   storage.

   Note that ``LOAD_CLOSURE`` is replaced with ``LOAD_FAST`` in the assembler.

   .. versionchanged:: 3.13
      This opcode is now a pseudo-instruction.


.. _opcode_collections:

Opcode collections
------------------

These collections are provided for automatic introspection of bytecode
instructions:

.. versionchanged:: 3.12
   The collections now contain pseudo instructions and instrumented
   instructions as well. These are opcodes with values ``>= MIN_PSEUDO_OPCODE``
   and ``>= MIN_INSTRUMENTED_OPCODE``.

.. data:: opname

   Sequence of operation names, indexable using the bytecode.


.. data:: opmap

   Dictionary mapping operation names to bytecodes.


.. data:: cmp_op

   Sequence of all compare operation names.


.. data:: hasarg

   Sequence of bytecodes that use their argument.

   .. versionadded:: 3.12


.. data:: hasconst

   Sequence of bytecodes that access a constant.


.. data:: hasfree

   Sequence of bytecodes that access a :term:`free (closure) variable <closure variable>`.
   'free' in this context refers to names in the current scope that are
   referenced by inner scopes or names in outer scopes that are referenced
   from this scope.  It does *not* include references to global or builtin scopes.


.. data:: hasname

   Sequence of bytecodes that access an attribute by name.


.. data:: hasjump

   Sequence of bytecodes that have a jump target. All jumps
   are relative.

   .. versionadded:: 3.13

.. data:: haslocal

   Sequence of bytecodes that access a local variable.


.. data:: hascompare

   Sequence of bytecodes of Boolean operations.

.. data:: hasexc

   Sequence of bytecodes that set an exception handler.

   .. versionadded:: 3.12


.. data:: hasjrel

   Sequence of bytecodes that have a relative jump target.

   .. deprecated:: 3.13
      All jumps are now relative. Use :data:`hasjump`.


.. data:: hasjabs

   Sequence of bytecodes that have an absolute jump target.

   .. deprecated:: 3.13
      All jumps are now relative. This list is empty.
