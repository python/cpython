
.. _expressions:

***********
Expressions
***********

.. index:: expression, BNF

This chapter explains the meaning of the elements of expressions in Python.

**Syntax Notes:** In this and the following chapters, extended BNF notation will
be used to describe syntax, not lexical analysis.  When (one alternative of) a
syntax rule has the form

.. productionlist:: *
   name: `othername`

and no semantics are given, the semantics of this form of ``name`` are the same
as for ``othername``.


.. _conversions:

Arithmetic conversions
======================

.. index:: pair: arithmetic; conversion

When a description of an arithmetic operator below uses the phrase "the numeric
arguments are converted to a common type," this means that the operator
implementation for built-in types works as follows:

* If either argument is a complex number, the other is converted to complex;

* otherwise, if either argument is a floating point number, the other is
  converted to floating point;

* otherwise, both must be integers and no conversion is necessary.

Some additional rules apply for certain operators (e.g., a string as a left
argument to the '%' operator).  Extensions must define their own conversion
behavior.


.. _atoms:

Atoms
=====

.. index:: atom

Atoms are the most basic elements of expressions.  The simplest atoms are
identifiers or literals.  Forms enclosed in parentheses, brackets or braces are
also categorized syntactically as atoms.  The syntax for atoms is:

.. productionlist::
   atom: `identifier` | `literal` | `enclosure`
   enclosure: `parenth_form` | `list_display` | `dict_display` | `set_display`
            : | `generator_expression` | `yield_atom`


.. _atom-identifiers:

Identifiers (Names)
-------------------

.. index:: name, identifier

An identifier occurring as an atom is a name.  See section :ref:`identifiers`
for lexical definition and section :ref:`naming` for documentation of naming and
binding.

.. index:: exception: NameError

When the name is bound to an object, evaluation of the atom yields that object.
When a name is not bound, an attempt to evaluate it raises a :exc:`NameError`
exception.

.. index::
   pair: name; mangling
   pair: private; names

**Private name mangling:** When an identifier that textually occurs in a class
definition begins with two or more underscore characters and does not end in two
or more underscores, it is considered a :dfn:`private name` of that class.
Private names are transformed to a longer form before code is generated for
them.  The transformation inserts the class name, with leading underscores
removed and a single underscore inserted, in front of the name.  For example,
the identifier ``__spam`` occurring in a class named ``Ham`` will be transformed
to ``_Ham__spam``.  This transformation is independent of the syntactical
context in which the identifier is used.  If the transformed name is extremely
long (longer than 255 characters), implementation defined truncation may happen.
If the class name consists only of underscores, no transformation is done.


.. _atom-literals:

Literals
--------

.. index:: single: literal

Python supports string and bytes literals and various numeric literals:

.. productionlist::
   literal: `stringliteral` | `bytesliteral`
          : | `integer` | `floatnumber` | `imagnumber`

Evaluation of a literal yields an object of the given type (string, bytes,
integer, floating point number, complex number) with the given value.  The value
may be approximated in the case of floating point and imaginary (complex)
literals.  See section :ref:`literals` for details.

.. index::
   triple: immutable; data; type
   pair: immutable; object

All literals correspond to immutable data types, and hence the object's identity
is less important than its value.  Multiple evaluations of literals with the
same value (either the same occurrence in the program text or a different
occurrence) may obtain the same object or a different object with the same
value.


.. _parenthesized:

Parenthesized forms
-------------------

.. index::
   single: parenthesized form
   single: () (parentheses); tuple display

A parenthesized form is an optional expression list enclosed in parentheses:

.. productionlist::
   parenth_form: "(" [`starred_expression`] ")"

A parenthesized expression list yields whatever that expression list yields: if
the list contains at least one comma, it yields a tuple; otherwise, it yields
the single expression that makes up the expression list.

.. index:: pair: empty; tuple

An empty pair of parentheses yields an empty tuple object.  Since tuples are
immutable, the rules for literals apply (i.e., two occurrences of the empty
tuple may or may not yield the same object).

.. index::
   single: comma; tuple display
   pair: tuple; display
   single: , (comma); tuple display

Note that tuples are not formed by the parentheses, but rather by use of the
comma operator.  The exception is the empty tuple, for which parentheses *are*
required --- allowing unparenthesized "nothing" in expressions would cause
ambiguities and allow common typos to pass uncaught.


.. _comprehensions:

Displays for lists, sets and dictionaries
-----------------------------------------

For constructing a list, a set or a dictionary Python provides special syntax
called "displays", each of them in two flavors:

* either the container contents are listed explicitly, or

* they are computed via a set of looping and filtering instructions, called a
  :dfn:`comprehension`.

.. index::
   single: for; in comprehensions
   single: if; in comprehensions
   single: async for; in comprehensions

Common syntax elements for comprehensions are:

.. productionlist::
   comprehension: `expression` `comp_for`
   comp_for: [ASYNC] "for" `target_list` "in" `or_test` [`comp_iter`]
   comp_iter: `comp_for` | `comp_if`
   comp_if: "if" `expression_nocond` [`comp_iter`]

The comprehension consists of a single expression followed by at least one
:keyword:`for` clause and zero or more :keyword:`for` or :keyword:`if` clauses.
In this case, the elements of the new container are those that would be produced
by considering each of the :keyword:`for` or :keyword:`if` clauses a block,
nesting from left to right, and evaluating the expression to produce an element
each time the innermost block is reached.

Note that the comprehension is executed in a separate scope, so names assigned
to in the target list don't "leak" into the enclosing scope.

.. index::
   single: await; in comprehensions

Since Python 3.6, in an :keyword:`async def` function, an :keyword:`async for`
clause may be used to iterate over a :term:`asynchronous iterator`.
A comprehension in an :keyword:`async def` function may consist of either a
:keyword:`for` or :keyword:`async for` clause following the leading
expression, may contain additional :keyword:`for` or :keyword:`async for`
clauses, and may also use :keyword:`await` expressions.
If a comprehension contains either :keyword:`async for` clauses
or :keyword:`await` expressions it is called an
:dfn:`asynchronous comprehension`.  An asynchronous comprehension may
suspend the execution of the coroutine function in which it appears.
See also :pep:`530`.

.. _lists:

List displays
-------------

.. index::
   pair: list; display
   pair: list; comprehensions
   pair: empty; list
   object: list
   single: [] (square brackets); list expression
   single: , (comma); expression list

A list display is a possibly empty series of expressions enclosed in square
brackets:

.. productionlist::
   list_display: "[" [`starred_list` | `comprehension`] "]"

A list display yields a new list object, the contents being specified by either
a list of expressions or a comprehension.  When a comma-separated list of
expressions is supplied, its elements are evaluated from left to right and
placed into the list object in that order.  When a comprehension is supplied,
the list is constructed from the elements resulting from the comprehension.


.. _set:

Set displays
------------

.. index::
   pair: set; display
   object: set
   single: {} (curly brackets); set expression
   single: , (comma); expression list

A set display is denoted by curly braces and distinguishable from dictionary
displays by the lack of colons separating keys and values:

.. productionlist::
   set_display: "{" (`starred_list` | `comprehension`) "}"

A set display yields a new mutable set object, the contents being specified by
either a sequence of expressions or a comprehension.  When a comma-separated
list of expressions is supplied, its elements are evaluated from left to right
and added to the set object.  When a comprehension is supplied, the set is
constructed from the elements resulting from the comprehension.

An empty set cannot be constructed with ``{}``; this literal constructs an empty
dictionary.


.. _dict:

Dictionary displays
-------------------

.. index::
   pair: dictionary; display
   key, datum, key/datum pair
   object: dictionary
   single: {} (curly brackets); dictionary expression
   single: : (colon); in dictionary expressions
   single: , (comma); in dictionary displays

A dictionary display is a possibly empty series of key/datum pairs enclosed in
curly braces:

.. productionlist::
   dict_display: "{" [`key_datum_list` | `dict_comprehension`] "}"
   key_datum_list: `key_datum` ("," `key_datum`)* [","]
   key_datum: `expression` ":" `expression` | "**" `or_expr`
   dict_comprehension: `expression` ":" `expression` `comp_for`

A dictionary display yields a new dictionary object.

If a comma-separated sequence of key/datum pairs is given, they are evaluated
from left to right to define the entries of the dictionary: each key object is
used as a key into the dictionary to store the corresponding datum.  This means
that you can specify the same key multiple times in the key/datum list, and the
final dictionary's value for that key will be the last one given.

.. index::
   unpacking; dictionary
   single: **; in dictionary displays

A double asterisk ``**`` denotes :dfn:`dictionary unpacking`.
Its operand must be a :term:`mapping`.  Each mapping item is added
to the new dictionary.  Later values replace values already set by
earlier key/datum pairs and earlier dictionary unpackings.

.. versionadded:: 3.5
   Unpacking into dictionary displays, originally proposed by :pep:`448`.

A dict comprehension, in contrast to list and set comprehensions, needs two
expressions separated with a colon followed by the usual "for" and "if" clauses.
When the comprehension is run, the resulting key and value elements are inserted
in the new dictionary in the order they are produced.

.. index:: pair: immutable; object
           hashable

Restrictions on the types of the key values are listed earlier in section
:ref:`types`.  (To summarize, the key type should be :term:`hashable`, which excludes
all mutable objects.)  Clashes between duplicate keys are not detected; the last
datum (textually rightmost in the display) stored for a given key value
prevails.


.. _genexpr:

Generator expressions
---------------------

.. index::
   pair: generator; expression
   object: generator
   single: () (parentheses); generator expression

A generator expression is a compact generator notation in parentheses:

.. productionlist::
   generator_expression: "(" `expression` `comp_for` ")"

A generator expression yields a new generator object.  Its syntax is the same as
for comprehensions, except that it is enclosed in parentheses instead of
brackets or curly braces.

Variables used in the generator expression are evaluated lazily when the
:meth:`~generator.__next__` method is called for the generator object (in the same
fashion as normal generators).  However, the leftmost :keyword:`for` clause is
immediately evaluated, so that an error produced by it can be seen before any
other possible error in the code that handles the generator expression.
Subsequent :keyword:`for` clauses cannot be evaluated immediately since they
may depend on the previous :keyword:`for` loop. For example: ``(x*y for x in
range(10) for y in bar(x))``.

The parentheses can be omitted on calls with only one argument.  See section
:ref:`calls` for details.

Since Python 3.6, if the generator appears in an :keyword:`async def` function,
then :keyword:`async for` clauses and :keyword:`await` expressions are permitted
as with an asynchronous comprehension.  If a generator expression
contains either :keyword:`async for` clauses or :keyword:`await` expressions
it is called an :dfn:`asynchronous generator expression`.
An asynchronous generator expression yields a new asynchronous
generator object, which is an asynchronous iterator
(see :ref:`async-iterators`).

.. _yieldexpr:

Yield expressions
-----------------

.. index::
   keyword: yield
   keyword: from
   pair: yield; expression
   pair: generator; function

.. productionlist::
   yield_atom: "(" `yield_expression` ")"
   yield_expression: "yield" [`expression_list` | "from" `expression`]

The yield expression is used when defining a :term:`generator` function
or an :term:`asynchronous generator` function and
thus can only be used in the body of a function definition.  Using a yield
expression in a function's body causes that function to be a generator,
and using it in an :keyword:`async def` function's body causes that
coroutine function to be an asynchronous generator. For example::

    def gen():  # defines a generator function
        yield 123

    async def agen(): # defines an asynchronous generator function (PEP 525)
        yield 123

Generator functions are described below, while asynchronous generator
functions are described separately in section
:ref:`asynchronous-generator-functions`.

When a generator function is called, it returns an iterator known as a
generator.  That generator then controls the execution of the generator function.
The execution starts when one of the generator's methods is called.  At that
time, the execution proceeds to the first yield expression, where it is
suspended again, returning the value of :token:`expression_list` to the generator's
caller.  By suspended, we mean that all local state is retained, including the
current bindings of local variables, the instruction pointer, the internal
evaluation stack, and the state of any exception handling.  When the execution
is resumed by calling one of the
generator's methods, the function can proceed exactly as if the yield expression
were just another external call.  The value of the yield expression after
resuming depends on the method which resumed the execution.  If
:meth:`~generator.__next__` is used (typically via either a :keyword:`for` or
the :func:`next` builtin) then the result is :const:`None`.  Otherwise, if
:meth:`~generator.send` is used, then the result will be the value passed in to
that method.

.. index:: single: coroutine

All of this makes generator functions quite similar to coroutines; they yield
multiple times, they have more than one entry point and their execution can be
suspended.  The only difference is that a generator function cannot control
where the execution should continue after it yields; the control is always
transferred to the generator's caller.

Yield expressions are allowed anywhere in a :keyword:`try` construct.  If the
generator is not resumed before it is
finalized (by reaching a zero reference count or by being garbage collected),
the generator-iterator's :meth:`~generator.close` method will be called,
allowing any pending :keyword:`finally` clauses to execute.

.. index::
   single: from; yield from expression

When ``yield from <expr>`` is used, it treats the supplied expression as
a subiterator. All values produced by that subiterator are passed directly
to the caller of the current generator's methods. Any values passed in with
:meth:`~generator.send` and any exceptions passed in with
:meth:`~generator.throw` are passed to the underlying iterator if it has the
appropriate methods.  If this is not the case, then :meth:`~generator.send`
will raise :exc:`AttributeError` or :exc:`TypeError`, while
:meth:`~generator.throw` will just raise the passed in exception immediately.

When the underlying iterator is complete, the :attr:`~StopIteration.value`
attribute of the raised :exc:`StopIteration` instance becomes the value of
the yield expression. It can be either set explicitly when raising
:exc:`StopIteration`, or automatically when the sub-iterator is a generator
(by returning a value from the sub-generator).

   .. versionchanged:: 3.3
      Added ``yield from <expr>`` to delegate control flow to a subiterator.

The parentheses may be omitted when the yield expression is the sole expression
on the right hand side of an assignment statement.

.. seealso::

   :pep:`255` - Simple Generators
      The proposal for adding generators and the :keyword:`yield` statement to Python.

   :pep:`342` - Coroutines via Enhanced Generators
      The proposal to enhance the API and syntax of generators, making them
      usable as simple coroutines.

   :pep:`380` - Syntax for Delegating to a Subgenerator
      The proposal to introduce the :token:`yield_from` syntax, making delegation
      to sub-generators easy.

.. index:: object: generator
.. _generator-methods:

Generator-iterator methods
^^^^^^^^^^^^^^^^^^^^^^^^^^

This subsection describes the methods of a generator iterator.  They can
be used to control the execution of a generator function.

Note that calling any of the generator methods below when the generator
is already executing raises a :exc:`ValueError` exception.

.. index:: exception: StopIteration


.. method:: generator.__next__()

   Starts the execution of a generator function or resumes it at the last
   executed yield expression.  When a generator function is resumed with a
   :meth:`~generator.__next__` method, the current yield expression always
   evaluates to :const:`None`.  The execution then continues to the next yield
   expression, where the generator is suspended again, and the value of the
   :token:`expression_list` is returned to :meth:`__next__`'s caller.  If the
   generator exits without yielding another value, a :exc:`StopIteration`
   exception is raised.

   This method is normally called implicitly, e.g. by a :keyword:`for` loop, or
   by the built-in :func:`next` function.


.. method:: generator.send(value)

   Resumes the execution and "sends" a value into the generator function.  The
   *value* argument becomes the result of the current yield expression.  The
   :meth:`send` method returns the next value yielded by the generator, or
   raises :exc:`StopIteration` if the generator exits without yielding another
   value.  When :meth:`send` is called to start the generator, it must be called
   with :const:`None` as the argument, because there is no yield expression that
   could receive the value.


.. method:: generator.throw(type[, value[, traceback]])

   Raises an exception of type ``type`` at the point where the generator was paused,
   and returns the next value yielded by the generator function.  If the generator
   exits without yielding another value, a :exc:`StopIteration` exception is
   raised.  If the generator function does not catch the passed-in exception, or
   raises a different exception, then that exception propagates to the caller.

.. index:: exception: GeneratorExit


.. method:: generator.close()

   Raises a :exc:`GeneratorExit` at the point where the generator function was
   paused.  If the generator function then exits gracefully, is already closed,
   or raises :exc:`GeneratorExit` (by not catching the exception), close
   returns to its caller.  If the generator yields a value, a
   :exc:`RuntimeError` is raised.  If the generator raises any other exception,
   it is propagated to the caller.  :meth:`close` does nothing if the generator
   has already exited due to an exception or normal exit.

.. index:: single: yield; examples

Examples
^^^^^^^^

Here is a simple example that demonstrates the behavior of generators and
generator functions::

   >>> def echo(value=None):
   ...     print("Execution starts when 'next()' is called for the first time.")
   ...     try:
   ...         while True:
   ...             try:
   ...                 value = (yield value)
   ...             except Exception as e:
   ...                 value = e
   ...     finally:
   ...         print("Don't forget to clean up when 'close()' is called.")
   ...
   >>> generator = echo(1)
   >>> print(next(generator))
   Execution starts when 'next()' is called for the first time.
   1
   >>> print(next(generator))
   None
   >>> print(generator.send(2))
   2
   >>> generator.throw(TypeError, "spam")
   TypeError('spam',)
   >>> generator.close()
   Don't forget to clean up when 'close()' is called.

For examples using ``yield from``, see :ref:`pep-380` in "What's New in
Python."

.. _asynchronous-generator-functions:

Asynchronous generator functions
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The presence of a yield expression in a function or method defined using
:keyword:`async def` further defines the function as a
:term:`asynchronous generator` function.

When an asynchronous generator function is called, it returns an
asynchronous iterator known as an asynchronous generator object.
That object then controls the execution of the generator function.
An asynchronous generator object is typically used in an
:keyword:`async for` statement in a coroutine function analogously to
how a generator object would be used in a :keyword:`for` statement.

Calling one of the asynchronous generator's methods returns an
:term:`awaitable` object, and the execution starts when this object
is awaited on. At that time, the execution proceeds to the first yield
expression, where it is suspended again, returning the value of
:token:`expression_list` to the awaiting coroutine. As with a generator,
suspension means that all local state is retained, including the
current bindings of local variables, the instruction pointer, the internal
evaluation stack, and the state of any exception handling.  When the execution
is resumed by awaiting on the next object returned by the asynchronous
generator's methods, the function can proceed exactly as if the yield
expression were just another external call. The value of the yield expression
after resuming depends on the method which resumed the execution.  If
:meth:`~agen.__anext__` is used then the result is :const:`None`. Otherwise, if
:meth:`~agen.asend` is used, then the result will be the value passed in to
that method.

In an asynchronous generator function, yield expressions are allowed anywhere
in a :keyword:`try` construct. However, if an asynchronous generator is not
resumed before it is finalized (by reaching a zero reference count or by
being garbage collected), then a yield expression within a :keyword:`try`
construct could result in a failure to execute pending :keyword:`finally`
clauses.  In this case, it is the responsibility of the event loop or
scheduler running the asynchronous generator to call the asynchronous
generator-iterator's :meth:`~agen.aclose` method and run the resulting
coroutine object, thus allowing any pending :keyword:`finally` clauses
to execute.

To take care of finalization, an event loop should define
a *finalizer* function which takes an asynchronous generator-iterator
and presumably calls :meth:`~agen.aclose` and executes the coroutine.
This  *finalizer* may be registered by calling :func:`sys.set_asyncgen_hooks`.
When first iterated over, an asynchronous generator-iterator will store the
registered *finalizer* to be called upon finalization. For a reference example
of a *finalizer* method see the implementation of
``asyncio.Loop.shutdown_asyncgens`` in :source:`Lib/asyncio/base_events.py`.

The expression ``yield from <expr>`` is a syntax error when used in an
asynchronous generator function.

.. index:: object: asynchronous-generator
.. _asynchronous-generator-methods:

Asynchronous generator-iterator methods
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This subsection describes the methods of an asynchronous generator iterator,
which are used to control the execution of a generator function.


.. index:: exception: StopAsyncIteration

.. coroutinemethod:: agen.__anext__()

   Returns an awaitable which when run starts to execute the asynchronous
   generator or resumes it at the last executed yield expression.  When an
   asynchronous generator function is resumed with a :meth:`~agen.__anext__`
   method, the current yield expression always evaluates to :const:`None` in
   the returned awaitable, which when run will continue to the next yield
   expression. The value of the :token:`expression_list` of the yield
   expression is the value of the :exc:`StopIteration` exception raised by
   the completing coroutine.  If the asynchronous generator exits without
   yielding another value, the awaitable instead raises an
   :exc:`StopAsyncIteration` exception, signalling that the asynchronous
   iteration has completed.

   This method is normally called implicitly by a :keyword:`async for` loop.


.. coroutinemethod:: agen.asend(value)

   Returns an awaitable which when run resumes the execution of the
   asynchronous generator. As with the :meth:`~generator.send()` method for a
   generator, this "sends" a value into the asynchronous generator function,
   and the *value* argument becomes the result of the current yield expression.
   The awaitable returned by the :meth:`asend` method will return the next
   value yielded by the generator as the value of the raised
   :exc:`StopIteration`, or raises :exc:`StopAsyncIteration` if the
   asynchronous generator exits without yielding another value.  When
   :meth:`asend` is called to start the asynchronous
   generator, it must be called with :const:`None` as the argument,
   because there is no yield expression that could receive the value.


.. coroutinemethod:: agen.athrow(type[, value[, traceback]])

   Returns an awaitable that raises an exception of type ``type`` at the point
   where the asynchronous generator was paused, and returns the next value
   yielded by the generator function as the value of the raised
   :exc:`StopIteration` exception.  If the asynchronous generator exits
   without yielding another value, an :exc:`StopAsyncIteration` exception is
   raised by the awaitable.
   If the generator function does not catch the passed-in exception, or
   raises a different exception, then when the awaitable is run that exception
   propagates to the caller of the awaitable.

.. index:: exception: GeneratorExit


.. coroutinemethod:: agen.aclose()

   Returns an awaitable that when run will throw a :exc:`GeneratorExit` into
   the asynchronous generator function at the point where it was paused.
   If the asynchronous generator function then exits gracefully, is already
   closed, or raises :exc:`GeneratorExit` (by not catching the exception),
   then the returned awaitable will raise a :exc:`StopIteration` exception.
   Any further awaitables returned by subsequent calls to the asynchronous
   generator will raise a :exc:`StopAsyncIteration` exception.  If the
   asynchronous generator yields a value, a :exc:`RuntimeError` is raised
   by the awaitable.  If the asynchronous generator raises any other exception,
   it is propagated to the caller of the awaitable.  If the asynchronous
   generator has already exited due to an exception or normal exit, then
   further calls to :meth:`aclose` will return an awaitable that does nothing.

.. _primaries:

Primaries
=========

.. index:: single: primary

Primaries represent the most tightly bound operations of the language. Their
syntax is:

.. productionlist::
   primary: `atom` | `attributeref` | `subscription` | `slicing` | `call`


.. _attribute-references:

Attribute references
--------------------

.. index::
   pair: attribute; reference
   single: . (dot); attribute reference

An attribute reference is a primary followed by a period and a name:

.. productionlist::
   attributeref: `primary` "." `identifier`

.. index::
   exception: AttributeError
   object: module
   object: list

The primary must evaluate to an object of a type that supports attribute
references, which most objects do.  This object is then asked to produce the
attribute whose name is the identifier.  This production can be customized by
overriding the :meth:`__getattr__` method.  If this attribute is not available,
the exception :exc:`AttributeError` is raised.  Otherwise, the type and value of
the object produced is determined by the object.  Multiple evaluations of the
same attribute reference may yield different objects.


.. _subscriptions:

Subscriptions
-------------

.. index::
   single: subscription
   single: [] (square brackets); subscription

.. index::
   object: sequence
   object: mapping
   object: string
   object: tuple
   object: list
   object: dictionary
   pair: sequence; item

A subscription selects an item of a sequence (string, tuple or list) or mapping
(dictionary) object:

.. productionlist::
   subscription: `primary` "[" `expression_list` "]"

The primary must evaluate to an object that supports subscription (lists or
dictionaries for example).  User-defined objects can support subscription by
defining a :meth:`__getitem__` method.

For built-in objects, there are two types of objects that support subscription:

If the primary is a mapping, the expression list must evaluate to an object
whose value is one of the keys of the mapping, and the subscription selects the
value in the mapping that corresponds to that key.  (The expression list is a
tuple except if it has exactly one item.)

If the primary is a sequence, the expression list must evaluate to an integer
or a slice (as discussed in the following section).

The formal syntax makes no special provision for negative indices in
sequences; however, built-in sequences all provide a :meth:`__getitem__`
method that interprets negative indices by adding the length of the sequence
to the index (so that ``x[-1]`` selects the last item of ``x``).  The
resulting value must be a nonnegative integer less than the number of items in
the sequence, and the subscription selects the item whose index is that value
(counting from zero). Since the support for negative indices and slicing
occurs in the object's :meth:`__getitem__` method, subclasses overriding
this method will need to explicitly add that support.

.. index::
   single: character
   pair: string; item

A string's items are characters.  A character is not a separate data type but a
string of exactly one character.


.. _slicings:

Slicings
--------

.. index::
   single: slicing
   single: slice
   single: : (colon); slicing
   single: , (comma); slicing

.. index::
   object: sequence
   object: string
   object: tuple
   object: list

A slicing selects a range of items in a sequence object (e.g., a string, tuple
or list).  Slicings may be used as expressions or as targets in assignment or
:keyword:`del` statements.  The syntax for a slicing:

.. productionlist::
   slicing: `primary` "[" `slice_list` "]"
   slice_list: `slice_item` ("," `slice_item`)* [","]
   slice_item: `expression` | `proper_slice`
   proper_slice: [`lower_bound`] ":" [`upper_bound`] [ ":" [`stride`] ]
   lower_bound: `expression`
   upper_bound: `expression`
   stride: `expression`

There is ambiguity in the formal syntax here: anything that looks like an
expression list also looks like a slice list, so any subscription can be
interpreted as a slicing.  Rather than further complicating the syntax, this is
disambiguated by defining that in this case the interpretation as a subscription
takes priority over the interpretation as a slicing (this is the case if the
slice list contains no proper slice).

.. index::
   single: start (slice object attribute)
   single: stop (slice object attribute)
   single: step (slice object attribute)

The semantics for a slicing are as follows.  The primary is indexed (using the
same :meth:`__getitem__` method as
normal subscription) with a key that is constructed from the slice list, as
follows.  If the slice list contains at least one comma, the key is a tuple
containing the conversion of the slice items; otherwise, the conversion of the
lone slice item is the key.  The conversion of a slice item that is an
expression is that expression.  The conversion of a proper slice is a slice
object (see section :ref:`types`) whose :attr:`~slice.start`,
:attr:`~slice.stop` and :attr:`~slice.step` attributes are the values of the
expressions given as lower bound, upper bound and stride, respectively,
substituting ``None`` for missing expressions.


.. index::
   object: callable
   single: call
   single: argument; call semantics
   single: () (parentheses); call
   single: , (comma); argument list
   single: = (equals); in function calls

.. _calls:

Calls
-----

A call calls a callable object (e.g., a :term:`function`) with a possibly empty
series of :term:`arguments <argument>`:

.. productionlist::
   call: `primary` "(" [`argument_list` [","] | `comprehension`] ")"
   argument_list: `positional_arguments` ["," `starred_and_keywords`]
                :   ["," `keywords_arguments`]
                : | `starred_and_keywords` ["," `keywords_arguments`]
                : | `keywords_arguments`
   positional_arguments: ["*"] `expression` ("," ["*"] `expression`)*
   starred_and_keywords: ("*" `expression` | `keyword_item`)
                : ("," "*" `expression` | "," `keyword_item`)*
   keywords_arguments: (`keyword_item` | "**" `expression`)
                : ("," `keyword_item` | "," "**" `expression`)*
   keyword_item: `identifier` "=" `expression`

An optional trailing comma may be present after the positional and keyword arguments
but does not affect the semantics.

.. index::
   single: parameter; call semantics

The primary must evaluate to a callable object (user-defined functions, built-in
functions, methods of built-in objects, class objects, methods of class
instances, and all objects having a :meth:`__call__` method are callable).  All
argument expressions are evaluated before the call is attempted.  Please refer
to section :ref:`function` for the syntax of formal :term:`parameter` lists.

.. XXX update with kwonly args PEP

If keyword arguments are present, they are first converted to positional
arguments, as follows.  First, a list of unfilled slots is created for the
formal parameters.  If there are N positional arguments, they are placed in the
first N slots.  Next, for each keyword argument, the identifier is used to
determine the corresponding slot (if the identifier is the same as the first
formal parameter name, the first slot is used, and so on).  If the slot is
already filled, a :exc:`TypeError` exception is raised. Otherwise, the value of
the argument is placed in the slot, filling it (even if the expression is
``None``, it fills the slot).  When all arguments have been processed, the slots
that are still unfilled are filled with the corresponding default value from the
function definition.  (Default values are calculated, once, when the function is
defined; thus, a mutable object such as a list or dictionary used as default
value will be shared by all calls that don't specify an argument value for the
corresponding slot; this should usually be avoided.)  If there are any unfilled
slots for which no default value is specified, a :exc:`TypeError` exception is
raised.  Otherwise, the list of filled slots is used as the argument list for
the call.

.. impl-detail::

   An implementation may provide built-in functions whose positional parameters
   do not have names, even if they are 'named' for the purpose of documentation,
   and which therefore cannot be supplied by keyword.  In CPython, this is the
   case for functions implemented in C that use :c:func:`PyArg_ParseTuple` to
   parse their arguments.

If there are more positional arguments than there are formal parameter slots, a
:exc:`TypeError` exception is raised, unless a formal parameter using the syntax
``*identifier`` is present; in this case, that formal parameter receives a tuple
containing the excess positional arguments (or an empty tuple if there were no
excess positional arguments).

If any keyword argument does not correspond to a formal parameter name, a
:exc:`TypeError` exception is raised, unless a formal parameter using the syntax
``**identifier`` is present; in this case, that formal parameter receives a
dictionary containing the excess keyword arguments (using the keywords as keys
and the argument values as corresponding values), or a (new) empty dictionary if
there were no excess keyword arguments.

.. index::
   single: * (asterisk); in function calls
   single: unpacking; in function calls

If the syntax ``*expression`` appears in the function call, ``expression`` must
evaluate to an :term:`iterable`.  Elements from these iterables are
treated as if they were additional positional arguments.  For the call
``f(x1, x2, *y, x3, x4)``, if *y* evaluates to a sequence *y1*, ..., *yM*,
this is equivalent to a call with M+4 positional arguments *x1*, *x2*,
*y1*, ..., *yM*, *x3*, *x4*.

A consequence of this is that although the ``*expression`` syntax may appear
*after* explicit keyword arguments, it is processed *before* the
keyword arguments (and any ``**expression`` arguments -- see below).  So::

   >>> def f(a, b):
   ...     print(a, b)
   ...
   >>> f(b=1, *(2,))
   2 1
   >>> f(a=1, *(2,))
   Traceback (most recent call last):
     File "<stdin>", line 1, in <module>
   TypeError: f() got multiple values for keyword argument 'a'
   >>> f(1, *(2,))
   1 2

It is unusual for both keyword arguments and the ``*expression`` syntax to be
used in the same call, so in practice this confusion does not arise.

.. index::
   single: **; in function calls

If the syntax ``**expression`` appears in the function call, ``expression`` must
evaluate to a :term:`mapping`, the contents of which are treated as
additional keyword arguments.  If a keyword is already present
(as an explicit keyword argument, or from another unpacking),
a :exc:`TypeError` exception is raised.

Formal parameters using the syntax ``*identifier`` or ``**identifier`` cannot be
used as positional argument slots or as keyword argument names.

.. versionchanged:: 3.5
   Function calls accept any number of ``*`` and ``**`` unpackings,
   positional arguments may follow iterable unpackings (``*``),
   and keyword arguments may follow dictionary unpackings (``**``).
   Originally proposed by :pep:`448`.

A call always returns some value, possibly ``None``, unless it raises an
exception.  How this value is computed depends on the type of the callable
object.

If it is---

a user-defined function:
   .. index::
      pair: function; call
      triple: user-defined; function; call
      object: user-defined function
      object: function

   The code block for the function is executed, passing it the argument list.  The
   first thing the code block will do is bind the formal parameters to the
   arguments; this is described in section :ref:`function`.  When the code block
   executes a :keyword:`return` statement, this specifies the return value of the
   function call.

a built-in function or method:
   .. index::
      pair: function; call
      pair: built-in function; call
      pair: method; call
      pair: built-in method; call
      object: built-in method
      object: built-in function
      object: method
      object: function

   The result is up to the interpreter; see :ref:`built-in-funcs` for the
   descriptions of built-in functions and methods.

a class object:
   .. index::
      object: class
      pair: class object; call

   A new instance of that class is returned.

a class instance method:
   .. index::
      object: class instance
      object: instance
      pair: class instance; call

   The corresponding user-defined function is called, with an argument list that is
   one longer than the argument list of the call: the instance becomes the first
   argument.

a class instance:
   .. index::
      pair: instance; call
      single: __call__() (object method)

   The class must define a :meth:`__call__` method; the effect is then the same as
   if that method was called.


.. index:: keyword: await
.. _await:

Await expression
================

Suspend the execution of :term:`coroutine` on an :term:`awaitable` object.
Can only be used inside a :term:`coroutine function`.

.. productionlist::
   await_expr: "await" `primary`

.. versionadded:: 3.5


.. _power:

The power operator
==================

.. index::
   pair: power; operation
   operator: **

The power operator binds more tightly than unary operators on its left; it binds
less tightly than unary operators on its right.  The syntax is:

.. productionlist::
   power: (`await_expr` | `primary`) ["**" `u_expr`]

Thus, in an unparenthesized sequence of power and unary operators, the operators
are evaluated from right to left (this does not constrain the evaluation order
for the operands): ``-1**2`` results in ``-1``.

The power operator has the same semantics as the built-in :func:`pow` function,
when called with two arguments: it yields its left argument raised to the power
of its right argument.  The numeric arguments are first converted to a common
type, and the result is of that type.

For int operands, the result has the same type as the operands unless the second
argument is negative; in that case, all arguments are converted to float and a
float result is delivered. For example, ``10**2`` returns ``100``, but
``10**-2`` returns ``0.01``.

Raising ``0.0`` to a negative power results in a :exc:`ZeroDivisionError`.
Raising a negative number to a fractional power results in a :class:`complex`
number. (In earlier versions it raised a :exc:`ValueError`.)


.. _unary:

Unary arithmetic and bitwise operations
=======================================

.. index::
   triple: unary; arithmetic; operation
   triple: unary; bitwise; operation

All unary arithmetic and bitwise operations have the same priority:

.. productionlist::
   u_expr: `power` | "-" `u_expr` | "+" `u_expr` | "~" `u_expr`

.. index::
   single: negation
   single: minus
   single: operator; - (minus)
   single: - (minus); unary operator

The unary ``-`` (minus) operator yields the negation of its numeric argument.

.. index::
   single: plus
   single: operator; + (plus)
   single: + (plus); unary operator

The unary ``+`` (plus) operator yields its numeric argument unchanged.

.. index::
   single: inversion
   operator: ~ (tilde)

The unary ``~`` (invert) operator yields the bitwise inversion of its integer
argument.  The bitwise inversion of ``x`` is defined as ``-(x+1)``.  It only
applies to integral numbers.

.. index:: exception: TypeError

In all three cases, if the argument does not have the proper type, a
:exc:`TypeError` exception is raised.


.. _binary:

Binary arithmetic operations
============================

.. index:: triple: binary; arithmetic; operation

The binary arithmetic operations have the conventional priority levels.  Note
that some of these operations also apply to certain non-numeric types.  Apart
from the power operator, there are only two levels, one for multiplicative
operators and one for additive operators:

.. productionlist::
   m_expr: `u_expr` | `m_expr` "*" `u_expr` | `m_expr` "@" `m_expr` |
         : `m_expr` "//" `u_expr` | `m_expr` "/" `u_expr` |
         : `m_expr` "%" `u_expr`
   a_expr: `m_expr` | `a_expr` "+" `m_expr` | `a_expr` "-" `m_expr`

.. index::
   single: multiplication
   operator: * (asterisk)

The ``*`` (multiplication) operator yields the product of its arguments.  The
arguments must either both be numbers, or one argument must be an integer and
the other must be a sequence. In the former case, the numbers are converted to a
common type and then multiplied together.  In the latter case, sequence
repetition is performed; a negative repetition factor yields an empty sequence.

.. index::
   single: matrix multiplication
   operator: @ (at)

The ``@`` (at) operator is intended to be used for matrix multiplication.  No
builtin Python types implement this operator.

.. versionadded:: 3.5

.. index::
   exception: ZeroDivisionError
   single: division
   operator: / (slash)
   operator: //

The ``/`` (division) and ``//`` (floor division) operators yield the quotient of
their arguments.  The numeric arguments are first converted to a common type.
Division of integers yields a float, while floor division of integers results in an
integer; the result is that of mathematical division with the 'floor' function
applied to the result.  Division by zero raises the :exc:`ZeroDivisionError`
exception.

.. index::
   single: modulo
   operator: % (percent)

The ``%`` (modulo) operator yields the remainder from the division of the first
argument by the second.  The numeric arguments are first converted to a common
type.  A zero right argument raises the :exc:`ZeroDivisionError` exception.  The
arguments may be floating point numbers, e.g., ``3.14%0.7`` equals ``0.34``
(since ``3.14`` equals ``4*0.7 + 0.34``.)  The modulo operator always yields a
result with the same sign as its second operand (or zero); the absolute value of
the result is strictly smaller than the absolute value of the second operand
[#]_.

The floor division and modulo operators are connected by the following
identity: ``x == (x//y)*y + (x%y)``.  Floor division and modulo are also
connected with the built-in function :func:`divmod`: ``divmod(x, y) == (x//y,
x%y)``. [#]_.

In addition to performing the modulo operation on numbers, the ``%`` operator is
also overloaded by string objects to perform old-style string formatting (also
known as interpolation).  The syntax for string formatting is described in the
Python Library Reference, section :ref:`old-string-formatting`.

The floor division operator, the modulo operator, and the :func:`divmod`
function are not defined for complex numbers.  Instead, convert to a floating
point number using the :func:`abs` function if appropriate.

.. index::
   single: addition
   single: operator; + (plus)
   single: + (plus); binary operator

The ``+`` (addition) operator yields the sum of its arguments.  The arguments
must either both be numbers or both be sequences of the same type.  In the
former case, the numbers are converted to a common type and then added together.
In the latter case, the sequences are concatenated.

.. index::
   single: subtraction
   single: operator; - (minus)
   single: - (minus); binary operator

The ``-`` (subtraction) operator yields the difference of its arguments.  The
numeric arguments are first converted to a common type.


.. _shifting:

Shifting operations
===================

.. index::
   pair: shifting; operation
   operator: <<
   operator: >>

The shifting operations have lower priority than the arithmetic operations:

.. productionlist::
   shift_expr: `a_expr` | `shift_expr` ("<<" | ">>") `a_expr`

These operators accept integers as arguments.  They shift the first argument to
the left or right by the number of bits given by the second argument.

.. index:: exception: ValueError

A right shift by *n* bits is defined as floor division by ``pow(2,n)``.  A left
shift by *n* bits is defined as multiplication with ``pow(2,n)``.

.. note::

   In the current implementation, the right-hand operand is required
   to be at most :attr:`sys.maxsize`.  If the right-hand operand is larger than
   :attr:`sys.maxsize` an :exc:`OverflowError` exception is raised.

.. _bitwise:

Binary bitwise operations
=========================

.. index:: triple: binary; bitwise; operation

Each of the three bitwise operations has a different priority level:

.. productionlist::
   and_expr: `shift_expr` | `and_expr` "&" `shift_expr`
   xor_expr: `and_expr` | `xor_expr` "^" `and_expr`
   or_expr: `xor_expr` | `or_expr` "|" `xor_expr`

.. index::
   pair: bitwise; and
   operator: & (ampersand)

The ``&`` operator yields the bitwise AND of its arguments, which must be
integers.

.. index::
   pair: bitwise; xor
   pair: exclusive; or
   operator: ^ (caret)

The ``^`` operator yields the bitwise XOR (exclusive OR) of its arguments, which
must be integers.

.. index::
   pair: bitwise; or
   pair: inclusive; or
   operator: | (vertical bar)

The ``|`` operator yields the bitwise (inclusive) OR of its arguments, which
must be integers.


.. _comparisons:

Comparisons
===========

.. index::
   single: comparison
   pair: C; language
   operator: < (less)
   operator: > (greater)
   operator: <=
   operator: >=
   operator: ==
   operator: !=

Unlike C, all comparison operations in Python have the same priority, which is
lower than that of any arithmetic, shifting or bitwise operation.  Also unlike
C, expressions like ``a < b < c`` have the interpretation that is conventional
in mathematics:

.. productionlist::
   comparison: `or_expr` (`comp_operator` `or_expr`)*
   comp_operator: "<" | ">" | "==" | ">=" | "<=" | "!="
                : | "is" ["not"] | ["not"] "in"

Comparisons yield boolean values: ``True`` or ``False``.

.. index:: pair: chaining; comparisons

Comparisons can be chained arbitrarily, e.g., ``x < y <= z`` is equivalent to
``x < y and y <= z``, except that ``y`` is evaluated only once (but in both
cases ``z`` is not evaluated at all when ``x < y`` is found to be false).

Formally, if *a*, *b*, *c*, ..., *y*, *z* are expressions and *op1*, *op2*, ...,
*opN* are comparison operators, then ``a op1 b op2 c ... y opN z`` is equivalent
to ``a op1 b and b op2 c and ... y opN z``, except that each expression is
evaluated at most once.

Note that ``a op1 b op2 c`` doesn't imply any kind of comparison between *a* and
*c*, so that, e.g., ``x < y > z`` is perfectly legal (though perhaps not
pretty).

Value comparisons
-----------------

The operators ``<``, ``>``, ``==``, ``>=``, ``<=``, and ``!=`` compare the
values of two objects.  The objects do not need to have the same type.

Chapter :ref:`objects` states that objects have a value (in addition to type
and identity).  The value of an object is a rather abstract notion in Python:
For example, there is no canonical access method for an object's value.  Also,
there is no requirement that the value of an object should be constructed in a
particular way, e.g. comprised of all its data attributes. Comparison operators
implement a particular notion of what the value of an object is.  One can think
of them as defining the value of an object indirectly, by means of their
comparison implementation.

Because all types are (direct or indirect) subtypes of :class:`object`, they
inherit the default comparison behavior from :class:`object`.  Types can
customize their comparison behavior by implementing
:dfn:`rich comparison methods` like :meth:`__lt__`, described in
:ref:`customization`.

The default behavior for equality comparison (``==`` and ``!=``) is based on
the identity of the objects.  Hence, equality comparison of instances with the
same identity results in equality, and equality comparison of instances with
different identities results in inequality.  A motivation for this default
behavior is the desire that all objects should be reflexive (i.e. ``x is y``
implies ``x == y``).

A default order comparison (``<``, ``>``, ``<=``, and ``>=``) is not provided;
an attempt raises :exc:`TypeError`.  A motivation for this default behavior is
the lack of a similar invariant as for equality.

The behavior of the default equality comparison, that instances with different
identities are always unequal, may be in contrast to what types will need that
have a sensible definition of object value and value-based equality.  Such
types will need to customize their comparison behavior, and in fact, a number
of built-in types have done that.

The following list describes the comparison behavior of the most important
built-in types.

* Numbers of built-in numeric types (:ref:`typesnumeric`) and of the standard
  library types :class:`fractions.Fraction` and :class:`decimal.Decimal` can be
  compared within and across their types, with the restriction that complex
  numbers do not support order comparison.  Within the limits of the types
  involved, they compare mathematically (algorithmically) correct without loss
  of precision.

  The not-a-number values :const:`float('NaN')` and :const:`Decimal('NaN')`
  are special.  They are identical to themselves (``x is x`` is true) but
  are not equal to themselves (``x == x`` is false).  Additionally,
  comparing any number to a not-a-number value
  will return ``False``.  For example, both ``3 < float('NaN')`` and
  ``float('NaN') < 3`` will return ``False``.

* Binary sequences (instances of :class:`bytes` or :class:`bytearray`) can be
  compared within and across their types.  They compare lexicographically using
  the numeric values of their elements.

* Strings (instances of :class:`str`) compare lexicographically using the
  numerical Unicode code points (the result of the built-in function
  :func:`ord`) of their characters. [#]_

  Strings and binary sequences cannot be directly compared.

* Sequences (instances of :class:`tuple`, :class:`list`, or :class:`range`) can
  be compared only within each of their types, with the restriction that ranges
  do not support order comparison.  Equality comparison across these types
  results in inequality, and ordering comparison across these types raises
  :exc:`TypeError`.

  Sequences compare lexicographically using comparison of corresponding
  elements, whereby reflexivity of the elements is enforced.

  In enforcing reflexivity of elements, the comparison of collections assumes
  that for a collection element ``x``, ``x == x`` is always true.  Based on
  that assumption, element identity is compared first, and element comparison
  is performed only for distinct elements.  This approach yields the same
  result as a strict element comparison would, if the compared elements are
  reflexive.  For non-reflexive elements, the result is different than for
  strict element comparison, and may be surprising:  The non-reflexive
  not-a-number values for example result in the following comparison behavior
  when used in a list::

    >>> nan = float('NaN')
    >>> nan is nan
    True
    >>> nan == nan
    False                 <-- the defined non-reflexive behavior of NaN
    >>> [nan] == [nan]
    True                  <-- list enforces reflexivity and tests identity first

  Lexicographical comparison between built-in collections works as follows:

  - For two collections to compare equal, they must be of the same type, have
    the same length, and each pair of corresponding elements must compare
    equal (for example, ``[1,2] == (1,2)`` is false because the type is not the
    same).

  - Collections that support order comparison are ordered the same as their
    first unequal elements (for example, ``[1,2,x] <= [1,2,y]`` has the same
    value as ``x <= y``).  If a corresponding element does not exist, the
    shorter collection is ordered first (for example, ``[1,2] < [1,2,3]`` is
    true).

* Mappings (instances of :class:`dict`) compare equal if and only if they have
  equal `(key, value)` pairs. Equality comparison of the keys and values
  enforces reflexivity.

  Order comparisons (``<``, ``>``, ``<=``, and ``>=``) raise :exc:`TypeError`.

* Sets (instances of :class:`set` or :class:`frozenset`) can be compared within
  and across their types.

  They define order
  comparison operators to mean subset and superset tests.  Those relations do
  not define total orderings (for example, the two sets ``{1,2}`` and ``{2,3}``
  are not equal, nor subsets of one another, nor supersets of one
  another).  Accordingly, sets are not appropriate arguments for functions
  which depend on total ordering (for example, :func:`min`, :func:`max`, and
  :func:`sorted` produce undefined results given a list of sets as inputs).

  Comparison of sets enforces reflexivity of its elements.

* Most other built-in types have no comparison methods implemented, so they
  inherit the default comparison behavior.

User-defined classes that customize their comparison behavior should follow
some consistency rules, if possible:

* Equality comparison should be reflexive.
  In other words, identical objects should compare equal:

    ``x is y`` implies ``x == y``

* Comparison should be symmetric.
  In other words, the following expressions should have the same result:

    ``x == y`` and ``y == x``

    ``x != y`` and ``y != x``

    ``x < y`` and ``y > x``

    ``x <= y`` and ``y >= x``

* Comparison should be transitive.
  The following (non-exhaustive) examples illustrate that:

    ``x > y and y > z`` implies ``x > z``

    ``x < y and y <= z`` implies ``x < z``

* Inverse comparison should result in the boolean negation.
  In other words, the following expressions should have the same result:

    ``x == y`` and ``not x != y``

    ``x < y`` and ``not x >= y`` (for total ordering)

    ``x > y`` and ``not x <= y`` (for total ordering)

  The last two expressions apply to totally ordered collections (e.g. to
  sequences, but not to sets or mappings). See also the
  :func:`~functools.total_ordering` decorator.

* The :func:`hash` result should be consistent with equality.
  Objects that are equal should either have the same hash value,
  or be marked as unhashable.

Python does not enforce these consistency rules. In fact, the not-a-number
values are an example for not following these rules.


.. _in:
.. _not in:
.. _membership-test-details:

Membership test operations
--------------------------

The operators :keyword:`in` and :keyword:`not in` test for membership.  ``x in
s`` evaluates to ``True`` if *x* is a member of *s*, and ``False`` otherwise.
``x not in s`` returns the negation of ``x in s``.  All built-in sequences and
set types support this as well as dictionary, for which :keyword:`in` tests
whether the dictionary has a given key. For container types such as list, tuple,
set, frozenset, dict, or collections.deque, the expression ``x in y`` is equivalent
to ``any(x is e or x == e for e in y)``.

For the string and bytes types, ``x in y`` is ``True`` if and only if *x* is a
substring of *y*.  An equivalent test is ``y.find(x) != -1``.  Empty strings are
always considered to be a substring of any other string, so ``"" in "abc"`` will
return ``True``.

For user-defined classes which define the :meth:`__contains__` method, ``x in
y`` returns ``True`` if ``y.__contains__(x)`` returns a true value, and
``False`` otherwise.

For user-defined classes which do not define :meth:`__contains__` but do define
:meth:`__iter__`, ``x in y`` is ``True`` if some value ``z`` with ``x == z`` is
produced while iterating over ``y``.  If an exception is raised during the
iteration, it is as if :keyword:`in` raised that exception.

Lastly, the old-style iteration protocol is tried: if a class defines
:meth:`__getitem__`, ``x in y`` is ``True`` if and only if there is a non-negative
integer index *i* such that ``x == y[i]``, and all lower integer indices do not
raise :exc:`IndexError` exception.  (If any other exception is raised, it is as
if :keyword:`in` raised that exception).

.. index::
   operator: in
   operator: not in
   pair: membership; test
   object: sequence

The operator :keyword:`not in` is defined to have the inverse true value of
:keyword:`in`.

.. index::
   operator: is
   operator: is not
   pair: identity; test


.. _is:
.. _is not:

Identity comparisons
--------------------

The operators :keyword:`is` and :keyword:`is not` test for object identity: ``x
is y`` is true if and only if *x* and *y* are the same object.  Object identity
is determined using the :meth:`id` function.  ``x is not y`` yields the inverse
truth value. [#]_


.. _booleans:
.. _and:
.. _or:
.. _not:

Boolean operations
==================

.. index::
   pair: Conditional; expression
   pair: Boolean; operation

.. productionlist::
   or_test: `and_test` | `or_test` "or" `and_test`
   and_test: `not_test` | `and_test` "and" `not_test`
   not_test: `comparison` | "not" `not_test`

In the context of Boolean operations, and also when expressions are used by
control flow statements, the following values are interpreted as false:
``False``, ``None``, numeric zero of all types, and empty strings and containers
(including strings, tuples, lists, dictionaries, sets and frozensets).  All
other values are interpreted as true.  User-defined objects can customize their
truth value by providing a :meth:`__bool__` method.

.. index:: operator: not

The operator :keyword:`not` yields ``True`` if its argument is false, ``False``
otherwise.

.. index:: operator: and

The expression ``x and y`` first evaluates *x*; if *x* is false, its value is
returned; otherwise, *y* is evaluated and the resulting value is returned.

.. index:: operator: or

The expression ``x or y`` first evaluates *x*; if *x* is true, its value is
returned; otherwise, *y* is evaluated and the resulting value is returned.

(Note that neither :keyword:`and` nor :keyword:`or` restrict the value and type
they return to ``False`` and ``True``, but rather return the last evaluated
argument.  This is sometimes useful, e.g., if ``s`` is a string that should be
replaced by a default value if it is empty, the expression ``s or 'foo'`` yields
the desired value.  Because :keyword:`not` has to create a new value, it
returns a boolean value regardless of the type of its argument
(for example, ``not 'foo'`` produces ``False`` rather than ``''``.)


Conditional expressions
=======================

.. index::
   pair: conditional; expression
   pair: ternary; operator
   single: if; conditional expression
   single: else; conditional expression

.. productionlist::
   conditional_expression: `or_test` ["if" `or_test` "else" `expression`]
   expression: `conditional_expression` | `lambda_expr`
   expression_nocond: `or_test` | `lambda_expr_nocond`

Conditional expressions (sometimes called a "ternary operator") have the lowest
priority of all Python operations.

The expression ``x if C else y`` first evaluates the condition, *C* rather than *x*.
If *C* is true, *x* is evaluated and its value is returned; otherwise, *y* is
evaluated and its value is returned.

See :pep:`308` for more details about conditional expressions.


.. _lambdas:
.. _lambda:

Lambdas
=======

.. index::
   pair: lambda; expression
   pair: lambda; form
   pair: anonymous; function
   single: : (colon); lambda expression

.. productionlist::
   lambda_expr: "lambda" [`parameter_list`] ":" `expression`
   lambda_expr_nocond: "lambda" [`parameter_list`] ":" `expression_nocond`

Lambda expressions (sometimes called lambda forms) are used to create anonymous
functions. The expression ``lambda parameters: expression`` yields a function
object.  The unnamed object behaves like a function object defined with:

.. code-block:: none

   def <lambda>(parameters):
       return expression

See section :ref:`function` for the syntax of parameter lists.  Note that
functions created with lambda expressions cannot contain statements or
annotations.


.. _exprlists:

Expression lists
================

.. index::
   pair: expression; list
   single: , (comma); expression list

.. productionlist::
   expression_list: `expression` ("," `expression`)* [","]
   starred_list: `starred_item` ("," `starred_item`)* [","]
   starred_expression: `expression` | (`starred_item` ",")* [`starred_item`]
   starred_item: `expression` | "*" `or_expr`

.. index:: object: tuple

Except when part of a list or set display, an expression list
containing at least one comma yields a tuple.  The length of
the tuple is the number of expressions in the list.  The expressions are
evaluated from left to right.

.. index::
   pair: iterable; unpacking
   single: * (asterisk); in expression lists

An asterisk ``*`` denotes :dfn:`iterable unpacking`.  Its operand must be
an :term:`iterable`.  The iterable is expanded into a sequence of items,
which are included in the new tuple, list, or set, at the site of
the unpacking.

.. versionadded:: 3.5
   Iterable unpacking in expression lists, originally proposed by :pep:`448`.

.. index:: pair: trailing; comma

The trailing comma is required only to create a single tuple (a.k.a. a
*singleton*); it is optional in all other cases.  A single expression without a
trailing comma doesn't create a tuple, but rather yields the value of that
expression. (To create an empty tuple, use an empty pair of parentheses:
``()``.)


.. _evalorder:

Evaluation order
================

.. index:: pair: evaluation; order

Python evaluates expressions from left to right.  Notice that while evaluating
an assignment, the right-hand side is evaluated before the left-hand side.

In the following lines, expressions will be evaluated in the arithmetic order of
their suffixes::

   expr1, expr2, expr3, expr4
   (expr1, expr2, expr3, expr4)
   {expr1: expr2, expr3: expr4}
   expr1 + expr2 * (expr3 - expr4)
   expr1(expr2, expr3, *expr4, **expr5)
   expr3, expr4 = expr1, expr2


.. _operator-summary:

Operator precedence
===================

.. index::
   pair: operator; precedence

The following table summarizes the operator precedence in Python, from lowest
precedence (least binding) to highest precedence (most binding).  Operators in
the same box have the same precedence.  Unless the syntax is explicitly given,
operators are binary.  Operators in the same box group left to right (except for
exponentiation, which groups from right to left).

Note that comparisons, membership tests, and identity tests, all have the same
precedence and have a left-to-right chaining feature as described in the
:ref:`comparisons` section.


+-----------------------------------------------+-------------------------------------+
| Operator                                      | Description                         |
+===============================================+=====================================+
| :keyword:`lambda`                             | Lambda expression                   |
+-----------------------------------------------+-------------------------------------+
| :keyword:`if` -- :keyword:`else`              | Conditional expression              |
+-----------------------------------------------+-------------------------------------+
| :keyword:`or`                                 | Boolean OR                          |
+-----------------------------------------------+-------------------------------------+
| :keyword:`and`                                | Boolean AND                         |
+-----------------------------------------------+-------------------------------------+
| :keyword:`not` ``x``                          | Boolean NOT                         |
+-----------------------------------------------+-------------------------------------+
| :keyword:`in`, :keyword:`not in`,             | Comparisons, including membership   |
| :keyword:`is`, :keyword:`is not`, ``<``,      | tests and identity tests            |
| ``<=``, ``>``, ``>=``, ``!=``, ``==``         |                                     |
+-----------------------------------------------+-------------------------------------+
| ``|``                                         | Bitwise OR                          |
+-----------------------------------------------+-------------------------------------+
| ``^``                                         | Bitwise XOR                         |
+-----------------------------------------------+-------------------------------------+
| ``&``                                         | Bitwise AND                         |
+-----------------------------------------------+-------------------------------------+
| ``<<``, ``>>``                                | Shifts                              |
+-----------------------------------------------+-------------------------------------+
| ``+``, ``-``                                  | Addition and subtraction            |
+-----------------------------------------------+-------------------------------------+
| ``*``, ``@``, ``/``, ``//``, ``%``            | Multiplication, matrix              |
|                                               | multiplication, division, floor     |
|                                               | division, remainder [#]_            |
+-----------------------------------------------+-------------------------------------+
| ``+x``, ``-x``, ``~x``                        | Positive, negative, bitwise NOT     |
+-----------------------------------------------+-------------------------------------+
| ``**``                                        | Exponentiation [#]_                 |
+-----------------------------------------------+-------------------------------------+
| :keyword:`await` ``x``                        | Await expression                    |
+-----------------------------------------------+-------------------------------------+
| ``x[index]``, ``x[index:index]``,             | Subscription, slicing,              |
| ``x(arguments...)``, ``x.attribute``          | call, attribute reference           |
+-----------------------------------------------+-------------------------------------+
| ``(expressions...)``,                         | Binding or tuple display,           |
| ``[expressions...]``,                         | list display,                       |
| ``{key: value...}``,                          | dictionary display,                 |
| ``{expressions...}``                          | set display                         |
+-----------------------------------------------+-------------------------------------+


.. rubric:: Footnotes

.. [#] While ``abs(x%y) < abs(y)`` is true mathematically, for floats it may not be
   true numerically due to roundoff.  For example, and assuming a platform on which
   a Python float is an IEEE 754 double-precision number, in order that ``-1e-100 %
   1e100`` have the same sign as ``1e100``, the computed result is ``-1e-100 +
   1e100``, which is numerically exactly equal to ``1e100``.  The function
   :func:`math.fmod` returns a result whose sign matches the sign of the
   first argument instead, and so returns ``-1e-100`` in this case. Which approach
   is more appropriate depends on the application.

.. [#] If x is very close to an exact integer multiple of y, it's possible for
   ``x//y`` to be one larger than ``(x-x%y)//y`` due to rounding.  In such
   cases, Python returns the latter result, in order to preserve that
   ``divmod(x,y)[0] * y + x % y`` be very close to ``x``.

.. [#] The Unicode standard distinguishes between :dfn:`code points`
   (e.g. U+0041) and :dfn:`abstract characters` (e.g. "LATIN CAPITAL LETTER A").
   While most abstract characters in Unicode are only represented using one
   code point, there is a number of abstract characters that can in addition be
   represented using a sequence of more than one code point.  For example, the
   abstract character "LATIN CAPITAL LETTER C WITH CEDILLA" can be represented
   as a single :dfn:`precomposed character` at code position U+00C7, or as a
   sequence of a :dfn:`base character` at code position U+0043 (LATIN CAPITAL
   LETTER C), followed by a :dfn:`combining character` at code position U+0327
   (COMBINING CEDILLA).

   The comparison operators on strings compare at the level of Unicode code
   points. This may be counter-intuitive to humans.  For example,
   ``"\u00C7" == "\u0043\u0327"`` is ``False``, even though both strings
   represent the same abstract character "LATIN CAPITAL LETTER C WITH CEDILLA".

   To compare strings at the level of abstract characters (that is, in a way
   intuitive to humans), use :func:`unicodedata.normalize`.

.. [#] Due to automatic garbage-collection, free lists, and the dynamic nature of
   descriptors, you may notice seemingly unusual behaviour in certain uses of
   the :keyword:`is` operator, like those involving comparisons between instance
   methods, or constants.  Check their documentation for more info.

.. [#] The ``%`` operator is also used for string formatting; the same
   precedence applies.

.. [#] The power operator ``**`` binds less tightly than an arithmetic or
   bitwise unary operator on its right, that is, ``2**-1`` is ``0.5``.
