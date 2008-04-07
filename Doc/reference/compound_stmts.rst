
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

Compound statements consist of one or more 'clauses.'  A clause consists of a
header and a 'suite.'  The clause headers of a particular compound statement are
all at the same indentation level. Each clause header begins with a uniquely
identifying keyword and ends with a colon.  A suite is a group of statements
controlled by a clause.  A suite can be one or more semicolon-separated simple
statements on the same line as the header, following the header's colon, or it
can be one or more indented statements on subsequent lines.  Only the latter
form of suite can contain nested compound statements; the following is illegal,
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
                : | `decorated`
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
then executed once for each item provided by the iterator, in the order of
ascending indices.  Each item in turn is assigned to the target list using the
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
with the next item, or with the :keyword:`else` clause if there was no next
item.

The suite may assign to the variable(s) in the target list; this does not affect
the next item assigned to it.

.. index::
   builtin: range

Names in the target list are not deleted when the loop is finished, but if the
sequence is empty, it will not have been assigned to at all by the loop.  Hint:
the built-in function :func:`range` returns an iterator of integers suitable to
emulate the effect of Pascal's ``for i := a to b do``; e.g., ``range(3)``
returns the list ``[0, 1, 2]``.

.. warning::

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
            : ("except" [`expression` ["as" `target`]] ":" `suite`)+
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
           N = None
           del N

That means that you have to assign the exception to a different name if you want
to be able to refer to it after the except clause.  The reason for this is that
with the traceback attached to them, exceptions will form a reference cycle with
the stack frame, keeping all locals in that frame alive until the next garbage
collection occurs.

.. index::
   module: sys
   object: traceback

Before an except clause's suite is executed, details about the exception are
stored in the :mod:`sys` module and can be access via :func:`sys.exc_info`.
:func:`sys.exc_info` returns a 3-tuple consisting of: ``exc_type``, the
exception class; ``exc_value``, the exception instance; ``exc_traceback``, a
traceback object (see section :ref:`types`) identifying the point in the program
where the exception occurred. :func:`sys.exc_info` values are restored to their
previous values (before the call) when returning from a function that handled an
exception.

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
is executed.  If there is a saved exception, it is re-raised at the end of the
:keyword:`finally` clause. If the :keyword:`finally` clause raises another
exception or executes a :keyword:`return` or :keyword:`break` statement, the
saved exception is lost.  The exception information is not available to the
program during execution of the :keyword:`finally` clause.

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

Additional information on exceptions can be found in section :ref:`exceptions`,
and information on using the :keyword:`raise` statement to generate exceptions
may be found in section :ref:`raise`.

.. seealso::

   :pep:`3110` - Catching exceptions in Python 3000
      Describes the differences in :keyword:`try` statements between Python 2.x
      and 3.0.


.. _with:
.. _as:

The :keyword:`with` statement
=============================

.. index:: statement: with

The :keyword:`with` statement is used to wrap the execution of a block with
methods defined by a context manager (see section :ref:`context-managers`).
This allows common :keyword:`try`...\ :keyword:`except`...\ :keyword:`finally`
usage patterns to be encapsulated for convenient reuse.

.. productionlist::
   with_stmt: "with" `expression` ["as" `target`] ":" `suite`

The execution of the :keyword:`with` statement proceeds as follows:

#. The context expression is evaluated to obtain a context manager.

#. The context manager's :meth:`__enter__` method is invoked.

#. If a target was included in the :keyword:`with` statement, the return value
   from :meth:`__enter__` is assigned to it.

   .. note::

      The :keyword:`with` statement guarantees that if the :meth:`__enter__`
      method returns without an error, then :meth:`__exit__` will always be
      called.  Thus, if an error occurs during the assignment to the target
      list, it will be treated the same as an error occurring within the suite
      would be.  See step 5 below.

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


   In Python 2.5, the :keyword:`with` statement is only allowed when the
   ``with_statement`` feature has been enabled.  It is always enabled in
   Python 2.6.

.. seealso::

   :pep:`0343` - The "with" statement
      The specification, background, and examples for the Python :keyword:`with`
      statement.


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
   funcdef: [`decorators`] "def" `funcname` "(" [`parameter_list`] ")" ["->" `expression`]? ":" `suite`
   decorators: `decorator`+
   decorator: "@" `dotted_name` ["(" [`argument_list` [","]] ")"] NEWLINE
   funcdef: "def" `funcname` "(" [`parameter_list`] ")" ":" `suite`
   dotted_name: `identifier` ("." `identifier`)*
   parameter_list: (`defparameter` ",")*
                 : (  "*" [`parameter`] ("," `defparameter`)*
                 : [, "**" `parameter`]
                 : | "**" `parameter`
                 : | `defparameter` [","] )
   parameter: `identifier` [":" `expression`]
   defparameter: `parameter` ["=" `expression`]
   funcname: `identifier`


A function definition is an executable statement.  Its execution binds the
function name in the current local namespace to a function object (a wrapper
around the executable code for the function).  This function object contains a
reference to the current global namespace as the global namespace to be used
when the function is called.

The function definition does not execute the function body; this gets executed
only when the function is called.

A function definition may be wrapped by one or more :term:`decorator` expressions.
Decorator expressions are evaluated when the function is defined, in the scope
that contains the function definition.  The result must be a callable, which is
invoked with the function object as the only argument. The returned value is
bound to the function name instead of the function object.  Multiple decorators
are applied in nested fashion. For example, the following code ::

   @f1(arg)
   @f2
   def func(): pass

is equivalent to ::

   def func(): pass
   func = f1(arg)(f2(func))

.. index:: triple: default; parameter; value

When one or more parameters have the form *parameter* ``=`` *expression*, the
function is said to have "default parameter values."  For a parameter with a
default value, the corresponding argument may be omitted from a call, in which
case the parameter's default value is substituted.  If a parameter has a default
value, all following parameters up until the "``*``" must also have a default
value --- this is a syntactic restriction that is not expressed by the grammar.

**Default parameter values are evaluated when the function definition is
executed.** This means that the expression is evaluated once, when the function
is defined, and that that same "pre-computed" value is used for each call.  This
is especially important to understand when a default parameter is a mutable
object, such as a list or a dictionary: if the function modifies the object
(e.g. by appending an item to a list), the default value is in effect modified.
This is generally not what was intended.  A way around this is to use ``None``
as the default, and explicitly test for it in the body of the function, e.g.::

   def whats_on_the_telly(penguin=None):
       if penguin is None:
           penguin = []
       penguin.append("property of the zoo")
       return penguin

Function call semantics are described in more detail in section :ref:`calls`.  A
function call always assigns values to all parameters mentioned in the parameter
list, either from position arguments, from keyword arguments, or from default
values.  If the form "``*identifier``" is present, it is initialized to a tuple
receiving any excess positional parameters, defaulting to the empty tuple.  If
the form "``**identifier``" is present, it is initialized to a new dictionary
receiving any excess keyword arguments, defaulting to a new empty dictionary.
Parameters after "``*``" or "``*identifier``" are keyword-only parameters and
may only be passed used keyword arguments.

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

.. index:: pair: lambda; form

It is also possible to create anonymous functions (functions not bound to a
name), for immediate use in expressions.  This uses lambda forms, described in
section :ref:`lambda`.  Note that the lambda form is merely a shorthand for a
simplified function definition; a function defined in a ":keyword:`def`"
statement can be passed around or assigned to another name just like a function
defined by a lambda form.  The ":keyword:`def`" form is actually more powerful
since it allows the execution of multiple statements and annotations.

**Programmer's note:** Functions are first-class objects.  A "``def``" form
executed inside a function definition defines a local function that can be
returned or passed around.  Free variables used in the nested function can
access the local variables of the function containing the def.  See section
:ref:`naming` for details.


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

A class definition defines a class object (see section :ref:`types`):

.. XXX need to document PEP 3115 changes here (new metaclasses)

.. productionlist::
   classdef: [`decorators`] "class" `classname` [`inheritance`] ":" `suite`
   inheritance: "(" [`expression_list`] ")"
   classname: `identifier`


A class definition is an executable statement.  It first evaluates the
inheritance list, if present.  Each item in the inheritance list should evaluate
to a class object or class type which allows subclassing.  The class's suite is
then executed in a new execution frame (see section :ref:`naming`), using a
newly created local namespace and the original global namespace. (Usually, the
suite contains only function definitions.)  When the class's suite finishes
execution, its execution frame is discarded but its local namespace is saved.  A
class object is then created using the inheritance list for the base classes and
the saved local namespace for the attribute dictionary.  The class name is bound
to this class object in the original local namespace.

Classes can also be decorated; as with functions, ::

   @f1(arg)
   @f2
   class Foo: pass

is equivalent to ::

   class Foo: pass
   Foo = f1(arg)(f2(Foo))

**Programmer's note:** Variables defined in the class definition are class
can be set in a method with ``self.name = value``.  Both class and instance
variables are accessible through the notation "``self.name``", and an instance
variable hides a class variable with the same name when accessed in this way.
Class variables can be used as defaults for instance variables, but using
mutable values there can lead to unexpected results.  Descriptors can be used
to create instance variables with different implementation details.

.. XXX add link to descriptor docs above

.. seealso::

   :pep:`3129` - Class Decorators

Class definitions, like function definitions, may be wrapped by one or
more :term:`decorator` expressions.  The evaluation rules for the
decorator expressions are the same as for functions.  The result must
be a class object, which is then bound to the class name.


.. rubric:: Footnotes

.. [#] The exception is propagated to the invocation stack only if there is no
   :keyword:`finally` clause that negates the exception.

.. [#] Currently, control "flows off the end" except in the case of an exception or the
   execution of a :keyword:`return`, :keyword:`continue`, or :keyword:`break`
   statement.
