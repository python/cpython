
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
interpreter command line with the '**-c**' option) is a code block.  The string
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

.. index:: statement: from

The following constructs bind names: formal parameters to functions,
:keyword:`import` statements, class and function definitions (these bind the
class or function name in the defining block), and targets that are identifiers
if occurring in an assignment, :keyword:`for` loop header, or after
:keyword:`as` in a :keyword:`with` statement or :keyword:`except` clause.
The :keyword:`import` statement
of the form ``from ... import *`` binds all names defined in the imported
module, except those beginning with an underscore.  This form may only be used
at the module level.

A target occurring in a :keyword:`del` statement is also considered bound for
this purpose (though the actual semantics are to unbind the name).

Each assignment or import statement occurs within a block defined by a class or
function definition or at the module level (the top-level code block).

.. index:: pair: free; variable

If a name is bound in a block, it is a local variable of that block, unless
declared as :keyword:`nonlocal` or :keyword:`global`.  If a name is bound at
the module level, it is a global variable.  (The variables of the module code
block are local and global.)  If a variable is used in a code block but not
defined there, it is a :dfn:`free variable`.

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

If the :keyword:`global` statement occurs within a block, all uses of the name
specified in the statement refer to the binding of that name in the top-level
namespace.  Names are resolved in the top-level namespace by searching the
global namespace, i.e. the namespace of the module containing the code block,
and the builtins namespace, the namespace of the module :mod:`builtins`.  The
global namespace is searched first.  If the name is not found there, the
builtins namespace is searched.  The :keyword:`global` statement must precede
all uses of the name.

The :keyword:`global` statement has the same scope as a name binding operation
in the same block.  If the nearest enclosing scope for a free variable contains
a global statement, the free variable is treated as a global.

.. XXX say more about "nonlocal" semantics here

The :keyword:`nonlocal` statement causes corresponding names to refer
to previously bound variables in the nearest enclosing function scope.
:exc:`SyntaxError` is raised at compile time if the given name does not
exist in any enclosing function scope.

.. index:: module: __main__

The namespace for a module is automatically created the first time a module is
imported.  The main module for a script is always called :mod:`__main__`.

Class definition blocks and arguments to :func:`exec` and :func:`eval` are
special in the context of name resolution.
A class definition is an executable statement that may use and define names.
These references follow the normal rules for name resolution with an exception
that unbound local variables are looked up in the global namespace.
The namespace of the class definition becomes the attribute dictionary of
the class. The scope of names defined in a class block is limited to the
class block; it does not extend to the code blocks of methods -- this includes
comprehensions and generator expressions since they are implemented using a
function scope.  This means that the following will fail::

   class A:
       a = 42
       b = list(a + i for i in range(10))

.. _restrict_exec:

Builtins and restricted execution
---------------------------------

.. index:: pair: restricted; execution

The builtins namespace associated with the execution of a code block is actually
found by looking up the name ``__builtins__`` in its global namespace; this
should be a dictionary or a module (in the latter case the module's dictionary
is used).  By default, when in the :mod:`__main__` module, ``__builtins__`` is
the built-in module :mod:`builtins`; when in any other module,
``__builtins__`` is an alias for the dictionary of the :mod:`builtins` module
itself.  ``__builtins__`` can be set to a user-created dictionary to create a
weak form of restricted execution.

.. impl-detail::

   Users should not touch ``__builtins__``; it is strictly an implementation
   detail.  Users wanting to override values in the builtins namespace should
   :keyword:`import` the :mod:`builtins` module and modify its
   attributes appropriately.

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

There are several cases where Python statements are illegal when used in
conjunction with nested scopes that contain free variables.

If a variable is referenced in an enclosing scope, it is illegal to delete the
name.  An error will be reported at compile time.

.. XXX from * also invalid with relative imports (at least currently)

The :func:`eval` and :func:`exec` functions do not have access to the full
environment for resolving names.  Names may be resolved in the local and global
namespaces of the caller.  Free variables are not resolved in the nearest
enclosing namespace, but in the global namespace.  [#]_ The :func:`exec` and
:func:`eval` functions have optional arguments to override the global and local
namespace.  If only one namespace is specified, it is used for both.


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
a stack backtrace, except when the exception is :exc:`SystemExit`.

Exceptions are identified by class instances.  The :keyword:`except` clause is
selected depending on the class of the instance: it must reference the class of
the instance or a base class thereof.  The instance can be received by the
handler and can carry additional information about the exceptional condition.

.. note::

   Exception messages are not part of the Python API.  Their contents may change
   from one version of Python to the next without warning and should not be
   relied on by code which will run under multiple versions of the interpreter.

See also the description of the :keyword:`try` statement in section :ref:`try`
and :keyword:`raise` statement in section :ref:`raise`.


.. rubric:: Footnotes

.. [#] This limitation occurs because the code that is executed by these operations
       is not available at the time the module is compiled.
