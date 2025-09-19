.. _tut-morecontrol:

***********************
More Control Flow Tools
***********************

As well as the :keyword:`while` statement just introduced, Python uses a few more
that we will encounter in this chapter.


.. _tut-if:

:keyword:`!if` Statements
=========================

Perhaps the most well-known statement type is the :keyword:`if` statement.  For
example::

   >>> x = int(input("Please enter an integer: "))
   Please enter an integer: 42
   >>> if x < 0:
   ...     x = 0
   ...     print('Negative changed to zero')
   ... elif x == 0:
   ...     print('Zero')
   ... elif x == 1:
   ...     print('Single')
   ... else:
   ...     print('More')
   ...
   More

There can be zero or more :keyword:`elif` parts, and the :keyword:`else` part is
optional.  The keyword ':keyword:`!elif`' is short for 'else if', and is useful
to avoid excessive indentation.  An  :keyword:`!if` ... :keyword:`!elif` ...
:keyword:`!elif` ... sequence is a substitute for the ``switch`` or
``case`` statements found in other languages.

If you're comparing the same value to several constants, or checking for specific types or
attributes, you may also find the :keyword:`!match` statement useful. For more
details see :ref:`tut-match`.

.. _tut-for:

:keyword:`!for` Statements
==========================

.. index::
   pair: statement; for

The :keyword:`for` statement in Python differs a bit from what you may be used
to in C or Pascal.  Rather than always iterating over an arithmetic progression
of numbers (like in Pascal), or giving the user the ability to define both the
iteration step and halting condition (as C), Python's :keyword:`!for` statement
iterates over the items of any sequence (a list or a string), in the order that
they appear in the sequence.  For example (no pun intended):

.. One suggestion was to give a real C example here, but that may only serve to
   confuse non-C programmers.

::

   >>> # Measure some strings:
   >>> words = ['cat', 'window', 'defenestrate']
   >>> for w in words:
   ...     print(w, len(w))
   ...
   cat 3
   window 6
   defenestrate 12

Code that modifies a collection while iterating over that same collection can
be tricky to get right.  Instead, it is usually more straight-forward to loop
over a copy of the collection or to create a new collection::

    # Create a sample collection
    users = {'Hans': 'active', 'Éléonore': 'inactive', '景太郎': 'active'}

    # Strategy:  Iterate over a copy
    for user, status in users.copy().items():
        if status == 'inactive':
            del users[user]

    # Strategy:  Create a new collection
    active_users = {}
    for user, status in users.items():
        if status == 'active':
            active_users[user] = status


.. _tut-range:

The :func:`range` Function
==========================

If you do need to iterate over a sequence of numbers, the built-in function
:func:`range` comes in handy.  It generates arithmetic progressions::

    >>> for i in range(5):
    ...     print(i)
    ...
    0
    1
    2
    3
    4

The given end point is never part of the generated sequence; ``range(10)`` generates
10 values, the legal indices for items of a sequence of length 10.  It
is possible to let the range start at another number, or to specify a different
increment (even negative; sometimes this is called the 'step')::

    >>> list(range(5, 10))
    [5, 6, 7, 8, 9]

    >>> list(range(0, 10, 3))
    [0, 3, 6, 9]

    >>> list(range(-10, -100, -30))
    [-10, -40, -70]

To iterate over the indices of a sequence, you can combine :func:`range` and
:func:`len` as follows::

   >>> a = ['Mary', 'had', 'a', 'little', 'lamb']
   >>> for i in range(len(a)):
   ...     print(i, a[i])
   ...
   0 Mary
   1 had
   2 a
   3 little
   4 lamb

In most such cases, however, it is convenient to use the :func:`enumerate`
function, see :ref:`tut-loopidioms`.

A strange thing happens if you just print a range::

   >>> range(10)
   range(0, 10)

In many ways the object returned by :func:`range` behaves as if it is a list,
but in fact it isn't. It is an object which returns the successive items of
the desired sequence when you iterate over it, but it doesn't really make
the list, thus saving space.

We say such an object is :term:`iterable`, that is, suitable as a target for
functions and constructs that expect something from which they can
obtain successive items until the supply is exhausted.  We have seen that
the :keyword:`for` statement is such a construct, while an example of a function
that takes an iterable is :func:`sum`::

    >>> sum(range(4))  # 0 + 1 + 2 + 3
    6

Later we will see more functions that return iterables and take iterables as
arguments.  In chapter :ref:`tut-structures`, we will discuss in more detail about
:func:`list`.

.. _tut-break:

:keyword:`!break` and :keyword:`!continue` Statements
=====================================================

The :keyword:`break` statement breaks out of the innermost enclosing
:keyword:`for` or :keyword:`while` loop::

    >>> for n in range(2, 10):
    ...     for x in range(2, n):
    ...         if n % x == 0:
    ...             print(f"{n} equals {x} * {n//x}")
    ...             break
    ...
    4 equals 2 * 2
    6 equals 2 * 3
    8 equals 2 * 4
    9 equals 3 * 3

The :keyword:`continue` statement continues with the next
iteration of the loop::

    >>> for num in range(2, 10):
    ...     if num % 2 == 0:
    ...         print(f"Found an even number {num}")
    ...         continue
    ...     print(f"Found an odd number {num}")
    ...
    Found an even number 2
    Found an odd number 3
    Found an even number 4
    Found an odd number 5
    Found an even number 6
    Found an odd number 7
    Found an even number 8
    Found an odd number 9

.. _tut-for-else:
.. _break-and-continue-statements-and-else-clauses-on-loops:

:keyword:`!else` Clauses on Loops
=================================

In a :keyword:`!for` or :keyword:`!while` loop the :keyword:`!break` statement
may be paired with an :keyword:`!else` clause.  If the loop finishes without
executing the :keyword:`!break`, the :keyword:`!else` clause executes.

In a :keyword:`for` loop, the :keyword:`!else` clause is executed
after the loop finishes its final iteration, that is, if no break occurred.

In a :keyword:`while` loop, it's executed after the loop's condition becomes false.

In either kind of loop, the :keyword:`!else` clause is **not** executed if the
loop was terminated by a :keyword:`break`.  Of course, other ways of ending the
loop early, such as a :keyword:`return` or a raised exception, will also skip
execution of the :keyword:`else` clause.

This is exemplified in the following :keyword:`!for` loop,
which searches for prime numbers::

   >>> for n in range(2, 10):
   ...     for x in range(2, n):
   ...         if n % x == 0:
   ...             print(n, 'equals', x, '*', n//x)
   ...             break
   ...     else:
   ...         # loop fell through without finding a factor
   ...         print(n, 'is a prime number')
   ...
   2 is a prime number
   3 is a prime number
   4 equals 2 * 2
   5 is a prime number
   6 equals 2 * 3
   7 is a prime number
   8 equals 2 * 4
   9 equals 3 * 3

(Yes, this is the correct code.  Look closely: the ``else`` clause belongs to
the ``for`` loop, **not** the ``if`` statement.)

One way to think of the else clause is to imagine it paired with the ``if``
inside the loop.  As the loop executes, it will run a sequence like
if/if/if/else. The ``if`` is inside the loop, encountered a number of times. If
the condition is ever true, a ``break`` will happen. If the condition is never
true, the ``else`` clause outside the loop will execute.

When used with a loop, the ``else`` clause has more in common with the ``else``
clause of a :keyword:`try` statement than it does with that of ``if``
statements: a ``try`` statement's ``else`` clause runs when no exception
occurs, and a loop's ``else`` clause runs when no ``break`` occurs. For more on
the ``try`` statement and exceptions, see :ref:`tut-handling`.

.. _tut-pass:

:keyword:`!pass` Statements
===========================

The :keyword:`pass` statement does nothing. It can be used when a statement is
required syntactically but the program requires no action. For example::

   >>> while True:
   ...     pass  # Busy-wait for keyboard interrupt (Ctrl+C)
   ...

This is commonly used for creating minimal classes::

   >>> class MyEmptyClass:
   ...     pass
   ...

Another place :keyword:`pass` can be used is as a place-holder for a function or
conditional body when you are working on new code, allowing you to keep thinking
at a more abstract level.  The :keyword:`!pass` is silently ignored::

   >>> def initlog(*args):
   ...     pass   # Remember to implement this!
   ...


.. _tut-match:

:keyword:`!match` Statements
============================

A :keyword:`match` statement takes an expression and compares its value to successive
patterns given as one or more case blocks.  This is superficially
similar to a switch statement in C, Java or JavaScript (and many
other languages), but it's more similar to pattern matching in
languages like Rust or Haskell. Only the first pattern that matches
gets executed and it can also extract components (sequence elements
or object attributes) from the value into variables. If no case matches,
none of the branches is executed.

The simplest form compares a subject value against one or more literals::

    def http_error(status):
        match status:
            case 400:
                return "Bad request"
            case 404:
                return "Not found"
            case 418:
                return "I'm a teapot"
            case _:
                return "Something's wrong with the internet"

Note the last block: the "variable name" ``_`` acts as a *wildcard* and
never fails to match.

You can combine several literals in a single pattern using ``|`` ("or")::

            case 401 | 403 | 404:
                return "Not allowed"

Patterns can look like unpacking assignments, and can be used to bind
variables::

    # point is an (x, y) tuple
    match point:
        case (0, 0):
            print("Origin")
        case (0, y):
            print(f"Y={y}")
        case (x, 0):
            print(f"X={x}")
        case (x, y):
            print(f"X={x}, Y={y}")
        case _:
            raise ValueError("Not a point")

Study that one carefully!  The first pattern has two literals, and can
be thought of as an extension of the literal pattern shown above.  But
the next two patterns combine a literal and a variable, and the
variable *binds* a value from the subject (``point``).  The fourth
pattern captures two values, which makes it conceptually similar to
the unpacking assignment ``(x, y) = point``.

If you are using classes to structure your data
you can use the class name followed by an argument list resembling a
constructor, but with the ability to capture attributes into variables::

    class Point:
        def __init__(self, x, y):
            self.x = x
            self.y = y

    def where_is(point):
        match point:
            case Point(x=0, y=0):
                print("Origin")
            case Point(x=0, y=y):
                print(f"Y={y}")
            case Point(x=x, y=0):
                print(f"X={x}")
            case Point():
                print("Somewhere else")
            case _:
                print("Not a point")

You can use positional parameters with some builtin classes that provide an
ordering for their attributes (e.g. dataclasses). You can also define a specific
position for attributes in patterns by setting the ``__match_args__`` special
attribute in your classes. If it's set to ("x", "y"), the following patterns are all
equivalent (and all bind the ``y`` attribute to the ``var`` variable)::

    Point(1, var)
    Point(1, y=var)
    Point(x=1, y=var)
    Point(y=var, x=1)

A recommended way to read patterns is to look at them as an extended form of what you
would put on the left of an assignment, to understand which variables would be set to
what.
Only the standalone names (like ``var`` above) are assigned to by a match statement.
Dotted names (like ``foo.bar``), attribute names (the ``x=`` and ``y=`` above) or class names
(recognized by the "(...)" next to them like ``Point`` above) are never assigned to.

Patterns can be arbitrarily nested.  For example, if we have a short
list of Points, with ``__match_args__`` added, we could match it like this::

    class Point:
        __match_args__ = ('x', 'y')
        def __init__(self, x, y):
            self.x = x
            self.y = y

    match points:
        case []:
            print("No points")
        case [Point(0, 0)]:
            print("The origin")
        case [Point(x, y)]:
            print(f"Single point {x}, {y}")
        case [Point(0, y1), Point(0, y2)]:
            print(f"Two on the Y axis at {y1}, {y2}")
        case _:
            print("Something else")

We can add an ``if`` clause to a pattern, known as a "guard".  If the
guard is false, ``match`` goes on to try the next case block.  Note
that value capture happens before the guard is evaluated::

    match point:
        case Point(x, y) if x == y:
            print(f"Y=X at {x}")
        case Point(x, y):
            print(f"Not on the diagonal")

Several other key features of this statement:

- Like unpacking assignments, tuple and list patterns have exactly the
  same meaning and actually match arbitrary sequences.  An important
  exception is that they don't match iterators or strings.

- Sequence patterns support extended unpacking: ``[x, y, *rest]`` and ``(x, y,
  *rest)`` work similar to unpacking assignments.  The
  name after ``*`` may also be ``_``, so ``(x, y, *_)`` matches a sequence
  of at least two items without binding the remaining items.

- Mapping patterns: ``{"bandwidth": b, "latency": l}`` captures the
  ``"bandwidth"`` and ``"latency"`` values from a dictionary.  Unlike sequence
  patterns, extra keys are ignored.  An unpacking like ``**rest`` is also
  supported.  (But ``**_`` would be redundant, so it is not allowed.)

- Subpatterns may be captured using the ``as`` keyword::

      case (Point(x1, y1), Point(x2, y2) as p2): ...

  will capture the second element of the input as ``p2`` (as long as the input is
  a sequence of two points)

- Most literals are compared by equality, however the singletons ``True``,
  ``False`` and ``None`` are compared by identity.

- Patterns may use named constants.  These must be dotted names
  to prevent them from being interpreted as capture variable::

      from enum import Enum
      class Color(Enum):
          RED = 'red'
          GREEN = 'green'
          BLUE = 'blue'

      color = Color(input("Enter your choice of 'red', 'blue' or 'green': "))

      match color:
          case Color.RED:
              print("I see red!")
          case Color.GREEN:
              print("Grass is green")
          case Color.BLUE:
              print("I'm feeling the blues :(")

For a more detailed explanation and additional examples, you can look into
:pep:`636` which is written in a tutorial format.

.. _tut-functions:

Defining Functions
==================

We can create a function that writes the Fibonacci series to an arbitrary
boundary::

   >>> def fib(n):    # write Fibonacci series less than n
   ...     """Print a Fibonacci series less than n."""
   ...     a, b = 0, 1
   ...     while a < n:
   ...         print(a, end=' ')
   ...         a, b = b, a+b
   ...     print()
   ...
   >>> # Now call the function we just defined:
   >>> fib(2000)
   0 1 1 2 3 5 8 13 21 34 55 89 144 233 377 610 987 1597

.. index::
   single: documentation strings
   single: docstrings
   single: strings, documentation

The keyword :keyword:`def` introduces a function *definition*.  It must be
followed by the function name and the parenthesized list of formal parameters.
The statements that form the body of the function start at the next line, and
must be indented.

The first statement of the function body can optionally be a string literal;
this string literal is the function's documentation string, or :dfn:`docstring`.
(More about docstrings can be found in the section :ref:`tut-docstrings`.)
There are tools which use docstrings to automatically produce online or printed
documentation, or to let the user interactively browse through code; it's good
practice to include docstrings in code that you write, so make a habit of it.

The *execution* of a function introduces a new symbol table used for the local
variables of the function.  More precisely, all variable assignments in a
function store the value in the local symbol table; whereas variable references
first look in the local symbol table, then in the local symbol tables of
enclosing functions, then in the global symbol table, and finally in the table
of built-in names. Thus, global variables and variables of enclosing functions
cannot be directly assigned a value within a function (unless, for global
variables, named in a :keyword:`global` statement, or, for variables of enclosing
functions, named in a :keyword:`nonlocal` statement), although they may be
referenced.

The actual parameters (arguments) to a function call are introduced in the local
symbol table of the called function when it is called; thus, arguments are
passed using *call by value* (where the *value* is always an object *reference*,
not the value of the object). [#]_ When a function calls another function,
or calls itself recursively, a new
local symbol table is created for that call.

A function definition associates the function name with the function object in
the current symbol table.  The interpreter recognizes the object pointed to by
that name as a user-defined function.  Other names can also point to that same
function object and can also be used to access the function::

   >>> fib
   <function fib at 10042ed0>
   >>> f = fib
   >>> f(100)
   0 1 1 2 3 5 8 13 21 34 55 89

Coming from other languages, you might object that ``fib`` is not a function but
a procedure since it doesn't return a value.  In fact, even functions without a
:keyword:`return` statement do return a value, albeit a rather boring one.  This
value is called ``None`` (it's a built-in name).  Writing the value ``None`` is
normally suppressed by the interpreter if it would be the only value written.
You can see it if you really want to using :func:`print`::

   >>> fib(0)
   >>> print(fib(0))
   None

It is simple to write a function that returns a list of the numbers of the
Fibonacci series, instead of printing it::

   >>> def fib2(n):  # return Fibonacci series up to n
   ...     """Return a list containing the Fibonacci series up to n."""
   ...     result = []
   ...     a, b = 0, 1
   ...     while a < n:
   ...         result.append(a)    # see below
   ...         a, b = b, a+b
   ...     return result
   ...
   >>> f100 = fib2(100)    # call it
   >>> f100                # write the result
   [0, 1, 1, 2, 3, 5, 8, 13, 21, 34, 55, 89]

This example, as usual, demonstrates some new Python features:

* The :keyword:`return` statement returns with a value from a function.
  :keyword:`!return` without an expression argument returns ``None``. Falling off
  the end of a function also returns ``None``.

* The statement ``result.append(a)`` calls a *method* of the list object
  ``result``.  A method is a function that 'belongs' to an object and is named
  ``obj.methodname``, where ``obj`` is some object (this may be an expression),
  and ``methodname`` is the name of a method that is defined by the object's type.
  Different types define different methods.  Methods of different types may have
  the same name without causing ambiguity.  (It is possible to define your own
  object types and methods, using *classes*, see :ref:`tut-classes`)
  The method :meth:`!append` shown in the example is defined for list objects; it
  adds a new element at the end of the list.  In this example it is equivalent to
  ``result = result + [a]``, but more efficient.


.. _tut-defining:

More on Defining Functions
==========================

It is also possible to define functions with a variable number of arguments.
There are three forms, which can be combined.


.. _tut-defaultargs:

Default Argument Values
-----------------------

The most useful form is to specify a default value for one or more arguments.
This creates a function that can be called with fewer arguments than it is
defined to allow.  For example::

   def ask_ok(prompt, retries=4, reminder='Please try again!'):
       while True:
           reply = input(prompt)
           if reply in {'y', 'ye', 'yes'}:
               return True
           if reply in {'n', 'no', 'nop', 'nope'}:
               return False
           retries = retries - 1
           if retries < 0:
               raise ValueError('invalid user response')
           print(reminder)

This function can be called in several ways:

* giving only the mandatory argument:
  ``ask_ok('Do you really want to quit?')``
* giving one of the optional arguments:
  ``ask_ok('OK to overwrite the file?', 2)``
* or even giving all arguments:
  ``ask_ok('OK to overwrite the file?', 2, 'Come on, only yes or no!')``

This example also introduces the :keyword:`in` keyword. This tests whether or
not a sequence contains a certain value.

The default values are evaluated at the point of function definition in the
*defining* scope, so that ::

   i = 5

   def f(arg=i):
       print(arg)

   i = 6
   f()

will print ``5``.

**Important warning:**  The default value is evaluated only once. This makes a
difference when the default is a mutable object such as a list, dictionary, or
instances of most classes.  For example, the following function accumulates the
arguments passed to it on subsequent calls::

   def f(a, L=[]):
       L.append(a)
       return L

   print(f(1))
   print(f(2))
   print(f(3))

This will print ::

   [1]
   [1, 2]
   [1, 2, 3]

If you don't want the default to be shared between subsequent calls, you can
write the function like this instead::

   def f(a, L=None):
       if L is None:
           L = []
       L.append(a)
       return L


.. _tut-keywordargs:

Keyword Arguments
-----------------

Functions can also be called using :term:`keyword arguments <keyword argument>`
of the form ``kwarg=value``.  For instance, the following function::

   def parrot(voltage, state='a stiff', action='voom', type='Norwegian Blue'):
       print("-- This parrot wouldn't", action, end=' ')
       print("if you put", voltage, "volts through it.")
       print("-- Lovely plumage, the", type)
       print("-- It's", state, "!")

accepts one required argument (``voltage``) and three optional arguments
(``state``, ``action``, and ``type``).  This function can be called in any
of the following ways::

   parrot(1000)                                          # 1 positional argument
   parrot(voltage=1000)                                  # 1 keyword argument
   parrot(voltage=1000000, action='VOOOOOM')             # 2 keyword arguments
   parrot(action='VOOOOOM', voltage=1000000)             # 2 keyword arguments
   parrot('a million', 'bereft of life', 'jump')         # 3 positional arguments
   parrot('a thousand', state='pushing up the daisies')  # 1 positional, 1 keyword

but all the following calls would be invalid::

   parrot()                     # required argument missing
   parrot(voltage=5.0, 'dead')  # non-keyword argument after a keyword argument
   parrot(110, voltage=220)     # duplicate value for the same argument
   parrot(actor='John Cleese')  # unknown keyword argument

In a function call, keyword arguments must follow positional arguments.
All the keyword arguments passed must match one of the arguments
accepted by the function (e.g. ``actor`` is not a valid argument for the
``parrot`` function), and their order is not important.  This also includes
non-optional arguments (e.g. ``parrot(voltage=1000)`` is valid too).
No argument may receive a value more than once.
Here's an example that fails due to this restriction::

   >>> def function(a):
   ...     pass
   ...
   >>> function(0, a=0)
   Traceback (most recent call last):
     File "<stdin>", line 1, in <module>
   TypeError: function() got multiple values for argument 'a'

When a final formal parameter of the form ``**name`` is present, it receives a
dictionary (see :ref:`typesmapping`) containing all keyword arguments except for
those corresponding to a formal parameter.  This may be combined with a formal
parameter of the form ``*name`` (described in the next subsection) which
receives a :ref:`tuple <tut-tuples>` containing the positional
arguments beyond the formal parameter list.  (``*name`` must occur
before ``**name``.) For example, if we define a function like this::

   def cheeseshop(kind, *arguments, **keywords):
       print("-- Do you have any", kind, "?")
       print("-- I'm sorry, we're all out of", kind)
       for arg in arguments:
           print(arg)
       print("-" * 40)
       for kw in keywords:
           print(kw, ":", keywords[kw])

It could be called like this::

   cheeseshop("Limburger", "It's very runny, sir.",
              "It's really very, VERY runny, sir.",
              shopkeeper="Michael Palin",
              client="John Cleese",
              sketch="Cheese Shop Sketch")

and of course it would print:

.. code-block:: none

   -- Do you have any Limburger ?
   -- I'm sorry, we're all out of Limburger
   It's very runny, sir.
   It's really very, VERY runny, sir.
   ----------------------------------------
   shopkeeper : Michael Palin
   client : John Cleese
   sketch : Cheese Shop Sketch

Note that the order in which the keyword arguments are printed is guaranteed
to match the order in which they were provided in the function call.

Special parameters
------------------

By default, arguments may be passed to a Python function either by position
or explicitly by keyword. For readability and performance, it makes sense to
restrict the way arguments can be passed so that a developer need only look
at the function definition to determine if items are passed by position, by
position or keyword, or by keyword.

A function definition may look like:

.. code-block:: none

   def f(pos1, pos2, /, pos_or_kwd, *, kwd1, kwd2):
         -----------    ----------     ----------
           |             |                  |
           |        Positional or keyword   |
           |                                - Keyword only
            -- Positional only

where ``/`` and ``*`` are optional. If used, these symbols indicate the kind of
parameter by how the arguments may be passed to the function:
positional-only, positional-or-keyword, and keyword-only. Keyword parameters
are also referred to as named parameters.

-------------------------------
Positional-or-Keyword Arguments
-------------------------------

If ``/`` and ``*`` are not present in the function definition, arguments may
be passed to a function by position or by keyword.

--------------------------
Positional-Only Parameters
--------------------------

Looking at this in a bit more detail, it is possible to mark certain parameters
as *positional-only*. If *positional-only*, the parameters' order matters, and
the parameters cannot be passed by keyword. Positional-only parameters are
placed before a ``/`` (forward-slash). The ``/`` is used to logically
separate the positional-only parameters from the rest of the parameters.
If there is no ``/`` in the function definition, there are no positional-only
parameters.

Parameters following the ``/`` may be *positional-or-keyword* or *keyword-only*.

----------------------
Keyword-Only Arguments
----------------------

To mark parameters as *keyword-only*, indicating the parameters must be passed
by keyword argument, place an ``*`` in the arguments list just before the first
*keyword-only* parameter.

-----------------
Function Examples
-----------------

Consider the following example function definitions paying close attention to the
markers ``/`` and ``*``::

   >>> def standard_arg(arg):
   ...     print(arg)
   ...
   >>> def pos_only_arg(arg, /):
   ...     print(arg)
   ...
   >>> def kwd_only_arg(*, arg):
   ...     print(arg)
   ...
   >>> def combined_example(pos_only, /, standard, *, kwd_only):
   ...     print(pos_only, standard, kwd_only)


The first function definition, ``standard_arg``, the most familiar form,
places no restrictions on the calling convention and arguments may be
passed by position or keyword::

   >>> standard_arg(2)
   2

   >>> standard_arg(arg=2)
   2

The second function ``pos_only_arg`` is restricted to only use positional
parameters as there is a ``/`` in the function definition::

   >>> pos_only_arg(1)
   1

   >>> pos_only_arg(arg=1)
   Traceback (most recent call last):
     File "<stdin>", line 1, in <module>
   TypeError: pos_only_arg() got some positional-only arguments passed as keyword arguments: 'arg'

The third function ``kwd_only_arg`` only allows keyword arguments as indicated
by a ``*`` in the function definition::

   >>> kwd_only_arg(3)
   Traceback (most recent call last):
     File "<stdin>", line 1, in <module>
   TypeError: kwd_only_arg() takes 0 positional arguments but 1 was given

   >>> kwd_only_arg(arg=3)
   3

And the last uses all three calling conventions in the same function
definition::

   >>> combined_example(1, 2, 3)
   Traceback (most recent call last):
     File "<stdin>", line 1, in <module>
   TypeError: combined_example() takes 2 positional arguments but 3 were given

   >>> combined_example(1, 2, kwd_only=3)
   1 2 3

   >>> combined_example(1, standard=2, kwd_only=3)
   1 2 3

   >>> combined_example(pos_only=1, standard=2, kwd_only=3)
   Traceback (most recent call last):
     File "<stdin>", line 1, in <module>
   TypeError: combined_example() got some positional-only arguments passed as keyword arguments: 'pos_only'


Finally, consider this function definition which has a potential collision between the positional argument ``name``  and ``**kwds`` which has ``name`` as a key::

    def foo(name, **kwds):
        return 'name' in kwds

There is no possible call that will make it return ``True`` as the keyword ``'name'``
will always bind to the first parameter. For example::

    >>> foo(1, **{'name': 2})
    Traceback (most recent call last):
      File "<stdin>", line 1, in <module>
    TypeError: foo() got multiple values for argument 'name'
    >>>

But using ``/`` (positional only arguments), it is possible since it allows ``name`` as a positional argument and ``'name'`` as a key in the keyword arguments::

    >>> def foo(name, /, **kwds):
    ...     return 'name' in kwds
    ...
    >>> foo(1, **{'name': 2})
    True

In other words, the names of positional-only parameters can be used in
``**kwds`` without ambiguity.

-----
Recap
-----

The use case will determine which parameters to use in the function definition::

   def f(pos1, pos2, /, pos_or_kwd, *, kwd1, kwd2):

As guidance:

* Use positional-only if you want the name of the parameters to not be
  available to the user. This is useful when parameter names have no real
  meaning, if you want to enforce the order of the arguments when the function
  is called or if you need to take some positional parameters and arbitrary
  keywords.
* Use keyword-only when names have meaning and the function definition is
  more understandable by being explicit with names or you want to prevent
  users relying on the position of the argument being passed.
* For an API, use positional-only to prevent breaking API changes
  if the parameter's name is modified in the future.

.. _tut-arbitraryargs:

Arbitrary Argument Lists
------------------------

.. index::
   single: * (asterisk); in function calls

Finally, the least frequently used option is to specify that a function can be
called with an arbitrary number of arguments.  These arguments will be wrapped
up in a tuple (see :ref:`tut-tuples`).  Before the variable number of arguments,
zero or more normal arguments may occur. ::

   def write_multiple_items(file, separator, *args):
       file.write(separator.join(args))


Normally, these *variadic* arguments will be last in the list of formal
parameters, because they scoop up all remaining input arguments that are
passed to the function. Any formal parameters which occur after the ``*args``
parameter are 'keyword-only' arguments, meaning that they can only be used as
keywords rather than positional arguments. ::

   >>> def concat(*args, sep="/"):
   ...     return sep.join(args)
   ...
   >>> concat("earth", "mars", "venus")
   'earth/mars/venus'
   >>> concat("earth", "mars", "venus", sep=".")
   'earth.mars.venus'

.. _tut-unpacking-arguments:

Unpacking Argument Lists
------------------------

The reverse situation occurs when the arguments are already in a list or tuple
but need to be unpacked for a function call requiring separate positional
arguments.  For instance, the built-in :func:`range` function expects separate
*start* and *stop* arguments.  If they are not available separately, write the
function call with the  ``*``\ -operator to unpack the arguments out of a list
or tuple::

   >>> list(range(3, 6))            # normal call with separate arguments
   [3, 4, 5]
   >>> args = [3, 6]
   >>> list(range(*args))            # call with arguments unpacked from a list
   [3, 4, 5]

.. index::
   single: **; in function calls

In the same fashion, dictionaries can deliver keyword arguments with the
``**``\ -operator::

   >>> def parrot(voltage, state='a stiff', action='voom'):
   ...     print("-- This parrot wouldn't", action, end=' ')
   ...     print("if you put", voltage, "volts through it.", end=' ')
   ...     print("E's", state, "!")
   ...
   >>> d = {"voltage": "four million", "state": "bleedin' demised", "action": "VOOM"}
   >>> parrot(**d)
   -- This parrot wouldn't VOOM if you put four million volts through it. E's bleedin' demised !


.. _tut-lambda:

Lambda Expressions
------------------

Small anonymous functions can be created with the :keyword:`lambda` keyword.
This function returns the sum of its two arguments: ``lambda a, b: a+b``.
Lambda functions can be used wherever function objects are required.  They are
syntactically restricted to a single expression.  Semantically, they are just
syntactic sugar for a normal function definition.  Like nested function
definitions, lambda functions can reference variables from the containing
scope::

   >>> def make_incrementor(n):
   ...     return lambda x: x + n
   ...
   >>> f = make_incrementor(42)
   >>> f(0)
   42
   >>> f(1)
   43

The above example uses a lambda expression to return a function.  Another use
is to pass a small function as an argument.  For instance, :meth:`list.sort`
takes a sorting key function *key* which can be a lambda function::

   >>> pairs = [(1, 'one'), (2, 'two'), (3, 'three'), (4, 'four')]
   >>> pairs.sort(key=lambda pair: pair[1])
   >>> pairs
   [(4, 'four'), (1, 'one'), (3, 'three'), (2, 'two')]


.. _tut-docstrings:

Documentation Strings
---------------------

.. index::
   single: docstrings
   single: documentation strings
   single: strings, documentation

Here are some conventions about the content and formatting of documentation
strings.

The first line should always be a short, concise summary of the object's
purpose.  For brevity, it should not explicitly state the object's name or type,
since these are available by other means (except if the name happens to be a
verb describing a function's operation).  This line should begin with a capital
letter and end with a period.

If there are more lines in the documentation string, the second line should be
blank, visually separating the summary from the rest of the description.  The
following lines should be one or more paragraphs describing the object's calling
conventions, its side effects, etc.

The Python parser does not strip indentation from multi-line string literals in
Python, so tools that process documentation have to strip indentation if
desired.  This is done using the following convention. The first non-blank line
*after* the first line of the string determines the amount of indentation for
the entire documentation string.  (We can't use the first line since it is
generally adjacent to the string's opening quotes so its indentation is not
apparent in the string literal.)  Whitespace "equivalent" to this indentation is
then stripped from the start of all lines of the string.  Lines that are
indented less should not occur, but if they occur all their leading whitespace
should be stripped.  Equivalence of whitespace should be tested after expansion
of tabs (to 8 spaces, normally).

Here is an example of a multi-line docstring::

   >>> def my_function():
   ...     """Do nothing, but document it.
   ...
   ...     No, really, it doesn't do anything.
   ...     """
   ...     pass
   ...
   >>> print(my_function.__doc__)
   Do nothing, but document it.

   No, really, it doesn't do anything.


.. _tut-annotations:

Function Annotations
--------------------

.. sectionauthor:: Zachary Ware <zachary.ware@gmail.com>
.. index::
   pair: function; annotations
   single: ->; function annotations
   single: : (colon); function annotations

:ref:`Function annotations <function>` are completely optional metadata
information about the types used by user-defined functions (see :pep:`3107` and
:pep:`484` for more information).

:term:`Annotations <function annotation>` are stored in the :attr:`!__annotations__`
attribute of the function as a dictionary and have no effect on any other part of the
function.  Parameter annotations are defined by a colon after the parameter name, followed
by an expression evaluating to the value of the annotation.  Return annotations are
defined by a literal ``->``, followed by an expression, between the parameter
list and the colon denoting the end of the :keyword:`def` statement.  The
following example has a required argument, an optional argument, and the return
value annotated::

   >>> def f(ham: str, eggs: str = 'eggs') -> str:
   ...     print("Annotations:", f.__annotations__)
   ...     print("Arguments:", ham, eggs)
   ...     return ham + ' and ' + eggs
   ...
   >>> f('spam')
   Annotations: {'ham': <class 'str'>, 'return': <class 'str'>, 'eggs': <class 'str'>}
   Arguments: spam eggs
   'spam and eggs'

.. _tut-codingstyle:

Intermezzo: Coding Style
========================

.. sectionauthor:: Georg Brandl <georg@python.org>
.. index:: pair: coding; style

Now that you are about to write longer, more complex pieces of Python, it is a
good time to talk about *coding style*.  Most languages can be written (or more
concise, *formatted*) in different styles; some are more readable than others.
Making it easy for others to read your code is always a good idea, and adopting
a nice coding style helps tremendously for that.

For Python, :pep:`8` has emerged as the style guide that most projects adhere to;
it promotes a very readable and eye-pleasing coding style.  Every Python
developer should read it at some point; here are the most important points
extracted for you:

* Use 4-space indentation, and no tabs.

  4 spaces are a good compromise between small indentation (allows greater
  nesting depth) and large indentation (easier to read).  Tabs introduce
  confusion, and are best left out.

* Wrap lines so that they don't exceed 79 characters.

  This helps users with small displays and makes it possible to have several
  code files side-by-side on larger displays.

* Use blank lines to separate functions and classes, and larger blocks of
  code inside functions.

* When possible, put comments on a line of their own.

* Use docstrings.

* Use spaces around operators and after commas, but not directly inside
  bracketing constructs: ``a = f(1, 2) + g(3, 4)``.

* Name your classes and functions consistently; the convention is to use
  ``UpperCamelCase`` for classes and ``lowercase_with_underscores`` for functions
  and methods.  Always use ``self`` as the name for the first method argument
  (see :ref:`tut-firstclasses` for more on classes and methods).

* Don't use fancy encodings if your code is meant to be used in international
  environments.  Python's default, UTF-8, or even plain ASCII work best in any
  case.

* Likewise, don't use non-ASCII characters in identifiers if there is only the
  slightest chance people speaking a different language will read or maintain
  the code.


.. rubric:: Footnotes

.. [#] Actually, *call by object reference* would be a better description,
   since if a mutable object is passed, the caller will see any changes the
   callee makes to it (items inserted into a list).
