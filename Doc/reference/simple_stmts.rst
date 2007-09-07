
.. _simple:

*****************
Simple statements
*****************

.. index:: pair: simple; statement

Simple statements are comprised within a single logical line. Several simple
statements may occur on a single line separated by semicolons.  The syntax for
simple statements is:

.. productionlist::
   simple_stmt: `expression_stmt`
              : | `assert_stmt`
              : | `assignment_stmt`
              : | `augmented_assignment_stmt`
              : | `pass_stmt`
              : | `del_stmt`
              : | `return_stmt`
              : | `yield_stmt`
              : | `raise_stmt`
              : | `break_stmt`
              : | `continue_stmt`
              : | `import_stmt`
              : | `global_stmt`
              : | `nonlocal_stmt`


.. _exprstmts:

Expression statements
=====================

.. index:: pair: expression; statement
.. index:: pair: expression; list

Expression statements are used (mostly interactively) to compute and write a
value, or (usually) to call a procedure (a function that returns no meaningful
result; in Python, procedures return the value ``None``).  Other uses of
expression statements are allowed and occasionally useful.  The syntax for an
expression statement is:

.. productionlist::
   expression_stmt: `expression_list`

An expression statement evaluates the expression list (which may be a single
expression).

.. index::
   builtin: repr
   object: None
   pair: string; conversion
   single: output
   pair: standard; output
   pair: writing; values
   pair: procedure; call

In interactive mode, if the value is not ``None``, it is converted to a string
using the built-in :func:`repr` function and the resulting string is written to
standard output on a line by itself (except if the result is ``None``, so that
procedure calls do not cause any output.)


.. _assert:

Assert statements
=================

.. index::
   statement: assert
   pair: debugging; assertions
   single: __debug__
   exception: AssertionError

Assert statements are a convenient way to insert debugging assertions into a
program:

.. productionlist::
   assert_stmt: "assert" `expression` ["," `expression`]

The simple form, ``assert expression``, is equivalent to ::

   if __debug__:
      if not expression: raise AssertionError

The extended form, ``assert expression1, expression2``, is equivalent to ::

   if __debug__:
      if not expression1: raise AssertionError(expression2)

These equivalences assume that :data:`__debug__` and :exc:`AssertionError` refer
to the built-in variables with those names.  In the current implementation, the
built-in variable :data:`__debug__` is ``True`` under normal circumstances,
``False`` when optimization is requested (command line option ``-O``).  The
current code generator emits no code for an assert statement when optimization
is requested at compile time.  Note that it is unnecessary to include the source
code for the expression that failed in the error message; it will be displayed
as part of the stack trace.

Assignments to :data:`__debug__` are illegal.  The value for the built-in
variable is determined when the interpreter starts.


.. _assignment:

Assignment statements
=====================

.. index::
   pair: assignment; statement
   pair: binding; name
   pair: rebinding; name
   object: mutable
   pair: attribute; assignment

Assignment statements are used to (re)bind names to values and to modify
attributes or items of mutable objects:

.. productionlist::
   assignment_stmt: (`target_list` "=")+ (`expression_list` | `yield_expression`)
   target_list: `target` ("," `target`)* [","]
   target: `identifier`
         : | "(" `target_list` ")"
         : | "[" `target_list` "]"
         : | `attributeref`
         : | `subscription`
         : | `slicing`
         : | "*" `target`

(See section :ref:`primaries` for the syntax definitions for the last three
symbols.)

An assignment statement evaluates the expression list (remember that this can be
a single expression or a comma-separated list, the latter yielding a tuple) and
assigns the single resulting object to each of the target lists, from left to
right.

.. index::
   single: target
   pair: target; list

Assignment is defined recursively depending on the form of the target (list).
When a target is part of a mutable object (an attribute reference, subscription
or slicing), the mutable object must ultimately perform the assignment and
decide about its validity, and may raise an exception if the assignment is
unacceptable.  The rules observed by various types and the exceptions raised are
given with the definition of the object types (see section :ref:`types`).

.. index:: triple: target; list; assignment

Assignment of an object to a target list, optionally enclosed in parentheses or
square brackets, is recursively defined as follows.

* If the target list is a single target: The object is assigned to that target.

* If the target list is a comma-separated list of targets:

  * If the target list contains one target prefixed with an asterisk, called a
    "starred" target: The object must be a sequence with at least as many items
    as there are targets in the target list, minus one.  The first items of the
    sequence are assigned, from left to right, to the targets before the starred
    target.  The final items of the sequence are assigned to the targets after
    the starred target.  A list of the remaining items in the sequence is then
    assigned to the starred target (the list can be empty).

  * Else: The object must be a sequence with the same number of items as there
    are targets in the target list, and the items are assigned, from left to
    right, to the corresponding targets.

Assignment of an object to a single target is recursively defined as follows.

* If the target is an identifier (name):

  * If the name does not occur in a :keyword:`global` or :keyword:`nonlocal`
    statement in the current code block: the name is bound to the object in the
    current local namespace.

  * Otherwise: the name is bound to the object in the global namespace or the
    outer namespace determined by :keyword:`nonlocal`, respectively.

  The name is rebound if it was already bound.  This may cause the reference
  count for the object previously bound to the name to reach zero, causing the
  object to be deallocated and its destructor (if it has one) to be called.

* If the target is an attribute reference: The primary expression in the
  reference is evaluated.  It should yield an object with assignable attributes;
  if this is not the case, :exc:`TypeError` is raised.  That object is then
  asked to assign the assigned object to the given attribute; if it cannot
  perform the assignment, it raises an exception (usually but not necessarily
  :exc:`AttributeError`).

  .. index::
     pair: subscription; assignment
     object: mutable

* If the target is a subscription: The primary expression in the reference is
  evaluated.  It should yield either a mutable sequence object (such as a list)
  or a mapping object (such as a dictionary).  Next, the subscript expression is
  evaluated.

  .. index::
     object: sequence
     object: list

  If the primary is a mutable sequence object (such as a list), the subscript
  must yield an integer.  If it is negative, the sequence's length is added to
  it.  The resulting value must be a nonnegative integer less than the
  sequence's length, and the sequence is asked to assign the assigned object to
  its item with that index.  If the index is out of range, :exc:`IndexError` is
  raised (assignment to a subscripted sequence cannot add new items to a list).

  .. index::
     object: mapping
     object: dictionary

  If the primary is a mapping object (such as a dictionary), the subscript must
  have a type compatible with the mapping's key type, and the mapping is then
  asked to create a key/datum pair which maps the subscript to the assigned
  object.  This can either replace an existing key/value pair with the same key
  value, or insert a new key/value pair (if no key with the same value existed).

  For user-defined objects, the :meth:`__setitem__` method is called with
  appropriate arguments.

  .. index:: pair: slicing; assignment

* If the target is a slicing: The primary expression in the reference is
  evaluated.  It should yield a mutable sequence object (such as a list).  The
  assigned object should be a sequence object of the same type.  Next, the lower
  and upper bound expressions are evaluated, insofar they are present; defaults
  are zero and the sequence's length.  The bounds should evaluate to integers.
  If either bound is negative, the sequence's length is added to it.  The
  resulting bounds are clipped to lie between zero and the sequence's length,
  inclusive.  Finally, the sequence object is asked to replace the slice with
  the items of the assigned sequence.  The length of the slice may be different
  from the length of the assigned sequence, thus changing the length of the
  target sequence, if the object allows it.

(In the current implementation, the syntax for targets is taken to be the same
as for expressions, and invalid syntax is rejected during the code generation
phase, causing less detailed error messages.)

WARNING: Although the definition of assignment implies that overlaps between the
left-hand side and the right-hand side are 'safe' (for example ``a, b = b, a``
swaps two variables), overlaps *within* the collection of assigned-to variables
are not safe!  For instance, the following program prints ``[0, 2]``::

   x = [0, 1]
   i = 0
   i, x[i] = 1, 2
   print(x)


.. seealso::

   :pep:`3132` - Extended Iterable Unpacking
      The specification for the ``*target`` feature.


.. _augassign:

Augmented assignment statements
-------------------------------

.. index::
   pair: augmented; assignment
   single: statement; assignment, augmented

Augmented assignment is the combination, in a single statement, of a binary
operation and an assignment statement:

.. productionlist::
   augmented_assignment_stmt: `target` `augop` (`expression_list` | `yield_expression`)
   augop: "+=" | "-=" | "*=" | "/=" | "%=" | "**="
        : | ">>=" | "<<=" | "&=" | "^=" | "|="

(See section :ref:`primaries` for the syntax definitions for the last three
symbols.)

An augmented assignment evaluates the target (which, unlike normal assignment
statements, cannot be an unpacking) and the expression list, performs the binary
operation specific to the type of assignment on the two operands, and assigns
the result to the original target.  The target is only evaluated once.

An augmented assignment expression like ``x += 1`` can be rewritten as ``x = x +
1`` to achieve a similar, but not exactly equal effect. In the augmented
version, ``x`` is only evaluated once. Also, when possible, the actual operation
is performed *in-place*, meaning that rather than creating a new object and
assigning that to the target, the old object is modified instead.

With the exception of assigning to tuples and multiple targets in a single
statement, the assignment done by augmented assignment statements is handled the
same way as normal assignments. Similarly, with the exception of the possible
*in-place* behavior, the binary operation performed by augmented assignment is
the same as the normal binary operations.

For targets which are attribute references, the initial value is retrieved with
a :meth:`getattr` and the result is assigned with a :meth:`setattr`.  Notice
that the two methods do not necessarily refer to the same variable.  When
:meth:`getattr` refers to a class variable, :meth:`setattr` still writes to an
instance variable. For example::

   class A:
       x = 3    # class variable
   a = A()
   a.x += 1     # writes a.x as 4 leaving A.x as 3


.. _pass:

The :keyword:`pass` statement
=============================

.. index:: statement: pass
           pair: null; operation

.. productionlist::
   pass_stmt: "pass"

:keyword:`pass` is a null operation --- when it is executed, nothing happens.
It is useful as a placeholder when a statement is required syntactically, but no
code needs to be executed, for example::

   def f(arg): pass    # a function that does nothing (yet)

   class C: pass       # a class with no methods (yet)


.. _del:

The :keyword:`del` statement
============================

.. index:: statement: del
           pair: deletion; target
           triple: deletion; target; list

.. productionlist::
   del_stmt: "del" `target_list`

Deletion is recursively defined very similar to the way assignment is defined.
Rather that spelling it out in full details, here are some hints.

Deletion of a target list recursively deletes each target, from left to right.

.. index::
   statement: global
   pair: unbinding; name

Deletion of a name removes the binding of that name from the local or global
namespace, depending on whether the name occurs in a :keyword:`global` statement
in the same code block.  If the name is unbound, a :exc:`NameError` exception
will be raised.

.. index:: pair: free; variable

It is illegal to delete a name from the local namespace if it occurs as a free
variable in a nested block.

.. index:: pair: attribute; deletion

Deletion of attribute references, subscriptions and slicings is passed to the
primary object involved; deletion of a slicing is in general equivalent to
assignment of an empty slice of the right type (but even this is determined by
the sliced object).


.. _return:

The :keyword:`return` statement
===============================

.. index:: statement: return
           pair: function; definition
           pair: class; definition

.. productionlist::
   return_stmt: "return" [`expression_list`]

:keyword:`return` may only occur syntactically nested in a function definition,
not within a nested class definition.

If an expression list is present, it is evaluated, else ``None`` is substituted.

:keyword:`return` leaves the current function call with the expression list (or
``None``) as return value.

.. index:: keyword: finally

When :keyword:`return` passes control out of a :keyword:`try` statement with a
:keyword:`finally` clause, that :keyword:`finally` clause is executed before
really leaving the function.

In a generator function, the :keyword:`return` statement is not allowed to
include an :token:`expression_list`.  In that context, a bare :keyword:`return`
indicates that the generator is done and will cause :exc:`StopIteration` to be
raised.


.. _yield:

The :keyword:`yield` statement
==============================

.. productionlist::
   yield_stmt: `yield_expression`

The yield statement is nothing but a yield expression used as a statement,
see :ref:`yieldexpr`.


.. _raise:

The :keyword:`raise` statement
==============================

.. index:: statement: raise
           pair: raising; exception

.. productionlist::
   raise_stmt: "raise" [`expression` ["from" `expression`]]

If no expressions are present, :keyword:`raise` re-raises the last exception
that was active in the current scope.  If no exception is active in the current
scope, a :exc:`TypeError` exception is raised indicating that this is an error
(if running under IDLE, a :exc:`Queue.Empty` exception is raised instead).

Otherwise, :keyword:`raise` evaluates the first expression as the exception
object.  It must be either a subclass or an instance of :class:`BaseException`.
If it is a class, the exception instance will be obtained when needed by
instantiating the class with no arguments.

The :dfn:`type` of the exception is the exception instance's class, the
:dfn:`value` is the instance itself.

.. index:: object: traceback

A traceback object is normally created automatically when an exception is raised
and attached to it as the :attr:`__traceback__` attribute; however, you can set
your own traceback using the :meth:`with_traceback` exception method, like so::

   raise RuntimeError("foo occurred").with_traceback(tracebackobj)

.. XXX document exception chaining

The "from" clause is used for exception chaining, which is not documented yet.

Additional information on exceptions can be found in section :ref:`exceptions`,
and information about handling exceptions is in section :ref:`try`.

.. seealso::

   :pep:`3109` - Raising exceptions in Python 3000
      Describes the differences in :keyword:`raise` statements between Python
      2.x and 3.0.


.. _break:

The :keyword:`break` statement
==============================

.. index:: statement: break
           statement: for
           statement: while
           pair: loop; statement

.. productionlist::
   break_stmt: "break"

:keyword:`break` may only occur syntactically nested in a :keyword:`for` or
:keyword:`while` loop, but not nested in a function or class definition within
that loop.

.. index:: keyword: else
           pair: loop control; target

It terminates the nearest enclosing loop, skipping the optional :keyword:`else`
clause if the loop has one.

If a :keyword:`for` loop is terminated by :keyword:`break`, the loop control
target keeps its current value.

.. index:: keyword: finally

When :keyword:`break` passes control out of a :keyword:`try` statement with a
:keyword:`finally` clause, that :keyword:`finally` clause is executed before
really leaving the loop.


.. _continue:

The :keyword:`continue` statement
=================================

.. index:: statement: continue
           statement: for
           statement: while
           pair: loop; statement
           keyword: finally

.. productionlist::
   continue_stmt: "continue"

:keyword:`continue` may only occur syntactically nested in a :keyword:`for` or
:keyword:`while` loop, but not nested in a function or class definition or
:keyword:`finally` statement within that loop. [#]_ It continues with the next
cycle of the nearest enclosing loop.


.. _import:

The :keyword:`import` statement
===============================

.. index::
   statement: import
   single: module; importing
   pair: name; binding
   keyword: from

.. productionlist::
   import_stmt: "import" `module` ["as" `name`] ( "," `module` ["as" `name`] )*
              : | "from" `relative_module` "import" `identifier` ["as" `name`]
              : ( "," `identifier` ["as" `name`] )*
              : | "from" `relative_module` "import" "(" `identifier` ["as" `name`]
              : ( "," `identifier` ["as" `name`] )* [","] ")"
              : | "from" `module` "import" "*"
   module: (`identifier` ".")* `identifier`
   relative_module: "."* `module` | "."+
   name: `identifier`

Import statements are executed in two steps: (1) find a module, and initialize
it if necessary; (2) define a name or names in the local namespace (of the scope
where the :keyword:`import` statement occurs).  The first form (without
:keyword:`from`) repeats these steps for each identifier in the list.  The form
with :keyword:`from` performs step (1) once, and then performs step (2)
repeatedly.

In this context, to "initialize" a built-in or extension module means to call an
initialization function that the module must provide for the purpose (in the
reference implementation, the function's name is obtained by prepending string
"init" to the module's name); to "initialize" a Python-coded module means to
execute the module's body.

.. index::
   single: modules (in module sys)
   single: sys.modules
   pair: module; name
   pair: built-in; module
   pair: user-defined; module
   pair: filename; extension
   triple: module; search; path
   module: sys

The system maintains a table of modules that have been or are being initialized,
indexed by module name.  This table is accessible as ``sys.modules``.  When a
module name is found in this table, step (1) is finished.  If not, a search for
a module definition is started.  When a module is found, it is loaded.  Details
of the module searching and loading process are implementation and platform
specific.  It generally involves searching for a "built-in" module with the
given name and then searching a list of locations given as ``sys.path``.

.. index::
   pair: module; initialization
   exception: ImportError
   single: code block
   exception: SyntaxError

If a built-in module is found, its built-in initialization code is executed and
step (1) is finished.  If no matching file is found, :exc:`ImportError` is
raised. If a file is found, it is parsed, yielding an executable code block.  If
a syntax error occurs, :exc:`SyntaxError` is raised.  Otherwise, an empty module
of the given name is created and inserted in the module table, and then the code
block is executed in the context of this module.  Exceptions during this
execution terminate step (1).

When step (1) finishes without raising an exception, step (2) can begin.

The first form of :keyword:`import` statement binds the module name in the local
namespace to the module object, and then goes on to import the next identifier,
if any.  If the module name is followed by :keyword:`as`, the name following
:keyword:`as` is used as the local name for the module.

.. index::
   pair: name; binding
   exception: ImportError

The :keyword:`from` form does not bind the module name: it goes through the list
of identifiers, looks each one of them up in the module found in step (1), and
binds the name in the local namespace to the object thus found.  As with the
first form of :keyword:`import`, an alternate local name can be supplied by
specifying ":keyword:`as` localname".  If a name is not found,
:exc:`ImportError` is raised.  If the list of identifiers is replaced by a star
(``'*'``), all public names defined in the module are bound in the local
namespace of the :keyword:`import` statement..

.. index:: single: __all__ (optional module attribute)

The *public names* defined by a module are determined by checking the module's
namespace for a variable named ``__all__``; if defined, it must be a sequence of
strings which are names defined or imported by that module.  The names given in
``__all__`` are all considered public and are required to exist.  If ``__all__``
is not defined, the set of public names includes all names found in the module's
namespace which do not begin with an underscore character (``'_'``).
``__all__`` should contain the entire public API. It is intended to avoid
accidentally exporting items that are not part of the API (such as library
modules which were imported and used within the module).

The :keyword:`from` form with ``*`` may only occur in a module scope.  If the
wild card form of import --- ``import *`` --- is used in a function and the
function contains or is a nested block with free variables, the compiler will
raise a :exc:`SyntaxError`.

.. index::
   keyword: from
   triple: hierarchical; module; names
   single: packages
   single: __init__.py

**Hierarchical module names:** when the module names contains one or more dots,
the module search path is carried out differently.  The sequence of identifiers
up to the last dot is used to find a "package"; the final identifier is then
searched inside the package.  A package is generally a subdirectory of a
directory on ``sys.path`` that has a file :file:`__init__.py`. [XXX Can't be
bothered to spell this out right now; see the URL
http://www.python.org/doc/essays/packages.html for more details, also about how
the module search works from inside a package.]

.. index:: builtin: __import__

The built-in function :func:`__import__` is provided to support applications
that determine which modules need to be loaded dynamically; refer to
:ref:`built-in-funcs` for additional information.


.. _future:

Future statements
-----------------

.. index:: pair: future; statement

A :dfn:`future statement` is a directive to the compiler that a particular
module should be compiled using syntax or semantics that will be available in a
specified future release of Python.  The future statement is intended to ease
migration to future versions of Python that introduce incompatible changes to
the language.  It allows use of the new features on a per-module basis before
the release in which the feature becomes standard.

.. productionlist:: *
   future_statement: "from" "__future__" "import" feature ["as" name]
                   : ("," feature ["as" name])*
                   : | "from" "__future__" "import" "(" feature ["as" name]
                   : ("," feature ["as" name])* [","] ")"
   feature: identifier
   name: identifier

A future statement must appear near the top of the module.  The only lines that
can appear before a future statement are:

* the module docstring (if any),
* comments,
* blank lines, and
* other future statements.

.. XXX change this if future is cleaned out

The features recognized by Python 3.0 are ``absolute_import``, ``division``,
``generators``, ``nested_scopes`` and ``with_statement``.  They are all
redundant because they are always enabled, and only kept for backwards
compatibility.

A future statement is recognized and treated specially at compile time: Changes
to the semantics of core constructs are often implemented by generating
different code.  It may even be the case that a new feature introduces new
incompatible syntax (such as a new reserved word), in which case the compiler
may need to parse the module differently.  Such decisions cannot be pushed off
until runtime.

For any given release, the compiler knows which feature names have been defined,
and raises a compile-time error if a future statement contains a feature not
known to it.

The direct runtime semantics are the same as for any import statement: there is
a standard module :mod:`__future__`, described later, and it will be imported in
the usual way at the time the future statement is executed.

The interesting runtime semantics depend on the specific feature enabled by the
future statement.

Note that there is nothing special about the statement::

   import __future__ [as name]

That is not a future statement; it's an ordinary import statement with no
special semantics or syntax restrictions.

Code compiled by calls to the builtin functions :func:`exec` and :func:`compile`
that occur in a module :mod:`M` containing a future statement will, by default,
use the new syntax or semantics associated with the future statement.  This can
be controlled by optional arguments to :func:`compile` --- see the documentation
of that function for details.

A future statement typed at an interactive interpreter prompt will take effect
for the rest of the interpreter session.  If an interpreter is started with the
:option:`-i` option, is passed a script name to execute, and the script includes
a future statement, it will be in effect in the interactive session started
after the script is executed.


.. _global:

The :keyword:`global` statement
===============================

.. index:: statement: global

.. productionlist::
   global_stmt: "global" `identifier` ("," `identifier`)*

.. index:: triple: global; name; binding

The :keyword:`global` statement is a declaration which holds for the entire
current code block.  It means that the listed identifiers are to be interpreted
as globals.  It would be impossible to assign to a global variable without
:keyword:`global`, although free variables may refer to globals without being
declared global.

Names listed in a :keyword:`global` statement must not be used in the same code
block textually preceding that :keyword:`global` statement.

Names listed in a :keyword:`global` statement must not be defined as formal
parameters or in a :keyword:`for` loop control target, :keyword:`class`
definition, function definition, or :keyword:`import` statement.

(The current implementation does not enforce the latter two restrictions, but
programs should not abuse this freedom, as future implementations may enforce
them or silently change the meaning of the program.)

.. index::
   builtin: exec
   builtin: eval
   builtin: compile

**Programmer's note:** the :keyword:`global` is a directive to the parser.  It
applies only to code parsed at the same time as the :keyword:`global` statement.
In particular, a :keyword:`global` statement contained in a string or code
object supplied to the builtin :func:`exec` function does not affect the code
block *containing* the function call, and code contained in such a string is
unaffected by :keyword:`global` statements in the code containing the function
call.  The same applies to the :func:`eval` and :func:`compile` functions.


.. _nonlocal:

The :keyword:`nonlocal` statement
=================================

.. index:: statement: nonlocal

.. productionlist::
   nonlocal_stmt: "nonlocal" `identifier` ("," `identifier`)*

XXX: To be documented.


.. rubric:: Footnotes

.. [#] It may occur within an :keyword:`except` or :keyword:`else` clause.  The
   restriction on occurring in the :keyword:`try` clause is implementor's laziness
   and will eventually be lifted.

