
.. _execmodel:

***************
Execution model
***************

.. index::
   single: execution model
   pair: code; block

.. _prog_structure:

Structure of a program
======================

.. index:: block

A Python program is constructed from code blocks.
A :dfn:`block` is a piece of Python program text that is executed as a unit.
The following are blocks: a module, a function body, and a class definition.
Each command typed interactively is a block.  A script file (a file given as
standard input to the interpreter or specified as a command line argument to the
interpreter) is a code block.  A script command (a command specified on the
interpreter command line with the :option:`-c` option) is a code block.
A module run as a top level script (as module ``__main__``) from the command
line using a :option:`-m` argument is also a code block. The string
argument passed to the built-in functions :func:`eval` and :func:`exec` is a
code block.

.. index:: pair: execution; frame

A code block is executed in an :dfn:`execution frame`.  A frame contains some
administrative information (used for debugging) and determines where and how
execution continues after the code block's execution has completed.

.. _naming:

Naming and binding
==================

.. index::
   single: namespace
   single: scope

.. _bind_names:

Binding of names
----------------

.. index::
   single: name
   pair: binding; name

:dfn:`Names` refer to objects.  Names are introduced by name binding operations.

.. index:: single: from; import statement

The following constructs bind names:

* formal parameters to functions,
* class definitions,
* function definitions,
* assignment expressions,
* :ref:`targets <assignment>` that are identifiers if occurring in
  an assignment:

  + :keyword:`for` loop header,
  + after :keyword:`!as` in a :keyword:`with` statement, :keyword:`except`
    clause, :keyword:`except* <except_star>` clause, or in the as-pattern in structural pattern matching,
  + in a capture pattern in structural pattern matching

* :keyword:`import` statements.
* :keyword:`type` statements.
* :ref:`type parameter lists <type-params>`.

The :keyword:`!import` statement of the form ``from ... import *`` binds all
names defined in the imported module, except those beginning with an underscore.
This form may only be used at the module level.

A target occurring in a :keyword:`del` statement is also considered bound for
this purpose (though the actual semantics are to unbind the name).

Each assignment or import statement occurs within a block defined by a class or
function definition or at the module level (the top-level code block).

.. index:: pair: free; variable

If a name is bound in a block, it is a local variable of that block, unless
declared as :keyword:`nonlocal` or :keyword:`global`.  If a name is bound at
the module level, it is a global variable.  (The variables of the module code
block are local and global.)  If a variable is used in a code block but not
defined there, it is a :term:`free variable`.

Each occurrence of a name in the program text refers to the :dfn:`binding` of
that name established by the following name resolution rules.

.. _resolve_names:

Resolution of names
-------------------

.. index:: scope

A :dfn:`scope` defines the visibility of a name within a block.  If a local
variable is defined in a block, its scope includes that block.  If the
definition occurs in a function block, the scope extends to any blocks contained
within the defining one, unless a contained block introduces a different binding
for the name.

.. index:: single: environment

When a name is used in a code block, it is resolved using the nearest enclosing
scope.  The set of all such scopes visible to a code block is called the block's
:dfn:`environment`.

.. index::
   single: NameError (built-in exception)
   single: UnboundLocalError

When a name is not found at all, a :exc:`NameError` exception is raised.
If the current scope is a function scope, and the name refers to a local
variable that has not yet been bound to a value at the point where the name is
used, an :exc:`UnboundLocalError` exception is raised.
:exc:`UnboundLocalError` is a subclass of :exc:`NameError`.

If a name binding operation occurs anywhere within a code block, all uses of the
name within the block are treated as references to the current block.  This can
lead to errors when a name is used within a block before it is bound.  This rule
is subtle.  Python lacks declarations and allows name binding operations to
occur anywhere within a code block.  The local variables of a code block can be
determined by scanning the entire text of the block for name binding operations.
See :ref:`the FAQ entry on UnboundLocalError <faq-unboundlocalerror>`
for examples.

If the :keyword:`global` statement occurs within a block, all uses of the names
specified in the statement refer to the bindings of those names in the top-level
namespace.  Names are resolved in the top-level namespace by searching the
global namespace, i.e. the namespace of the module containing the code block,
and the builtins namespace, the namespace of the module :mod:`builtins`.  The
global namespace is searched first.  If the names are not found there, the
builtins namespace is searched next. If the names are also not found in the
builtins namespace, new variables are created in the global namespace.
The global statement must precede all uses of the listed names.

The :keyword:`global` statement has the same scope as a name binding operation
in the same block.  If the nearest enclosing scope for a free variable contains
a global statement, the free variable is treated as a global.

.. XXX say more about "nonlocal" semantics here

The :keyword:`nonlocal` statement causes corresponding names to refer
to previously bound variables in the nearest enclosing function scope.
:exc:`SyntaxError` is raised at compile time if the given name does not
exist in any enclosing function scope. :ref:`Type parameters <type-params>`
cannot be rebound with the :keyword:`!nonlocal` statement.

.. index:: pair: module; __main__

The namespace for a module is automatically created the first time a module is
imported.  The main module for a script is always called :mod:`__main__`.

Class definition blocks and arguments to :func:`exec` and :func:`eval` are
special in the context of name resolution.
A class definition is an executable statement that may use and define names.
These references follow the normal rules for name resolution with an exception
that unbound local variables are looked up in the global namespace.
The namespace of the class definition becomes the attribute dictionary of
the class. The scope of names defined in a class block is limited to the
class block; it does not extend to the code blocks of methods. This includes
comprehensions and generator expressions, but it does not include
:ref:`annotation scopes <annotation-scopes>`,
which have access to their enclosing class scopes.
This means that the following will fail::

   class A:
       a = 42
       b = list(a + i for i in range(10))

However, the following will succeed::

   class A:
       type Alias = Nested
       class Nested: pass

   print(A.Alias.__value__)  # <type 'A.Nested'>

.. _annotation-scopes:

Annotation scopes
-----------------

:term:`Annotations <annotation>`, :ref:`type parameter lists <type-params>`
and :keyword:`type` statements
introduce *annotation scopes*, which behave mostly like function scopes,
but with some exceptions discussed below.

Annotation scopes are used in the following contexts:

* :term:`Function annotations <function annotation>`.
* :term:`Variable annotations <variable annotation>`.
* Type parameter lists for :ref:`generic type aliases <generic-type-aliases>`.
* Type parameter lists for :ref:`generic functions <generic-functions>`.
  A generic function's annotations are
  executed within the annotation scope, but its defaults and decorators are not.
* Type parameter lists for :ref:`generic classes <generic-classes>`.
  A generic class's base classes and
  keyword arguments are executed within the annotation scope, but its decorators are not.
* The bounds, constraints, and default values for type parameters
  (:ref:`lazily evaluated <lazy-evaluation>`).
* The value of type aliases (:ref:`lazily evaluated <lazy-evaluation>`).

Annotation scopes differ from function scopes in the following ways:

* Annotation scopes have access to their enclosing class namespace.
  If an annotation scope is immediately within a class scope, or within another
  annotation scope that is immediately within a class scope, the code in the
  annotation scope can use names defined in the class scope as if it were
  executed directly within the class body. This contrasts with regular
  functions defined within classes, which cannot access names defined in the class scope.
* Expressions in annotation scopes cannot contain :keyword:`yield`, ``yield from``,
  :keyword:`await`, or :token:`:= <python-grammar:assignment_expression>`
  expressions. (These expressions are allowed in other scopes contained within the
  annotation scope.)
* Names defined in annotation scopes cannot be rebound with :keyword:`nonlocal`
  statements in inner scopes. This includes only type parameters, as no other
  syntactic elements that can appear within annotation scopes can introduce new names.
* While annotation scopes have an internal name, that name is not reflected in the
  :term:`qualified name` of objects defined within the scope.
  Instead, the :attr:`~definition.__qualname__`
  of such objects is as if the object were defined in the enclosing scope.

.. versionadded:: 3.12
   Annotation scopes were introduced in Python 3.12 as part of :pep:`695`.

.. versionchanged:: 3.13
   Annotation scopes are also used for type parameter defaults, as
   introduced by :pep:`696`.

.. versionchanged:: 3.14
   Annotation scopes are now also used for annotations, as specified in
   :pep:`649` and :pep:`749`.

.. _lazy-evaluation:

Lazy evaluation
---------------

Most annotation scopes are *lazily evaluated*. This includes annotations,
the values of type aliases created through the :keyword:`type` statement, and
the bounds, constraints, and default values of type
variables created through the :ref:`type parameter syntax <type-params>`.
This means that they are not evaluated when the type alias or type variable is
created, or when the object carrying annotations is created. Instead, they
are only evaluated when necessary, for example when the ``__value__``
attribute on a type alias is accessed.

Example:

.. doctest::

   >>> type Alias = 1/0
   >>> Alias.__value__
   Traceback (most recent call last):
     ...
   ZeroDivisionError: division by zero
   >>> def func[T: 1/0](): pass
   >>> T = func.__type_params__[0]
   >>> T.__bound__
   Traceback (most recent call last):
     ...
   ZeroDivisionError: division by zero

Here the exception is raised only when the ``__value__`` attribute
of the type alias or the ``__bound__`` attribute of the type variable
is accessed.

This behavior is primarily useful for references to types that have not
yet been defined when the type alias or type variable is created. For example,
lazy evaluation enables creation of mutually recursive type aliases::

   from typing import Literal

   type SimpleExpr = int | Parenthesized
   type Parenthesized = tuple[Literal["("], Expr, Literal[")"]]
   type Expr = SimpleExpr | tuple[SimpleExpr, Literal["+", "-"], Expr]

Lazily evaluated values are evaluated in :ref:`annotation scope <annotation-scopes>`,
which means that names that appear inside the lazily evaluated value are looked up
as if they were used in the immediately enclosing scope.

.. versionadded:: 3.12

.. _restrict_exec:

Builtins and restricted execution
---------------------------------

.. index:: pair: restricted; execution

.. impl-detail::

   Users should not touch ``__builtins__``; it is strictly an implementation
   detail.  Users wanting to override values in the builtins namespace should
   :keyword:`import` the :mod:`builtins` module and modify its
   attributes appropriately.

The builtins namespace associated with the execution of a code block
is actually found by looking up the name ``__builtins__`` in its
global namespace; this should be a dictionary or a module (in the
latter case the module's dictionary is used).  By default, when in the
:mod:`__main__` module, ``__builtins__`` is the built-in module
:mod:`builtins`; when in any other module, ``__builtins__`` is an
alias for the dictionary of the :mod:`builtins` module itself.


.. _dynamic-features:

Interaction with dynamic features
---------------------------------

Name resolution of free variables occurs at runtime, not at compile time.
This means that the following code will print 42::

   i = 10
   def f():
       print(i)
   i = 42
   f()

.. XXX from * also invalid with relative imports (at least currently)

The :func:`eval` and :func:`exec` functions do not have access to the full
environment for resolving names.  Names may be resolved in the local and global
namespaces of the caller.  Free variables are not resolved in the nearest
enclosing namespace, but in the global namespace.  [#]_ The :func:`exec` and
:func:`eval` functions have optional arguments to override the global and local
namespace.  If only one namespace is specified, it is used for both.

.. XXX(ncoghlan) above is only accurate for string execution. When executing code objects,
   closure cells may now be passed explicitly to resolve co_freevars references.
   Docs issue: https://github.com/python/cpython/issues/122826

.. _exceptions:

Exceptions
==========

.. index:: single: exception

.. index::
   single: raise an exception
   single: handle an exception
   single: exception handler
   single: errors
   single: error handling

Exceptions are a means of breaking out of the normal flow of control of a code
block in order to handle errors or other exceptional conditions.  An exception
is *raised* at the point where the error is detected; it may be *handled* by the
surrounding code block or by any code block that directly or indirectly invoked
the code block where the error occurred.

The Python interpreter raises an exception when it detects a run-time error
(such as division by zero).  A Python program can also explicitly raise an
exception with the :keyword:`raise` statement. Exception handlers are specified
with the :keyword:`try` ... :keyword:`except` statement.  The :keyword:`finally`
clause of such a statement can be used to specify cleanup code which does not
handle the exception, but is executed whether an exception occurred or not in
the preceding code.

.. index:: single: termination model

Python uses the "termination" model of error handling: an exception handler can
find out what happened and continue execution at an outer level, but it cannot
repair the cause of the error and retry the failing operation (except by
re-entering the offending piece of code from the top).

.. index:: single: SystemExit (built-in exception)

When an exception is not handled at all, the interpreter terminates execution of
the program, or returns to its interactive main loop.  In either case, it prints
a stack traceback, except when the exception is :exc:`SystemExit`.

Exceptions are identified by class instances.  The :keyword:`except` clause is
selected depending on the class of the instance: it must reference the class of
the instance or a :term:`non-virtual base class <abstract base class>` thereof.
The instance can be received by the handler and can carry additional information
about the exceptional condition.

.. note::

   Exception messages are not part of the Python API.  Their contents may change
   from one version of Python to the next without warning and should not be
   relied on by code which will run under multiple versions of the interpreter.

See also the description of the :keyword:`try` statement in section :ref:`try`
and :keyword:`raise` statement in section :ref:`raise`.


.. _execcomponents:

Runtime Components
==================

General Computing Model
-----------------------

Python's execution model does not operate in a vacuum.  It runs on
a host machine and through that host's runtime environment, including
its operating system (OS), if there is one.  When a program runs,
the conceptual layers of how it runs on the host look something
like this:

   | **host machine**
   |   **process** (global resources)
   |     **thread** (runs machine code)

Each process represents a program running on the host.  Think of each
process itself as the data part of its program.  Think of the process'
threads as the execution part of the program.  This distinction will
be important to understand the conceptual Python runtime.

The process, as the data part, is the execution context in which the
program runs.  It mostly consists of the set of resources assigned to
the program by the host, including memory, signals, file handles,
sockets, and environment variables.

Processes are isolated and independent from one another.  (The same
is true for hosts.)  The host manages the process' access to its
assigned resources, in addition to coordinating between processes.

Each thread represents the actual execution of the program's machine
code, running relative to the resources assigned to the program's
process.  It's strictly up to the host how and when that execution
takes place.

From the point of view of Python, a program always starts with exactly
one thread.  However, the program may grow to run in multiple
simultaneous threads.  Not all hosts support multiple threads per
process, but most do.  Unlike processes, threads in a process are not
isolated and independent from one another.  Specifically, all threads
in a process share all of the process' resources.

The fundamental point of threads is that each one does *run*
independently, at the same time as the others.  That may be only
conceptually at the same time ("concurrently") or physically
("in parallel").  Either way, the threads effectively run
at a non-synchronized rate.

.. note::

   That non-synchronized rate means none of the process' memory is
   guaranteed to stay consistent for the code running in any given
   thread.  Thus multi-threaded programs must take care to coordinate
   access to intentionally shared resources.  Likewise, they must take
   care to be absolutely diligent about not accessing any *other*
   resources in multiple threads; otherwise two threads running at the
   same time might accidentally interfere with each other's use of some
   shared data.  All this is true for both Python programs and the
   Python runtime.

   The cost of this broad, unstructured requirement is the tradeoff for
   the kind of raw concurrency that threads provide.  The alternative
   to the required discipline generally means dealing with
   non-deterministic bugs and data corruption.

Python Runtime Model
--------------------

The same conceptual layers apply to each Python program, with some
extra data layers specific to Python:

   | **host machine**
   |   **process** (global resources)
   |     Python global runtime (*state*)
   |       Python interpreter (*state*)
   |         **thread** (runs Python bytecode and "C-API")
   |           Python thread *state*

At the conceptual level: when a Python program starts, it looks exactly
like that diagram, with one of each.  The runtime may grow to include
multiple interpreters, and each interpreter may grow to include
multiple thread states.

.. note::

   A Python implementation won't necessarily implement the runtime
   layers distinctly or even concretely.  The only exception is places
   where distinct layers are directly specified or exposed to users,
   like through the :mod:`threading` module.

.. note::

   The initial interpreter is typically called the "main" interpreter.
   Some Python implementations, like CPython, assign special roles
   to the main interpreter.

   Likewise, the host thread where the runtime was initialized is known
   as the "main" thread.  It may be different from the process' initial
   thread, though they are often the same.  In some cases "main thread"
   may be even more specific and refer to the initial thread state.
   A Python runtime might assign specific responsibilities
   to the main thread, such as handling signals.

As a whole, the Python runtime consists of the global runtime state,
interpreters, and thread states.  The runtime ensures all that state
stays consistent over its lifetime, particularly when used with
multiple host threads.

The global runtime, at the conceptual level, is just a set of
interpreters.  While those interpreters are otherwise isolated and
independent from one another, they may share some data or other
resources.  The runtime is responsible for managing these global
resources safely.  The actual nature and management of these resources
is implementation-specific.  Ultimately, the external utility of the
global runtime is limited to managing interpreters.

In contrast, an "interpreter" is conceptually what we would normally
think of as the (full-featured) "Python runtime".  When machine code
executing in a host thread interacts with the Python runtime, it calls
into Python in the context of a specific interpreter.

.. note::

   The term "interpreter" here is not the same as the "bytecode
   interpreter", which is what regularly runs in threads, executing
   compiled Python code.

   In an ideal world, "Python runtime" would refer to what we currently
   call "interpreter".  However, it's been called "interpreter" at least
   since introduced in 1997 (`CPython:a027efa5b`_).

   .. _CPython:a027efa5b: https://github.com/python/cpython/commit/a027efa5b

Each interpreter completely encapsulates all of the non-process-global,
non-thread-specific state needed for the Python runtime to work.
Notably, the interpreter's state persists between uses.  It includes
fundamental data like :data:`sys.modules`.  The runtime ensures
multiple threads using the same interpreter will safely
share it between them.

A Python implementation may support using multiple interpreters at the
same time in the same process.  They are independent and isolated from
one another.  For example, each interpreter has its own
:data:`sys.modules`.

For thread-specific runtime state, each interpreter has a set of thread
states, which it manages, in the same way the global runtime contains
a set of interpreters.  It can have thread states for as many host
threads as it needs.  It may even have multiple thread states for
the same host thread, though that isn't as common.

Each thread state, conceptually, has all the thread-specific runtime
data an interpreter needs to operate in one host thread.  The thread
state includes the current raised exception and the thread's Python
call stack.  It may include other thread-specific resources.

.. note::

   The term "Python thread" can sometimes refer to a thread state, but
   normally it means a thread created using the :mod:`threading` module.

Each thread state, over its lifetime, is always tied to exactly one
interpreter and exactly one host thread.  It will only ever be used in
that thread and with that interpreter.

Multiple thread states may be tied to the same host thread, whether for
different interpreters or even the same interpreter.  However, for any
given host thread, only one of the thread states tied to it can be used
by the thread at a time.

Thread states are isolated and independent from one another and don't
share any data, except for possibly sharing an interpreter and objects
or other resources belonging to that interpreter.

Once a program is running, new Python threads can be created using the
:mod:`threading` module (on platforms and Python implementations that
support threads).  Additional processes can be created using the
:mod:`os`, :mod:`subprocess`, and :mod:`multiprocessing` modules.
Interpreters can be created and used with the
:mod:`~concurrent.interpreters` module.  Coroutines (async) can
be run using :mod:`asyncio` in each interpreter, typically only
in a single thread (often the main thread).


.. rubric:: Footnotes

.. [#] This limitation occurs because the code that is executed by these operations
       is not available at the time the module is compiled.
