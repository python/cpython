
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
              : | `print_stmt`
              : | `return_stmt`
              : | `yield_stmt`
              : | `raise_stmt`
              : | `break_stmt`
              : | `continue_stmt`
              : | `import_stmt`
              : | `global_stmt`
              : | `exec_stmt`


.. _exprstmts:

Expression statements
=====================

.. index::
   pair: expression; statement
   pair: expression; list

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
standard output (see section :ref:`print`) on a line by itself.  (Expression
statements yielding ``None`` are not written, so that procedure calls do not
cause any output.)


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

(See section :ref:`primaries` for the syntax definitions for the last three
symbols.)

.. index:: pair: expression; list

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

Assignment of an object to a target list is recursively defined as follows.

* If the target list is a single target: The object is assigned to that target.

* If the target list is a comma-separated list of targets: The object must be an
  iterable with the same number of items as there are targets in the target list,
  and the items are assigned, from left to right, to the corresponding targets.

Assignment of an object to a single target is recursively defined as follows.

* If the target is an identifier (name):

    .. index:: statement: global

  * If the name does not occur in a :keyword:`global` statement in the current
    code block: the name is bound to the object in the current local namespace.

  * Otherwise: the name is bound to the object in the current global namespace.

  .. index:: single: destructor

  The name is rebound if it was already bound.  This may cause the reference count
  for the object previously bound to the name to reach zero, causing the object to
  be deallocated and its destructor (if it has one) to be called.

* If the target is a target list enclosed in parentheses or in square brackets:
  The object must be an iterable with the same number of items as there are
  targets in the target list, and its items are assigned, from left to right,
  to the corresponding targets.

  .. index:: pair: attribute; assignment

* If the target is an attribute reference: The primary expression in the
  reference is evaluated.  It should yield an object with assignable attributes;
  if this is not the case, :exc:`TypeError` is raised.  That object is then
  asked to assign the assigned object to the given attribute; if it cannot
  perform the assignment, it raises an exception (usually but not necessarily
  :exc:`AttributeError`).

  .. _attr-target-note:

  Note: If the object is a class instance and the attribute reference occurs on
  both sides of the assignment operator, the RHS expression, ``a.x`` can access
  either an instance attribute or (if no instance attribute exists) a class
  attribute.  The LHS target ``a.x`` is always set as an instance attribute,
  creating it if necessary.  Thus, the two occurrences of ``a.x`` do not
  necessarily refer to the same attribute: if the RHS expression refers to a
  class attribute, the LHS creates a new instance attribute as the target of the
  assignment::

     class Cls:
         x = 3             # class variable
     inst = Cls()
     inst.x = inst.x + 1   # writes inst.x as 4 leaving Cls.x as 3

  This description does not necessarily apply to descriptor attributes, such as
  properties created with :func:`property`.

  .. index::
     pair: subscription; assignment
     object: mutable

* If the target is a subscription: The primary expression in the reference is
  evaluated.  It should yield either a mutable sequence object (such as a list) or
  a mapping object (such as a dictionary). Next, the subscript expression is
  evaluated.

  .. index::
     object: sequence
     object: list

  If the primary is a mutable sequence object (such as a list), the subscript must
  yield a plain integer.  If it is negative, the sequence's length is added to it.
  The resulting value must be a nonnegative integer less than the sequence's
  length, and the sequence is asked to assign the assigned object to its item with
  that index.  If the index is out of range, :exc:`IndexError` is raised
  (assignment to a subscripted sequence cannot add new items to a list).

  .. index::
     object: mapping
     object: dictionary

  If the primary is a mapping object (such as a dictionary), the subscript must
  have a type compatible with the mapping's key type, and the mapping is then
  asked to create a key/datum pair which maps the subscript to the assigned
  object.  This can either replace an existing key/value pair with the same key
  value, or insert a new key/value pair (if no key with the same value existed).

  .. index:: pair: slicing; assignment

* If the target is a slicing: The primary expression in the reference is
  evaluated.  It should yield a mutable sequence object (such as a list).  The
  assigned object should be a sequence object of the same type.  Next, the lower
  and upper bound expressions are evaluated, insofar they are present; defaults
  are zero and the sequence's length.  The bounds should evaluate to (small)
  integers.  If either bound is negative, the sequence's length is added to it.
  The resulting bounds are clipped to lie between zero and the sequence's length,
  inclusive.  Finally, the sequence object is asked to replace the slice with the
  items of the assigned sequence.  The length of the slice may be different from
  the length of the assigned sequence, thus changing the length of the target
  sequence, if the object allows it.

.. impl-detail::

   In the current implementation, the syntax for targets is taken to be the same
   as for expressions, and invalid syntax is rejected during the code generation
   phase, causing less detailed error messages.

WARNING: Although the definition of assignment implies that overlaps between the
left-hand side and the right-hand side are 'safe' (for example ``a, b = b, a``
swaps two variables), overlaps *within* the collection of assigned-to variables
are not safe!  For instance, the following program prints ``[0, 2]``::

   x = [0, 1]
   i = 0
   i, x[i] = 1, 2
   print x


.. _augassign:

Augmented assignment statements
-------------------------------

.. index::
   pair: augmented; assignment
   single: statement; assignment, augmented

Augmented assignment is the combination, in a single statement, of a binary
operation and an assignment statement:

.. productionlist::
   augmented_assignment_stmt: `augtarget` `augop` (`expression_list` | `yield_expression`)
   augtarget: `identifier` | `attributeref` | `subscription` | `slicing`
   augop: "+=" | "-=" | "*=" | "/=" | "//=" | "%=" | "**="
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

For targets which are attribute references, the same :ref:`caveat about class
and instance attributes <attr-target-note>` applies as for regular assignments.


.. _assert:

The :keyword:`assert` statement
===============================

.. index::
   statement: assert
   pair: debugging; assertions

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

.. index::
   single: __debug__
   exception: AssertionError

These equivalences assume that :const:`__debug__` and :exc:`AssertionError` refer to
the built-in variables with those names.  In the current implementation, the
built-in variable :const:`__debug__` is ``True`` under normal circumstances,
``False`` when optimization is requested (command line option -O).  The current
code generator emits no code for an assert statement when optimization is
requested at compile time.  Note that it is unnecessary to include the source
code for the expression that failed in the error message; it will be displayed
as part of the stack trace.

Assignments to :const:`__debug__` are illegal.  The value for the built-in variable
is determined when the interpreter starts.


.. _pass:

The :keyword:`pass` statement
=============================

.. index::
   statement: pass
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

.. index::
   statement: del
   pair: deletion; target
   triple: deletion; target; list

.. productionlist::
   del_stmt: "del" `target_list`

Deletion is recursively defined very similar to the way assignment is defined.
Rather than spelling it out in full details, here are some hints.

Deletion of a target list recursively deletes each target, from left to right.

.. index::
   statement: global
   pair: unbinding; name

Deletion of a name removes the binding of that name  from the local or global
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


.. _print:

The :keyword:`print` statement
==============================

.. index:: statement: print

.. productionlist::
   print_stmt: "print" ([`expression` ("," `expression`)* [","]]
             : | ">>" `expression` [("," `expression`)+ [","]])

:keyword:`print` evaluates each expression in turn and writes the resulting
object to standard output (see below).  If an object is not a string, it is
first converted to a string using the rules for string conversions.  The
(resulting or original) string is then written.  A space is written before each
object is (converted and) written, unless the output system believes it is
positioned at the beginning of a line.  This is the case (1) when no characters
have yet been written to standard output, (2) when the last character written to
standard output is a whitespace character except ``' '``, or (3) when the last
write operation on standard output was not a :keyword:`print` statement.
(In some cases it may be functional to write an empty string to standard output
for this reason.)

.. note::

   Objects which act like file objects but which are not the built-in file objects
   often do not properly emulate this aspect of the file object's behavior, so it
   is best not to rely on this.

.. index::
   single: output
   pair: writing; values
   pair: trailing; comma
   pair: newline; suppression

A ``'\n'`` character is written at the end, unless the :keyword:`print`
statement ends with a comma.  This is the only action if the statement contains
just the keyword :keyword:`print`.

.. index::
   pair: standard; output
   module: sys
   single: stdout (in module sys)
   exception: RuntimeError

Standard output is defined as the file object named ``stdout`` in the built-in
module :mod:`sys`.  If no such object exists, or if it does not have a
:meth:`write` method, a :exc:`RuntimeError` exception is raised.

.. index:: single: extended print statement

:keyword:`print` also has an extended form, defined by the second portion of the
syntax described above. This form is sometimes referred to as ":keyword:`print`
chevron." In this form, the first expression after the ``>>`` must evaluate to a
"file-like" object, specifically an object that has a :meth:`write` method as
described above.  With this extended form, the subsequent expressions are
printed to this file object.  If the first expression evaluates to ``None``,
then ``sys.stdout`` is used as the file for output.


.. _return:

The :keyword:`return` statement
===============================

.. index::
   statement: return
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

.. index::
   statement: yield
   single: generator; function
   single: generator; iterator
   single: function; generator
   exception: StopIteration

.. productionlist::
   yield_stmt: `yield_expression`

The :keyword:`yield` statement is only used when defining a generator function,
and is only used in the body of the generator function. Using a :keyword:`yield`
statement in a function definition is sufficient to cause that definition to
create a generator function instead of a normal function.

When a generator function is called, it returns an iterator known as a generator
iterator, or more commonly, a generator.  The body of the generator function is
executed by calling the generator's :meth:`next` method repeatedly until it
raises an exception.

When a :keyword:`yield` statement is executed, the state of the generator is
frozen and the value of :token:`expression_list` is returned to :meth:`next`'s
caller.  By "frozen" we mean that all local state is retained, including the
current bindings of local variables, the instruction pointer, and the internal
evaluation stack: enough information is saved so that the next time :meth:`next`
is invoked, the function can proceed exactly as if the :keyword:`yield`
statement were just another external call.

As of Python version 2.5, the :keyword:`yield` statement is now allowed in the
:keyword:`try` clause of a :keyword:`try` ...  :keyword:`finally` construct.  If
the generator is not resumed before it is finalized (by reaching a zero
reference count or by being garbage collected), the generator-iterator's
:meth:`close` method will be called, allowing any pending :keyword:`finally`
clauses to execute.

.. note::

   In Python 2.2, the :keyword:`yield` statement was only allowed when the
   ``generators`` feature has been enabled.  This ``__future__``
   import statement was used to enable the feature::

      from __future__ import generators


.. seealso::

   :pep:`0255` - Simple Generators
      The proposal for adding generators and the :keyword:`yield` statement to Python.

   :pep:`0342` - Coroutines via Enhanced Generators
      The proposal that, among other generator enhancements, proposed allowing
      :keyword:`yield` to appear inside a :keyword:`try` ... :keyword:`finally` block.


.. _raise:

The :keyword:`raise` statement
==============================

.. index::
   statement: raise
   single: exception
   pair: raising; exception

.. productionlist::
   raise_stmt: "raise" [`expression` ["," `expression` ["," `expression`]]]

If no expressions are present, :keyword:`raise` re-raises the last exception
that was active in the current scope.  If no exception is active in the current
scope, a :exc:`TypeError` exception is raised indicating that this is an error
(if running under IDLE, a :exc:`Queue.Empty` exception is raised instead).

Otherwise, :keyword:`raise` evaluates the expressions to get three objects,
using ``None`` as the value of omitted expressions.  The first two objects are
used to determine the *type* and *value* of the exception.

If the first object is an instance, the type of the exception is the class of
the instance, the instance itself is the value, and the second object must be
``None``.

If the first object is a class, it becomes the type of the exception. The second
object is used to determine the exception value: If it is an instance of the
class, the instance becomes the exception value. If the second object is a
tuple, it is used as the argument list for the class constructor; if it is
``None``, an empty argument list is used, and any other object is treated as a
single argument to the constructor.  The instance so created by calling the
constructor is used as the exception value.

.. index:: object: traceback

If a third object is present and not ``None``, it must be a traceback object
(see section :ref:`types`), and it is substituted instead of the current
location as the place where the exception occurred.  If the third object is
present and not a traceback object or ``None``, a :exc:`TypeError` exception is
raised.  The three-expression form of :keyword:`raise` is useful to re-raise an
exception transparently in an except clause, but :keyword:`raise` with no
expressions should be preferred if the exception to be re-raised was the most
recently active exception in the current scope.

Additional information on exceptions can be found in section :ref:`exceptions`,
and information about handling exceptions is in section :ref:`try`.


.. _break:

The :keyword:`break` statement
==============================

.. index::
   statement: break
   statement: for
   statement: while
   pair: loop; statement

.. productionlist::
   break_stmt: "break"

:keyword:`break` may only occur syntactically nested in a :keyword:`for` or
:keyword:`while` loop, but not nested in a function or class definition within
that loop.

.. index:: keyword: else

It terminates the nearest enclosing loop, skipping the optional :keyword:`else`
clause if the loop has one.

.. index:: pair: loop control; target

If a :keyword:`for` loop is terminated by :keyword:`break`, the loop control
target keeps its current value.

.. index:: keyword: finally

When :keyword:`break` passes control out of a :keyword:`try` statement with a
:keyword:`finally` clause, that :keyword:`finally` clause is executed before
really leaving the loop.


.. _continue:

The :keyword:`continue` statement
=================================

.. index::
   statement: continue
   statement: for
   statement: while
   pair: loop; statement
   keyword: finally

.. productionlist::
   continue_stmt: "continue"

:keyword:`continue` may only occur syntactically nested in a :keyword:`for` or
:keyword:`while` loop, but not nested in a function or class definition or
:keyword:`finally` clause within that loop.  It continues with the next
cycle of the nearest enclosing loop.

When :keyword:`continue` passes control out of a :keyword:`try` statement with a
:keyword:`finally` clause, that :keyword:`finally` clause is executed before
really starting the next loop cycle.


.. _import:
.. _from:

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
where the :keyword:`import` statement occurs). The statement comes in two
forms differing on whether it uses the :keyword:`from` keyword. The first form
(without :keyword:`from`) repeats these steps for each identifier in the list.
The form with :keyword:`from` performs step (1) once, and then performs step
(2) repeatedly.

.. index::
    single: package

To understand how step (1) occurs, one must first understand how Python handles
hierarchical naming of modules. To help organize modules and provide a
hierarchy in naming, Python has a concept of packages. A package can contain
other packages and modules while modules cannot contain other modules or
packages. From a file system perspective, packages are directories and modules
are files. The original `specification for packages
<http://www.python.org/doc/essays/packages.html>`_ is still available to read,
although minor details have changed since the writing of that document.

.. index::
    single: sys.modules

Once the name of the module is known (unless otherwise specified, the term
"module" will refer to both packages and modules), searching
for the module or package can begin. The first place checked is
:data:`sys.modules`, the cache of all modules that have been imported
previously. If the module is found there then it is used in step (2) of import.

.. index::
    single: sys.meta_path
    single: finder
    pair: finder; find_module
    single: __path__

If the module is not found in the cache, then :data:`sys.meta_path` is searched
(the specification for :data:`sys.meta_path` can be found in :pep:`302`).
The object is a list of :term:`finder` objects which are queried in order as to
whether they know how to load the module by calling their :meth:`find_module`
method with the name of the module. If the module happens to be contained
within a package (as denoted by the existence of a dot in the name), then a
second argument to :meth:`find_module` is given as the value of the
:attr:`__path__` attribute from the parent package (everything up to the last
dot in the name of the module being imported). If a finder can find the module
it returns a :term:`loader` (discussed later) or returns ``None``.

.. index::
    single: sys.path_hooks
    single: sys.path_importer_cache
    single: sys.path

If none of the finders on :data:`sys.meta_path` are able to find the module
then some implicitly defined finders are queried. Implementations of Python
vary in what implicit meta path finders are defined. The one they all do
define, though, is one that handles :data:`sys.path_hooks`,
:data:`sys.path_importer_cache`, and :data:`sys.path`.

The implicit finder searches for the requested module in the "paths" specified
in one of two places ("paths" do not have to be file system paths). If the
module being imported is supposed to be contained within a package then the
second argument passed to :meth:`find_module`, :attr:`__path__` on the parent
package, is used as the source of paths. If the module is not contained in a
package then :data:`sys.path` is used as the source of paths.

Once the source of paths is chosen it is iterated over to find a finder that
can handle that path. The dict at :data:`sys.path_importer_cache` caches
finders for paths and is checked for a finder. If the path does not have a
finder cached then :data:`sys.path_hooks` is searched by calling each object in
the list with a single argument of the path, returning a finder or raises
:exc:`ImportError`. If a finder is returned then it is cached in
:data:`sys.path_importer_cache` and then used for that path entry. If no finder
can be found but the path exists then a value of ``None`` is
stored in :data:`sys.path_importer_cache` to signify that an implicit,
file-based finder that handles modules stored as individual files should be
used for that path. If the path does not exist then a finder which always
returns `None`` is placed in the cache for the path.

.. index::
    single: loader
    pair: loader; load_module
    exception: ImportError

If no finder can find the module then :exc:`ImportError` is raised. Otherwise
some finder returned a loader whose :meth:`load_module` method is called with
the name of the module to load (see :pep:`302` for the original definition of
loaders). A loader has several responsibilities to perform on a module it
loads. First, if the module already exists in :data:`sys.modules` (a
possibility if the loader is called outside of the import machinery) then it
is to use that module for initialization and not a new module. But if the
module does not exist in :data:`sys.modules` then it is to be added to that
dict before initialization begins. If an error occurs during loading of the
module and it was added to :data:`sys.modules` it is to be removed from the
dict. If an error occurs but the module was already in :data:`sys.modules` it
is left in the dict.

.. index::
    single: __name__
    single: __file__
    single: __path__
    single: __package__
    single: __loader__

The loader must set several attributes on the module. :data:`__name__` is to be
set to the name of the module. :data:`__file__` is to be the "path" to the file
unless the module is built-in (and thus listed in
:data:`sys.builtin_module_names`) in which case the attribute is not set.
If what is being imported is a package then :data:`__path__` is to be set to a
list of paths to be searched when looking for modules and packages contained
within the package being imported. :data:`__package__` is optional but should
be set to the name of package that contains the module or package (the empty
string is used for module not contained in a package). :data:`__loader__` is
also optional but should be set to the loader object that is loading the
module.

.. index::
    exception: ImportError

If an error occurs during loading then the loader raises :exc:`ImportError` if
some other exception is not already being propagated. Otherwise the loader
returns the module that was loaded and initialized.

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
    single: relative; import

When specifying what module to import you do not have to specify the absolute
name of the module. When a module or package is contained within another
package it is possible to make a relative import within the same top package
without having to mention the package name. By using leading dots in the
specified module or package after :keyword:`from` you can specify how high to
traverse up the current package hierarchy without specifying exact names. One
leading dot means the current package where the module making the import
exists. Two dots means up one package level. Three dots is up two levels, etc.
So if you execute ``from . import mod`` from a module in the ``pkg`` package
then you will end up importing ``pkg.mod``. If you execute ``from ..subpkg2
import mod`` from within ``pkg.subpkg1`` you will import ``pkg.subpkg2.mod``.
The specification for relative imports is contained within :pep:`328`.

:func:`importlib.import_module` is provided to support applications that
determine which modules need to be loaded dynamically.


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

The features recognized by Python 2.6 are ``unicode_literals``,
``print_function``, ``absolute_import``, ``division``, ``generators``,
``nested_scopes`` and ``with_statement``.  ``generators``, ``with_statement``,
``nested_scopes`` are redundant in Python version 2.6 and above because they are
always enabled.

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

Code compiled by an :keyword:`exec` statement or calls to the built-in functions
:func:`compile` and :func:`execfile` that occur in a module :mod:`M` containing
a future statement will, by default, use the new  syntax or semantics associated
with the future statement.  This can, starting with Python 2.2 be controlled by
optional arguments to :func:`compile` --- see the documentation of that function
for details.

A future statement typed at an interactive interpreter prompt will take effect
for the rest of the interpreter session.  If an interpreter is started with the
:option:`-i` option, is passed a script name to execute, and the script includes
a future statement, it will be in effect in the interactive session started
after the script is executed.

.. seealso::

   :pep:`236` - Back to the __future__
      The original proposal for the __future__ mechanism.


.. _global:

The :keyword:`global` statement
===============================

.. index::
   statement: global
   triple: global; name; binding

.. productionlist::
   global_stmt: "global" `identifier` ("," `identifier`)*

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

.. impl-detail::

   The current implementation does not enforce the latter two restrictions, but
   programs should not abuse this freedom, as future implementations may enforce
   them or silently change the meaning of the program.

.. index::
   statement: exec
   builtin: eval
   builtin: execfile
   builtin: compile

**Programmer's note:** the :keyword:`global` is a directive to the parser.  It
applies only to code parsed at the same time as the :keyword:`global` statement.
In particular, a :keyword:`global` statement contained in an :keyword:`exec`
statement does not affect the code block *containing* the :keyword:`exec`
statement, and code contained in an :keyword:`exec` statement is unaffected by
:keyword:`global` statements in the code containing the :keyword:`exec`
statement.  The same applies to the :func:`eval`, :func:`execfile` and
:func:`compile` functions.


.. _exec:

The :keyword:`exec` statement
=============================

.. index:: statement: exec

.. productionlist::
   exec_stmt: "exec" `or_expr` ["in" `expression` ["," `expression`]]

This statement supports dynamic execution of Python code.  The first expression
should evaluate to either a string, an open file object, or a code object.  If
it is a string, the string is parsed as a suite of Python statements which is
then executed (unless a syntax error occurs). [#]_  If it is an open file, the file
is parsed until EOF and executed.  If it is a code object, it is simply
executed.  In all cases, the code that's executed is expected to be valid as
file input (see section :ref:`file-input`).  Be aware that the
:keyword:`return` and :keyword:`yield` statements may not be used outside of
function definitions even within the context of code passed to the
:keyword:`exec` statement.

In all cases, if the optional parts are omitted, the code is executed in the
current scope.  If only the first expression after :keyword:`in` is specified,
it should be a dictionary, which will be used for both the global and the local
variables.  If two expressions are given, they are used for the global and local
variables, respectively. If provided, *locals* can be any mapping object.
Remember that at module level, globals and locals are the same dictionary. If
two separate objects are given as *globals* and *locals*, the code will be
executed as if it were embedded in a class definition.

.. versionchanged:: 2.4
   Formerly, *locals* was required to be a dictionary.

.. index::
   single: __builtins__
   module: __builtin__

As a side effect, an implementation may insert additional keys into the
dictionaries given besides those corresponding to variable names set by the
executed code.  For example, the current implementation may add a reference to
the dictionary of the built-in module :mod:`__builtin__` under the key
``__builtins__`` (!).

.. index::
   builtin: eval
   builtin: globals
   builtin: locals

**Programmer's hints:** dynamic evaluation of expressions is supported by the
built-in function :func:`eval`.  The built-in functions :func:`globals` and
:func:`locals` return the current global and local dictionary, respectively,
which may be useful to pass around for use by :keyword:`exec`.


.. rubric:: Footnotes

.. [#] Note that the parser only accepts the Unix-style end of line convention.
       If you are reading the code from a file, make sure to use universal
       newline mode to convert Windows or Mac-style newlines.
