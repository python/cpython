.. _compound:

*******************
Compound statements
*******************

.. index:: pair: compound; statement

Compound statements contain (groups of) other statements; they affect or control
the execution of those other statements in some way.  In general, compound
statements span multiple lines, although in simple incarnations a whole compound
statement may be contained in one line.

The :keyword:`if`, :keyword:`while` and :keyword:`for` statements implement
traditional control flow constructs.  :keyword:`try` specifies exception
handlers and/or cleanup code for a group of statements, while the
:keyword:`with` statement allows the execution of initialization and
finalization code around a block of code.  Function and class definitions are
also syntactically compound statements.

.. index::
   single: clause
   single: suite

A compound statement consists of one or more 'clauses.'  A clause consists of a
header and a 'suite.'  The clause headers of a particular compound statement are
all at the same indentation level. Each clause header begins with a uniquely
identifying keyword and ends with a colon.  A suite is a group of statements
controlled by a clause.  A suite can be one or more semicolon-separated simple
statements on the same line as the header, following the header's colon, or it
can be one or more indented statements on subsequent lines.  Only the latter
form of a suite can contain nested compound statements; the following is illegal,
mostly because it wouldn't be clear to which :keyword:`if` clause a following
:keyword:`else` clause would belong::

   if test1: if test2: print(x)

Also note that the semicolon binds tighter than the colon in this context, so
that in the following example, either all or none of the :func:`print` calls are
executed::

   if x < y < z: print(x); print(y); print(z)

Summarizing:

.. productionlist::
   compound_stmt: `if_stmt`
                : | `while_stmt`
                : | `for_stmt`
                : | `try_stmt`
                : | `with_stmt`
                : | `funcdef`
                : | `classdef`
                : | `async_with_stmt`
                : | `async_for_stmt`
                : | `async_funcdef`
   suite: `stmt_list` NEWLINE | NEWLINE INDENT `statement`+ DEDENT
   statement: `stmt_list` NEWLINE | `compound_stmt`
   stmt_list: `simple_stmt` (";" `simple_stmt`)* [";"]

.. index::
   single: NEWLINE token
   single: DEDENT token
   pair: dangling; else

Note that statements always end in a ``NEWLINE`` possibly followed by a
``DEDENT``.  Also note that optional continuation clauses always begin with a
keyword that cannot start a statement, thus there are no ambiguities (the
'dangling :keyword:`else`' problem is solved in Python by requiring nested
:keyword:`if` statements to be indented).

The formatting of the grammar rules in the following sections places each clause
on a separate line for clarity.


.. _if:
.. _elif:
.. _else:

The :keyword:`if` statement
===========================

.. index::
   statement: if
   keyword: elif
   keyword: else
           keyword: elif
           keyword: else

The :keyword:`if` statement is used for conditional execution:

.. productionlist::
   if_stmt: "if" `expression` ":" `suite`
          : ( "elif" `expression` ":" `suite` )*
          : ["else" ":" `suite`]

It selects exactly one of the suites by evaluating the expressions one by one
until one is found to be true (see section :ref:`booleans` for the definition of
true and false); then that suite is executed (and no other part of the
:keyword:`if` statement is executed or evaluated).  If all expressions are
false, the suite of the :keyword:`else` clause, if present, is executed.


.. _while:

The :keyword:`while` statement
==============================

.. index::
   statement: while
   keyword: else
   pair: loop; statement
   keyword: else

The :keyword:`while` statement is used for repeated execution as long as an
expression is true:

.. productionlist::
   while_stmt: "while" `expression` ":" `suite`
             : ["else" ":" `suite`]

This repeatedly tests the expression and, if it is true, executes the first
suite; if the expression is false (which may be the first time it is tested) the
suite of the :keyword:`else` clause, if present, is executed and the loop
terminates.

.. index::
   statement: break
   statement: continue

A :keyword:`break` statement executed in the first suite terminates the loop
without executing the :keyword:`else` clause's suite.  A :keyword:`continue`
statement executed in the first suite skips the rest of the suite and goes back
to testing the expression.


.. _for:

The :keyword:`for` statement
============================

.. index::
   statement: for
   keyword: in
   keyword: else
   pair: target; list
   pair: loop; statement
   keyword: in
   keyword: else
   pair: target; list
   object: sequence

The :keyword:`for` statement is used to iterate over the elements of a sequence
(such as a string, tuple or list) or other iterable object:

.. productionlist::
   for_stmt: "for" `target_list` "in" `expression_list` ":" `suite`
           : ["else" ":" `suite`]

The expression list is evaluated once; it should yield an iterable object.  An
iterator is created for the result of the ``expression_list``.  The suite is
then executed once for each item provided by the iterator, in the order returned
by the iterator.  Each item in turn is assigned to the target list using the
standard rules for assignments (see :ref:`assignment`), and then the suite is
executed.  When the items are exhausted (which is immediately when the sequence
is empty or an iterator raises a :exc:`StopIteration` exception), the suite in
the :keyword:`else` clause, if present, is executed, and the loop terminates.

.. index::
   statement: break
   statement: continue

A :keyword:`break` statement executed in the first suite terminates the loop
without executing the :keyword:`else` clause's suite.  A :keyword:`continue`
statement executed in the first suite skips the rest of the suite and continues
with the next item, or with the :keyword:`else` clause if there is no next
item.

The for-loop makes assignments to the variables(s) in the target list.
This overwrites all previous assignments to those variables including
those made in the suite of the for-loop::

   for i in range(10):
       print(i)
       i = 5             # this will not affect the for-loop
                         # because i will be overwritten with the next
                         # index in the range


.. index::
   builtin: range

Names in the target list are not deleted when the loop is finished, but if the
sequence is empty, they will not have been assigned to at all by the loop.  Hint:
the built-in function :func:`range` returns an iterator of integers suitable to
emulate the effect of Pascal's ``for i := a to b do``; e.g., ``list(range(3))``
returns the list ``[0, 1, 2]``.

.. note::

   .. index::
      single: loop; over mutable sequence
      single: mutable sequence; loop over

   There is a subtlety when the sequence is being modified by the loop (this can
   only occur for mutable sequences, i.e. lists).  An internal counter is used
   to keep track of which item is used next, and this is incremented on each
   iteration.  When this counter has reached the length of the sequence the loop
   terminates.  This means that if the suite deletes the current (or a previous)
   item from the sequence, the next item will be skipped (since it gets the
   index of the current item which has already been treated).  Likewise, if the
   suite inserts an item in the sequence before the current item, the current
   item will be treated again the next time through the loop. This can lead to
   nasty bugs that can be avoided by making a temporary copy using a slice of
   the whole sequence, e.g., ::

      for x in a[:]:
          if x < 0: a.remove(x)


.. _try:
.. _except:
.. _finally:

The :keyword:`try` statement
============================

.. index::
   statement: try
   keyword: except
   keyword: finally
.. index:: keyword: except

The :keyword:`try` statement specifies exception handlers and/or cleanup code
for a group of statements:

.. productionlist::
   try_stmt: try1_stmt | try2_stmt
   try1_stmt: "try" ":" `suite`
            : ("except" [`expression` ["as" `identifier`]] ":" `suite`)+
            : ["else" ":" `suite`]
            : ["finally" ":" `suite`]
   try2_stmt: "try" ":" `suite`
            : "finally" ":" `suite`


The :keyword:`except` clause(s) specify one or more exception handlers. When no
exception occurs in the :keyword:`try` clause, no exception handler is executed.
When an exception occurs in the :keyword:`try` suite, a search for an exception
handler is started.  This search inspects the except clauses in turn until one
is found that matches the exception.  An expression-less except clause, if
present, must be last; it matches any exception.  For an except clause with an
expression, that expression is evaluated, and the clause matches the exception
if the resulting object is "compatible" with the exception.  An object is
compatible with an exception if it is the class or a base class of the exception
object or a tuple containing an item compatible with the exception.

If no except clause matches the exception, the search for an exception handler
continues in the surrounding code and on the invocation stack.  [#]_

If the evaluation of an expression in the header of an except clause raises an
exception, the original search for a handler is canceled and a search starts for
the new exception in the surrounding code and on the call stack (it is treated
as if the entire :keyword:`try` statement raised the exception).

When a matching except clause is found, the exception is assigned to the target
specified after the :keyword:`as` keyword in that except clause, if present, and
the except clause's suite is executed.  All except clauses must have an
executable block.  When the end of this block is reached, execution continues
normally after the entire try statement.  (This means that if two nested
handlers exist for the same exception, and the exception occurs in the try
clause of the inner handler, the outer handler will not handle the exception.)

When an exception has been assigned using ``as target``, it is cleared at the
end of the except clause.  This is as if ::

   except E as N:
       foo

was translated to ::

   except E as N:
       try:
           foo
       finally:
           del N

This means the exception must be assigned to a different name to be able to
refer to it after the except clause.  Exceptions are cleared because with the
traceback attached to them, they form a reference cycle with the stack frame,
keeping all locals in that frame alive until the next garbage collection occurs.

.. index::
   module: sys
   object: traceback

Before an except clause's suite is executed, details about the exception are
stored in the :mod:`sys` module and can be accessed via :func:`sys.exc_info`.
:func:`sys.exc_info` returns a 3-tuple consisting of the exception class, the
exception instance and a traceback object (see section :ref:`types`) identifying
the point in the program where the exception occurred.  :func:`sys.exc_info`
values are restored to their previous values (before the call) when returning
from a function that handled an exception.

.. index::
   keyword: else
   statement: return
   statement: break
   statement: continue

The optional :keyword:`else` clause is executed if and when control flows off
the end of the :keyword:`try` clause. [#]_ Exceptions in the :keyword:`else`
clause are not handled by the preceding :keyword:`except` clauses.

.. index:: keyword: finally

If :keyword:`finally` is present, it specifies a 'cleanup' handler.  The
:keyword:`try` clause is executed, including any :keyword:`except` and
:keyword:`else` clauses.  If an exception occurs in any of the clauses and is
not handled, the exception is temporarily saved. The :keyword:`finally` clause
is executed.  If there is a saved exception it is re-raised at the end of the
:keyword:`finally` clause.  If the :keyword:`finally` clause raises another
exception, the saved exception is set as the context of the new exception.
If the :keyword:`finally` clause executes a :keyword:`return` or :keyword:`break`
statement, the saved exception is discarded::

   >>> def f():
   ...     try:
   ...         1/0
   ...     finally:
   ...         return 42
   ...
   >>> f()
   42

The exception information is not available to the program during execution of
the :keyword:`finally` clause.

.. index::
   statement: return
   statement: break
   statement: continue

When a :keyword:`return`, :keyword:`break` or :keyword:`continue` statement is
executed in the :keyword:`try` suite of a :keyword:`try`...\ :keyword:`finally`
statement, the :keyword:`finally` clause is also executed 'on the way out.' A
:keyword:`continue` statement is illegal in the :keyword:`finally` clause. (The
reason is a problem with the current implementation --- this restriction may be
lifted in the future).

The return value of a function is determined by the last :keyword:`return`
statement executed.  Since the :keyword:`finally` clause always executes, a
:keyword:`return` statement executed in the :keyword:`finally` clause will
always be the last one executed::

   >>> def foo():
   ...     try:
   ...         return 'try'
   ...     finally:
   ...         return 'finally'
   ...
   >>> foo()
   'finally'

Additional information on exceptions can be found in section :ref:`exceptions`,
and information on using the :keyword:`raise` statement to generate exceptions
may be found in section :ref:`raise`.


.. _with:
.. _as:

The :keyword:`with` statement
=============================

.. index::
    statement: with
    single: as; with statement

The :keyword:`with` statement is used to wrap the execution of a block with
methods defined by a context manager (see section :ref:`context-managers`).
This allows common :keyword:`try`...\ :keyword:`except`...\ :keyword:`finally`
usage patterns to be encapsulated for convenient reuse.

.. productionlist::
   with_stmt: "with" with_item ("," with_item)* ":" `suite`
   with_item: `expression` ["as" `target`]

The execution of the :keyword:`with` statement with one "item" proceeds as follows:

#. The context expression (the expression given in the :token:`with_item`) is
   evaluated to obtain a context manager.

#. The context manager's :meth:`__exit__` is loaded for later use.

#. The context manager's :meth:`__enter__` method is invoked.

#. If a target was included in the :keyword:`with` statement, the return value
   from :meth:`__enter__` is assigned to it.

   .. note::

      The :keyword:`with` statement guarantees that if the :meth:`__enter__`
      method returns without an error, then :meth:`__exit__` will always be
      called. Thus, if an error occurs during the assignment to the target list,
      it will be treated the same as an error occurring within the suite would
      be. See step 6 below.

#. The suite is executed.

#. The context manager's :meth:`__exit__` method is invoked.  If an exception
   caused the suite to be exited, its type, value, and traceback are passed as
   arguments to :meth:`__exit__`. Otherwise, three :const:`None` arguments are
   supplied.

   If the suite was exited due to an exception, and the return value from the
   :meth:`__exit__` method was false, the exception is reraised.  If the return
   value was true, the exception is suppressed, and execution continues with the
   statement following the :keyword:`with` statement.

   If the suite was exited for any reason other than an exception, the return
   value from :meth:`__exit__` is ignored, and execution proceeds at the normal
   location for the kind of exit that was taken.

With more than one item, the context managers are processed as if multiple
:keyword:`with` statements were nested::

   with A() as a, B() as b:
       suite

is equivalent to ::

   with A() as a:
       with B() as b:
           suite

.. versionchanged:: 3.1
   Support for multiple context expressions.

.. seealso::

   :pep:`343` - The "with" statement
      The specification, background, and examples for the Python :keyword:`with`
      statement.


.. index::
   single: parameter; function definition

.. _function:
.. _def:

Function definitions
====================

.. index::
   statement: def
   pair: function; definition
   pair: function; name
   pair: name; binding
   object: user-defined function
   object: function
   pair: function; name
   pair: name; binding

A function definition defines a user-defined function object (see section
:ref:`types`):

.. productionlist::
   funcdef: [`decorators`] "def" `funcname` "(" [`parameter_list`] ")" ["->" `expression`] ":" `suite`
   decorators: `decorator`+
   decorator: "@" `dotted_name` ["(" [`argument_list` [","]] ")"] NEWLINE
   dotted_name: `identifier` ("." `identifier`)*
   parameter_list: `defparameter` ("," `defparameter`)* ["," [`parameter_list_starargs`]]
                 : | `parameter_list_starargs`
   parameter_list_starargs: "*" [`parameter`] ("," `defparameter`)* ["," ["**" `parameter` [","]]]
                         : | "**" `parameter` [","]
   parameter: `identifier` [":" `expression`]
   defparameter: `parameter` ["=" `expression`]
   funcname: `identifier`


A function definition is an executable statement.  Its execution binds the
function name in the current local namespace to a function object (a wrapper
around the executable code for the function).  This function object contains a
reference to the current global namespace as the global namespace to be used
when the function is called.

The function definition does not execute the function body; this gets executed
only when the function is called. [#]_

.. index::
  statement: @

A function definition may be wrapped by one or more :term:`decorator` expressions.
Decorator expressions are evaluated when the function is defined, in the scope
that contains the function definition.  The result must be a callable, which is
invoked with the function object as the only argument. The returned value is
bound to the function name instead of the function object.  Multiple decorators
are applied in nested fashion. For example, the following code ::

   @f1(arg)
   @f2
   def func(): pass

is roughly equivalent to ::

   def func(): pass
   func = f1(arg)(f2(func))

except that the original function is not temporarily bound to the name ``func``.

.. index::
   triple: default; parameter; value
   single: argument; function definition

When one or more :term:`parameters <parameter>` have the form *parameter* ``=``
*expression*, the function is said to have "default parameter values."  For a
parameter with a default value, the corresponding :term:`argument` may be
omitted from a call, in which
case the parameter's default value is substituted.  If a parameter has a default
value, all following parameters up until the "``*``" must also have a default
value --- this is a syntactic restriction that is not expressed by the grammar.

**Default parameter values are evaluated from left to right when the function
definition is executed.** This means that the expression is evaluated once, when
the function is defined, and that the same "pre-computed" value is used for each
call.  This is especially important to understand when a default parameter is a
mutable object, such as a list or a dictionary: if the function modifies the
object (e.g. by appending an item to a list), the default value is in effect
modified.  This is generally not what was intended.  A way around this is to use
``None`` as the default, and explicitly test for it in the body of the function,
e.g.::

   def whats_on_the_telly(penguin=None):
       if penguin is None:
           penguin = []
       penguin.append("property of the zoo")
       return penguin

.. index::
  statement: *
  statement: **

Function call semantics are described in more detail in section :ref:`calls`. A
function call always assigns values to all parameters mentioned in the parameter
list, either from position arguments, from keyword arguments, or from default
values.  If the form "``*identifier``" is present, it is initialized to a tuple
receiving any excess positional parameters, defaulting to the empty tuple.
If the form "``**identifier``" is present, it is initialized to a new
ordered mapping receiving any excess keyword arguments, defaulting to a
new empty mapping of the same type.  Parameters after "``*``" or
"``*identifier``" are keyword-only parameters and may only be passed
used keyword arguments.

.. index:: pair: function; annotations

Parameters may have annotations of the form "``: expression``" following the
parameter name.  Any parameter may have an annotation even those of the form
``*identifier`` or ``**identifier``.  Functions may have "return" annotation of
the form "``-> expression``" after the parameter list.  These annotations can be
any valid Python expression and are evaluated when the function definition is
executed.  Annotations may be evaluated in a different order than they appear in
the source code.  The presence of annotations does not change the semantics of a
function.  The annotation values are available as values of a dictionary keyed
by the parameters' names in the :attr:`__annotations__` attribute of the
function object.

.. index:: pair: lambda; expression

It is also possible to create anonymous functions (functions not bound to a
name), for immediate use in expressions.  This uses lambda expressions, described in
section :ref:`lambda`.  Note that the lambda expression is merely a shorthand for a
simplified function definition; a function defined in a ":keyword:`def`"
statement can be passed around or assigned to another name just like a function
defined by a lambda expression.  The ":keyword:`def`" form is actually more powerful
since it allows the execution of multiple statements and annotations.

**Programmer's note:** Functions are first-class objects.  A "``def``" statement
executed inside a function definition defines a local function that can be
returned or passed around.  Free variables used in the nested function can
access the local variables of the function containing the def.  See section
:ref:`naming` for details.

.. seealso::

   :pep:`3107` - Function Annotations
      The original specification for function annotations.


.. _class:

Class definitions
=================

.. index::
   object: class
   statement: class
   pair: class; definition
   pair: class; name
   pair: name; binding
   pair: execution; frame
   single: inheritance
   single: docstring

A class definition defines a class object (see section :ref:`types`):

.. productionlist::
   classdef: [`decorators`] "class" `classname` [`inheritance`] ":" `suite`
   inheritance: "(" [`argument_list`] ")"
   classname: `identifier`

A class definition is an executable statement.  The inheritance list usually
gives a list of base classes (see :ref:`metaclasses` for more advanced uses), so
each item in the list should evaluate to a class object which allows
subclassing.  Classes without an inheritance list inherit, by default, from the
base class :class:`object`; hence, ::

   class Foo:
       pass

is equivalent to ::

   class Foo(object):
       pass

The class's suite is then executed in a new execution frame (see :ref:`naming`),
using a newly created local namespace and the original global namespace.
(Usually, the suite contains mostly function definitions.)  When the class's
suite finishes execution, its execution frame is discarded but its local
namespace is saved. [#]_ A class object is then created using the inheritance
list for the base classes and the saved local namespace for the attribute
dictionary.  The class name is bound to this class object in the original local
namespace.

The order in which attributes are defined in the class body is preserved
in the new class's ``__dict__``.  Note that this is reliable only right
after the class is created and only for classes that were defined using
the definition syntax.

Class creation can be customized heavily using :ref:`metaclasses <metaclasses>`.

Classes can also be decorated: just like when decorating functions, ::

   @f1(arg)
   @f2
   class Foo: pass

is roughly equivalent to ::

   class Foo: pass
   Foo = f1(arg)(f2(Foo))

The evaluation rules for the decorator expressions are the same as for function
decorators.  The result is then bound to the class name.

**Programmer's note:** Variables defined in the class definition are class
attributes; they are shared by instances.  Instance attributes can be set in a
method with ``self.name = value``.  Both class and instance attributes are
accessible through the notation "``self.name``", and an instance attribute hides
a class attribute with the same name when accessed in this way.  Class
attributes can be used as defaults for instance attributes, but using mutable
values there can lead to unexpected results.  :ref:`Descriptors <descriptors>`
can be used to create instance variables with different implementation details.


.. seealso::

   :pep:`3115` - Metaclasses in Python 3
   :pep:`3129` - Class Decorators


Coroutines
==========

.. versionadded:: 3.5

.. index:: statement: async def
.. _`async def`:

Coroutine function definition
-----------------------------

.. productionlist::
   async_funcdef: [`decorators`] "async" "def" `funcname` "(" [`parameter_list`] ")" ["->" `expression`] ":" `suite`

.. index::
   keyword: async
   keyword: await

Execution of Python coroutines can be suspended and resumed at many points
(see :term:`coroutine`).  In the body of a coroutine, any ``await`` and
``async`` identifiers become reserved keywords; :keyword:`await` expressions,
:keyword:`async for` and :keyword:`async with` can only be used in
coroutine bodies.

Functions defined with ``async def`` syntax are always coroutine functions,
even if they do not contain ``await`` or ``async`` keywords.

It is a :exc:`SyntaxError` to use ``yield from`` expressions in
``async def`` coroutines.

An example of a coroutine function::

    async def func(param1, param2):
        do_stuff()
        await some_coroutine()


.. index:: statement: async for
.. _`async for`:

The :keyword:`async for` statement
----------------------------------

.. productionlist::
   async_for_stmt: "async" `for_stmt`

An :term:`asynchronous iterable` is able to call asynchronous code in its
*iter* implementation, and :term:`asynchronous iterator` can call asynchronous
code in its *next* method.

The ``async for`` statement allows convenient iteration over asynchronous
iterators.

The following code::

    async for TARGET in ITER:
        BLOCK
    else:
        BLOCK2

Is semantically equivalent to::

    iter = (ITER)
    iter = type(iter).__aiter__(iter)
    running = True
    while running:
        try:
            TARGET = await type(iter).__anext__(iter)
        except StopAsyncIteration:
            running = False
        else:
            BLOCK
    else:
        BLOCK2

See also :meth:`__aiter__` and :meth:`__anext__` for details.

It is a :exc:`SyntaxError` to use ``async for`` statement outside of an
:keyword:`async def` function.


.. index:: statement: async with
.. _`async with`:

The :keyword:`async with` statement
-----------------------------------

.. productionlist::
   async_with_stmt: "async" `with_stmt`

An :term:`asynchronous context manager` is a :term:`context manager` that is
able to suspend execution in its *enter* and *exit* methods.

The following code::

    async with EXPR as VAR:
        BLOCK

Is semantically equivalent to::

    mgr = (EXPR)
    aexit = type(mgr).__aexit__
    aenter = type(mgr).__aenter__(mgr)
    exc = True

    VAR = await aenter
    try:
        BLOCK
    except:
        if not await aexit(mgr, *sys.exc_info()):
            raise
    else:
        await aexit(mgr, None, None, None)

See also :meth:`__aenter__` and :meth:`__aexit__` for details.

It is a :exc:`SyntaxError` to use ``async with`` statement outside of an
:keyword:`async def` function.

.. seealso::

   :pep:`492` - Coroutines with async and await syntax


.. rubric:: Footnotes

.. [#] The exception is propagated to the invocation stack unless
   there is a :keyword:`finally` clause which happens to raise another
   exception. That new exception causes the old one to be lost.

.. [#] Currently, control "flows off the end" except in the case of an exception
   or the execution of a :keyword:`return`, :keyword:`continue`, or
   :keyword:`break` statement.

.. [#] A string literal appearing as the first statement in the function body is
   transformed into the function's ``__doc__`` attribute and therefore the
   function's :term:`docstring`.

.. [#] A string literal appearing as the first statement in the class body is
   transformed into the namespace's ``__doc__`` item and therefore the class's
   :term:`docstring`.
