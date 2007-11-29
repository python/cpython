
.. _datamodel:

**********
Data model
**********


.. _objects:

Objects, values and types
=========================

.. index::
   single: object
   single: data

:dfn:`Objects` are Python's abstraction for data.  All data in a Python program
is represented by objects or by relations between objects. (In a sense, and in
conformance to Von Neumann's model of a "stored program computer," code is also
represented by objects.)

.. index::
   builtin: id
   builtin: type
   single: identity of an object
   single: value of an object
   single: type of an object
   single: mutable object
   single: immutable object

.. XXX it *is* now possible in some cases to change an object's
   type, under certain controlled conditions

Every object has an identity, a type and a value.  An object's *identity* never
changes once it has been created; you may think of it as the object's address in
memory.  The ':keyword:`is`' operator compares the identity of two objects; the
:func:`id` function returns an integer representing its identity (currently
implemented as its address). An object's :dfn:`type` is also unchangeable.
An object's type determines the operations that the object supports (e.g., "does
it have a length?") and also defines the possible values for objects of that
type.  The :func:`type` function returns an object's type (which is an object
itself).  The *value* of some objects can change.  Objects whose value can
change are said to be *mutable*; objects whose value is unchangeable once they
are created are called *immutable*. (The value of an immutable container object
that contains a reference to a mutable object can change when the latter's value
is changed; however the container is still considered immutable, because the
collection of objects it contains cannot be changed.  So, immutability is not
strictly the same as having an unchangeable value, it is more subtle.) An
object's mutability is determined by its type; for instance, numbers, strings
and tuples are immutable, while dictionaries and lists are mutable.

.. index::
   single: garbage collection
   single: reference counting
   single: unreachable object

Objects are never explicitly destroyed; however, when they become unreachable
they may be garbage-collected.  An implementation is allowed to postpone garbage
collection or omit it altogether --- it is a matter of implementation quality
how garbage collection is implemented, as long as no objects are collected that
are still reachable.  (Implementation note: the current implementation uses a
reference-counting scheme with (optional) delayed detection of cyclically linked
garbage, which collects most objects as soon as they become unreachable, but is
not guaranteed to collect garbage containing circular references.  See the
documentation of the :mod:`gc` module for information on controlling the
collection of cyclic garbage.)

Note that the use of the implementation's tracing or debugging facilities may
keep objects alive that would normally be collectable. Also note that catching
an exception with a ':keyword:`try`...\ :keyword:`except`' statement may keep
objects alive.

Some objects contain references to "external" resources such as open files or
windows.  It is understood that these resources are freed when the object is
garbage-collected, but since garbage collection is not guaranteed to happen,
such objects also provide an explicit way to release the external resource,
usually a :meth:`close` method. Programs are strongly recommended to explicitly
close such objects.  The ':keyword:`try`...\ :keyword:`finally`' statement
provides a convenient way to do this.

.. index:: single: container

Some objects contain references to other objects; these are called *containers*.
Examples of containers are tuples, lists and dictionaries.  The references are
part of a container's value.  In most cases, when we talk about the value of a
container, we imply the values, not the identities of the contained objects;
however, when we talk about the mutability of a container, only the identities
of the immediately contained objects are implied.  So, if an immutable container
(like a tuple) contains a reference to a mutable object, its value changes if
that mutable object is changed.

Types affect almost all aspects of object behavior.  Even the importance of
object identity is affected in some sense: for immutable types, operations that
compute new values may actually return a reference to any existing object with
the same type and value, while for mutable objects this is not allowed.  E.g.,
after ``a = 1; b = 1``, ``a`` and ``b`` may or may not refer to the same object
with the value one, depending on the implementation, but after ``c = []; d =
[]``, ``c`` and ``d`` are guaranteed to refer to two different, unique, newly
created empty lists. (Note that ``c = d = []`` assigns the same object to both
``c`` and ``d``.)


.. _types:

The standard type hierarchy
===========================

.. index::
   single: type
   pair: data; type
   pair: type; hierarchy
   pair: extension; module
   pair: C; language

Below is a list of the types that are built into Python.  Extension modules
(written in C, Java, or other languages, depending on the implementation) can
define additional types.  Future versions of Python may add types to the type
hierarchy (e.g., rational numbers, efficiently stored arrays of integers, etc.).

.. index::
   single: attribute
   pair: special; attribute
   triple: generic; special; attribute

Some of the type descriptions below contain a paragraph listing 'special
attributes.'  These are attributes that provide access to the implementation and
are not intended for general use.  Their definition may change in the future.

None
   .. index:: object: None

   This type has a single value.  There is a single object with this value. This
   object is accessed through the built-in name ``None``. It is used to signify the
   absence of a value in many situations, e.g., it is returned from functions that
   don't explicitly return anything. Its truth value is false.

NotImplemented
   .. index:: object: NotImplemented

   This type has a single value.  There is a single object with this value. This
   object is accessed through the built-in name ``NotImplemented``. Numeric methods
   and rich comparison methods may return this value if they do not implement the
   operation for the operands provided.  (The interpreter will then try the
   reflected operation, or some other fallback, depending on the operator.)  Its
   truth value is true.

Ellipsis
   .. index:: object: Ellipsis

   This type has a single value.  There is a single object with this value. This
   object is accessed through the literal ``...`` or the built-in name
   ``Ellipsis``.  Its truth value is true.

Numbers
   .. index:: object: numeric

   These are created by numeric literals and returned as results by arithmetic
   operators and arithmetic built-in functions.  Numeric objects are immutable;
   once created their value never changes.  Python numbers are of course strongly
   related to mathematical numbers, but subject to the limitations of numerical
   representation in computers.

   Python distinguishes between integers, floating point numbers, and complex
   numbers:

   Integers
      .. index:: object: integer

      These represent elements from the mathematical set of integers (positive and
      negative).

      There are three types of integers:

      Plain integers
         .. index::
            object: plain integer
            single: OverflowError (built-in exception)

         These represent numbers in an unlimited range, subject to available (virtual)
         memory only.  For the purpose of shift and mask operations, a binary
         representation is assumed, and negative numbers are represented in a variant of
         2's complement which gives the illusion of an infinite string of sign bits
         extending to the left.

      Booleans
         .. index::
            object: Boolean
            single: False
            single: True

         These represent the truth values False and True.  The two objects representing
         the values False and True are the only Boolean objects. The Boolean type is a
         subtype of plain integers, and Boolean values behave like the values 0 and 1,
         respectively, in almost all contexts, the exception being that when converted to
         a string, the strings ``"False"`` or ``"True"`` are returned, respectively.

      .. index:: pair: integer; representation

      The rules for integer representation are intended to give the most meaningful
      interpretation of shift and mask operations involving negative integers.  Any
      operation except left shift, if it yields a result in the plain integer domain
      without causing overflow, will yield the same result when using mixed operands.

      .. % Integers

   Floating point numbers
      .. index::
         object: floating point
         pair: floating point; number
         pair: C; language
         pair: Java; language

      These represent machine-level double precision floating point numbers. You are
      at the mercy of the underlying machine architecture (and C or Java
      implementation) for the accepted range and handling of overflow. Python does not
      support single-precision floating point numbers; the savings in processor and
      memory usage that are usually the reason for using these is dwarfed by the
      overhead of using objects in Python, so there is no reason to complicate the
      language with two kinds of floating point numbers.

   Complex numbers
      .. index::
         object: complex
         pair: complex; number

      These represent complex numbers as a pair of machine-level double precision
      floating point numbers.  The same caveats apply as for floating point numbers.
      The real and imaginary parts of a complex number ``z`` can be retrieved through
      the read-only attributes ``z.real`` and ``z.imag``.

   .. % Numbers

Sequences
   .. index::
      builtin: len
      object: sequence
      single: index operation
      single: item selection
      single: subscription

   These represent finite ordered sets indexed by non-negative numbers. The
   built-in function :func:`len` returns the number of items of a sequence. When
   the length of a sequence is *n*, the index set contains the numbers 0, 1,
   ..., *n*-1.  Item *i* of sequence *a* is selected by ``a[i]``.

   .. index:: single: slicing

   Sequences also support slicing: ``a[i:j]`` selects all items with index *k* such
   that *i* ``<=`` *k* ``<`` *j*.  When used as an expression, a slice is a
   sequence of the same type.  This implies that the index set is renumbered so
   that it starts at 0.

   Some sequences also support "extended slicing" with a third "step" parameter:
   ``a[i:j:k]`` selects all items of *a* with index *x* where ``x = i + n*k``, *n*
   ``>=`` ``0`` and *i* ``<=`` *x* ``<`` *j*.

   Sequences are distinguished according to their mutability:

   Immutable sequences
      .. index::
         object: immutable sequence
         object: immutable

      An object of an immutable sequence type cannot change once it is created.  (If
      the object contains references to other objects, these other objects may be
      mutable and may be changed; however, the collection of objects directly
      referenced by an immutable object cannot change.)

      The following types are immutable sequences:

      Strings
         .. index::
            builtin: chr
            builtin: ord
            builtin: str
            single: character
            single: integer
            single: Unicode

         The items of a string object are Unicode code units.  A Unicode code
         unit is represented by a string object of one item and can hold either
         a 16-bit or 32-bit value representing a Unicode ordinal (the maximum
         value for the ordinal is given in ``sys.maxunicode``, and depends on
         how Python is configured at compile time).  Surrogate pairs may be
         present in the Unicode object, and will be reported as two separate
         items.  The built-in functions :func:`chr` and :func:`ord` convert
         between code units and nonnegative integers representing the Unicode
         ordinals as defined in the Unicode Standard 3.0. Conversion from and to
         other encodings are possible through the string method :meth:`encode`.

      Tuples
         .. index::
            object: tuple
            pair: singleton; tuple
            pair: empty; tuple

         The items of a tuple are arbitrary Python objects. Tuples of two or
         more items are formed by comma-separated lists of expressions.  A tuple
         of one item (a 'singleton') can be formed by affixing a comma to an
         expression (an expression by itself does not create a tuple, since
         parentheses must be usable for grouping of expressions).  An empty
         tuple can be formed by an empty pair of parentheses.

      .. % Immutable sequences

   Mutable sequences
      .. index::
         object: mutable sequence
         object: mutable
         pair: assignment; statement
         single: delete
         statement: del
         single: subscription
         single: slicing

      Mutable sequences can be changed after they are created.  The subscription and
      slicing notations can be used as the target of assignment and :keyword:`del`
      (delete) statements.

      There is currently a single intrinsic mutable sequence type:

      Lists
         .. index:: object: list

         The items of a list are arbitrary Python objects.  Lists are formed by
         placing a comma-separated list of expressions in square brackets. (Note
         that there are no special cases needed to form lists of length 0 or 1.)

      Bytes
         .. index:: bytes, byte

         A bytes object is a mutable array.  The items are 8-bit bytes,
         represented by integers in the range 0 <= x < 256.  Bytes literals
         (like ``b'abc'`` and the built-in function :func:`bytes` can be used to
         construct bytes objects.  Also, bytes objects can be decoded to strings
         via the :meth:`decode` method.

      .. index:: module: array

      The extension module :mod:`array` provides an additional example of a
      mutable sequence type.

      .. % Mutable sequences

   .. % Sequences

Set types
   .. index::
      builtin: len
      object: set type

   These represent unordered, finite sets of unique, immutable objects. As such,
   they cannot be indexed by any subscript. However, they can be iterated over, and
   the built-in function :func:`len` returns the number of items in a set. Common
   uses for sets are fast membership testing, removing duplicates from a sequence,
   and computing mathematical operations such as intersection, union, difference,
   and symmetric difference.

   For set elements, the same immutability rules apply as for dictionary keys. Note
   that numeric types obey the normal rules for numeric comparison: if two numbers
   compare equal (e.g., ``1`` and ``1.0``), only one of them can be contained in a
   set.

   There are currently two intrinsic set types:

   Sets
      .. index:: object: set

      These represent a mutable set. They are created by the built-in :func:`set`
      constructor and can be modified afterwards by several methods, such as
      :meth:`add`.

   Frozen sets
      .. index:: object: frozenset

      These represent an immutable set.  They are created by the built-in
      :func:`frozenset` constructor.  As a frozenset is immutable and
      :term:`hashable`, it can be used again as an element of another set, or as
      a dictionary key.

   .. % Set types

Mappings
   .. index::
      builtin: len
      single: subscription
      object: mapping

   These represent finite sets of objects indexed by arbitrary index sets. The
   subscript notation ``a[k]`` selects the item indexed by ``k`` from the mapping
   ``a``; this can be used in expressions and as the target of assignments or
   :keyword:`del` statements. The built-in function :func:`len` returns the number
   of items in a mapping.

   There is currently a single intrinsic mapping type:

   Dictionaries
      .. index:: object: dictionary

      These represent finite sets of objects indexed by nearly arbitrary values.  The
      only types of values not acceptable as keys are values containing lists or
      dictionaries or other mutable types that are compared by value rather than by
      object identity, the reason being that the efficient implementation of
      dictionaries requires a key's hash value to remain constant. Numeric types used
      for keys obey the normal rules for numeric comparison: if two numbers compare
      equal (e.g., ``1`` and ``1.0``) then they can be used interchangeably to index
      the same dictionary entry.

      Dictionaries are mutable; they can be created by the ``{...}`` notation (see
      section :ref:`dict`).

      .. index::
         module: dbm
         module: gdbm
         module: bsddb

      The extension modules :mod:`dbm`, :mod:`gdbm`, and :mod:`bsddb` provide
      additional examples of mapping types.

   .. % Mapping types

Callable types
   .. index::
      object: callable
      pair: function; call
      single: invocation
      pair: function; argument

   These are the types to which the function call operation (see section
   :ref:`calls`) can be applied:

   User-defined functions
      .. index::
         pair: user-defined; function
         object: function
         object: user-defined function

      A user-defined function object is created by a function definition (see
      section :ref:`function`).  It should be called with an argument list
      containing the same number of items as the function's formal parameter
      list.

      Special attributes:

      +-------------------------+-------------------------------+-----------+
      | Attribute               | Meaning                       |           |
      +=========================+===============================+===========+
      | :attr:`__doc__`         | The function's documentation  | Writable  |
      |                         | string, or ``None`` if        |           |
      |                         | unavailable                   |           |
      +-------------------------+-------------------------------+-----------+
      | :attr:`__name__`        | The function's name           | Writable  |
      +-------------------------+-------------------------------+-----------+
      | :attr:`__module__`      | The name of the module the    | Writable  |
      |                         | function was defined in, or   |           |
      |                         | ``None`` if unavailable.      |           |
      +-------------------------+-------------------------------+-----------+
      | :attr:`__defaults__`    | A tuple containing default    | Writable  |
      |                         | argument values for those     |           |
      |                         | arguments that have defaults, |           |
      |                         | or ``None`` if no arguments   |           |
      |                         | have a default value          |           |
      +-------------------------+-------------------------------+-----------+
      | :attr:`__code__`        | The code object representing  | Writable  |
      |                         | the compiled function body.   |           |
      +-------------------------+-------------------------------+-----------+
      | :attr:`__globals__`     | A reference to the dictionary | Read-only |
      |                         | that holds the function's     |           |
      |                         | global variables --- the      |           |
      |                         | global namespace of the       |           |
      |                         | module in which the function  |           |
      |                         | was defined.                  |           |
      +-------------------------+-------------------------------+-----------+
      | :attr:`__dict__`        | The namespace supporting      | Writable  |
      |                         | arbitrary function            |           |
      |                         | attributes.                   |           |
      +-------------------------+-------------------------------+-----------+
      | :attr:`__closure__`     | ``None`` or a tuple of cells  | Read-only |
      |                         | that contain bindings for the |           |
      |                         | function's free variables.    |           |
      +-------------------------+-------------------------------+-----------+
      | :attr:`__annotations__` | A dict containing annotations | Writable  |
      |                         | of parameters.  The keys of   |           |
      |                         | the dict are the parameter    |           |
      |                         | names, or ``'return'`` for    |           |
      |                         | the return annotation, if     |           |
      |                         | provided.                     |           |
      +-------------------------+-------------------------------+-----------+
      | :attr:`__kwdefaults__`  | A dict containing defaults    | Writable  |
      |                         | for keyword-only parameters.  |           |
      +-------------------------+-------------------------------+-----------+

      Most of the attributes labelled "Writable" check the type of the assigned value.

      Function objects also support getting and setting arbitrary attributes, which
      can be used, for example, to attach metadata to functions.  Regular attribute
      dot-notation is used to get and set such attributes. *Note that the current
      implementation only supports function attributes on user-defined functions.
      Function attributes on built-in functions may be supported in the future.*

      Additional information about a function's definition can be retrieved from its
      code object; see the description of internal types below.

      .. index::
         single: __doc__ (function attribute)
         single: __name__ (function attribute)
         single: __module__ (function attribute)
         single: __dict__ (function attribute)
         single: __defaults__ (function attribute)
         single: __closure__ (function attribute)
         single: __code__ (function attribute)
         single: __globals__ (function attribute)
         single: __annotations__ (function attribute)
         single: __kwdefaults__ (function attribute)
         pair: global; namespace

   Instance methods
      .. index::
         object: method
         object: user-defined method
         pair: user-defined; method

      An instance method object combines a class, a class instance and any
      callable object (normally a user-defined function).

      .. index::
         single: __func__ (method attribute)
         single: __self__ (method attribute)
         single: __doc__ (method attribute)
         single: __name__ (method attribute)
         single: __module__ (method attribute)

      Special read-only attributes: :attr:`__self__` is the class instance object,
      :attr:`__func__` is the function object; :attr:`__doc__` is the method's
      documentation (same as ``__func__.__doc__``); :attr:`__name__` is the
      method name (same as ``__func__.__name__``); :attr:`__module__` is the
      name of the module the method was defined in, or ``None`` if unavailable.

      Methods also support accessing (but not setting) the arbitrary function
      attributes on the underlying function object.

      User-defined method objects may be created when getting an attribute of a
      class (perhaps via an instance of that class), if that attribute is a
      user-defined function object or a class method object.
      
      When an instance method object is created by retrieving a user-defined
      function object from a class via one of its instances, its
      :attr:`__self__` attribute is the instance, and the method object is said
      to be bound.  The new method's :attr:`__func__` attribute is the original
      function object.

      When a user-defined method object is created by retrieving another method
      object from a class or instance, the behaviour is the same as for a
      function object, except that the :attr:`__func__` attribute of the new
      instance is not the original method object but its :attr:`__func__`
      attribute.

      When an instance method object is created by retrieving a class method
      object from a class or instance, its :attr:`__self__` attribute is the
      class itself, and its :attr:`__func__` attribute is the function object
      underlying the class method.

      When an instance method object is called, the underlying function
      (:attr:`__func__`) is called, inserting the class instance
      (:attr:`__self__`) in front of the argument list.  For instance, when
      :class:`C` is a class which contains a definition for a function
      :meth:`f`, and ``x`` is an instance of :class:`C`, calling ``x.f(1)`` is
      equivalent to calling ``C.f(x, 1)``.

      When an instance method object is derived from a class method object, the
      "class instance" stored in :attr:`__self__` will actually be the class
      itself, so that calling either ``x.f(1)`` or ``C.f(1)`` is equivalent to
      calling ``f(C,1)`` where ``f`` is the underlying function.

      Note that the transformation from function object to instance method
      object happens each time the attribute is retrieved from the instance.  In
      some cases, a fruitful optimization is to assign the attribute to a local
      variable and call that local variable. Also notice that this
      transformation only happens for user-defined functions; other callable
      objects (and all non-callable objects) are retrieved without
      transformation.  It is also important to note that user-defined functions
      which are attributes of a class instance are not converted to bound
      methods; this *only* happens when the function is an attribute of the
      class.

   Generator functions
      .. index::
         single: generator; function
         single: generator; iterator

      A function or method which uses the :keyword:`yield` statement (see section
      :ref:`yield`) is called a :dfn:`generator
      function`.  Such a function, when called, always returns an iterator object
      which can be used to execute the body of the function:  calling the iterator's
      :meth:`__next__` method will cause the function to execute until it provides a
      value using the :keyword:`yield` statement.  When the function executes a
      :keyword:`return` statement or falls off the end, a :exc:`StopIteration`
      exception is raised and the iterator will have reached the end of the set of
      values to be returned.

   Built-in functions
      .. index::
         object: built-in function
         object: function
         pair: C; language

      A built-in function object is a wrapper around a C function.  Examples of
      built-in functions are :func:`len` and :func:`math.sin` (:mod:`math` is a
      standard built-in module). The number and type of the arguments are
      determined by the C function. Special read-only attributes:
      :attr:`__doc__` is the function's documentation string, or ``None`` if
      unavailable; :attr:`__name__` is the function's name; :attr:`__self__` is
      set to ``None`` (but see the next item); :attr:`__module__` is the name of
      the module the function was defined in or ``None`` if unavailable.

   Built-in methods
      .. index::
         object: built-in method
         object: method
         pair: built-in; method

      This is really a different disguise of a built-in function, this time containing
      an object passed to the C function as an implicit extra argument.  An example of
      a built-in method is ``alist.append()``, assuming *alist* is a list object. In
      this case, the special read-only attribute :attr:`__self__` is set to the object
      denoted by *list*.

   Classes
      Classes are callable.  These objects normally act as factories for new
      instances of themselves, but variations are possible for class types that
      override :meth:`__new__`.  The arguments of the call are passed to
      :meth:`__new__` and, in the typical case, to :meth:`__init__` to
      initialize the new instance.

   Class Instances
      Instances of arbitrary classes can be made callable by defining a
      :meth:`__call__` method in their class.


Modules
   .. index::
      statement: import
      object: module

   Modules are imported by the :keyword:`import` statement (see section
   :ref:`import`). A module object has a
   namespace implemented by a dictionary object (this is the dictionary referenced
   by the __globals__ attribute of functions defined in the module).  Attribute
   references are translated to lookups in this dictionary, e.g., ``m.x`` is
   equivalent to ``m.__dict__["x"]``. A module object does not contain the code
   object used to initialize the module (since it isn't needed once the
   initialization is done).

   .. % 

   Attribute assignment updates the module's namespace dictionary, e.g., ``m.x =
   1`` is equivalent to ``m.__dict__["x"] = 1``.

   .. index:: single: __dict__ (module attribute)

   Special read-only attribute: :attr:`__dict__` is the module's namespace as a
   dictionary object.

   .. index::
      single: __name__ (module attribute)
      single: __doc__ (module attribute)
      single: __file__ (module attribute)
      pair: module; namespace

   Predefined (writable) attributes: :attr:`__name__` is the module's name;
   :attr:`__doc__` is the module's documentation string, or ``None`` if
   unavailable; :attr:`__file__` is the pathname of the file from which the module
   was loaded, if it was loaded from a file. The :attr:`__file__` attribute is not
   present for C modules that are statically linked into the interpreter; for
   extension modules loaded dynamically from a shared library, it is the pathname
   of the shared library file.

.. XXX "Classes" and "Instances" is outdated!
   see http://www.python.org/doc/newstyle.html for newstyle information

Custom classes
   Class objects are created by class definitions (see section :ref:`class`).  A
   class has a namespace implemented by a dictionary object. Class attribute
   references are translated to lookups in this dictionary, e.g., ``C.x`` is
   translated to ``C.__dict__["x"]``. When the attribute name is not found
   there, the attribute search continues in the base classes.  The search is
   depth-first, left-to-right in the order of occurrence in the base class list.

   .. XXX document descriptors and new MRO

   .. index::
      object: class
      object: class instance
      object: instance
      pair: class object; call
      single: container
      object: dictionary
      pair: class; attribute

   When a class attribute reference (for class :class:`C`, say) would yield a
   class method object, it is transformed into an instance method object whose
   :attr:`__self__` attributes is :class:`C`.  When it would yield a static
   method object, it is transformed into the object wrapped by the static method
   object. See section :ref:`descriptors` for another way in which attributes
   retrieved from a class may differ from those actually contained in its
   :attr:`__dict__`.

   .. index:: triple: class; attribute; assignment

   Class attribute assignments update the class's dictionary, never the dictionary
   of a base class.

   .. index:: pair: class object; call

   A class object can be called (see above) to yield a class instance (see below).

   .. index::
      single: __name__ (class attribute)
      single: __module__ (class attribute)
      single: __dict__ (class attribute)
      single: __bases__ (class attribute)
      single: __doc__ (class attribute)

   Special attributes: :attr:`__name__` is the class name; :attr:`__module__` is
   the module name in which the class was defined; :attr:`__dict__` is the
   dictionary containing the class's namespace; :attr:`__bases__` is a tuple
   (possibly empty or a singleton) containing the base classes, in the order of
   their occurrence in the base class list; :attr:`__doc__` is the class's
   documentation string, or None if undefined.

Class instances
   .. index::
      object: class instance
      object: instance
      pair: class; instance
      pair: class instance; attribute

   A class instance is created by calling a class object (see above).  A class
   instance has a namespace implemented as a dictionary which is the first place
   in which attribute references are searched.  When an attribute is not found
   there, and the instance's class has an attribute by that name, the search
   continues with the class attributes.  If a class attribute is found that is a
   user-defined function object, it is transformed into an instance method
   object whose :attr:`__self__` attribute is the instance.  Static method and
   class method objects are also transformed; see above under "Classes".  See
   section :ref:`descriptors` for another way in which attributes of a class
   retrieved via its instances may differ from the objects actually stored in
   the class's :attr:`__dict__`.  If no class attribute is found, and the
   object's class has a :meth:`__getattr__` method, that is called to satisfy
   the lookup.

   .. index:: triple: class instance; attribute; assignment

   Attribute assignments and deletions update the instance's dictionary, never a
   class's dictionary.  If the class has a :meth:`__setattr__` or
   :meth:`__delattr__` method, this is called instead of updating the instance
   dictionary directly.

   .. index::
      object: numeric
      object: sequence
      object: mapping

   Class instances can pretend to be numbers, sequences, or mappings if they have
   methods with certain special names.  See section :ref:`specialnames`.

   .. index::
      single: __dict__ (instance attribute)
      single: __class__ (instance attribute)

   Special attributes: :attr:`__dict__` is the attribute dictionary;
   :attr:`__class__` is the instance's class.

Files
   .. index::
      object: file
      builtin: open
      single: popen() (in module os)
      single: makefile() (socket method)
      single: sys.stdin
      single: sys.stdout
      single: sys.stderr
      single: stdio
      single: stdin (in module sys)
      single: stdout (in module sys)
      single: stderr (in module sys)

   A file object represents an open file.  File objects are created by the
   :func:`open` built-in function, and also by :func:`os.popen`,
   :func:`os.fdopen`, and the :meth:`makefile` method of socket objects (and
   perhaps by other functions or methods provided by extension modules).  The
   objects ``sys.stdin``, ``sys.stdout`` and ``sys.stderr`` are initialized to
   file objects corresponding to the interpreter's standard input, output and
   error streams.  See :ref:`bltin-file-objects` for complete documentation of
   file objects.

Internal types
   .. index::
      single: internal type
      single: types, internal

   A few types used internally by the interpreter are exposed to the user. Their
   definitions may change with future versions of the interpreter, but they are
   mentioned here for completeness.

   Code objects
      .. index::
         single: bytecode
         object: code

      Code objects represent *byte-compiled* executable Python code, or :term:`bytecode`.
      The difference between a code object and a function object is that the function
      object contains an explicit reference to the function's globals (the module in
      which it was defined), while a code object contains no context; also the default
      argument values are stored in the function object, not in the code object
      (because they represent values calculated at run-time).  Unlike function
      objects, code objects are immutable and contain no references (directly or
      indirectly) to mutable objects.

      Special read-only attributes: :attr:`co_name` gives the function name;
      :attr:`co_argcount` is the number of positional arguments (including arguments
      with default values); :attr:`co_nlocals` is the number of local variables used
      by the function (including arguments); :attr:`co_varnames` is a tuple containing
      the names of the local variables (starting with the argument names);
      :attr:`co_cellvars` is a tuple containing the names of local variables that are
      referenced by nested functions; :attr:`co_freevars` is a tuple containing the
      names of free variables; :attr:`co_code` is a string representing the sequence
      of bytecode instructions; :attr:`co_consts` is a tuple containing the literals
      used by the bytecode; :attr:`co_names` is a tuple containing the names used by
      the bytecode; :attr:`co_filename` is the filename from which the code was
      compiled; :attr:`co_firstlineno` is the first line number of the function;
      :attr:`co_lnotab` is a string encoding the mapping from bytecode offsets to
      line numbers (for details see the source code of the interpreter);
      :attr:`co_stacksize` is the required stack size (including local variables);
      :attr:`co_flags` is an integer encoding a number of flags for the interpreter.

      .. index::
         single: co_argcount (code object attribute)
         single: co_code (code object attribute)
         single: co_consts (code object attribute)
         single: co_filename (code object attribute)
         single: co_firstlineno (code object attribute)
         single: co_flags (code object attribute)
         single: co_lnotab (code object attribute)
         single: co_name (code object attribute)
         single: co_names (code object attribute)
         single: co_nlocals (code object attribute)
         single: co_stacksize (code object attribute)
         single: co_varnames (code object attribute)
         single: co_cellvars (code object attribute)
         single: co_freevars (code object attribute)

      .. index:: object: generator

      The following flag bits are defined for :attr:`co_flags`: bit ``0x04`` is set if
      the function uses the ``*arguments`` syntax to accept an arbitrary number of
      positional arguments; bit ``0x08`` is set if the function uses the
      ``**keywords`` syntax to accept arbitrary keyword arguments; bit ``0x20`` is set
      if the function is a generator.

      Future feature declarations (``from __future__ import division``) also use bits
      in :attr:`co_flags` to indicate whether a code object was compiled with a
      particular feature enabled: bit ``0x2000`` is set if the function was compiled
      with future division enabled; bits ``0x10`` and ``0x1000`` were used in earlier
      versions of Python.

      Other bits in :attr:`co_flags` are reserved for internal use.

      .. index:: single: documentation string

      If a code object represents a function, the first item in :attr:`co_consts` is
      the documentation string of the function, or ``None`` if undefined.

   Frame objects
      .. index:: object: frame

      Frame objects represent execution frames.  They may occur in traceback objects
      (see below).

      .. index::
         single: f_back (frame attribute)
         single: f_code (frame attribute)
         single: f_globals (frame attribute)
         single: f_locals (frame attribute)
         single: f_lasti (frame attribute)
         single: f_builtins (frame attribute)

      Special read-only attributes: :attr:`f_back` is to the previous stack frame
      (towards the caller), or ``None`` if this is the bottom stack frame;
      :attr:`f_code` is the code object being executed in this frame; :attr:`f_locals`
      is the dictionary used to look up local variables; :attr:`f_globals` is used for
      global variables; :attr:`f_builtins` is used for built-in (intrinsic) names;
      :attr:`f_lasti` gives the precise instruction (this is an index into the
      bytecode string of the code object).

      .. index::
         single: f_trace (frame attribute)
         single: f_exc_type (frame attribute)
         single: f_exc_value (frame attribute)
         single: f_exc_traceback (frame attribute)
         single: f_lineno (frame attribute)

      Special writable attributes: :attr:`f_trace`, if not ``None``, is a function
      called at the start of each source code line (this is used by the debugger);
      :attr:`f_exc_type`, :attr:`f_exc_value`, :attr:`f_exc_traceback` represent the
      last exception raised in the parent frame provided another exception was ever
      raised in the current frame (in all other cases they are None); :attr:`f_lineno`
      is the current line number of the frame --- writing to this from within a trace
      function jumps to the given line (only for the bottom-most frame).  A debugger
      can implement a Jump command (aka Set Next Statement) by writing to f_lineno.

   Traceback objects
      .. index::
         object: traceback
         pair: stack; trace
         pair: exception; handler
         pair: execution; stack
         single: exc_info (in module sys)
         single: exc_traceback (in module sys)
         single: last_traceback (in module sys)
         single: sys.exc_info
         single: sys.last_traceback

      Traceback objects represent a stack trace of an exception.  A traceback object
      is created when an exception occurs.  When the search for an exception handler
      unwinds the execution stack, at each unwound level a traceback object is
      inserted in front of the current traceback.  When an exception handler is
      entered, the stack trace is made available to the program. (See section
      :ref:`try`.) It is accessible as the third item of the
      tuple returned by ``sys.exc_info()``. When the program contains no suitable
      handler, the stack trace is written (nicely formatted) to the standard error
      stream; if the interpreter is interactive, it is also made available to the user
      as ``sys.last_traceback``.

      .. index::
         single: tb_next (traceback attribute)
         single: tb_frame (traceback attribute)
         single: tb_lineno (traceback attribute)
         single: tb_lasti (traceback attribute)
         statement: try

      Special read-only attributes: :attr:`tb_next` is the next level in the stack
      trace (towards the frame where the exception occurred), or ``None`` if there is
      no next level; :attr:`tb_frame` points to the execution frame of the current
      level; :attr:`tb_lineno` gives the line number where the exception occurred;
      :attr:`tb_lasti` indicates the precise instruction.  The line number and last
      instruction in the traceback may differ from the line number of its frame object
      if the exception occurred in a :keyword:`try` statement with no matching except
      clause or with a finally clause.

   Slice objects
      .. index:: builtin: slice

      Slice objects are used to represent slices for :meth:`__getitem__`
      methods.  They are also created by the built-in :func:`slice` function.

      .. index::
         single: start (slice object attribute)
         single: stop (slice object attribute)
         single: step (slice object attribute)

      Special read-only attributes: :attr:`start` is the lower bound; :attr:`stop` is
      the upper bound; :attr:`step` is the step value; each is ``None`` if omitted.
      These attributes can have any type.

      Slice objects support one method:

      .. method:: slice.indices(self, length)

         This method takes a single integer argument *length* and computes
         information about the slice that the slice object would describe if
         applied to a sequence of *length* items.  It returns a tuple of three
         integers; respectively these are the *start* and *stop* indices and the
         *step* or stride length of the slice. Missing or out-of-bounds indices
         are handled in a manner consistent with regular slices.

   Static method objects
      Static method objects provide a way of defeating the transformation of function
      objects to method objects described above. A static method object is a wrapper
      around any other object, usually a user-defined method object. When a static
      method object is retrieved from a class or a class instance, the object actually
      returned is the wrapped object, which is not subject to any further
      transformation. Static method objects are not themselves callable, although the
      objects they wrap usually are. Static method objects are created by the built-in
      :func:`staticmethod` constructor.

   Class method objects
      A class method object, like a static method object, is a wrapper around another
      object that alters the way in which that object is retrieved from classes and
      class instances. The behaviour of class method objects upon such retrieval is
      described above, under "User-defined methods". Class method objects are created
      by the built-in :func:`classmethod` constructor.

   .. % Internal types

.. % =========================================================================

.. _newstyle:

.. _specialnames:

Special method names
====================

.. index::
   pair: operator; overloading
   single: __getitem__() (mapping object method)

A class can implement certain operations that are invoked by special syntax
(such as arithmetic operations or subscripting and slicing) by defining methods
with special names. This is Python's approach to :dfn:`operator overloading`,
allowing classes to define their own behavior with respect to language
operators.  For instance, if a class defines a method named :meth:`__getitem__`,
and ``x`` is an instance of this class, then ``x[i]`` is equivalent to
``x.__getitem__(i)``.  Except where mentioned, attempts to execute an operation
raise an exception when no appropriate method is defined.

.. XXX above translation is not correct for new-style classes!

Special methods are only guaranteed to work if defined in an object's class, not
in the object's instance dictionary.  That explains why this won't work::

   >>> class C:
   ...     pass
   ...
   >>> c = C()
   >>> c.__len__ = lambda: 5
   >>> len(c)
   Traceback (most recent call last):
     File "<stdin>", line 1, in <module>
   TypeError: object of type 'C' has no len()


When implementing a class that emulates any built-in type, it is important that
the emulation only be implemented to the degree that it makes sense for the
object being modelled.  For example, some sequences may work well with retrieval
of individual elements, but extracting a slice may not make sense.  (One example
of this is the :class:`NodeList` interface in the W3C's Document Object Model.)


.. _customization:

Basic customization
-------------------


.. method:: object.__new__(cls[, ...])

   Called to create a new instance of class *cls*.  :meth:`__new__` is a static
   method (special-cased so you need not declare it as such) that takes the class
   of which an instance was requested as its first argument.  The remaining
   arguments are those passed to the object constructor expression (the call to the
   class).  The return value of :meth:`__new__` should be the new object instance
   (usually an instance of *cls*).

   Typical implementations create a new instance of the class by invoking the
   superclass's :meth:`__new__` method using ``super(currentclass,
   cls).__new__(cls[, ...])`` with appropriate arguments and then modifying the
   newly-created instance as necessary before returning it.

   If :meth:`__new__` returns an instance of *cls*, then the new instance's
   :meth:`__init__` method will be invoked like ``__init__(self[, ...])``, where
   *self* is the new instance and the remaining arguments are the same as were
   passed to :meth:`__new__`.

   If :meth:`__new__` does not return an instance of *cls*, then the new instance's
   :meth:`__init__` method will not be invoked.

   :meth:`__new__` is intended mainly to allow subclasses of immutable types (like
   int, str, or tuple) to customize instance creation.


.. method:: object.__init__(self[, ...])

   .. index:: pair: class; constructor

   Called when the instance is created.  The arguments are those passed to the
   class constructor expression.  If a base class has an :meth:`__init__` method,
   the derived class's :meth:`__init__` method, if any, must explicitly call it to
   ensure proper initialization of the base class part of the instance; for
   example: ``BaseClass.__init__(self, [args...])``.  As a special constraint on
   constructors, no value may be returned; doing so will cause a :exc:`TypeError`
   to be raised at runtime.


.. method:: object.__del__(self)

   .. index::
      single: destructor
      statement: del

   Called when the instance is about to be destroyed.  This is also called a
   destructor.  If a base class has a :meth:`__del__` method, the derived class's
   :meth:`__del__` method, if any, must explicitly call it to ensure proper
   deletion of the base class part of the instance.  Note that it is possible
   (though not recommended!) for the :meth:`__del__` method to postpone destruction
   of the instance by creating a new reference to it.  It may then be called at a
   later time when this new reference is deleted.  It is not guaranteed that
   :meth:`__del__` methods are called for objects that still exist when the
   interpreter exits.

   .. note::

      ``del x`` doesn't directly call ``x.__del__()`` --- the former decrements
      the reference count for ``x`` by one, and the latter is only called when
      ``x``'s reference count reaches zero.  Some common situations that may
      prevent the reference count of an object from going to zero include:
      circular references between objects (e.g., a doubly-linked list or a tree
      data structure with parent and child pointers); a reference to the object
      on the stack frame of a function that caught an exception (the traceback
      stored in ``sys.exc_info()[2]`` keeps the stack frame alive); or a
      reference to the object on the stack frame that raised an unhandled
      exception in interactive mode (the traceback stored in
      ``sys.last_traceback`` keeps the stack frame alive).  The first situation
      can only be remedied by explicitly breaking the cycles; the latter two
      situations can be resolved by storing ``None`` in ``sys.last_traceback``.
      Circular references which are garbage are detected when the option cycle
      detector is enabled (it's on by default), but can only be cleaned up if
      there are no Python- level :meth:`__del__` methods involved. Refer to the
      documentation for the :mod:`gc` module for more information about how
      :meth:`__del__` methods are handled by the cycle detector, particularly
      the description of the ``garbage`` value.

   .. warning::

      Due to the precarious circumstances under which :meth:`__del__` methods are
      invoked, exceptions that occur during their execution are ignored, and a warning
      is printed to ``sys.stderr`` instead.  Also, when :meth:`__del__` is invoked in
      response to a module being deleted (e.g., when execution of the program is
      done), other globals referenced by the :meth:`__del__` method may already have
      been deleted.  For this reason, :meth:`__del__` methods should do the absolute
      minimum needed to maintain external invariants.  Starting with version 1.5,
      Python guarantees that globals whose name begins with a single underscore are
      deleted from their module before other globals are deleted; if no other
      references to such globals exist, this may help in assuring that imported
      modules are still available at the time when the :meth:`__del__` method is
      called.


.. method:: object.__repr__(self)

   .. index:: builtin: repr

   Called by the :func:`repr` built-in function and by string conversions (reverse
   quotes) to compute the "official" string representation of an object.  If at all
   possible, this should look like a valid Python expression that could be used to
   recreate an object with the same value (given an appropriate environment).  If
   this is not possible, a string of the form ``<...some useful description...>``
   should be returned.  The return value must be a string object. If a class
   defines :meth:`__repr__` but not :meth:`__str__`, then :meth:`__repr__` is also
   used when an "informal" string representation of instances of that class is
   required.

   This is typically used for debugging, so it is important that the representation
   is information-rich and unambiguous.


.. method:: object.__str__(self)

   .. index::
      builtin: str
      builtin: print

   Called by the :func:`str` built-in function and by the :func:`print` function
   to compute the "informal" string representation of an object.  This differs
   from :meth:`__repr__` in that it does not have to be a valid Python
   expression: a more convenient or concise representation may be used instead.
   The return value must be a string object.

   .. XXX what about subclasses of string?


.. method:: object.__format__(self, format_spec)

   .. index::
      pair: string; conversion
      builtin: str
      builtin: print

   Called by the :func:`format` built-in function (and by extension, the
   :meth:`format` method of class :class:`str`) to produce a "formatted"
   string representation of an object. The ``format_spec`` argument is
   a string that contains a description of the formatting options desired.
   The interpretation of the ``format_spec`` argument is up to the type
   implementing :meth:`__format__`, however most classes will either
   delegate formatting to one of the built-in types, or use a similar
   formatting option syntax.
   
   See :ref:`formatspec` for a description of the standard formatting syntax.

   The return value must be a string object.


.. method:: object.__lt__(self, other)
            object.__le__(self, other)
            object.__eq__(self, other)
            object.__ne__(self, other)
            object.__gt__(self, other)
            object.__ge__(self, other)

   .. index::
      single: comparisons

   These are the so-called "rich comparison" methods, and are called for comparison
   operators in preference to :meth:`__cmp__` below. The correspondence between
   operator symbols and method names is as follows: ``x<y`` calls ``x.__lt__(y)``,
   ``x<=y`` calls ``x.__le__(y)``, ``x==y`` calls ``x.__eq__(y)``, ``x!=y`` calls
   ``x.__ne__(y)``, ``x>y`` calls ``x.__gt__(y)``, and ``x>=y`` calls
   ``x.__ge__(y)``.

   A rich comparison method may return the singleton ``NotImplemented`` if it does
   not implement the operation for a given pair of arguments. By convention,
   ``False`` and ``True`` are returned for a successful comparison. However, these
   methods can return any value, so if the comparison operator is used in a Boolean
   context (e.g., in the condition of an ``if`` statement), Python will call
   :func:`bool` on the value to determine if the result is true or false.

   There are no implied relationships among the comparison operators. The truth
   of ``x==y`` does not imply that ``x!=y`` is false.  Accordingly, when
   defining :meth:`__eq__`, one should also define :meth:`__ne__` so that the
   operators will behave as expected.  See the paragraph on :meth:`__hash__` for
   some important notes on creating :term:`hashable` objects which support
   custom comparison operations and are usable as dictionary keys.

   There are no swapped-argument versions of these methods (to be used when the
   left argument does not support the operation but the right argument does);
   rather, :meth:`__lt__` and :meth:`__gt__` are each other's reflection,
   :meth:`__le__` and :meth:`__ge__` are each other's reflection, and
   :meth:`__eq__` and :meth:`__ne__` are their own reflection.

   Arguments to rich comparison methods are never coerced.


.. method:: object.__cmp__(self, other)

   .. index::
      builtin: cmp
      single: comparisons

   Called by comparison operations if rich comparison (see above) is not
   defined.  Should return a negative integer if ``self < other``, zero if
   ``self == other``, a positive integer if ``self > other``.  If no
   :meth:`__cmp__`, :meth:`__eq__` or :meth:`__ne__` operation is defined, class
   instances are compared by object identity ("address").  See also the
   description of :meth:`__hash__` for some important notes on creating
   :term:`hashable` objects which support custom comparison operations and are
   usable as dictionary keys.


.. method:: object.__hash__(self)

   .. index::
      object: dictionary
      builtin: hash
      single: __cmp__() (object method)

   Called for the key object for dictionary operations, and by the built-in
   function :func:`hash`.  Should return an integer usable as a hash value
   for dictionary operations.  The only required property is that objects which
   compare equal have the same hash value; it is advised to somehow mix together
   (e.g., using exclusive or) the hash values for the components of the object that
   also play a part in comparison of objects.

   If a class does not define a :meth:`__cmp__` or :meth:`__eq__` method it
   should not define a :meth:`__hash__` operation either; if it defines
   :meth:`__cmp__` or :meth:`__eq__` but not :meth:`__hash__`, its instances
   will not be usable as dictionary keys.  If a class defines mutable objects
   and implements a :meth:`__cmp__` or :meth:`__eq__` method, it should not
   implement :meth:`__hash__`, since the dictionary implementation requires that
   a key's hash value is immutable (if the object's hash value changes, it will
   be in the wrong hash bucket).

   User-defined classes have :meth:`__cmp__` and :meth:`__hash__` methods
   by default; with them, all objects compare unequal and ``x.__hash__()``
   returns ``id(x)``.


.. method:: object.__bool__(self)

   .. index:: single: __len__() (mapping object method)

   Called to implement truth value testing, and the built-in operation ``bool()``;
   should return ``False`` or ``True``. When this method is not defined,
   :meth:`__len__` is called, if it is defined (see below) and ``True`` is returned
   when the length is not zero.  If a class defines neither :meth:`__len__` nor
   :meth:`__bool__`, all its instances are considered true.


.. _attribute-access:

Customizing attribute access
----------------------------

The following methods can be defined to customize the meaning of attribute
access (use of, assignment to, or deletion of ``x.name``) for class instances.

.. XXX explain how descriptors interfere here!


.. method:: object.__getattr__(self, name)

   Called when an attribute lookup has not found the attribute in the usual places
   (i.e. it is not an instance attribute nor is it found in the class tree for
   ``self``).  ``name`` is the attribute name. This method should return the
   (computed) attribute value or raise an :exc:`AttributeError` exception.

   Note that if the attribute is found through the normal mechanism,
   :meth:`__getattr__` is not called.  (This is an intentional asymmetry between
   :meth:`__getattr__` and :meth:`__setattr__`.) This is done both for efficiency
   reasons and because otherwise :meth:`__setattr__` would have no way to access
   other attributes of the instance.  Note that at least for instance variables,
   you can fake total control by not inserting any values in the instance attribute
   dictionary (but instead inserting them in another object).  See the
   :meth:`__getattribute__` method below for a way to actually get total control
   over attribute access.


.. method:: object.__getattribute__(self, name)

   Called unconditionally to implement attribute accesses for instances of the
   class. If the class also defines :meth:`__getattr__`, the latter will not be
   called unless :meth:`__getattribute__` either calls it explicitly or raises an
   :exc:`AttributeError`. This method should return the (computed) attribute value
   or raise an :exc:`AttributeError` exception. In order to avoid infinite
   recursion in this method, its implementation should always call the base class
   method with the same name to access any attributes it needs, for example,
   ``object.__getattribute__(self, name)``.


.. method:: object.__setattr__(self, name, value)

   Called when an attribute assignment is attempted.  This is called instead of
   the normal mechanism (i.e. store the value in the instance dictionary).
   *name* is the attribute name, *value* is the value to be assigned to it.

   If :meth:`__setattr__` wants to assign to an instance attribute, it should
   call the base class method with the same name, for example,
   ``object.__setattr__(self, name, value)``.


.. method:: object.__delattr__(self, name)

   Like :meth:`__setattr__` but for attribute deletion instead of assignment.  This
   should only be implemented if ``del obj.name`` is meaningful for the object.


.. _descriptors:

Implementing Descriptors
^^^^^^^^^^^^^^^^^^^^^^^^

The following methods only apply when an instance of the class containing the
method (a so-called *descriptor* class) appears in the class dictionary of
another class, known as the *owner* class.  In the examples below, "the
attribute" refers to the attribute whose name is the key of the property in the
owner class' :attr:`__dict__`.


.. method:: object.__get__(self, instance, owner)

   Called to get the attribute of the owner class (class attribute access) or of an
   instance of that class (instance attribute access). *owner* is always the owner
   class, while *instance* is the instance that the attribute was accessed through,
   or ``None`` when the attribute is accessed through the *owner*.  This method
   should return the (computed) attribute value or raise an :exc:`AttributeError`
   exception.


.. method:: object.__set__(self, instance, value)

   Called to set the attribute on an instance *instance* of the owner class to a
   new value, *value*.


.. method:: object.__delete__(self, instance)

   Called to delete the attribute on an instance *instance* of the owner class.


.. _descriptor-invocation:

Invoking Descriptors
^^^^^^^^^^^^^^^^^^^^

In general, a descriptor is an object attribute with "binding behavior", one
whose attribute access has been overridden by methods in the descriptor
protocol:  :meth:`__get__`, :meth:`__set__`, and :meth:`__delete__`. If any of
those methods are defined for an object, it is said to be a descriptor.

The default behavior for attribute access is to get, set, or delete the
attribute from an object's dictionary. For instance, ``a.x`` has a lookup chain
starting with ``a.__dict__['x']``, then ``type(a).__dict__['x']``, and
continuing through the base classes of ``type(a)`` excluding metaclasses.

However, if the looked-up value is an object defining one of the descriptor
methods, then Python may override the default behavior and invoke the descriptor
method instead.  Where this occurs in the precedence chain depends on which
descriptor methods were defined and how they were called.  Note that descriptors
are only invoked for new style objects or classes (ones that subclass
:class:`object()` or :class:`type()`).

The starting point for descriptor invocation is a binding, ``a.x``. How the
arguments are assembled depends on ``a``:

Direct Call
   The simplest and least common call is when user code directly invokes a
   descriptor method:    ``x.__get__(a)``.

Instance Binding
   If binding to an object instance, ``a.x`` is transformed into the call:
   ``type(a).__dict__['x'].__get__(a, type(a))``.

Class Binding
   If binding to a class, ``A.x`` is transformed into the call:
   ``A.__dict__['x'].__get__(None, A)``.

Super Binding
   If ``a`` is an instance of :class:`super`, then the binding ``super(B,
   obj).m()`` searches ``obj.__class__.__mro__`` for the base class ``A``
   immediately preceding ``B`` and then invokes the descriptor with the call:
   ``A.__dict__['m'].__get__(obj, A)``.

For instance bindings, the precedence of descriptor invocation depends on the
which descriptor methods are defined.  Normally, data descriptors define both
:meth:`__get__` and :meth:`__set__`, while non-data descriptors have just the
:meth:`__get__` method.  Data descriptors always override a redefinition in an
instance dictionary.  In contrast, non-data descriptors can be overridden by
instances. [#]_

Python methods (including :func:`staticmethod` and :func:`classmethod`) are
implemented as non-data descriptors.  Accordingly, instances can redefine and
override methods.  This allows individual instances to acquire behaviors that
differ from other instances of the same class.

The :func:`property` function is implemented as a data descriptor. Accordingly,
instances cannot override the behavior of a property.


.. _slots:

__slots__
^^^^^^^^^

By default, instances of classes have a dictionary for attribute storage.  This
wastes space for objects having very few instance variables.  The space
consumption can become acute when creating large numbers of instances.

The default can be overridden by defining *__slots__* in a class definition.
The *__slots__* declaration takes a sequence of instance variables and reserves
just enough space in each instance to hold a value for each variable.  Space is
saved because *__dict__* is not created for each instance.


.. data:: object.__slots__

   This class variable can be assigned a string, iterable, or sequence of
   strings with variable names used by instances.  If defined in a new-style
   class, *__slots__* reserves space for the declared variables and prevents the
   automatic creation of *__dict__* and *__weakref__* for each instance.


Notes on using *__slots__*
""""""""""""""""""""""""""

* Without a *__dict__* variable, instances cannot be assigned new variables not
  listed in the *__slots__* definition.  Attempts to assign to an unlisted
  variable name raises :exc:`AttributeError`. If dynamic assignment of new
  variables is desired, then add ``'__dict__'`` to the sequence of strings in
  the *__slots__* declaration.

* Without a *__weakref__* variable for each instance, classes defining
  *__slots__* do not support weak references to its instances. If weak reference
  support is needed, then add ``'__weakref__'`` to the sequence of strings in the
  *__slots__* declaration.

* *__slots__* are implemented at the class level by creating descriptors
  (:ref:`descriptors`) for each variable name.  As a result, class attributes
  cannot be used to set default values for instance variables defined by
  *__slots__*; otherwise, the class attribute would overwrite the descriptor
  assignment.

* If a class defines a slot also defined in a base class, the instance variable
  defined by the base class slot is inaccessible (except by retrieving its
  descriptor directly from the base class). This renders the meaning of the
  program undefined.  In the future, a check may be added to prevent this.

* The action of a *__slots__* declaration is limited to the class where it is
  defined.  As a result, subclasses will have a *__dict__* unless they also define
  *__slots__*.

* *__slots__* do not work for classes derived from "variable-length" built-in
  types such as :class:`int`, :class:`str` and :class:`tuple`.

* Any non-string iterable may be assigned to *__slots__*. Mappings may also be
  used; however, in the future, special meaning may be assigned to the values
  corresponding to each key.

* *__class__* assignment works only if both classes have the same *__slots__*.


.. _metaclasses:

Customizing class creation
--------------------------

By default, classes are constructed using :func:`type`. A class definition is
read into a separate namespace and the value of class name is bound to the
result of ``type(name, bases, dict)``.

When the class definition is read, if *__metaclass__* is defined then the
callable assigned to it will be called instead of :func:`type`. The allows
classes or functions to be written which monitor or alter the class creation
process:

* Modifying the class dictionary prior to the class being created.

* Returning an instance of another class -- essentially performing the role of a
  factory function.

.. XXX needs to be updated for the "new metaclasses" PEP
.. data:: __metaclass__

   This variable can be any callable accepting arguments for ``name``, ``bases``,
   and ``dict``.  Upon class creation, the callable is used instead of the built-in
   :func:`type`.

The appropriate metaclass is determined by the following precedence rules:

* If ``dict['__metaclass__']`` exists, it is used.

* Otherwise, if there is at least one base class, its metaclass is used (this
  looks for a *__class__* attribute first and if not found, uses its type).

* Otherwise, if a global variable named __metaclass__ exists, it is used.

* Otherwise, the default metaclass (:class:`type`) is used.

The potential uses for metaclasses are boundless. Some ideas that have been
explored including logging, interface checking, automatic delegation, automatic
property creation, proxies, frameworks, and automatic resource
locking/synchronization.


.. _callable-types:

Emulating callable objects
--------------------------


.. method:: object.__call__(self[, args...])

   .. index:: pair: call; instance

   Called when the instance is "called" as a function; if this method is defined,
   ``x(arg1, arg2, ...)`` is a shorthand for ``x.__call__(arg1, arg2, ...)``.


.. _sequence-types:

Emulating container types
-------------------------

The following methods can be defined to implement container objects.  Containers
usually are sequences (such as lists or tuples) or mappings (like dictionaries),
but can represent other containers as well.  The first set of methods is used
either to emulate a sequence or to emulate a mapping; the difference is that for
a sequence, the allowable keys should be the integers *k* for which ``0 <= k <
N`` where *N* is the length of the sequence, or slice objects, which define a
range of items.  It is also recommended that mappings provide the methods
:meth:`keys`, :meth:`values`, :meth:`items`, :meth:`get`,
:meth:`clear`, :meth:`setdefault`,
:meth:`pop`, :meth:`popitem`, :meth:`copy`, and
:meth:`update` behaving similar to those for Python's standard dictionary
objects.  The :mod:`UserDict` module provides a :class:`DictMixin` class to help
create those methods from a base set of :meth:`__getitem__`,
:meth:`__setitem__`, :meth:`__delitem__`, and :meth:`keys`. Mutable sequences
should provide methods :meth:`append`, :meth:`count`, :meth:`index`,
:meth:`extend`, :meth:`insert`, :meth:`pop`, :meth:`remove`, :meth:`reverse` and
:meth:`sort`, like Python standard list objects.  Finally, sequence types should
implement addition (meaning concatenation) and multiplication (meaning
repetition) by defining the methods :meth:`__add__`, :meth:`__radd__`,
:meth:`__iadd__`, :meth:`__mul__`, :meth:`__rmul__` and :meth:`__imul__`
described below; they should not define other numerical operators.  It is
recommended that both mappings and sequences implement the :meth:`__contains__`
method to allow efficient use of the ``in`` operator; for mappings, ``in``
should search the mapping's keys; for sequences, it should search
through the values.  It is further recommended that both mappings and sequences
implement the :meth:`__iter__` method to allow efficient iteration through the
container; for mappings, :meth:`__iter__` should be the same as
:meth:`keys`; for sequences, it should iterate through the values.

.. method:: object.__len__(self)

   .. index::
      builtin: len
      single: __bool__() (object method)

   Called to implement the built-in function :func:`len`.  Should return the length
   of the object, an integer ``>=`` 0.  Also, an object that doesn't define a
   :meth:`__bool__` method and whose :meth:`__len__` method returns zero is
   considered to be false in a Boolean context.


.. note::

   Slicing is done exclusively with the following three methods.  A call like ::

      a[1:2] = b

   is translated to ::

      a[slice(1, 2, None)] = b

   and so forth.  Missing slice items are always filled in with ``None``.


.. method:: object.__getitem__(self, key)

   .. index:: object: slice

   Called to implement evaluation of ``self[key]``. For sequence types, the
   accepted keys should be integers and slice objects.  Note that the special
   interpretation of negative indexes (if the class wishes to emulate a sequence
   type) is up to the :meth:`__getitem__` method. If *key* is of an inappropriate
   type, :exc:`TypeError` may be raised; if of a value outside the set of indexes
   for the sequence (after any special interpretation of negative values),
   :exc:`IndexError` should be raised. For mapping types, if *key* is missing (not
   in the container), :exc:`KeyError` should be raised.

   .. note::

      :keyword:`for` loops expect that an :exc:`IndexError` will be raised for illegal
      indexes to allow proper detection of the end of the sequence.


.. method:: object.__setitem__(self, key, value)

   Called to implement assignment to ``self[key]``.  Same note as for
   :meth:`__getitem__`.  This should only be implemented for mappings if the
   objects support changes to the values for keys, or if new keys can be added, or
   for sequences if elements can be replaced.  The same exceptions should be raised
   for improper *key* values as for the :meth:`__getitem__` method.


.. method:: object.__delitem__(self, key)

   Called to implement deletion of ``self[key]``.  Same note as for
   :meth:`__getitem__`.  This should only be implemented for mappings if the
   objects support removal of keys, or for sequences if elements can be removed
   from the sequence.  The same exceptions should be raised for improper *key*
   values as for the :meth:`__getitem__` method.


.. method:: object.__iter__(self)

   This method is called when an iterator is required for a container. This method
   should return a new iterator object that can iterate over all the objects in the
   container.  For mappings, it should iterate over the keys of the container, and
   should also be made available as the method :meth:`keys`.

   Iterator objects also need to implement this method; they are required to return
   themselves.  For more information on iterator objects, see :ref:`typeiter`.

The membership test operators (:keyword:`in` and :keyword:`not in`) are normally
implemented as an iteration through a sequence.  However, container objects can
supply the following special method with a more efficient implementation, which
also does not require the object be a sequence.


.. method:: object.__contains__(self, item)

   Called to implement membership test operators.  Should return true if *item* is
   in *self*, false otherwise.  For mapping objects, this should consider the keys
   of the mapping rather than the values or the key-item pairs.


.. _numeric-types:

Emulating numeric types
-----------------------

The following methods can be defined to emulate numeric objects. Methods
corresponding to operations that are not supported by the particular kind of
number implemented (e.g., bitwise operations for non-integral numbers) should be
left undefined.


.. method:: object.__add__(self, other)
            object.__sub__(self, other)
            object.__mul__(self, other)
            object.__floordiv__(self, other)
            object.__mod__(self, other)
            object.__divmod__(self, other)
            object.__pow__(self, other[, modulo])
            object.__lshift__(self, other)
            object.__rshift__(self, other)
            object.__and__(self, other)
            object.__xor__(self, other)
            object.__or__(self, other)

   .. index::
      builtin: divmod
      builtin: pow
      builtin: pow

   These methods are called to implement the binary arithmetic operations (``+``,
   ``-``, ``*``, ``//``, ``%``, :func:`divmod`, :func:`pow`, ``**``, ``<<``,
   ``>>``, ``&``, ``^``, ``|``).  For instance, to evaluate the expression
   *x*``+``*y*, where *x* is an instance of a class that has an :meth:`__add__`
   method, ``x.__add__(y)`` is called.  The :meth:`__divmod__` method should be the
   equivalent to using :meth:`__floordiv__` and :meth:`__mod__`; it should not be
   related to :meth:`__truediv__` (described below).  Note that :meth:`__pow__`
   should be defined to accept an optional third argument if the ternary version of
   the built-in :func:`pow` function is to be supported.

   If one of those methods does not support the operation with the supplied
   arguments, it should return ``NotImplemented``.


.. method:: object.__div__(self, other)
            object.__truediv__(self, other)

   The division operator (``/``) is implemented by these methods.  The
   :meth:`__truediv__` method is used when ``__future__.division`` is in effect,
   otherwise :meth:`__div__` is used.  If only one of these two methods is defined,
   the object will not support division in the alternate context; :exc:`TypeError`
   will be raised instead.


.. method:: object.__radd__(self, other)
            object.__rsub__(self, other)
            object.__rmul__(self, other)
            object.__rdiv__(self, other)
            object.__rtruediv__(self, other)
            object.__rfloordiv__(self, other)
            object.__rmod__(self, other)
            object.__rdivmod__(self, other)
            object.__rpow__(self, other)
            object.__rlshift__(self, other)
            object.__rrshift__(self, other)
            object.__rand__(self, other)
            object.__rxor__(self, other)
            object.__ror__(self, other)

   .. index::
      builtin: divmod
      builtin: pow

   These methods are called to implement the binary arithmetic operations (``+``,
   ``-``, ``*``, ``/``, ``%``, :func:`divmod`, :func:`pow`, ``**``, ``<<``, ``>>``,
   ``&``, ``^``, ``|``) with reflected (swapped) operands.  These functions are
   only called if the left operand does not support the corresponding operation and
   the operands are of different types. [#]_ For instance, to evaluate the
   expression *x*``-``*y*, where *y* is an instance of a class that has an
   :meth:`__rsub__` method, ``y.__rsub__(x)`` is called if ``x.__sub__(y)`` returns
   *NotImplemented*.

   .. index:: builtin: pow

   Note that ternary :func:`pow` will not try calling :meth:`__rpow__` (the
   coercion rules would become too complicated).

   .. note::

      If the right operand's type is a subclass of the left operand's type and that
      subclass provides the reflected method for the operation, this method will be
      called before the left operand's non-reflected method.  This behavior allows
      subclasses to override their ancestors' operations.


.. method:: object.__iadd__(self, other)
            object.__isub__(self, other)
            object.__imul__(self, other)
            object.__idiv__(self, other)
            object.__itruediv__(self, other)
            object.__ifloordiv__(self, other)
            object.__imod__(self, other)
            object.__ipow__(self, other[, modulo])
            object.__ilshift__(self, other)
            object.__irshift__(self, other)
            object.__iand__(self, other)
            object.__ixor__(self, other)
            object.__ior__(self, other)

   These methods are called to implement the augmented arithmetic operations
   (``+=``, ``-=``, ``*=``, ``/=``, ``//=``, ``%=``, ``**=``, ``<<=``, ``>>=``,
   ``&=``, ``^=``, ``|=``).  These methods should attempt to do the operation
   in-place (modifying *self*) and return the result (which could be, but does
   not have to be, *self*).  If a specific method is not defined, the augmented
   operation falls back to the normal methods.  For instance, to evaluate the
   expression *x*``+=``*y*, where *x* is an instance of a class that has an
   :meth:`__iadd__` method, ``x.__iadd__(y)`` is called.  If *x* is an instance
   of a class that does not define a :meth:`__iadd__` method, ``x.__add__(y)``
   and ``y.__radd__(x)`` are considered, as with the evaluation of *x*``+``*y*.


.. method:: object.__neg__(self)
            object.__pos__(self)
            object.__abs__(self)
            object.__invert__(self)

   .. index:: builtin: abs

   Called to implement the unary arithmetic operations (``-``, ``+``, :func:`abs`
   and ``~``).


.. method:: object.__complex__(self)
            object.__int__(self)
            object.__float__(self)

   .. index::
      builtin: complex
      builtin: int
      builtin: float

   Called to implement the built-in functions :func:`complex`, :func:`int`
   and :func:`float`.  Should return a value of the appropriate type.


.. method:: object.__index__(self)

   Called to implement :func:`operator.index`.  Also called whenever Python needs
   an integer object (such as in slicing, or in the built-in :func:`bin`,
   :func:`hex` and :func:`oct` functions). Must return an integer.


.. _context-managers:

With Statement Context Managers
-------------------------------

A :dfn:`context manager` is an object that defines the runtime context to be
established when executing a :keyword:`with` statement. The context manager
handles the entry into, and the exit from, the desired runtime context for the
execution of the block of code.  Context managers are normally invoked using the
:keyword:`with` statement (described in section :ref:`with`), but can also be
used by directly invoking their methods.

.. index::
   statement: with
   single: context manager

Typical uses of context managers include saving and restoring various kinds of
global state, locking and unlocking resources, closing opened files, etc.

For more information on context managers, see :ref:`typecontextmanager`.


.. method:: object.__enter__(self)

   Enter the runtime context related to this object. The :keyword:`with` statement
   will bind this method's return value to the target(s) specified in the
   :keyword:`as` clause of the statement, if any.


.. method:: object.__exit__(self, exc_type, exc_value, traceback)

   Exit the runtime context related to this object. The parameters describe the
   exception that caused the context to be exited. If the context was exited
   without an exception, all three arguments will be :const:`None`.

   If an exception is supplied, and the method wishes to suppress the exception
   (i.e., prevent it from being propagated), it should return a true value.
   Otherwise, the exception will be processed normally upon exit from this method.

   Note that :meth:`__exit__` methods should not reraise the passed-in exception;
   this is the caller's responsibility.


.. seealso::

   :pep:`0343` - The "with" statement
      The specification, background, and examples for the Python :keyword:`with`
      statement.

.. rubric:: Footnotes

.. [#] A descriptor can define any combination of :meth:`__get__`,
   :meth:`__set__` and :meth:`__delete__`.  If it does not define :meth:`__get__`,
   then accessing the attribute even on an instance will return the descriptor
   object itself.  If the descriptor defines :meth:`__set__` and/or
   :meth:`__delete__`, it is a data descriptor; if it defines neither, it is a
   non-data descriptor.

.. [#] For operands of the same type, it is assumed that if the non-reflected method
   (such as :meth:`__add__`) fails the operation is not supported, which is why the
   reflected method is not called.

