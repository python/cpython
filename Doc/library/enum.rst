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

.. sidebar:: Important

   This page contains the API reference information. For tutorial
   information and discussion of more advanced topics, see

   * :ref:`Basic Tutorial <enum-basic-tutorial>`
   * :ref:`Advanced Tutorial <enum-advanced-tutorial>`
   * :ref:`Enum Cookbook <enum-cookbook>`

----------------

An enumeration:

* is a set of symbolic names (members) bound to unique values
* can be iterated over to return its members in definition order
* uses :meth:`call` syntax to return members by value
* uses :meth:`index` syntax to return members by name

Enumerations are created either by using the :keyword:`class` syntax, or by
using function-call syntax::

   >>> from enum import Enum

   >>> # class syntax
   >>> class Color(Enum):
   ...     RED = 1
   ...     GREEN = 2
   ...     BLUE = 3

   >>> # functional syntax
   >>> Color = Enum('Color', ['RED', 'GREEN', 'BLUE'])

Even though we can use the :keyword:`class` syntax to create Enums, Enums
are not normal Python classes.  See
:ref:`How are Enums different? <enum-class-differences>` for more details.

.. note:: Nomenclature

   - The class :class:`Color` is an *enumeration* (or *enum*)
   - The attributes :attr:`Color.RED`, :attr:`Color.GREEN`, etc., are
     *enumeration members* (or *enum members*) and are functionally constants.
   - The enum members have *names* and *values* (the name of
     :attr:`Color.RED` is ``RED``, the value of :attr:`Color.BLUE` is
     ``3``, etc.)


Module Contents
---------------

   :class:`EnumType`

      The ``type`` for Enum and its subclasses.

   :class:`Enum`

      Base class for creating enumerated constants.

   :class:`IntEnum`

      Base class for creating enumerated constants that are also
      subclasses of :class:`int`.

   :class:`StrEnum`

      Base class for creating enumerated constants that are also
      subclasses of :class:`str`.

   :class:`Flag`

      Base class for creating enumerated constants that can be combined using
      the bitwise operations without losing their :class:`Flag` membership.

   :class:`IntFlag`

      Base class for creating enumerated constants that can be combined using
      the bitwise operators without losing their :class:`IntFlag` membership.
      :class:`IntFlag` members are also subclasses of :class:`int`.

   :class:`FlagBoundary`

      An enumeration with the values ``STRICT``, ``CONFORM``, ``EJECT``, and
      ``KEEP`` which allows for more fine-grained control over how invalid values
      are dealt with in an enumeration.

   :class:`auto`

      Instances are replaced with an appropriate value for Enum members.
      :class:`StrEnum` defaults to the lower-cased version of the member name,
      while other Enums default to 1 and increase from there.

   :func:`global_enum`

      :class:`Enum` class decorator to apply the appropriate global `__repr__`,
      and export its members into the global name space.

   :func:`property`

      Allows :class:`Enum` members to have attributes without conflicting with
      other members' names.

   :func:`unique`

      Enum class decorator that ensures only one name is bound to any one value.


.. versionadded:: 3.6  ``Flag``, ``IntFlag``, ``auto``
.. versionadded:: 3.10  ``StrEnum``


Data Types
----------


.. class:: EnumType

   *EnumType* is the :term:`metaclass` for *enum* enumerations.  It is possible
   to subclass *EnumType* -- see :ref:`Subclassing EnumType <enumtype-examples>`
   for details.

   .. method:: EnumType.__contains__(cls, member)

      Returns ``True`` if member belongs to the ``cls``::

        >>> some_var = Color.RED
        >>> some_var in Color
        True

   .. method:: EnumType.__dir__(cls)

      Returns ``['__class__', '__doc__', '__members__', '__module__']`` and the
      names of the members in *cls*::

        >>> dir(Color)
        ['BLUE', 'GREEN', 'RED', '__class__', '__doc__', '__members__', '__module__']

   .. method:: EnumType.__getattr__(cls, name)

      Returns the Enum member in *cls* matching *name*, or raises an :exc:`AttributeError`::

        >>> Color.GREEN
        Color.GREEN

   .. method:: EnumType.__getitem__(cls, name)

      Returns the Enum member in *cls* matching *name*, or raises an :exc:`KeyError`::

        >>> Color['BLUE']
        Color.BLUE

   .. method:: EnumType.__iter__(cls)

      Returns each member in *cls* in definition order::

        >>> list(Color)
        [Color.RED, Color.GREEN, Color.BLUE]

   .. method:: EnumType.__len__(cls)

      Returns the number of member in *cls*::

        >>> len(Color)
        3

   .. method:: EnumType.__reversed__(cls)

      Returns each member in *cls* in reverse definition order::

        >>> list(reversed(Color))
        [Color.BLUE, Color.GREEN, Color.RED]


.. class:: Enum

   *Enum* is the base class for all *enum* enumerations.

   .. attribute:: Enum.name

      The name used to define the ``Enum`` member::

        >>> Color.BLUE.name
        'BLUE'

   .. attribute:: Enum.value

      The value given to the ``Enum`` member::

         >>> Color.RED.value
         1

      .. note:: Enum member values

         Member values can be anything: :class:`int`, :class:`str`, etc..  If
         the exact value is unimportant you may use :class:`auto` instances and an
         appropriate value will be chosen for you.  Care must be taken if you mix
         :class:`auto` with other values.

   .. attribute:: Enum._ignore_

      ``_ignore_`` is only used during creation and is removed from the
      enumeration once that is complete.

      ``_ignore_`` is a list of names that will not become members, and whose
      names will also be removed from the completed enumeration.  See
      :ref:`TimePeriod <enum-time-period>` for an example.

   .. method:: Enum.__call__(cls, value, names=None, \*, module=None, qualname=None, type=None, start=1, boundary=None)

      This method is called in two different ways:

      * to look up an existing member:

         :cls:   The enum class being called.
         :value: The value to lookup.

      * to use the ``cls`` enum to create a new enum:

         :cls:   The enum class being called.
         :value: The name of the new Enum to create.
         :names: The names/values of the members for the new Enum.
         :module:    The name of the module the new Enum is created in.
         :qualname:  The actual location in the module where this Enum can be found.
         :type:  A mix-in type for the new Enum.
         :start: The first integer value for the Enum (used by :class:`auto`)
         :boundary:  How to handle out-of-range values from bit operations (:class:`Flag` only)

   .. method:: Enum.__dir__(self)

      Returns ``['__class__', '__doc__', '__module__', 'name', 'value']`` and
      any public methods defined on *self.__class__*::

         >>> from datetime import date
         >>> class Weekday(Enum):
         ...     MONDAY = 1
         ...     TUESDAY = 2
         ...     WEDNESDAY = 3
         ...     THURSDAY = 4
         ...     FRIDAY = 5
         ...     SATURDAY = 6
         ...     SUNDAY = 7
         ...     @classmethod
         ...     def today(cls):
         ...         print('today is %s' % cls(date.today.isoweekday).naem)
         >>> dir(Weekday.SATURDAY)
         ['__class__', '__doc__', '__module__', 'name', 'today', 'value']

   .. method:: Enum._generate_next_value_(name, start, count, last_values)

         :name: The name of the member being defined (e.g. 'RED').
         :start: The start value for the Enum; the default is 1.
         :count: The number of members currently defined, not including this one.
         :last_values: A list of the previous values.

      A *staticmethod* that is used to determine the next value returned by
      :class:`auto`::

         >>> from enum import auto
         >>> class PowersOfThree(Enum):
         ...     @staticmethod
         ...     def _generate_next_value_(name, start, count, last_values):
         ...         return (count + 1) * 3
         ...     FIRST = auto()
         ...     SECOND = auto()
         >>> PowersOfThree.SECOND.value
         6

   .. method:: Enum._missing_(cls, value)

      A *classmethod* for looking up values not found in *cls*.  By default it
      does nothing, but can be overridden to implement custom search behavior::

         >>> from enum import StrEnum
         >>> class Build(StrEnum):
         ...     DEBUG = auto()
         ...     OPTIMIZED = auto()
         ...     @classmethod
         ...     def _missing_(cls, value):
         ...         value = value.lower()
         ...         for member in cls:
         ...             if member.value == value:
         ...                 return member
         ...         return None
         >>> Build.DEBUG.value
         'debug'
         >>> Build('deBUG')
         Build.DEBUG

   .. method:: Enum.__repr__(self)

      Returns the string used for *repr()* calls.  By default, returns the
      *Enum* name and the member name, but can be overridden::

         >>> class OldStyle(Enum):
         ...     RETRO = auto()
         ...     OLD_SCHOOl = auto()
         ...     YESTERYEAR = auto()
         ...     def __repr__(self):
         ...         cls_name = self.__class__.__name__
         ...         return f'<{cls_name}.{self.name}: {self.value}>'
         >>> OldStyle.RETRO
         <OldStyle.RETRO: 1>

   .. method:: Enum.__str__(self)

      Returns the string used for *str()* calls.  By default, returns the
      member name, but can be overridden::

         >>> class OldStyle(Enum):
         ...     RETRO = auto()
         ...     OLD_SCHOOl = auto()
         ...     YESTERYEAR = auto()
         ...     def __str__(self):
         ...         cls_name = self.__class__.__name__
         ...         return f'{cls_name}.{self.name}'
         >>> OldStyle.RETRO
         OldStyle.RETRO

.. note::

   Using :class:`auto` with :class:`Enum` results in integers of increasing value,
   starting with ``1``.


.. class:: IntEnum

   *IntEnum* is the same as *Enum*, but its members are also integers and can be
   used anywhere that an integer can be used.  If any integer operation is performed
   with an *IntEnum* member, the resulting value loses its enumeration status.

      >>> from enum import IntEnum
      >>> class Numbers(IntEnum):
      ...     ONE = 1
      ...     TWO = 2
      ...     THREE = 3
      >>> Numbers.THREE
      Numbers.THREE
      >>> Numbers.ONE + Numbers.TWO
      3
      >>> Numbers.THREE + 5
      8
      >>> Numbers.THREE == 3
      True

.. note::

   Using :class:`auto` with :class:`IntEnum` results in integers of increasing value,
   starting with ``1``.


.. class:: StrEnum

   *StrEnum* is the same as *Enum*, but its members are also strings and can be used
   in most of the same places that a string can be used.  The result of any string
   operation performed on or with a *StrEnum* member is not part of the enumeration.

   .. note:: There are places in the stdlib that check for an exact :class:`str`
             instead of a :class:`str` subclass (i.e. ``type(unknown) == str``
             instead of ``isinstance(str, unknown)``), and in those locations you
             will need to use ``str(StrEnum.member)``.


.. note::

   Using :class:`auto` with :class:`StrEnum` results in values of the member name,
   lower-cased.


.. class:: Flag

   *Flag* members support the bitwise operators ``&`` (*AND*), ``|`` (*OR*),
   ``^`` (*XOR*), and ``~`` (*INVERT*); the results of those operators are members
   of the enumeration.

   .. method:: __contains__(self, value)

      Returns *True* if value is in self::

         >>> from enum import Flag, auto
         >>> class Color(Flag):
         ...     RED = auto()
         ...     GREEN = auto()
         ...     BLUE = auto()
         >>> purple = Color.RED | Color.BLUE
         >>> white = Color.RED | Color.GREEN | Color.BLUE
         >>> Color.GREEN in purple
         False
         >>> Color.GREEN in white
         True
         >>> purple in white
         True
         >>> white in purple
         False

   .. method:: __iter__(self):

      Returns all contained members::

         >>> list(Color.RED)
         [Color.RED]
         >>> list(purple)
         [Color.RED, Color.BLUE]

   .. method:: __len__(self):

      Returns number of members in flag::

         >>> len(Color.GREEN)
         1
         >>> len(white)
         3

   .. method:: __bool__(self):

      Returns *True* if any members in flag, *False* otherwise::

         >>> bool(Color.GREEN)
         True
         >>> bool(white)
         True
         >>> black = Color(0)
         >>> bool(black)
         False

   .. method:: __or__(self, other)

      Returns current flag binary or'ed with other::

         >>> Color.RED | Color.GREEN
         Color.RED|Color.GREEN

   .. method:: __and__(self, other)

      Returns current flag binary and'ed with other::

         >>> purple & white
         Color.RED|Color.BLUE
         >>> purple & Color.GREEN
         0x0

   .. method:: __xor__(self, other)

      Returns current flag binary xor'ed with other::

         >>> purple ^ white
         Color.GREEN
         >>> purple ^ Color.GREEN
         Color.RED|Color.GREEN|Color.BLUE

   .. method:: __invert__(self):

      Returns all the flags in *type(self)* that are not in self::

         >>> ~white
         0x0
         >>> ~purple
         Color.GREEN
         >>> ~Color.RED
         Color.GREEN|Color.BLUE

.. note::

   Using :class:`auto` with :class:`Flag` results in integers that are powers
   of two, starting with ``1``.


.. class:: IntFlag

   *IntFlag* is the same as *Flag*, but its members are also integers and can be
   used anywhere that an integer can be used.

      >>> from enum import IntFlag, auto
      >>> class Color(IntFlag):
      ...     RED = auto()
      ...     GREEN = auto()
      ...     BLUE = auto()
      >>> Color.RED & 2
      0x0
      >>> Color.RED | 2
      Color.RED|Color.GREEN

   If any integer operation is performed with an *IntFlag* member, the result is
   not an *IntFlag*::

        >>> Color.RED + 2
        3

   If a *Flag* operation is performed with an *IntFlag* member and:

      * the result is a valid *IntFlag*: an *IntFlag* is returned
      * the result is not a valid *IntFlag*: the result depends on the *FlagBoundary* setting

.. note::

   Using :class:`auto` with :class:`IntFlag` results in integers that are powers
   of two, starting with ``1``.

.. class:: FlagBoundary

   *FlagBoundary* controls how out-of-range values are handled in *Flag* and its
   subclasses.

   .. attribute:: STRICT

      Out-of-range values cause a :exc:`ValueError` to be raised.  This is the
      default for :class:`Flag`::

         >>> from enum import Flag, STRICT
         >>> class StrictFlag(Flag, boundary=STRICT):
         ...     RED = auto()
         ...     GREEN = auto()
         ...     BLUE = auto()
         >>> StrictFlag(2**2 + 2**4)
         Traceback (most recent call last):
         ...
         ValueError: StrictFlag: invalid value: 20
             given 0b0 10100
           allowed 0b0 00111

   .. attribute:: CONFORM

      Out-of-range values have invalid values removed, leaving a valid *Flag*
      value::

         >>> from enum import Flag, CONFORM
         >>> class ConformFlag(Flag, boundary=CONFORM):
         ...     RED = auto()
         ...     GREEN = auto()
         ...     BLUE = auto()
         >>> ConformFlag(2**2 + 2**4)
         ConformFlag.BLUE

   .. attribute:: EJECT

      Out-of-range values lose their *Flag* membership and revert to :class:`int`.
      This is the default for :class:`IntFlag`::

         >>> from enum import Flag, EJECT
         >>> class EjectFlag(Flag, boundary=EJECT):
         ...     RED = auto()
         ...     GREEN = auto()
         ...     BLUE = auto()
         >>> EjectFlag(2**2 + 2**4)
         20

   .. attribute:: KEEP

      Out-of-range values are kept, and the *Flag* membership is kept.  This is
      used for some stdlib flags:

         >>> from enum import Flag, KEEP
         >>> class KeepFlag(Flag, boundary=KEEP):
         ...     RED = auto()
         ...     GREEN = auto()
         ...     BLUE = auto()
         >>> KeepFlag(2**2 + 2**4)
         KeepFlag.BLUE|0x10


Utilites and Decorators
-----------------------

.. class:: auto

   *auto* can be used in place of a value.  If used, the *Enum* machinery will
   call an *Enum*'s :meth:`_generate_next_value_` to get an appropriate value.
   For *Enum* and *IntEnum* that appropriate value will be the last value plus
   one; for *Flag* and *IntFlag* it will be the first power-of-two greater
   than the last value; for *StrEnum* it will be the lower-cased version of the
   member's name.

   ``_generate_next_value_`` can be overridden to customize the values used by
   *auto*.

.. decorator:: global_enum

   A :keyword:`class` decorator specifically for enumerations.  It replaces the
   :meth:`__repr__` method with one that shows *module_name*.*member_name*.  It
   also injects the members, and their aliases, into the the global namespace
   they were defined in.


.. decorator:: property

   A decorator similar to the built-in *property*, but specifically for
   enumerations.  It allows member attributes to have the same names as members
   themselves.

   .. note:: the *property* and the member must be defined in separate classes;
             for example, the *value* and *name* attributes are defined in the
             *Enum* class, and *Enum* subclasses can define members with the
             names ``value`` and ``name``.

.. decorator:: unique

   A :keyword:`class` decorator specifically for enumerations.  It searches an
   enumeration's :attr:`__members__`, gathering any aliases it finds; if any are
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

