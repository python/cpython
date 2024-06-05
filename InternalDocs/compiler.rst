.. _compiler:

===============
Compiler design
===============

.. highlight:: none

Abstract
========

In CPython, the compilation from source code to bytecode involves several steps:

1. Tokenize the source code (:cpy-file:`Parser/lexer/` and :cpy-file:`Parser/tokenizer/`).
2. Parse the stream of tokens into an Abstract Syntax Tree
   (:cpy-file:`Parser/parser.c`).
3. Transform AST into an instruction sequence (:cpy-file:`Python/compile.c`).
4. Construct a Control Flow Graph and apply optimizations to it (:cpy-file:`Python/flowgraph.c`).
5. Emit bytecode based on the Control Flow Graph (:cpy-file:`Python/assemble.c`).

This document outlines how these steps of the process work.

This document only describes parsing in enough depth to explain what is needed
for understanding compilation.  This document provides a detailed, though not
exhaustive, view of the how the entire system works.  You will most likely need
to read some source code to have an exact understanding of all details.


Parsing
=======

As of Python 3.9, Python's parser is a PEG parser of a somewhat
unusual design. It is unusual in the sense that the parser's input is a stream
of tokens rather than a stream of characters which is more common with PEG
parsers.

The grammar file for Python can be found in
:cpy-file:`Grammar/python.gram`.  The definitions for literal tokens
(such as ``:``, numbers, etc.) can be found in :cpy-file:`Grammar/Tokens`.
Various C files, including :cpy-file:`Parser/parser.c` are generated from
these.

.. seealso::

  :ref:`parser` for a detailed description of the parser.

  :ref:`grammar` for a detailed description of the grammar.


Abstract syntax trees (AST)
===========================

.. _compiler-ast-trees:

.. sidebar:: Green Tree Snakes

   See also `Green Tree Snakes - the missing Python AST docs
   <https://greentreesnakes.readthedocs.io/en/latest/>`_ by Thomas Kluyver.

The abstract syntax tree (AST) is a high-level representation of the
program structure without the necessity of containing the source code;
it can be thought of as an abstract representation of the source code.  The
specification of the AST nodes is specified using the Zephyr Abstract
Syntax Definition Language (ASDL) [Wang97]_.

The definition of the AST nodes for Python is found in the file
:cpy-file:`Parser/Python.asdl`.

Each AST node (representing statements, expressions, and several
specialized types, like list comprehensions and exception handlers) is
defined by the ASDL.  Most definitions in the AST correspond to a
particular source construct, such as an 'if' statement or an attribute
lookup.  The definition is independent of its realization in any
particular programming language.

The following fragment of the Python ASDL construct demonstrates the
approach and syntax::

   module Python
   {
       stmt = FunctionDef(identifier name, arguments args, stmt* body,
                          expr* decorators)
              | Return(expr? value) | Yield(expr? value)
              attributes (int lineno)
   }

The preceding example describes two different kinds of statements and an
expression: function definitions, return statements, and yield expressions.
All three kinds are considered of type ``stmt`` as shown by ``|`` separating
the various kinds.  They all take arguments of various kinds and amounts.

Modifiers on the argument type specify the number of values needed; ``?``
means it is optional, ``*`` means 0 or more, while no modifier means only one
value for the argument and it is required.  ``FunctionDef``, for instance,
takes an ``identifier`` for the *name*, ``arguments`` for *args*, zero or more
``stmt`` arguments for *body*, and zero or more ``expr`` arguments for
*decorators*.

Do notice that something like 'arguments', which is a node type, is
represented as a single AST node and not as a sequence of nodes as with
stmt as one might expect.

All three kinds also have an 'attributes' argument; this is shown by the
fact that 'attributes' lacks a '|' before it.

The statement definitions above generate the following C structure type:

.. code-block:: c

  typedef struct _stmt *stmt_ty;

  struct _stmt {
        enum { FunctionDef_kind=1, Return_kind=2, Yield_kind=3 } kind;
        union {
                struct {
                        identifier name;
                        arguments_ty args;
                        asdl_seq *body;
                } FunctionDef;

                struct {
                        expr_ty value;
                } Return;

                struct {
                        expr_ty value;
                } Yield;
        } v;
        int lineno;
   }

Also generated are a series of constructor functions that allocate (in
this case) a ``stmt_ty`` struct with the appropriate initialization.  The
``kind`` field specifies which component of the union is initialized.  The
``FunctionDef()`` constructor function sets 'kind' to ``FunctionDef_kind`` and
initializes the *name*, *args*, *body*, and *attributes* fields.


Memory management
=================

Before discussing the actual implementation of the compiler, a discussion of
how memory is handled is in order.  To make memory management simple, an **arena**
is used that pools memory in a single location for easy
allocation and removal.  This enables the removal of explicit memory
deallocation.  Because memory allocation for all needed memory in the compiler
registers that memory with the arena, a single call to free the arena is all
that is needed to completely free all memory used by the compiler.

In general, unless you are working on the critical core of the compiler, memory
management can be completely ignored.  But if you are working at either the
very beginning of the compiler or the end, you need to care about how the arena
works.  All code relating to the arena is in either
:cpy-file:`Include/internal/pycore_pyarena.h` or :cpy-file:`Python/pyarena.c`.

``PyArena_New()`` will create a new arena.  The returned ``PyArena`` structure
will store pointers to all memory given to it.  This does the bookkeeping of
what memory needs to be freed when the compiler is finished with the memory it
used. That freeing is done with ``PyArena_Free()``.  This only needs to be
called in strategic areas where the compiler exits.

As stated above, in general you should not have to worry about memory
management when working on the compiler.  The technical details of memory
management have been designed to be hidden from you for most cases.

The only exception comes about when managing a PyObject.  Since the rest
of Python uses reference counting, there is extra support added
to the arena to cleanup each PyObject that was allocated.  These cases
are very rare.  However, if you've allocated a PyObject, you must tell
the arena about it by calling ``PyArena_AddPyObject()``.


Source code to AST
==================

The AST is generated from source code using the function
``_PyParser_ASTFromString()`` or ``_PyParser_ASTFromFile()``
(from :cpy-file:`Parser/peg_api.c`) depending on the input type.

After some checks, a helper function in :cpy-file:`Parser/parser.c` begins applying
production rules on the source code it receives; converting source code to
tokens and matching these tokens recursively to their corresponding rule.  The
production rule's corresponding rule function is called on every match.  These rule
functions follow the format :samp:`xx_rule`.  Where *xx* is the grammar rule
that the function handles and is automatically derived from
:cpy-file:`Grammar/python.gram`
:cpy-file:`Tools/peg_generator/pegen/c_generator.py`.

Each rule function in turn creates an AST node as it goes along.  It does this
by allocating all the new nodes it needs, calling the proper AST node creation
functions for any required supporting functions and connecting them as needed.
This continues until all nonterminal symbols are replaced with terminals.  If an
error occurs, the rule functions backtrack and try another rule function.  If
there are no more rules, an error is set and the parsing ends.

The AST node creation helper functions have the name :samp:`_PyAST_{xx}`
where *xx* is the AST node that the function creates.  These are defined by the
ASDL grammar and contained in :cpy-file:`Python/Python-ast.c` (which is
generated by :cpy-file:`Parser/asdl_c.py` from :cpy-file:`Parser/Python.asdl`).
This all leads to a sequence of AST nodes stored in ``asdl_seq`` structs.

To demonstrate everything explained so far, here's the
rule function responsible for a simple named import statement such as
``import sys``.  Note that error-checking and debugging code has been
omitted.  Removed parts are represented by ``...``.
Furthermore, some comments have been added for explanation.  These comments
may not be present in the actual code.

.. code-block:: c

   // This is the production rule (from python.gram) the rule function
   // corresponds to:
   // import_name: 'import' dotted_as_names
   static stmt_ty
   import_name_rule(Parser *p)
   {
       ...
       stmt_ty _res = NULL;
       { // 'import' dotted_as_names
           ...
           Token * _keyword;
           asdl_alias_seq* a;
           // The tokenizing steps.
           if (
               (_keyword = _PyPegen_expect_token(p, 513))  // token='import'
               &&
               (a = dotted_as_names_rule(p))  // dotted_as_names
           )
           {
               ...
               // Generate an AST for the import statement.
               _res = _PyAST_Import ( a , ...);
               ...
               goto done;
           }
           ...
       }
       _res = NULL;
     done:
       ...
       return _res;
   }


To improve backtracking performance, some rules (chosen by applying a
``(memo)`` flag in the grammar file) are memoized.  Each rule function checks if
a memoized version exists and returns that if so, else it continues in the
manner stated in the previous paragraphs.

There are macros for creating and using ``asdl_xx_seq *`` types, where *xx* is
a type of the ASDL sequence.  Three main types are defined
manually -- ``generic``, ``identifier`` and ``int``.  These types are found in
:cpy-file:`Python/asdl.c` and its corresponding header file
:cpy-file:`Include/internal/pycore_asdl.h`.  Functions and macros
for creating ``asdl_xx_seq *`` types are as follows:

``_Py_asdl_generic_seq_new(Py_ssize_t, PyArena *)``
        Allocate memory for an ``asdl_generic_seq`` of the specified length
``_Py_asdl_identifier_seq_new(Py_ssize_t, PyArena *)``
        Allocate memory for an ``asdl_identifier_seq`` of the specified length
``_Py_asdl_int_seq_new(Py_ssize_t, PyArena *)``
        Allocate memory for an ``asdl_int_seq`` of the specified length

In addition to the three types mentioned above, some ASDL sequence types are
automatically generated by :cpy-file:`Parser/asdl_c.py` and found in
:cpy-file:`Include/internal/pycore_ast.h`.  Macros for using both manually
defined and automatically generated ASDL sequence types are as follows:

``asdl_seq_GET(asdl_xx_seq *, int)``
        Get item held at a specific position in an ``asdl_xx_seq``
``asdl_seq_SET(asdl_xx_seq *, int, stmt_ty)``
        Set a specific index in an ``asdl_xx_seq`` to the specified value

Untyped counterparts exist for some of the typed macros.  These are useful
when a function needs to manipulate a generic ASDL sequence:

``asdl_seq_GET_UNTYPED(asdl_seq *, int)``
        Get item held at a specific position in an ``asdl_seq``
``asdl_seq_SET_UNTYPED(asdl_seq *, int, stmt_ty)``
        Set a specific index in an ``asdl_seq`` to the specified value
``asdl_seq_LEN(asdl_seq *)``
        Return the length of an ``asdl_seq`` or ``asdl_xx_seq``

Note that typed macros and functions are recommended over their untyped
counterparts.  Typed macros carry out checks in debug mode and aid
debugging errors caused by incorrectly casting from ``void *``.

If you are working with statements, you must also worry about keeping
track of what line number generated the statement.  Currently the line
number is passed as the last parameter to each ``stmt_ty`` function.

.. versionchanged:: 3.9
   The new PEG parser generates an AST directly without creating a
   parse tree. ``Python/ast.c`` is now only used to validate the AST for
   debugging purposes.

.. seealso:: :pep:`617` (PEP 617 -- New PEG parser for CPython)


Control flow graphs
===================

A **control flow graph** (often referenced by its acronym, **CFG**) is a
directed graph that models the flow of a program.  A node of a CFG is
not an individual bytecode instruction, but instead represents a
sequence of bytecode instructions that always execute sequentially.
Each node is called a *basic block* and must always execute from
start to finish, with a single entry point at the beginning and a
single exit point at the end.  If some bytecode instruction *a* needs
to jump to some other bytecode instruction *b*, then *a* must occur at
the end of its basic block, and *b* must occur at the start of its
basic block.

As an example, consider the following code snippet:

.. code-block:: Python

   if x < 10:
       f1()
       f2()
   else:
       g()
   end()

The ``x < 10`` guard is represented by its own basic block that
compares ``x`` with ``10`` and then ends in a conditional jump based on
the result of the comparison.  This conditional jump allows the block
to point to both the body of the ``if`` and the body of the ``else``.  The
``if`` basic block contains the ``f1()`` and ``f2()`` calls and points to
the ``end()`` basic block. The ``else`` basic block contains the ``g()``
call and similarly points to the ``end()`` block.

Note that more complex code in the guard, the ``if`` body, or the ``else``
body may be represented by multiple basic blocks. For instance,
short-circuiting boolean logic in a guard like ``if x or y:``
will produce one basic block that tests the truth value of ``x``
and then points both (1) to the start of the ``if`` body and (2) to
a different basic block that tests the truth value of y.

CFGs are usually one step away from final code output.  Code is directly
generated from the basic blocks (with jump targets adjusted based on the
output order) by doing a post-order depth-first search on the CFG
following the edges.


AST to CFG to bytecode
======================

With the AST created, the next step is to create the CFG. The first step
is to convert the AST to Python bytecode without having jump targets
resolved to specific offsets (this is calculated when the CFG goes to
final bytecode). Essentially, this transforms the AST into Python
bytecode with control flow represented by the edges of the CFG.

Conversion is done in two passes.  The first creates the namespace
(variables can be classified as local, free/cell for closures, or
global).  With that done, the second pass essentially flattens the CFG
into a list and calculates jump offsets for final output of bytecode.

The conversion process is initiated by a call to the function
``_PyAST_Compile()`` in :cpy-file:`Python/compile.c`.  This function does both
the conversion of the AST to a CFG and outputting final bytecode from the CFG.
The AST to CFG step is handled mostly by two functions called by
``_PyAST_Compile()``; ``_PySymtable_Build()`` and ``compiler_mod()``.
The former is in :cpy-file:`Python/symtable.c` while the latter is
:cpy-file:`Python/compile.c`.

``_PySymtable_Build()`` begins by entering the starting code block for the
AST (passed-in) and then calling the proper :samp:`symtable_visit_{xx}` function
(with *xx* being the AST node type).  Next, the AST tree is walked with
the various code blocks that delineate the reach of a local variable
as blocks are entered and exited using ``symtable_enter_block()`` and
``symtable_exit_block()``, respectively.

Once the symbol table is created, it is time for CFG creation, whose
code is in :cpy-file:`Python/compile.c`.  This is handled by several functions
that break the task down by various AST node types.  The functions are
all named :samp:`compiler_visit_{xx}` where *xx* is the name of the node type (such
as ``stmt``, ``expr``, etc.).  Each function receives a ``struct compiler *``
and :samp:`{xx}_ty` where *xx* is the AST node type.  Typically these functions
consist of a large 'switch' statement, branching based on the kind of
node type passed to it.  Simple things are handled inline in the
'switch' statement with more complex transformations farmed out to other
functions named :samp:`compiler_{xx}` with *xx* being a descriptive name of what is
being handled.

When transforming an arbitrary AST node, use the ``VISIT()`` macro.
The appropriate :samp:`compiler_visit_{xx}` function is called, based on the value
passed in for <node type> (so :samp:`VISIT({c}, expr, {node})` calls
:samp:`compiler_visit_expr({c}, {node})`).  The ``VISIT_SEQ()`` macro is very similar,
but is called on AST node sequences (those values that were created as
arguments to a node that used the '*' modifier).  There is also
``VISIT_SLICE()`` just for handling slices.

Emission of bytecode is handled by the following macros:

``ADDOP(struct compiler *, int)``
    add a specified opcode
``ADDOP_NOLINE(struct compiler *, int)``
    like ``ADDOP`` without a line number; used for artificial opcodes without
    no corresponding token in the source code
``ADDOP_IN_SCOPE(struct compiler *, int)``
    like ``ADDOP``, but also exits current scope; used for adding return value
    opcodes in lambdas and closures
``ADDOP_I(struct compiler *, int, Py_ssize_t)``
    add an opcode that takes an integer argument
``ADDOP_O(struct compiler *, int, PyObject *, TYPE)``
    add an opcode with the proper argument based on the position of the
    specified PyObject in PyObject sequence object, but with no handling of
    mangled names; used for when you
    need to do named lookups of objects such as globals, consts, or
    parameters where name mangling is not possible and the scope of the
    name is known; *TYPE* is the name of PyObject sequence
    (``names`` or ``varnames``)
``ADDOP_N(struct compiler *, int, PyObject *, TYPE)``
    just like ``ADDOP_O``, but steals a reference to PyObject
``ADDOP_NAME(struct compiler *, int, PyObject *, TYPE)``
    just like ``ADDOP_O``, but name mangling is also handled; used for
    attribute loading or importing based on name
``ADDOP_LOAD_CONST(struct compiler *, PyObject *)``
    add the ``LOAD_CONST`` opcode with the proper argument based on the
    position of the specified PyObject in the consts table.
``ADDOP_LOAD_CONST_NEW(struct compiler *, PyObject *)``
    just like ``ADDOP_LOAD_CONST_NEW``, but steals a reference to PyObject
``ADDOP_JUMP(struct compiler *, int, basicblock *)``
    create a jump to a basic block
``ADDOP_JUMP_NOLINE(struct compiler *, int, basicblock *)``
    like ``ADDOP_JUMP`` without a line number; used for artificial jumps
    without no corresponding token in the source code.
``ADDOP_JUMP_COMPARE(struct compiler *, cmpop_ty)``
    depending on the second argument, add an ``ADDOP_I`` with either an
    ``IS_OP``, ``CONTAINS_OP``, or ``COMPARE_OP`` opcode.

Several helper functions that will emit bytecode and are named
:samp:`compiler_{xx}()` where *xx* is what the function helps with (``list``,
``boolop``, etc.).  A rather useful one is ``compiler_nameop()``.
This function looks up the scope of a variable and, based on the
expression context, emits the proper opcode to load, store, or delete
the variable.

As for handling the line number on which a statement is defined, this is
handled by ``compiler_visit_stmt()`` and thus is not a worry.

Once the CFG is created, it must be flattened and then final emission of
bytecode occurs.  Flattening is handled using a post-order depth-first
search.  Once flattened, jump offsets are backpatched based on the
flattening and then a ``PyCodeObject`` is created.  All of this is
handled by calling ``assemble()``.


Code objects
============

The result of ``PyAST_CompileObject()`` is a ``PyCodeObject`` which is defined in
:cpy-file:`Include/cpython/code.h`.  And with that you now have executable
Python bytecode!

The code objects (byte code) are executed in :cpy-file:`Python/ceval.c`.  This file
will also need a new case statement for the new opcode in the big switch
statement in ``_PyEval_EvalFrameDefault()``.


Important files
===============

* :cpy-file:`Parser/`

  * :cpy-file:`Parser/Python.asdl`: ASDL syntax file.

  * :cpy-file:`Parser/asdl.py`: Parser for ASDL definition files.
    Reads in an ASDL description and parses it into an AST that describes it.

  * :cpy-file:`Parser/asdl_c.py`: Generate C code from an ASDL description.
    Generates :cpy-file:`Python/Python-ast.c` and
    :cpy-file:`Include/internal/pycore_ast.h`.

  * :cpy-file:`Parser/parser.c`: The new PEG parser introduced in Python 3.9.
    Generated by :cpy-file:`Tools/peg_generator/pegen/c_generator.py`
    from the grammar :cpy-file:`Grammar/python.gram`.  Creates the AST from
    source code.  Rule functions for their corresponding production rules
    are found here.

  * :cpy-file:`Parser/peg_api.c`: Contains high-level functions which are
    used by the interpreter to create an AST from source code.

  * :cpy-file:`Parser/pegen.c`: Contains helper functions which are used
    by functions in :cpy-file:`Parser/parser.c` to construct the AST.
    Also contains helper functions which help raise better error messages
    when parsing source code.

  * :cpy-file:`Parser/pegen.h`: Header file for the corresponding
    :cpy-file:`Parser/pegen.c`. Also contains definitions of the ``Parser``
    and ``Token`` structs.

* :cpy-file:`Python/`

  * :cpy-file:`Python/Python-ast.c`: Creates C structs corresponding to
    the ASDL types.  Also contains code for marshalling AST nodes (core
    ASDL types have marshalling code in :cpy-file:`Python/asdl.c`).
    "File automatically generated by :cpy-file:`Parser/asdl_c.py`".
    This file must be committed separately after every grammar change
    is committed since the ``__version__`` value is set to the latest
    grammar change revision number.

  * :cpy-file:`Python/asdl.c`: Contains code to handle the ASDL sequence type.
    Also has code to handle marshalling the core ASDL types, such as number
    and identifier.  Used by :cpy-file:`Python/Python-ast.c` for marshalling
    AST nodes.

  * :cpy-file:`Python/ast.c`: Used for validating the AST.

  * :cpy-file:`Python/ast_opt.c`: Optimizes the AST.

  * :cpy-file:`Python/ast_unparse.c`: Converts the AST expression node
    back into a string (for string annotations).

  * :cpy-file:`Python/ceval.c`: Executes byte code (aka, eval loop).

  * :cpy-file:`Python/compile.c`: Emits bytecode based on the AST.

  * :cpy-file:`Python/symtable.c`: Generates a symbol table from AST.

  * :cpy-file:`Python/pyarena.c`: Implementation of the arena memory manager.

  * :cpy-file:`Python/opcode_targets.h`: One of the files that must be
    modified if :cpy-file:`Lib/opcode.py` is.

* :cpy-file:`Include/`

  * :cpy-file:`Include/cpython/code.h`: Header file for
    :cpy-file:`Objects/codeobject.c`; contains definition of ``PyCodeObject``.

  * :cpy-file:`Include/opcode.h`: One of the files that must be modified if
    :cpy-file:`Lib/opcode.py` is.

  * :cpy-file:`Include/internal/pycore_ast.h`: Contains the actual definitions
    of the C structs as generated by :cpy-file:`Python/Python-ast.c`.
    "Automatically generated by :cpy-file:`Parser/asdl_c.py`".

  * :cpy-file:`Include/internal/pycore_asdl.h`: Header for the corresponding
    :cpy-file:`Python/ast.c`.

  * :cpy-file:`Include/internal/pycore_ast.h`: Declares ``_PyAST_Validate()``
    external (from :cpy-file:`Python/ast.c`).

  * :cpy-file:`Include/internal/pycore_symtable.h`: Header for
    :cpy-file:`Python/symtable.c`.  ``struct symtable`` and ``PySTEntryObject``
    are defined here.

  * :cpy-file:`Include/internal/pycore_parser.h`: Header for the
    corresponding :cpy-file:`Parser/peg_api.c`.

  * :cpy-file:`Include/internal/pycore_pyarena.h`: Header file for the
    corresponding :cpy-file:`Python/pyarena.c`.

* :cpy-file:`Objects/`

  * :cpy-file:`Objects/codeobject.c`: Contains PyCodeObject-related code
    (originally in :cpy-file:`Python/compile.c`).

  * :cpy-file:`Objects/frameobject.c`: Contains the ``frame_setlineno()``
    function which should determine whether it is allowed to make a jump
    between two points in a bytecode.

* :cpy-file:`Lib/`

  * :cpy-file:`Lib/opcode.py`: Master list of bytecode; if this file is
    modified you must modify several other files accordingly

  * :cpy-file:`Lib/importlib/_bootstrap_external.py`: Home of the magic number
    (named ``MAGIC_NUMBER``) for bytecode versioning.


Objects
=======

* :cpy-file:`Objects/locations.md`: Describes the location table
* :cpy-file:`Objects/frame_layout.md`: Describes the frame stack
* :cpy-file:`Objects/object_layout.md`: Descibes object layout for 3.11 and later
* :cpy-file:`Objects/exception_handling_notes.txt`: Exception handling notes


Specializing Adaptive Interpreter
=================================

Adding a specializing, adaptive interpreter to CPython will bring significant
performance improvements. These documents provide more information:

* :pep:`659`: Specializing Adaptive Interpreter
* :cpy-file:`Python/adaptive.md`: Adding or extending a family of adaptive instructions


References
==========

.. [Wang97]  Daniel C. Wang, Andrew W. Appel, Jeff L. Korn, and Chris
   S. Serra.  `The Zephyr Abstract Syntax Description Language.`_
   In Proceedings of the Conference on Domain-Specific Languages, pp.
   213--227, 1997.

.. _The Zephyr Abstract Syntax Description Language.:
   https://www.cs.princeton.edu/research/techreps/TR-554-97
