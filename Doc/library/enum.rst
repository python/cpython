:mod:`enum` --- Support for enumerations
========================================

.. module:: enum
   :synopsis: Implementation of an enumeration class.

.. moduleauthor:: Ethan Furman <ethan@stoneleaf.us>
.. sectionauthor:: Barry Warsaw <barry@python.org>
.. sectionauthor:: Eli Bendersky <eliben@gmail.com>
.. sectionauthor:: Ethan Furman <ethan@stoneleaf.us>

.. versionadded:: 3.4

**Source code:** :source:`Lib/enum.py`

----------------

An enumeration is a set of symbolic names (members) bound to unique,
constant values.  Within an enumeration, the members can be compared
by identity, and the enumeration itself can be iterated over.

.. note:: Case of Enum Members

    Because Enums are used to represent constants we recommend using
    UPPER_CASE names for enum members, and will be using that style
    in our examples.


Module Contents
---------------

This module defines four enumeration classes that can be used to define unique
sets of names and values: :class:`Enum`, :class:`IntEnum`, :class:`Flag`, and
:class:`IntFlag`.  It also defines one decorator, :func:`unique`, and one
helper, :class:`auto`.

.. class:: Enum

    Base class for creating enumerated constants.  See section
    `Functional API`_ for an alternate construction syntax.

.. class:: IntEnum

    Base class for creating enumerated constants that are also
    subclasses of :class:`int`.

.. class:: StrEnum

    Base class for creating enumerated constants that are also
    subclasses of :class:`str`.

.. class:: IntFlag

    Base class for creating enumerated constants that can be combined using
    the bitwise operators without losing their :class:`IntFlag` membership.
    :class:`IntFlag` members are also subclasses of :class:`int`.

.. class:: Flag

    Base class for creating enumerated constants that can be combined using
    the bitwise operations without losing their :class:`Flag` membership.

.. function:: unique
    :noindex:

    Enum class decorator that ensures only one name is bound to any one value.

.. class:: auto

    Instances are replaced with an appropriate value for Enum members.
    :class:`StrEnum` defaults to the lower-cased version of the member name,
    while other Enums default to 1 and increase from there.

.. versionadded:: 3.6  ``Flag``, ``IntFlag``, ``auto``
.. versionadded:: 3.10  ``StrEnum``

Creating an Enum
----------------

Enumerations are created using the :keyword:`class` syntax, which makes them
easy to read and write.  An alternative creation method is described in
`Functional API`_.  To define an enumeration, subclass :class:`Enum` as
follows::

    >>> from enum import Enum
    >>> class Color(Enum):
    ...     RED = 1
    ...     GREEN = 2
    ...     BLUE = 3
    ...

.. note:: Enum member values

    Member values can be anything: :class:`int`, :class:`str`, etc..  If
    the exact value is unimportant you may use :class:`auto` instances and an
    appropriate value will be chosen for you.  Care must be taken if you mix
    :class:`auto` with other values.

.. note:: Nomenclature

  - The class :class:`Color` is an *enumeration* (or *enum*)
  - The attributes :attr:`Color.RED`, :attr:`Color.GREEN`, etc., are
    *enumeration members* (or *enum members*) and are functionally constants.
  - The enum members have *names* and *values* (the name of
    :attr:`Color.RED` is ``RED``, the value of :attr:`Color.BLUE` is
    ``3``, etc.)

.. note::

    Even though we use the :keyword:`class` syntax to create Enums, Enums
    are not normal Python classes.  See `How are Enums different?`_ for
    more details.

Enumeration members have human readable string representations::

    >>> print(Color.RED)
    Color.RED

...while their ``repr`` has more information::

    >>> print(repr(Color.RED))
    <Color.RED: 1>

The *type* of an enumeration member is the enumeration it belongs to::

    >>> type(Color.RED)
    <enum 'Color'>
    >>> isinstance(Color.GREEN, Color)
    True

Enum members also have a property that contains just their item name::

    >>> print(Color.RED.name)
    RED

Enumerations support iteration, in definition order::

    >>> class Shake(Enum):
    ...     VANILLA = 7
    ...     CHOCOLATE = 4
    ...     COOKIES = 9
    ...     MINT = 3
    ...
    >>> for shake in Shake:
    ...     print(shake)
    ...
    Shake.VANILLA
    Shake.CHOCOLATE
    Shake.COOKIES
    Shake.MINT

Enumeration members are hashable, so they can be used in dictionaries and sets::

    >>> apples = {}
    >>> apples[Color.RED] = 'red delicious'
    >>> apples[Color.GREEN] = 'granny smith'
    >>> apples == {Color.RED: 'red delicious', Color.GREEN: 'granny smith'}
    True


Programmatic access to enumeration members and their attributes
---------------------------------------------------------------

Sometimes it's useful to access members in enumerations programmatically (i.e.
situations where ``Color.RED`` won't do because the exact color is not known
at program-writing time).  ``Enum`` allows such access::

    >>> Color(1)
    <Color.RED: 1>
    >>> Color(3)
    <Color.BLUE: 3>

If you want to access enum members by *name*, use item access::

    >>> Color['RED']
    <Color.RED: 1>
    >>> Color['GREEN']
    <Color.GREEN: 2>

If you have an enum member and need its :attr:`name` or :attr:`value`::

    >>> member = Color.RED
    >>> member.name
    'RED'
    >>> member.value
    1


Duplicating enum members and values
-----------------------------------

Having two enum members with the same name is invalid::

    >>> class Shape(Enum):
    ...     SQUARE = 2
    ...     SQUARE = 3
    ...
    Traceback (most recent call last):
    ...
    TypeError: 'SQUARE' already defined as: 2

However, two enum members are allowed to have the same value.  Given two members
A and B with the same value (and A defined first), B is an alias to A.  By-value
lookup of the value of A and B will return A.  By-name lookup of B will also
return A::

    >>> class Shape(Enum):
    ...     SQUARE = 2
    ...     DIAMOND = 1
    ...     CIRCLE = 3
    ...     ALIAS_FOR_SQUARE = 2
    ...
    >>> Shape.SQUARE
    <Shape.SQUARE: 2>
    >>> Shape.ALIAS_FOR_SQUARE
    <Shape.SQUARE: 2>
    >>> Shape(2)
    <Shape.SQUARE: 2>

.. note::

    Attempting to create a member with the same name as an already
    defined attribute (another member, a method, etc.) or attempting to create
    an attribute with the same name as a member is not allowed.


Ensuring unique enumeration values
----------------------------------

By default, enumerations allow multiple names as aliases for the same value.
When this behavior isn't desired, the following decorator can be used to
ensure each value is used only once in the enumeration:

.. decorator:: unique

A :keyword:`class` decorator specifically for enumerations.  It searches an
enumeration's :attr:`__members__` gathering any aliases it finds; if any are
found :exc:`ValueError` is raised with the details::

    >>> from enum import Enum, unique
    >>> @unique
    ... class Mistake(Enum):
    ...     ONE = 1
    ...     TWO = 2
    ...     THREE = 3
    ...     FOUR = 3
    ...
    Traceback (most recent call last):
    ...
    ValueError: duplicate values found in <enum 'Mistake'>: FOUR -> THREE


Using automatic values
----------------------

If the exact value is unimportant you can use :class:`auto`::

    >>> from enum import Enum, auto
    >>> class Color(Enum):
    ...     RED = auto()
    ...     BLUE = auto()
    ...     GREEN = auto()
    ...
    >>> list(Color)
    [<Color.RED: 1>, <Color.BLUE: 2>, <Color.GREEN: 3>]

The values are chosen by :func:`_generate_next_value_`, which can be
overridden::

    >>> class AutoName(Enum):
    ...     def _generate_next_value_(name, start, count, last_values):
    ...         return name
    ...
    >>> class Ordinal(AutoName):
    ...     NORTH = auto()
    ...     SOUTH = auto()
    ...     EAST = auto()
    ...     WEST = auto()
    ...
    >>> list(Ordinal)
    [<Ordinal.NORTH: 'NORTH'>, <Ordinal.SOUTH: 'SOUTH'>, <Ordinal.EAST: 'EAST'>, <Ordinal.WEST: 'WEST'>]

.. note::

    The goal of the default :meth:`_generate_next_value_` method is to provide
    the next :class:`int` in sequence with the last :class:`int` provided, but
    the way it does this is an implementation detail and may change.

.. note::

    The :meth:`_generate_next_value_` method must be defined before any members.

Iteration
---------

Iterating over the members of an enum does not provide the aliases::

    >>> list(Shape)
    [<Shape.SQUARE: 2>, <Shape.DIAMOND: 1>, <Shape.CIRCLE: 3>]

The special attribute ``__members__`` is a read-only ordered mapping of names
to members.  It includes all names defined in the enumeration, including the
aliases::

    >>> for name, member in Shape.__members__.items():
    ...     name, member
    ...
    ('SQUARE', <Shape.SQUARE: 2>)
    ('DIAMOND', <Shape.DIAMOND: 1>)
    ('CIRCLE', <Shape.CIRCLE: 3>)
    ('ALIAS_FOR_SQUARE', <Shape.SQUARE: 2>)

The ``__members__`` attribute can be used for detailed programmatic access to
the enumeration members.  For example, finding all the aliases::

    >>> [name for name, member in Shape.__members__.items() if member.name != name]
    ['ALIAS_FOR_SQUARE']


Comparisons
-----------

Enumeration members are compared by identity::

    >>> Color.RED is Color.RED
    True
    >>> Color.RED is Color.BLUE
    False
    >>> Color.RED is not Color.BLUE
    True

Ordered comparisons between enumeration values are *not* supported.  Enum
members are not integers (but see `IntEnum`_ below)::

    >>> Color.RED < Color.BLUE
    Traceback (most recent call last):
      File "<stdin>", line 1, in <module>
    TypeError: '<' not supported between instances of 'Color' and 'Color'

Equality comparisons are defined though::

    >>> Color.BLUE == Color.RED
    False
    >>> Color.BLUE != Color.RED
    True
    >>> Color.BLUE == Color.BLUE
    True

Comparisons against non-enumeration values will always compare not equal
(again, :class:`IntEnum` was explicitly designed to behave differently, see
below)::

    >>> Color.BLUE == 2
    False


Allowed members and attributes of enumerations
----------------------------------------------

The examples above use integers for enumeration values.  Using integers is
short and handy (and provided by default by the `Functional API`_), but not
strictly enforced.  In the vast majority of use-cases, one doesn't care what
the actual value of an enumeration is.  But if the value *is* important,
enumerations can have arbitrary values.

Enumerations are Python classes, and can have methods and special methods as
usual.  If we have this enumeration::

    >>> class Mood(Enum):
    ...     FUNKY = 1
    ...     HAPPY = 3
    ...
    ...     def describe(self):
    ...         # self is the member here
    ...         return self.name, self.value
    ...
    ...     def __str__(self):
    ...         return 'my custom str! {0}'.format(self.value)
    ...
    ...     @classmethod
    ...     def favorite_mood(cls):
    ...         # cls here is the enumeration
    ...         return cls.HAPPY
    ...

Then::

    >>> Mood.favorite_mood()
    <Mood.HAPPY: 3>
    >>> Mood.HAPPY.describe()
    ('HAPPY', 3)
    >>> str(Mood.FUNKY)
    'my custom str! 1'

The rules for what is allowed are as follows: names that start and end with
a single underscore are reserved by enum and cannot be used; all other
attributes defined within an enumeration will become members of this
enumeration, with the exception of special methods (:meth:`__str__`,
:meth:`__add__`, etc.), descriptors (methods are also descriptors), and
variable names listed in :attr:`_ignore_`.

Note:  if your enumeration defines :meth:`__new__` and/or :meth:`__init__` then
any value(s) given to the enum member will be passed into those methods.
See `Planet`_ for an example.


Restricted Enum subclassing
---------------------------

A new :class:`Enum` class must have one base Enum class, up to one concrete
data type, and as many :class:`object`-based mixin classes as needed.  The
order of these base classes is::

    class EnumName([mix-in, ...,] [data-type,] base-enum):
        pass

Also, subclassing an enumeration is allowed only if the enumeration does not define
any members.  So this is forbidden::

    >>> class MoreColor(Color):
    ...     PINK = 17
    ...
    Traceback (most recent call last):
    ...
    TypeError: MoreColor: cannot extend enumeration 'Color'

But this is allowed::

    >>> class Foo(Enum):
    ...     def some_behavior(self):
    ...         pass
    ...
    >>> class Bar(Foo):
    ...     HAPPY = 1
    ...     SAD = 2
    ...

Allowing subclassing of enums that define members would lead to a violation of
some important invariants of types and instances.  On the other hand, it makes
sense to allow sharing some common behavior between a group of enumerations.
(See `OrderedEnum`_ for an example.)


Pickling
--------

Enumerations can be pickled and unpickled::

    >>> from test.test_enum import Fruit
    >>> from pickle import dumps, loads
    >>> Fruit.TOMATO is loads(dumps(Fruit.TOMATO))
    True

The usual restrictions for pickling apply: picklable enums must be defined in
the top level of a module, since unpickling requires them to be importable
from that module.

.. note::

    With pickle protocol version 4 it is possible to easily pickle enums
    nested in other classes.

It is possible to modify how Enum members are pickled/unpickled by defining
:meth:`__reduce_ex__` in the enumeration class.


Functional API
--------------

The :class:`Enum` class is callable, providing the following functional API::

    >>> Animal = Enum('Animal', 'ANT BEE CAT DOG')
    >>> Animal
    <enum 'Animal'>
    >>> Animal.ANT
    <Animal.ANT: 1>
    >>> Animal.ANT.value
    1
    >>> list(Animal)
    [<Animal.ANT: 1>, <Animal.BEE: 2>, <Animal.CAT: 3>, <Animal.DOG: 4>]

The semantics of this API resemble :class:`~collections.namedtuple`. The first
argument of the call to :class:`Enum` is the name of the enumeration.

The second argument is the *source* of enumeration member names.  It can be a
whitespace-separated string of names, a sequence of names, a sequence of
2-tuples with key/value pairs, or a mapping (e.g. dictionary) of names to
values.  The last two options enable assigning arbitrary values to
enumerations; the others auto-assign increasing integers starting with 1 (use
the ``start`` parameter to specify a different starting value).  A
new class derived from :class:`Enum` is returned.  In other words, the above
assignment to :class:`Animal` is equivalent to::

    >>> class Animal(Enum):
    ...     ANT = 1
    ...     BEE = 2
    ...     CAT = 3
    ...     DOG = 4
    ...

The reason for defaulting to ``1`` as the starting number and not ``0`` is
that ``0`` is ``False`` in a boolean sense, but enum members all evaluate
to ``True``.

Pickling enums created with the functional API can be tricky as frame stack
implementation details are used to try and figure out which module the
enumeration is being created in (e.g. it will fail if you use a utility
function in separate module, and also may not work on IronPython or Jython).
The solution is to specify the module name explicitly as follows::

    >>> Animal = Enum('Animal', 'ANT BEE CAT DOG', module=__name__)

.. warning::

    If ``module`` is not supplied, and Enum cannot determine what it is,
    the new Enum members will not be unpicklable; to keep errors closer to
    the source, pickling will be disabled.

The new pickle protocol 4 also, in some circumstances, relies on
:attr:`~definition.__qualname__` being set to the location where pickle will be able
to find the class.  For example, if the class was made available in class
SomeData in the global scope::

    >>> Animal = Enum('Animal', 'ANT BEE CAT DOG', qualname='SomeData.Animal')

The complete signature is::

    Enum(value='NewEnumName', names=<...>, *, module='...', qualname='...', type=<mixed-in class>, start=1)

:value: What the new Enum class will record as its name.

:names: The Enum members.  This can be a whitespace or comma separated string
  (values will start at 1 unless otherwise specified)::

    'RED GREEN BLUE' | 'RED,GREEN,BLUE' | 'RED, GREEN, BLUE'

  or an iterator of names::

    ['RED', 'GREEN', 'BLUE']

  or an iterator of (name, value) pairs::

    [('CYAN', 4), ('MAGENTA', 5), ('YELLOW', 6)]

  or a mapping::

    {'CHARTREUSE': 7, 'SEA_GREEN': 11, 'ROSEMARY': 42}

:module: name of module where new Enum class can be found.

:qualname: where in module new Enum class can be found.

:type: type to mix in to new Enum class.

:start: number to start counting at if only names are passed in.

.. versionchanged:: 3.5
   The *start* parameter was added.


Derived Enumerations
--------------------

IntEnum
^^^^^^^

The first variation of :class:`Enum` that is provided is also a subclass of
:class:`int`.  Members of an :class:`IntEnum` can be compared to integers;
by extension, integer enumerations of different types can also be compared
to each other::

    >>> from enum import IntEnum
    >>> class Shape(IntEnum):
    ...     CIRCLE = 1
    ...     SQUARE = 2
    ...
    >>> class Request(IntEnum):
    ...     POST = 1
    ...     GET = 2
    ...
    >>> Shape == 1
    False
    >>> Shape.CIRCLE == 1
    True
    >>> Shape.CIRCLE == Request.POST
    True

However, they still can't be compared to standard :class:`Enum` enumerations::

    >>> class Shape(IntEnum):
    ...     CIRCLE = 1
    ...     SQUARE = 2
    ...
    >>> class Color(Enum):
    ...     RED = 1
    ...     GREEN = 2
    ...
    >>> Shape.CIRCLE == Color.RED
    False

:class:`IntEnum` values behave like integers in other ways you'd expect::

    >>> int(Shape.CIRCLE)
    1
    >>> ['a', 'b', 'c'][Shape.CIRCLE]
    'b'
    >>> [i for i in range(Shape.SQUARE)]
    [0, 1]


StrEnum
^^^^^^^

The second variation of :class:`Enum` that is provided is also a subclass of
:class:`str`.  Members of a :class:`StrEnum` can be compared to strings;
by extension, string enumerations of different types can also be compared
to each other.  :class:`StrEnum` exists to help avoid the problem of getting
an incorrect member::

    >>> from enum import StrEnum
    >>> class Directions(StrEnum):
    ...     NORTH = 'north',    # notice the trailing comma
    ...     SOUTH = 'south'

Before :class:`StrEnum`, ``Directions.NORTH`` would have been the :class:`tuple`
``('north',)``.

.. note::

    Unlike other Enum's, ``str(StrEnum.member)`` will return the value of the
    member instead of the usual ``"EnumClass.member"``.

.. versionadded:: 3.10


IntFlag
^^^^^^^

The next variation of :class:`Enum` provided, :class:`IntFlag`, is also based
on :class:`int`.  The difference being :class:`IntFlag` members can be combined
using the bitwise operators (&, \|, ^, ~) and the result is still an
:class:`IntFlag` member, if possible.  However, as the name implies, :class:`IntFlag`
members also subclass :class:`int` and can be used wherever an :class:`int` is
used.

.. note::

    Any operation on an :class:`IntFlag` member besides the bit-wise operations will
    lose the :class:`IntFlag` membership.

.. note::

    Bit-wise operations that result in invalid :class:`IntFlag` values will lose the
    :class:`IntFlag` membership.

.. versionadded:: 3.6
.. versionchanged:: 3.10

Sample :class:`IntFlag` class::

    >>> from enum import IntFlag
    >>> class Perm(IntFlag):
    ...     R = 4
    ...     W = 2
    ...     X = 1
    ...
    >>> Perm.R | Perm.W
    <Perm.R|W: 6>
    >>> Perm.R + Perm.W
    6
    >>> RW = Perm.R | Perm.W
    >>> Perm.R in RW
    True

It is also possible to name the combinations::

    >>> class Perm(IntFlag):
    ...     R = 4
    ...     W = 2
    ...     X = 1
    ...     RWX = 7
    >>> Perm.RWX
    <Perm.RWX: 7>
    >>> ~Perm.RWX
    <Perm: 0>
    >>> Perm(7)
    <Perm.RWX: 7>

.. note::

    Named combinations are considered aliases.  Aliases do not show up during
    iteration, but can be returned from by-value lookups.

.. versionchanged:: 3.10

Another important difference between :class:`IntFlag` and :class:`Enum` is that
if no flags are set (the value is 0), its boolean evaluation is :data:`False`::

    >>> Perm.R & Perm.X
    <Perm: 0>
    >>> bool(Perm.R & Perm.X)
    False

Because :class:`IntFlag` members are also subclasses of :class:`int` they can
be combined with them (but may lose :class:`IntFlag` membership::

    >>> Perm.X | 4
    <Perm.R|X: 5>

    >>> Perm.X | 8
    9

.. note::

    The negation operator, ``~``, always returns an :class:`IntFlag` member with a
    positive value::

        >>> (~Perm.X).value == (Perm.R|Perm.W).value == 6
        True

:class:`IntFlag` members can also be iterated over::

    >>> list(RW)
    [<Perm.R: 4>, <Perm.W: 2>]

.. versionadded:: 3.10


Flag
^^^^

The last variation is :class:`Flag`.  Like :class:`IntFlag`, :class:`Flag`
members can be combined using the bitwise operators (&, \|, ^, ~).  Unlike
:class:`IntFlag`, they cannot be combined with, nor compared against, any
other :class:`Flag` enumeration, nor :class:`int`.  While it is possible to
specify the values directly it is recommended to use :class:`auto` as the
value and let :class:`Flag` select an appropriate value.

.. versionadded:: 3.6

Like :class:`IntFlag`, if a combination of :class:`Flag` members results in no
flags being set, the boolean evaluation is :data:`False`::

    >>> from enum import Flag, auto
    >>> class Color(Flag):
    ...     RED = auto()
    ...     BLUE = auto()
    ...     GREEN = auto()
    ...
    >>> Color.RED & Color.GREEN
    <Color: 0>
    >>> bool(Color.RED & Color.GREEN)
    False

Individual flags should have values that are powers of two (1, 2, 4, 8, ...),
while combinations of flags won't::

    >>> class Color(Flag):
    ...     RED = auto()
    ...     BLUE = auto()
    ...     GREEN = auto()
    ...     WHITE = RED | BLUE | GREEN
    ...
    >>> Color.WHITE
    <Color.WHITE: 7>

Giving a name to the "no flags set" condition does not change its boolean
value::

    >>> class Color(Flag):
    ...     BLACK = 0
    ...     RED = auto()
    ...     BLUE = auto()
    ...     GREEN = auto()
    ...
    >>> Color.BLACK
    <Color.BLACK: 0>
    >>> bool(Color.BLACK)
    False

:class:`Flag` members can also be iterated over::

    >>> purple = Color.RED | Color.BLUE
    >>> list(purple)
    [<Color.RED: 1>, <Color.BLUE: 2>]

.. versionadded:: 3.10

.. note::

    For the majority of new code, :class:`Enum` and :class:`Flag` are strongly
    recommended, since :class:`IntEnum` and :class:`IntFlag` break some
    semantic promises of an enumeration (by being comparable to integers, and
    thus by transitivity to other unrelated enumerations).  :class:`IntEnum`
    and :class:`IntFlag` should be used only in cases where :class:`Enum` and
    :class:`Flag` will not do; for example, when integer constants are replaced
    with enumerations, or for interoperability with other systems.


Others
^^^^^^

While :class:`IntEnum` is part of the :mod:`enum` module, it would be very
simple to implement independently::

    class IntEnum(int, Enum):
        pass

This demonstrates how similar derived enumerations can be defined; for example
a :class:`StrEnum` that mixes in :class:`str` instead of :class:`int`.

Some rules:

1. When subclassing :class:`Enum`, mix-in types must appear before
   :class:`Enum` itself in the sequence of bases, as in the :class:`IntEnum`
   example above.
2. While :class:`Enum` can have members of any type, once you mix in an
   additional type, all the members must have values of that type, e.g.
   :class:`int` above.  This restriction does not apply to mix-ins which only
   add methods and don't specify another type.
3. When another data type is mixed in, the :attr:`value` attribute is *not the
   same* as the enum member itself, although it is equivalent and will compare
   equal.
4. %-style formatting:  `%s` and `%r` call the :class:`Enum` class's
   :meth:`__str__` and :meth:`__repr__` respectively; other codes (such as
   `%i` or `%h` for IntEnum) treat the enum member as its mixed-in type.
5. :ref:`Formatted string literals <f-strings>`, :meth:`str.format`,
   and :func:`format` will use the mixed-in type's :meth:`__format__`
   unless :meth:`__str__` or :meth:`__format__` is overridden in the subclass,
   in which case the overridden methods or :class:`Enum` methods will be used.
   Use the !s and !r format codes to force usage of the :class:`Enum` class's
   :meth:`__str__` and :meth:`__repr__` methods.

When to use :meth:`__new__` vs. :meth:`__init__`
------------------------------------------------

:meth:`__new__` must be used whenever you want to customize the actual value of
the :class:`Enum` member.  Any other modifications may go in either
:meth:`__new__` or :meth:`__init__`, with :meth:`__init__` being preferred.

For example, if you want to pass several items to the constructor, but only
want one of them to be the value::

    >>> class Coordinate(bytes, Enum):
    ...     """
    ...     Coordinate with binary codes that can be indexed by the int code.
    ...     """
    ...     def __new__(cls, value, label, unit):
    ...         obj = bytes.__new__(cls, [value])
    ...         obj._value_ = value
    ...         obj.label = label
    ...         obj.unit = unit
    ...         return obj
    ...     PX = (0, 'P.X', 'km')
    ...     PY = (1, 'P.Y', 'km')
    ...     VX = (2, 'V.X', 'km/s')
    ...     VY = (3, 'V.Y', 'km/s')
    ...

    >>> print(Coordinate['PY'])
    Coordinate.PY

    >>> print(Coordinate(3))
    Coordinate.VY

Interesting examples
--------------------

While :class:`Enum`, :class:`IntEnum`, :class:`IntFlag`, and :class:`Flag` are
expected to cover the majority of use-cases, they cannot cover them all.  Here
are recipes for some different types of enumerations that can be used directly,
or as examples for creating one's own.


Omitting values
^^^^^^^^^^^^^^^

In many use-cases one doesn't care what the actual value of an enumeration
is. There are several ways to define this type of simple enumeration:

- use instances of :class:`auto` for the value
- use instances of :class:`object` as the value
- use a descriptive string as the value
- use a tuple as the value and a custom :meth:`__new__` to replace the
  tuple with an :class:`int` value

Using any of these methods signifies to the user that these values are not
important, and also enables one to add, remove, or reorder members without
having to renumber the remaining members.

Whichever method you choose, you should provide a :meth:`repr` that also hides
the (unimportant) value::

    >>> class NoValue(Enum):
    ...     def __repr__(self):
    ...         return '<%s.%s>' % (self.__class__.__name__, self.name)
    ...


Using :class:`auto`
"""""""""""""""""""

Using :class:`auto` would look like::

    >>> class Color(NoValue):
    ...     RED = auto()
    ...     BLUE = auto()
    ...     GREEN = auto()
    ...
    >>> Color.GREEN
    <Color.GREEN>


Using :class:`object`
"""""""""""""""""""""

Using :class:`object` would look like::

    >>> class Color(NoValue):
    ...     RED = object()
    ...     GREEN = object()
    ...     BLUE = object()
    ...
    >>> Color.GREEN
    <Color.GREEN>


Using a descriptive string
""""""""""""""""""""""""""

Using a string as the value would look like::

    >>> class Color(NoValue):
    ...     RED = 'stop'
    ...     GREEN = 'go'
    ...     BLUE = 'too fast!'
    ...
    >>> Color.GREEN
    <Color.GREEN>
    >>> Color.GREEN.value
    'go'


Using a custom :meth:`__new__`
""""""""""""""""""""""""""""""

Using an auto-numbering :meth:`__new__` would look like::

    >>> class AutoNumber(NoValue):
    ...     def __new__(cls):
    ...         value = len(cls.__members__) + 1
    ...         obj = object.__new__(cls)
    ...         obj._value_ = value
    ...         return obj
    ...
    >>> class Color(AutoNumber):
    ...     RED = ()
    ...     GREEN = ()
    ...     BLUE = ()
    ...
    >>> Color.GREEN
    <Color.GREEN>
    >>> Color.GREEN.value
    2

To make a more general purpose ``AutoNumber``, add ``*args`` to the signature::

    >>> class AutoNumber(NoValue):
    ...     def __new__(cls, *args):      # this is the only change from above
    ...         value = len(cls.__members__) + 1
    ...         obj = object.__new__(cls)
    ...         obj._value_ = value
    ...         return obj
    ...

Then when you inherit from ``AutoNumber`` you can write your own ``__init__``
to handle any extra arguments::

    >>> class Swatch(AutoNumber):
    ...     def __init__(self, pantone='unknown'):
    ...         self.pantone = pantone
    ...     AUBURN = '3497'
    ...     SEA_GREEN = '1246'
    ...     BLEACHED_CORAL = () # New color, no Pantone code yet!
    ...
    >>> Swatch.SEA_GREEN
    <Swatch.SEA_GREEN>
    >>> Swatch.SEA_GREEN.pantone
    '1246'
    >>> Swatch.BLEACHED_CORAL.pantone
    'unknown'

.. note::

    The :meth:`__new__` method, if defined, is used during creation of the Enum
    members; it is then replaced by Enum's :meth:`__new__` which is used after
    class creation for lookup of existing members.


OrderedEnum
^^^^^^^^^^^

An ordered enumeration that is not based on :class:`IntEnum` and so maintains
the normal :class:`Enum` invariants (such as not being comparable to other
enumerations)::

    >>> class OrderedEnum(Enum):
    ...     def __ge__(self, other):
    ...         if self.__class__ is other.__class__:
    ...             return self.value >= other.value
    ...         return NotImplemented
    ...     def __gt__(self, other):
    ...         if self.__class__ is other.__class__:
    ...             return self.value > other.value
    ...         return NotImplemented
    ...     def __le__(self, other):
    ...         if self.__class__ is other.__class__:
    ...             return self.value <= other.value
    ...         return NotImplemented
    ...     def __lt__(self, other):
    ...         if self.__class__ is other.__class__:
    ...             return self.value < other.value
    ...         return NotImplemented
    ...
    >>> class Grade(OrderedEnum):
    ...     A = 5
    ...     B = 4
    ...     C = 3
    ...     D = 2
    ...     F = 1
    ...
    >>> Grade.C < Grade.A
    True


DuplicateFreeEnum
^^^^^^^^^^^^^^^^^

Raises an error if a duplicate member name is found instead of creating an
alias::

    >>> class DuplicateFreeEnum(Enum):
    ...     def __init__(self, *args):
    ...         cls = self.__class__
    ...         if any(self.value == e.value for e in cls):
    ...             a = self.name
    ...             e = cls(self.value).name
    ...             raise ValueError(
    ...                 "aliases not allowed in DuplicateFreeEnum:  %r --> %r"
    ...                 % (a, e))
    ...
    >>> class Color(DuplicateFreeEnum):
    ...     RED = 1
    ...     GREEN = 2
    ...     BLUE = 3
    ...     GRENE = 2
    ...
    Traceback (most recent call last):
    ...
    ValueError: aliases not allowed in DuplicateFreeEnum:  'GRENE' --> 'GREEN'

.. note::

    This is a useful example for subclassing Enum to add or change other
    behaviors as well as disallowing aliases.  If the only desired change is
    disallowing aliases, the :func:`unique` decorator can be used instead.


Planet
^^^^^^

If :meth:`__new__` or :meth:`__init__` is defined the value of the enum member
will be passed to those methods::

    >>> class Planet(Enum):
    ...     MERCURY = (3.303e+23, 2.4397e6)
    ...     VENUS   = (4.869e+24, 6.0518e6)
    ...     EARTH   = (5.976e+24, 6.37814e6)
    ...     MARS    = (6.421e+23, 3.3972e6)
    ...     JUPITER = (1.9e+27,   7.1492e7)
    ...     SATURN  = (5.688e+26, 6.0268e7)
    ...     URANUS  = (8.686e+25, 2.5559e7)
    ...     NEPTUNE = (1.024e+26, 2.4746e7)
    ...     def __init__(self, mass, radius):
    ...         self.mass = mass       # in kilograms
    ...         self.radius = radius   # in meters
    ...     @property
    ...     def surface_gravity(self):
    ...         # universal gravitational constant  (m3 kg-1 s-2)
    ...         G = 6.67300E-11
    ...         return G * self.mass / (self.radius * self.radius)
    ...
    >>> Planet.EARTH.value
    (5.976e+24, 6378140.0)
    >>> Planet.EARTH.surface_gravity
    9.802652743337129


TimePeriod
^^^^^^^^^^

An example to show the :attr:`_ignore_` attribute in use::

    >>> from datetime import timedelta
    >>> class Period(timedelta, Enum):
    ...     "different lengths of time"
    ...     _ignore_ = 'Period i'
    ...     Period = vars()
    ...     for i in range(367):
    ...         Period['day_%d' % i] = i
    ...
    >>> list(Period)[:2]
    [<Period.day_0: datetime.timedelta(0)>, <Period.day_1: datetime.timedelta(days=1)>]
    >>> list(Period)[-2:]
    [<Period.day_365: datetime.timedelta(days=365)>, <Period.day_366: datetime.timedelta(days=366)>]


How are Enums different?
------------------------

Enums have a custom metaclass that affects many aspects of both derived Enum
classes and their instances (members).


Enum Classes
^^^^^^^^^^^^

The :class:`EnumMeta` metaclass is responsible for providing the
:meth:`__contains__`, :meth:`__dir__`, :meth:`__iter__` and other methods that
allow one to do things with an :class:`Enum` class that fail on a typical
class, such as `list(Color)` or `some_enum_var in Color`.  :class:`EnumMeta` is
responsible for ensuring that various other methods on the final :class:`Enum`
class are correct (such as :meth:`__new__`, :meth:`__getnewargs__`,
:meth:`__str__` and :meth:`__repr__`).


Enum Members (aka instances)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The most interesting thing about Enum members is that they are singletons.
:class:`EnumMeta` creates them all while it is creating the :class:`Enum`
class itself, and then puts a custom :meth:`__new__` in place to ensure
that no new ones are ever instantiated by returning only the existing
member instances.


Finer Points
^^^^^^^^^^^^

Supported ``__dunder__`` names
""""""""""""""""""""""""""""""

:attr:`__members__` is a read-only ordered mapping of ``member_name``:``member``
items.  It is only available on the class.

:meth:`__new__`, if specified, must create and return the enum members; it is
also a very good idea to set the member's :attr:`_value_` appropriately.  Once
all the members are created it is no longer used.


Supported ``_sunder_`` names
""""""""""""""""""""""""""""

- ``_name_`` -- name of the member
- ``_value_`` -- value of the member; can be set / modified in ``__new__``

- ``_missing_`` -- a lookup function used when a value is not found; may be
  overridden
- ``_ignore_`` -- a list of names, either as a :class:`list` or a :class:`str`,
  that will not be transformed into members, and will be removed from the final
  class
- ``_order_`` -- used in Python 2/3 code to ensure member order is consistent
  (class attribute, removed during class creation)
- ``_generate_next_value_`` -- used by the `Functional API`_ and by
  :class:`auto` to get an appropriate value for an enum member; may be
  overridden

.. note::

    For standard :class:`Enum` classes the next value chosen is the last value seen
    incremented by one.

    For :class:`Flag`-type classes the next value chosen will be the next highest
    power-of-two, regardless of the last value seen.

.. versionadded:: 3.6 ``_missing_``, ``_order_``, ``_generate_next_value_``
.. versionadded:: 3.7 ``_ignore_``

To help keep Python 2 / Python 3 code in sync an :attr:`_order_` attribute can
be provided.  It will be checked against the actual order of the enumeration
and raise an error if the two do not match::

    >>> class Color(Enum):
    ...     _order_ = 'RED GREEN BLUE'
    ...     RED = 1
    ...     BLUE = 3
    ...     GREEN = 2
    ...
    Traceback (most recent call last):
    ...
    TypeError: member order does not match _order_:
    ['RED', 'BLUE', 'GREEN']
    ['RED', 'GREEN', 'BLUE']

.. note::

    In Python 2 code the :attr:`_order_` attribute is necessary as definition
    order is lost before it can be recorded.


_Private__names
"""""""""""""""

Private names are not converted to Enum members, but remain normal attributes.

.. versionchanged:: 3.10


``Enum`` member type
""""""""""""""""""""

:class:`Enum` members are instances of their :class:`Enum` class, and are
normally accessed as ``EnumClass.member``.  In Python versions ``3.5`` to
``3.9`` you could access members from other members -- this practice was
discouraged, and in ``3.10`` :class:`Enum` has returned to not allowing it::

    >>> class FieldTypes(Enum):
    ...     name = 0
    ...     value = 1
    ...     size = 2
    ...
    >>> FieldTypes.value.size
    Traceback (most recent call last):
    ...
    AttributeError: FieldTypes: no attribute 'size'

.. versionchanged:: 3.5
.. versionchanged:: 3.10


Creating members that are mixed with other data types
"""""""""""""""""""""""""""""""""""""""""""""""""""""

When subclassing other data types, such as :class:`int` or :class:`str`, with
an :class:`Enum`, all values after the `=` are passed to that data type's
constructor.  For example::

    >>> class MyEnum(IntEnum):
    ...     example = '11', 16      # '11' will be interpreted as a hexadecimal
    ...                             # number
    >>> MyEnum.example
    <MyEnum.example: 17>


Boolean value of ``Enum`` classes and members
"""""""""""""""""""""""""""""""""""""""""""""

:class:`Enum` members that are mixed with non-:class:`Enum` types (such as
:class:`int`, :class:`str`, etc.) are evaluated according to the mixed-in
type's rules; otherwise, all members evaluate as :data:`True`.  To make your
own Enum's boolean evaluation depend on the member's value add the following to
your class::

    def __bool__(self):
        return bool(self.value)

:class:`Enum` classes always evaluate as :data:`True`.


``Enum`` classes with methods
"""""""""""""""""""""""""""""

If you give your :class:`Enum` subclass extra methods, like the `Planet`_
class above, those methods will show up in a :func:`dir` of the member,
but not of the class::

    >>> dir(Planet)
    ['EARTH', 'JUPITER', 'MARS', 'MERCURY', 'NEPTUNE', 'SATURN', 'URANUS', 'VENUS', '__class__', '__doc__', '__members__', '__module__']
    >>> dir(Planet.EARTH)
    ['__class__', '__doc__', '__module__', 'mass', 'name', 'radius', 'surface_gravity', 'value']


Combining members of ``Flag``
"""""""""""""""""""""""""""""

Iterating over a combination of Flag members will only return the members that
are comprised of a single bit::

    >>> class Color(Flag):
    ...     RED = auto()
    ...     GREEN = auto()
    ...     BLUE = auto()
    ...     MAGENTA = RED | BLUE
    ...     YELLOW = RED | GREEN
    ...     CYAN = GREEN | BLUE
    ...
    >>> Color(3)
    <Color.YELLOW: 3>
    >>> Color(7)
    <Color.RED|GREEN|BLUE: 7>

``StrEnum`` and :meth:`str.__str__`
"""""""""""""""""""""""""""""""""""

An important difference between :class:`StrEnum` and other Enums is the
:meth:`__str__` method; because :class:`StrEnum` members are strings, some
parts of Python will read the string data directly, while others will call
:meth:`str()`. To make those two operations have the same result,
:meth:`StrEnum.__str__` will be the same as :meth:`str.__str__` so that
``str(StrEnum.member) == StrEnum.member`` is true.

``Flag`` and ``IntFlag`` minutia
""""""""""""""""""""""""""""""""

The code sample::

    >>> class Color(IntFlag):
    ...     BLACK = 0
    ...     RED = 1
    ...     GREEN = 2
    ...     BLUE = 4
    ...     PURPLE = RED | BLUE
    ...     WHITE = RED | GREEN | BLUE
    ...

- single-bit flags are canonical
- multi-bit and zero-bit flags are aliases
- only canonical flags are returned during iteration::

    >>> list(Color.WHITE)
    [<Color.RED: 1>, <Color.GREEN: 2>, <Color.BLUE: 4>]

- negating a flag or flag set returns a new flag/flag set with the
  corresponding positive integer value::

    >>> Color.GREEN
    <Color.GREEN: 2>

    >>> ~Color.GREEN
    <Color.PURPLE: 5>

- names of pseudo-flags are constructed from their members' names::

    >>> (Color.RED | Color.GREEN).name
    'RED|GREEN'

- multi-bit flags, aka aliases, can be returned from operations::

    >>> Color.RED | Color.BLUE
    <Color.PURPLE: 5>

    >>> Color(7)  # or Color(-1)
    <Color.WHITE: 7>

- membership / containment checking has changed slightly -- zero valued flags
  are never considered to be contained::

    >>> Color.BLACK in Color.WHITE
    False

  otherwise, if all bits of one flag are in the other flag, True is returned::

    >>> Color.PURPLE in Color.WHITE
    True

There is a new boundary mechanism that controls how out-of-range / invalid
bits are handled: ``STRICT``, ``CONFORM``, ``EJECT``, and ``KEEP``:

  * STRICT --> raises an exception when presented with invalid values
  * CONFORM --> discards any invalid bits
  * EJECT --> lose Flag status and become a normal int with the given value
  * KEEP --> keep the extra bits
           - keeps Flag status and extra bits
           - extra bits do not show up in iteration
           - extra bits do show up in repr() and str()

The default for Flag is ``STRICT``, the default for ``IntFlag`` is ``DISCARD``,
and the default for ``_convert_`` is ``KEEP`` (see ``ssl.Options`` for an
example of when ``KEEP`` is needed).
