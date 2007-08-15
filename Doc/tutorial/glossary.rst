
.. _tut-glossary:

********
Glossary
********

.. % %% keep the entries sorted and include at least one \index{} item for each
.. % %% cross-references are marked with \emph{entry}

``>>>``
   The typical Python prompt of the interactive shell.  Often seen for code
   examples that can be tried right away in the interpreter.

   .. index:: single: ...

``...``
   The typical Python prompt of the interactive shell when entering code for an
   indented code block.

   .. index:: single: BDFL

BDFL
   Benevolent Dictator For Life, a.k.a. `Guido van Rossum
   <http://www.python.org/~guido/>`_, Python's creator.

   .. index:: single: byte code

byte code
   The internal representation of a Python program in the interpreter. The byte
   code is also cached in ``.pyc`` and ``.pyo`` files so that executing the same
   file is faster the second time (recompilation from source to byte code can be
   avoided).  This "intermediate language" is said to run on a "virtual machine"
   that calls the subroutines corresponding to each bytecode.

   .. index:: single: classic class

classic class
   Any class which does not inherit from :class:`object`.  See *new-style class*.

   .. index:: single: complex number

complex number
   An extension of the familiar real number system in which all numbers are
   expressed as a sum of a real part and an imaginary part.  Imaginary numbers are
   real multiples of the imaginary unit (the square root of ``-1``), often written
   ``i`` in mathematics or ``j`` in engineering. Python has builtin support for
   complex numbers, which are written with this latter notation; the imaginary part
   is written with a ``j`` suffix, e.g., ``3+1j``.  To get access to complex
   equivalents of the :mod:`math` module, use :mod:`cmath`.  Use of complex numbers
   is a fairly advanced mathematical feature.  If you're not aware of a need for
   them, it's almost certain you can safely ignore them.

   .. index:: single: descriptor

descriptor
   Any *new-style* object that defines the methods :meth:`__get__`,
   :meth:`__set__`, or :meth:`__delete__`. When a class attribute is a descriptor,
   its special binding behavior is triggered upon attribute lookup.  Normally,
   writing *a.b* looks up the object *b* in the class dictionary for *a*, but if
   *b* is a descriptor, the defined method gets called. Understanding descriptors
   is a key to a deep understanding of Python because they are the basis for many
   features including functions, methods, properties, class methods, static
   methods, and reference to super classes.

   .. index:: single: dictionary

dictionary
   An associative array, where arbitrary keys are mapped to values.  The use of
   :class:`dict` much resembles that for :class:`list`, but the keys can be any
   object with a :meth:`__hash__` function, not just integers starting from zero.
   Called a hash in Perl.

   .. index:: single: duck-typing

duck-typing
   Pythonic programming style that determines an object's type by inspection of its
   method or attribute signature rather than by explicit relationship to some type
   object ("If it looks like a duck and quacks like a duck, it must be a duck.")
   By emphasizing interfaces rather than specific types, well-designed code
   improves its flexibility by allowing polymorphic substitution.  Duck-typing
   avoids tests using :func:`type` or :func:`isinstance`. Instead, it typically
   employs :func:`hasattr` tests or *EAFP* programming.

   .. index:: single: EAFP

EAFP
   Easier to ask for forgiveness than permission.  This common Python coding style
   assumes the existence of valid keys or attributes and catches exceptions if the
   assumption proves false.  This clean and fast style is characterized by the
   presence of many :keyword:`try` and :keyword:`except` statements.  The technique
   contrasts with the *LBYL* style that is common in many other languages such as
   C.

   .. index:: single: __future__

__future__
   A pseudo module which programmers can use to enable new language features which
   are not compatible with the current interpreter. To enable ``new_feature`` ::

      from __future__ import new_feature

   By importing the :mod:`__future__` module and evaluating its variables, you
   can see when a new feature was first added to the language and when it will
   become the default::

      >>> import __future__
      >>> __future__.division
      _Feature((2, 2, 0, 'alpha', 2), (3, 0, 0, 'alpha', 0), 8192)

   .. index:: single: generator

generator
   A function that returns an iterator.  It looks like a normal function except
   that values are returned to the caller using a :keyword:`yield` statement
   instead of a :keyword:`return` statement.  Generator functions often contain one
   or more :keyword:`for` or :keyword:`while` loops that :keyword:`yield` elements
   back to the caller.  The function execution is stopped at the :keyword:`yield`
   keyword (returning the result) and is resumed there when the next element is
   requested by calling the :meth:`__next__` method of the returned iterator.

   .. index:: single: generator expression

generator expression
   An expression that returns a generator.  It looks like a normal expression
   followed by a :keyword:`for` expression defining a loop variable, range, and an
   optional :keyword:`if` expression.  The combined expression generates values for
   an enclosing function::

      >>> sum(i*i for i in range(10))         # sum of squares 0, 1, 4, ... 81
      285

   .. index:: single: GIL

GIL
   See *global interpreter lock*.

   .. index:: single: global interpreter lock

global interpreter lock
   The lock used by Python threads to assure that only one thread can be run at
   a time.  This simplifies Python by assuring that no two processes can access
   the same memory at the same time.  Locking the entire interpreter makes it
   easier for the interpreter to be multi-threaded, at the expense of some
   parallelism on multi-processor machines.  Efforts have been made in the past
   to create a "free-threaded" interpreter (one which locks shared data at a
   much finer granularity), but performance suffered in the common
   single-processor case.

   .. index:: single: IDLE

IDLE
   An Integrated Development Environment for Python.  IDLE is a basic editor and
   interpreter environment that ships with the standard distribution of Python.
   Good for beginners, it also serves as clear example code for those wanting to
   implement a moderately sophisticated, multi-platform GUI application.

   .. index:: single: immutable

immutable
   An object with fixed value.  Immutable objects are numbers, strings or tuples
   (and more).  Such an object cannot be altered.  A new object has to be created
   if a different value has to be stored.  They play an important role in places
   where a constant hash value is needed, for example as a key in a dictionary.

   .. index:: single: integer division

integer division
   Mathematical division including any remainder.  The result will always be a
   float.  For example, the expression ``11/4`` evaluates to ``2.75``. Integer
   division can be forced by using the ``//`` operator instead of the ``/``
   operator.

   .. index:: single: interactive

interactive
   Python has an interactive interpreter which means that you can try out things
   and immediately see their results.  Just launch ``python`` with no arguments
   (possibly by selecting it from your computer's main menu). It is a very powerful
   way to test out new ideas or inspect modules and packages (remember
   ``help(x)``).

   .. index:: single: interpreted

interpreted
   Python is an interpreted language, as opposed to a compiled one.  This means
   that the source files can be run directly without first creating an executable
   which is then run.  Interpreted languages typically have a shorter
   development/debug cycle than compiled ones, though their programs generally also
   run more slowly.  See also *interactive*.

   .. index:: single: iterable

iterable
   A container object capable of returning its members one at a time. Examples of
   iterables include all sequence types (such as :class:`list`, :class:`str`, and
   :class:`tuple`) and some non-sequence types like :class:`dict` and :class:`file`
   and objects of any classes you define with an :meth:`__iter__` or
   :meth:`__getitem__` method.  Iterables can be used in a :keyword:`for` loop and
   in many other places where a sequence is needed (:func:`zip`, :func:`map`, ...).
   When an iterable object is passed as an argument to the builtin function
   :func:`iter`, it returns an iterator for the object.  This iterator is good for
   one pass over the set of values.  When using iterables, it is usually not
   necessary to call :func:`iter` or deal with iterator objects yourself.  The
   ``for`` statement does that automatically for you, creating a temporary unnamed
   variable to hold the iterator for the duration of the loop.  See also
   *iterator*, *sequence*, and *generator*.

   .. index:: single: iterator

iterator
   An object representing a stream of data.  Repeated calls to the iterator's
   :meth:`__next__` method return successive items in the stream.  When no more
   data is available a :exc:`StopIteration` exception is raised instead.  At this
   point, the iterator object is exhausted and any further calls to its
   :meth:`__next__` method just raise :exc:`StopIteration` again.  Iterators are
   required to have an :meth:`__iter__` method that returns the iterator object
   itself so every iterator is also iterable and may be used in most places where
   other iterables are accepted.  One notable exception is code that attempts
   multiple iteration passes.  A container object (such as a :class:`list`)
   produces a fresh new iterator each time you pass it to the :func:`iter` function
   or use it in a :keyword:`for` loop.  Attempting this with an iterator will just
   return the same exhausted iterator object used in the previous iteration pass,
   making it appear like an empty container.

   .. index:: single: LBYL

LBYL
   Look before you leap.  This coding style explicitly tests for pre-conditions
   before making calls or lookups.  This style contrasts with the *EAFP* approach
   and is characterized by the presence of many :keyword:`if` statements.

   .. index:: single: list comprehension

list comprehension
   A compact way to process all or a subset of elements in a sequence and return a
   list with the results.  ``result = ["0x%02x" % x for x in range(256) if x % 2 ==
   0]`` generates a list of strings containing hex numbers (0x..) that are even and
   in the range from 0 to 255. The :keyword:`if` clause is optional.  If omitted,
   all elements in ``range(256)`` are processed.

   .. index:: single: mapping

mapping
   A container object (such as :class:`dict`) that supports arbitrary key lookups
   using the special method :meth:`__getitem__`.

   .. index:: single: metaclass

metaclass
   The class of a class.  Class definitions create a class name, a class
   dictionary, and a list of base classes.  The metaclass is responsible for taking
   those three arguments and creating the class.  Most object oriented programming
   languages provide a default implementation.  What makes Python special is that
   it is possible to create custom metaclasses.  Most users never need this tool,
   but when the need arises, metaclasses can provide powerful, elegant solutions.
   They have been used for logging attribute access, adding thread-safety, tracking
   object creation, implementing singletons, and many other tasks.

   .. index:: single: mutable

mutable
   Mutable objects can change their value but keep their :func:`id`. See also
   *immutable*.

   .. index:: single: namespace

namespace
   The place where a variable is stored.  Namespaces are implemented as
   dictionaries.  There are the local, global and builtin namespaces as well as
   nested namespaces in objects (in methods).  Namespaces support modularity by
   preventing naming conflicts.  For instance, the functions
   :func:`__builtin__.open` and :func:`os.open` are distinguished by their
   namespaces.  Namespaces also aid readability and maintainability by making it
   clear which module implements a function.  For instance, writing
   :func:`random.seed` or :func:`itertools.izip` makes it clear that those
   functions are implemented by the :mod:`random` and :mod:`itertools` modules
   respectively.

   .. index:: single: nested scope

nested scope
   The ability to refer to a variable in an enclosing definition.  For instance, a
   function defined inside another function can refer to variables in the outer
   function.  Note that nested scopes work only for reference and not for
   assignment which will always write to the innermost scope.  In contrast, local
   variables both read and write in the innermost scope.  Likewise, global
   variables read and write to the global namespace.

   .. index:: single: new-style class

new-style class
   Any class that inherits from :class:`object`.  This includes all built-in types
   like :class:`list` and :class:`dict`.  Only new-style classes can use Python's
   newer, versatile features like :meth:`__slots__`, descriptors, properties,
   :meth:`__getattribute__`, class methods, and static methods.

   .. index:: single: Python3000

Python3000
   A mythical python release, not required to be backward compatible, with
   telepathic interface.

   .. index:: single: __slots__

__slots__
   A declaration inside a *new-style class* that saves memory by pre-declaring
   space for instance attributes and eliminating instance dictionaries.  Though
   popular, the technique is somewhat tricky to get right and is best reserved for
   rare cases where there are large numbers of instances in a memory-critical
   application.

   .. index:: single: sequence

sequence
   An *iterable* which supports efficient element access using integer indices via
   the :meth:`__getitem__` and :meth:`__len__` special methods.  Some built-in
   sequence types are :class:`list`, :class:`str`, :class:`tuple`, and
   :class:`unicode`. Note that :class:`dict` also supports :meth:`__getitem__` and
   :meth:`__len__`, but is considered a mapping rather than a sequence because the
   lookups use arbitrary *immutable* keys rather than integers.

   .. index:: single: Zen of Python

Zen of Python
   Listing of Python design principles and philosophies that are helpful in
   understanding and using the language.  The listing can be found by typing
   "``import this``" at the interactive prompt.

