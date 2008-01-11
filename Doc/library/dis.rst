
:mod:`dis` --- Disassembler for Python bytecode
===============================================

.. module:: dis
   :synopsis: Disassembler for Python bytecode.


The :mod:`dis` module supports the analysis of Python :term:`bytecode` by disassembling
it.  Since there is no Python assembler, this module defines the Python assembly
language.  The Python bytecode which this module takes as an input is defined
in the file  :file:`Include/opcode.h` and used by the compiler and the
interpreter.

Example: Given the function :func:`myfunc`::

   def myfunc(alist):
       return len(alist)

the following command can be used to get the disassembly of :func:`myfunc`::

   >>> dis.dis(myfunc)
     2           0 LOAD_GLOBAL              0 (len)
                 3 LOAD_FAST                0 (alist)
                 6 CALL_FUNCTION            1
                 9 RETURN_VALUE

(The "2" is a line number).

The :mod:`dis` module defines the following functions and constants:


.. function:: dis([bytesource])

   Disassemble the *bytesource* object. *bytesource* can denote either a module, a
   class, a method, a function, or a code object.   For a module, it disassembles
   all functions.  For a class, it disassembles all methods.  For a single code
   sequence, it prints one line per bytecode instruction.  If no object is
   provided, it disassembles the last traceback.


.. function:: distb([tb])

   Disassembles the top-of-stack function of a traceback, using the last traceback
   if none was passed.  The instruction causing the exception is indicated.


.. function:: disassemble(code[, lasti])

   Disassembles a code object, indicating the last instruction if *lasti* was
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


.. function:: disco(code[, lasti])

   A synonym for disassemble.  It is more convenient to type, and kept for
   compatibility with earlier Python releases.


.. data:: opname

   Sequence of operation names, indexable using the bytecode.


.. data:: opmap

   Dictionary mapping bytecodes to operation names.


.. data:: cmp_op

   Sequence of all compare operation names.


.. data:: hasconst

   Sequence of bytecodes that have a constant parameter.


.. data:: hasfree

   Sequence of bytecodes that access a free variable.


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


.. _bytecodes:

Python Bytecode Instructions
----------------------------

The Python compiler currently generates the following bytecode instructions.


.. opcode:: STOP_CODE ()

   Indicates end-of-code to the compiler, not used by the interpreter.


.. opcode:: NOP ()

   Do nothing code.  Used as a placeholder by the bytecode optimizer.


.. opcode:: POP_TOP ()

   Removes the top-of-stack (TOS) item.


.. opcode:: ROT_TWO ()

   Swaps the two top-most stack items.


.. opcode:: ROT_THREE ()

   Lifts second and third stack item one position up, moves top down to position
   three.


.. opcode:: ROT_FOUR ()

   Lifts second, third and forth stack item one position up, moves top down to
   position four.


.. opcode:: DUP_TOP ()

   Duplicates the reference on top of the stack.

Unary Operations take the top of the stack, apply the operation, and push the
result back on the stack.


.. opcode:: UNARY_POSITIVE ()

   Implements ``TOS = +TOS``.


.. opcode:: UNARY_NEGATIVE ()

   Implements ``TOS = -TOS``.


.. opcode:: UNARY_NOT ()

   Implements ``TOS = not TOS``.


.. opcode:: UNARY_CONVERT ()

   Implements ``TOS = `TOS```.


.. opcode:: UNARY_INVERT ()

   Implements ``TOS = ~TOS``.


.. opcode:: GET_ITER ()

   Implements ``TOS = iter(TOS)``.

Binary operations remove the top of the stack (TOS) and the second top-most
stack item (TOS1) from the stack.  They perform the operation, and put the
result back on the stack.


.. opcode:: BINARY_POWER ()

   Implements ``TOS = TOS1 ** TOS``.


.. opcode:: BINARY_MULTIPLY ()

   Implements ``TOS = TOS1 * TOS``.


.. opcode:: BINARY_DIVIDE ()

   Implements ``TOS = TOS1 / TOS`` when ``from __future__ import division`` is not
   in effect.


.. opcode:: BINARY_FLOOR_DIVIDE ()

   Implements ``TOS = TOS1 // TOS``.


.. opcode:: BINARY_TRUE_DIVIDE ()

   Implements ``TOS = TOS1 / TOS`` when ``from __future__ import division`` is in
   effect.


.. opcode:: BINARY_MODULO ()

   Implements ``TOS = TOS1 % TOS``.


.. opcode:: BINARY_ADD ()

   Implements ``TOS = TOS1 + TOS``.


.. opcode:: BINARY_SUBTRACT ()

   Implements ``TOS = TOS1 - TOS``.


.. opcode:: BINARY_SUBSCR ()

   Implements ``TOS = TOS1[TOS]``.


.. opcode:: BINARY_LSHIFT ()

   Implements ``TOS = TOS1 << TOS``.


.. opcode:: BINARY_RSHIFT ()

   Implements ``TOS = TOS1 >> TOS``.


.. opcode:: BINARY_AND ()

   Implements ``TOS = TOS1 & TOS``.


.. opcode:: BINARY_XOR ()

   Implements ``TOS = TOS1 ^ TOS``.


.. opcode:: BINARY_OR ()

   Implements ``TOS = TOS1 | TOS``.

In-place operations are like binary operations, in that they remove TOS and
TOS1, and push the result back on the stack, but the operation is done in-place
when TOS1 supports it, and the resulting TOS may be (but does not have to be)
the original TOS1.


.. opcode:: INPLACE_POWER ()

   Implements in-place ``TOS = TOS1 ** TOS``.


.. opcode:: INPLACE_MULTIPLY ()

   Implements in-place ``TOS = TOS1 * TOS``.


.. opcode:: INPLACE_DIVIDE ()

   Implements in-place ``TOS = TOS1 / TOS`` when ``from __future__ import
   division`` is not in effect.


.. opcode:: INPLACE_FLOOR_DIVIDE ()

   Implements in-place ``TOS = TOS1 // TOS``.


.. opcode:: INPLACE_TRUE_DIVIDE ()

   Implements in-place ``TOS = TOS1 / TOS`` when ``from __future__ import
   division`` is in effect.


.. opcode:: INPLACE_MODULO ()

   Implements in-place ``TOS = TOS1 % TOS``.


.. opcode:: INPLACE_ADD ()

   Implements in-place ``TOS = TOS1 + TOS``.


.. opcode:: INPLACE_SUBTRACT ()

   Implements in-place ``TOS = TOS1 - TOS``.


.. opcode:: INPLACE_LSHIFT ()

   Implements in-place ``TOS = TOS1 << TOS``.


.. opcode:: INPLACE_RSHIFT ()

   Implements in-place ``TOS = TOS1 >> TOS``.


.. opcode:: INPLACE_AND ()

   Implements in-place ``TOS = TOS1 & TOS``.


.. opcode:: INPLACE_XOR ()

   Implements in-place ``TOS = TOS1 ^ TOS``.


.. opcode:: INPLACE_OR ()

   Implements in-place ``TOS = TOS1 | TOS``.

The slice opcodes take up to three parameters.


.. opcode:: SLICE+0 ()

   Implements ``TOS = TOS[:]``.


.. opcode:: SLICE+1 ()

   Implements ``TOS = TOS1[TOS:]``.


.. opcode:: SLICE+2 ()

   Implements ``TOS = TOS1[:TOS]``.


.. opcode:: SLICE+3 ()

   Implements ``TOS = TOS2[TOS1:TOS]``.

Slice assignment needs even an additional parameter.  As any statement, they put
nothing on the stack.


.. opcode:: STORE_SLICE+0 ()

   Implements ``TOS[:] = TOS1``.


.. opcode:: STORE_SLICE+1 ()

   Implements ``TOS1[TOS:] = TOS2``.


.. opcode:: STORE_SLICE+2 ()

   Implements ``TOS1[:TOS] = TOS2``.


.. opcode:: STORE_SLICE+3 ()

   Implements ``TOS2[TOS1:TOS] = TOS3``.


.. opcode:: DELETE_SLICE+0 ()

   Implements ``del TOS[:]``.


.. opcode:: DELETE_SLICE+1 ()

   Implements ``del TOS1[TOS:]``.


.. opcode:: DELETE_SLICE+2 ()

   Implements ``del TOS1[:TOS]``.


.. opcode:: DELETE_SLICE+3 ()

   Implements ``del TOS2[TOS1:TOS]``.


.. opcode:: STORE_SUBSCR ()

   Implements ``TOS1[TOS] = TOS2``.


.. opcode:: DELETE_SUBSCR ()

   Implements ``del TOS1[TOS]``.

Miscellaneous opcodes.


.. opcode:: PRINT_EXPR ()

   Implements the expression statement for the interactive mode.  TOS is removed
   from the stack and printed.  In non-interactive mode, an expression statement is
   terminated with ``POP_STACK``.


.. opcode:: PRINT_ITEM ()

   Prints TOS to the file-like object bound to ``sys.stdout``.  There is one such
   instruction for each item in the :keyword:`print` statement.


.. opcode:: PRINT_ITEM_TO ()

   Like ``PRINT_ITEM``, but prints the item second from TOS to the file-like object
   at TOS.  This is used by the extended print statement.


.. opcode:: PRINT_NEWLINE ()

   Prints a new line on ``sys.stdout``.  This is generated as the last operation of
   a :keyword:`print` statement, unless the statement ends with a comma.


.. opcode:: PRINT_NEWLINE_TO ()

   Like ``PRINT_NEWLINE``, but prints the new line on the file-like object on the
   TOS.  This is used by the extended print statement.


.. opcode:: BREAK_LOOP ()

   Terminates a loop due to a :keyword:`break` statement.


.. opcode:: CONTINUE_LOOP (target)

   Continues a loop due to a :keyword:`continue` statement.  *target* is the
   address to jump to (which should be a ``FOR_ITER`` instruction).


.. opcode:: LIST_APPEND ()

   Calls ``list.append(TOS1, TOS)``.  Used to implement list comprehensions.


.. opcode:: LOAD_LOCALS ()

   Pushes a reference to the locals of the current scope on the stack. This is used
   in the code for a class definition: After the class body is evaluated, the
   locals are passed to the class definition.


.. opcode:: RETURN_VALUE ()

   Returns with TOS to the caller of the function.


.. opcode:: YIELD_VALUE ()

   Pops ``TOS`` and yields it from a :term:`generator`.


.. opcode:: IMPORT_STAR ()

   Loads all symbols not starting with ``'_'`` directly from the module TOS to the
   local namespace. The module is popped after loading all names. This opcode
   implements ``from module import *``.


.. opcode:: EXEC_STMT ()

   Implements ``exec TOS2,TOS1,TOS``.  The compiler fills missing optional
   parameters with ``None``.


.. opcode:: POP_BLOCK ()

   Removes one block from the block stack.  Per frame, there is a  stack of blocks,
   denoting nested loops, try statements, and such.


.. opcode:: END_FINALLY ()

   Terminates a :keyword:`finally` clause.  The interpreter recalls whether the
   exception has to be re-raised, or whether the function returns, and continues
   with the outer-next block.


.. opcode:: BUILD_CLASS ()

   Creates a new class object.  TOS is the methods dictionary, TOS1 the tuple of
   the names of the base classes, and TOS2 the class name.


.. opcode:: WITH_CLEANUP ()

   Cleans up the stack when a :keyword:`with` statement block exits.  TOS is the
   context manager's :meth:`__exit__` bound method.  Below that are 1--3 values
   indicating how/why the finally clause was entered:

   * SECOND = ``None``
   * (SECOND, THIRD) = (``WHY_{RETURN,CONTINUE}``), retval
   * SECOND = ``WHY_*``; no retval below it
   * (SECOND, THIRD, FOURTH) = exc_info()

   In the last case, ``TOS(SECOND, THIRD, FOURTH)`` is called, otherwise
   ``TOS(None, None, None)``.

   In addition, if the stack represents an exception, *and* the function call
   returns a 'true' value, this information is "zapped", to prevent ``END_FINALLY``
   from re-raising the exception.  (But non-local gotos should still be resumed.)

   .. XXX explain the WHY stuff!


All of the following opcodes expect arguments.  An argument is two bytes, with
the more significant byte last.

.. opcode:: STORE_NAME (namei)

   Implements ``name = TOS``. *namei* is the index of *name* in the attribute
   :attr:`co_names` of the code object. The compiler tries to use ``STORE_LOCAL``
   or ``STORE_GLOBAL`` if possible.


.. opcode:: DELETE_NAME (namei)

   Implements ``del name``, where *namei* is the index into :attr:`co_names`
   attribute of the code object.


.. opcode:: UNPACK_SEQUENCE (count)

   Unpacks TOS into *count* individual values, which are put onto the stack
   right-to-left.


.. opcode:: DUP_TOPX (count)

   Duplicate *count* items, keeping them in the same order. Due to implementation
   limits, *count* should be between 1 and 5 inclusive.


.. opcode:: STORE_ATTR (namei)

   Implements ``TOS.name = TOS1``, where *namei* is the index of name in
   :attr:`co_names`.


.. opcode:: DELETE_ATTR (namei)

   Implements ``del TOS.name``, using *namei* as index into :attr:`co_names`.


.. opcode:: STORE_GLOBAL (namei)

   Works as ``STORE_NAME``, but stores the name as a global.


.. opcode:: DELETE_GLOBAL (namei)

   Works as ``DELETE_NAME``, but deletes a global name.


.. opcode:: LOAD_CONST (consti)

   Pushes ``co_consts[consti]`` onto the stack.


.. opcode:: LOAD_NAME (namei)

   Pushes the value associated with ``co_names[namei]`` onto the stack.


.. opcode:: BUILD_TUPLE (count)

   Creates a tuple consuming *count* items from the stack, and pushes the resulting
   tuple onto the stack.


.. opcode:: BUILD_LIST (count)

   Works as ``BUILD_TUPLE``, but creates a list.


.. opcode:: BUILD_MAP (count)

   Pushes a new dictionary object onto the stack.  The dictionary is pre-sized
   to hold *count* entries.


.. opcode:: LOAD_ATTR (namei)

   Replaces TOS with ``getattr(TOS, co_names[namei])``.


.. opcode:: COMPARE_OP (opname)

   Performs a Boolean operation.  The operation name can be found in
   ``cmp_op[opname]``.


.. opcode:: IMPORT_NAME (namei)

   Imports the module ``co_names[namei]``.  The module object is pushed onto the
   stack.  The current namespace is not affected: for a proper import statement, a
   subsequent ``STORE_FAST`` instruction modifies the namespace.


.. opcode:: IMPORT_FROM (namei)

   Loads the attribute ``co_names[namei]`` from the module found in TOS. The
   resulting object is pushed onto the stack, to be subsequently stored by a
   ``STORE_FAST`` instruction.


.. opcode:: JUMP_FORWARD (delta)

   Increments bytecode counter by *delta*.


.. opcode:: JUMP_IF_TRUE (delta)

   If TOS is true, increment the bytecode counter by *delta*.  TOS is left on the
   stack.


.. opcode:: JUMP_IF_FALSE (delta)

   If TOS is false, increment the bytecode counter by *delta*.  TOS is not
   changed.


.. opcode:: JUMP_ABSOLUTE (target)

   Set bytecode counter to *target*.


.. opcode:: FOR_ITER (delta)

   ``TOS`` is an :term:`iterator`.  Call its :meth:`next` method.  If this
   yields a new value, push it on the stack (leaving the iterator below it).  If
   the iterator indicates it is exhausted ``TOS`` is popped, and the bytecode
   counter is incremented by *delta*.


.. opcode:: LOAD_GLOBAL (namei)

   Loads the global named ``co_names[namei]`` onto the stack.


.. opcode:: SETUP_LOOP (delta)

   Pushes a block for a loop onto the block stack.  The block spans from the
   current instruction with a size of *delta* bytes.


.. opcode:: SETUP_EXCEPT (delta)

   Pushes a try block from a try-except clause onto the block stack. *delta* points
   to the first except block.


.. opcode:: SETUP_FINALLY (delta)

   Pushes a try block from a try-except clause onto the block stack. *delta* points
   to the finally block.

.. opcode:: STORE_MAP ()

   Store a key and value pair in a dictionary.  Pops the key and value while leaving
   the dictionary on the stack.

.. opcode:: LOAD_FAST (var_num)

   Pushes a reference to the local ``co_varnames[var_num]`` onto the stack.


.. opcode:: STORE_FAST (var_num)

   Stores TOS into the local ``co_varnames[var_num]``.


.. opcode:: DELETE_FAST (var_num)

   Deletes local ``co_varnames[var_num]``.


.. opcode:: LOAD_CLOSURE (i)

   Pushes a reference to the cell contained in slot *i* of the cell and free
   variable storage.  The name of the variable is  ``co_cellvars[i]`` if *i* is
   less than the length of *co_cellvars*.  Otherwise it is  ``co_freevars[i -
   len(co_cellvars)]``.


.. opcode:: LOAD_DEREF (i)

   Loads the cell contained in slot *i* of the cell and free variable storage.
   Pushes a reference to the object the cell contains on the stack.


.. opcode:: STORE_DEREF (i)

   Stores TOS into the cell contained in slot *i* of the cell and free variable
   storage.


.. opcode:: SET_LINENO (lineno)

   This opcode is obsolete.


.. opcode:: RAISE_VARARGS (argc)

   Raises an exception. *argc* indicates the number of parameters to the raise
   statement, ranging from 0 to 3.  The handler will find the traceback as TOS2,
   the parameter as TOS1, and the exception as TOS.


.. opcode:: CALL_FUNCTION (argc)

   Calls a function.  The low byte of *argc* indicates the number of positional
   parameters, the high byte the number of keyword parameters. On the stack, the
   opcode finds the keyword parameters first.  For each keyword argument, the value
   is on top of the key.  Below the keyword parameters, the positional parameters
   are on the stack, with the right-most parameter on top.  Below the parameters,
   the function object to call is on the stack.


.. opcode:: MAKE_FUNCTION (argc)

   Pushes a new function object on the stack.  TOS is the code associated with the
   function.  The function object is defined to have *argc* default parameters,
   which are found below TOS.


.. opcode:: MAKE_CLOSURE (argc)

   Creates a new function object, sets its *func_closure* slot, and pushes it on
   the stack.  TOS is the code associated with the function, TOS1 the tuple
   containing cells for the closure's free variables.  The function also has
   *argc* default parameters, which are found below the cells.


.. opcode:: BUILD_SLICE (argc)

   .. index:: builtin: slice

   Pushes a slice object on the stack.  *argc* must be 2 or 3.  If it is 2,
   ``slice(TOS1, TOS)`` is pushed; if it is 3, ``slice(TOS2, TOS1, TOS)`` is
   pushed. See the :func:`slice` built-in function for more information.


.. opcode:: EXTENDED_ARG (ext)

   Prefixes any opcode which has an argument too big to fit into the default two
   bytes.  *ext* holds two additional bytes which, taken together with the
   subsequent opcode's argument, comprise a four-byte argument, *ext* being the two
   most-significant bytes.


.. opcode:: CALL_FUNCTION_VAR (argc)

   Calls a function. *argc* is interpreted as in ``CALL_FUNCTION``. The top element
   on the stack contains the variable argument list, followed by keyword and
   positional arguments.


.. opcode:: CALL_FUNCTION_KW (argc)

   Calls a function. *argc* is interpreted as in ``CALL_FUNCTION``. The top element
   on the stack contains the keyword arguments dictionary,  followed by explicit
   keyword and positional arguments.


.. opcode:: CALL_FUNCTION_VAR_KW (argc)

   Calls a function. *argc* is interpreted as in ``CALL_FUNCTION``.  The top
   element on the stack contains the keyword arguments dictionary, followed by the
   variable-arguments tuple, followed by explicit keyword and positional arguments.


.. opcode:: HAVE_ARGUMENT ()

   This is not really an opcode.  It identifies the dividing line between opcodes
   which don't take arguments ``< HAVE_ARGUMENT`` and those which do ``>=
   HAVE_ARGUMENT``.

