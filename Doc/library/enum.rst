:mod:`!enum` --- Support for enumerations
=========================================

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

---------------

An enumeration:

* is a set of symbolic names (members) bound to unique values
* can be iterated over to return its canonical (i.e. non-alias) members in
  definition order
* uses *call* syntax to return members by value
* uses *index* syntax to return members by name

Enumerations are created either by using :keyword:`class` syntax, or by
using function-call syntax::

   >>> from enum import Enum

   >>> # class syntax
   >>> class Color(Enum):
   ...     RED = 1
   ...     GREEN = 2
   ...     BLUE = 3

   >>> # functional syntax
   >>> Color = Enum('Color', ['RED', 'GREEN', 'BLUE'])

Even though we can use :keyword:`class` syntax to create Enums, Enums
are not normal Python classes.  See
:ref:`How are Enums different? <enum-class-differences>` for more details.

.. note:: Nomenclature

   - The class :class:`!Color` is an *enumeration* (or *enum*)
   - The attributes :attr:`!Color.RED`, :attr:`!Color.GREEN`, etc., are
     *enumeration members* (or *members*) and are functionally constants.
   - The enum members have *names* and *values* (the name of
     :attr:`!Color.RED` is ``RED``, the value of :attr:`!Color.BLUE` is
     ``3``, etc.)

---------------

Module Contents
---------------

   :class:`EnumType`

      The ``type`` for Enum and its subclasses.

   :class:`Enum`

      Base class for creating enumerated constants.

   :class:`IntEnum`

      Base class for creating enumerated constants that are also
      subclasses of :class:`int`. (`Notes`_)

   :class:`StrEnum`

      Base class for creating enumerated constants that are also
      subclasses of :class:`str`. (`Notes`_)

   :class:`Flag`

      Base class for creating enumerated constants that can be combined using
      the bitwise operations without losing their :class:`Flag` membership.

   :class:`IntFlag`

      Base class for creating enumerated constants that can be combined using
      the bitwise operators without losing their :class:`IntFlag` membership.
      :class:`IntFlag` members are also subclasses of :class:`int`. (`Notes`_)

   :class:`ReprEnum`

      Used by :class:`IntEnum`, :class:`StrEnum`, and :class:`IntFlag`
      to keep the :class:`str() <str>` of the mixed-in type.

   :class:`EnumCheck`

      An enumeration with the values ``CONTINUOUS``, ``NAMED_FLAGS``, and
      ``UNIQUE``, for use with :func:`verify` to ensure various constraints
      are met by a given enumeration.

   :class:`FlagBoundary`

      An enumeration with the values ``STRICT``, ``CONFORM``, ``EJECT``, and
      ``KEEP`` which allows for more fine-grained control over how invalid values
      are dealt with in an enumeration.

   :class:`auto`

      Instances are replaced with an appropriate value for Enum members.
      :class:`StrEnum` defaults to the lower-cased version of the member name,
      while other Enums default to 1 and increase from there.

   :func:`~enum.property`

      Allows :class:`Enum` members to have attributes without conflicting with
      member names.  The ``value`` and ``name`` attributes are implemented this
      way.

   :func:`unique`

      Enum class decorator that ensures only one name is bound to any one value.

   :func:`verify`

      Enum class decorator that checks user-selectable constraints on an
      enumeration.

   :func:`member`

      Make ``obj`` a member.  Can be used as a decorator.

   :func:`nonmember`

      Do not make ``obj`` a member.  Can be used as a decorator.

   :func:`global_enum`

      Modify the :class:`str() <str>` and :func:`repr` of an enum
      to show its members as belonging to the module instead of its class,
      and export the enum members to the global namespace.

   :func:`show_flag_values`

      Return a list of all power-of-two integers contained in a flag.


.. versionadded:: 3.6  ``Flag``, ``IntFlag``, ``auto``
.. versionadded:: 3.11  ``StrEnum``, ``EnumCheck``, ``ReprEnum``, ``FlagBoundary``, ``property``, ``member``, ``nonmember``, ``global_enum``, ``show_flag_values``

---------------

Data Types
----------


.. class:: EnumType

   *EnumType* is the :term:`metaclass` for *enum* enumerations.  It is possible
   to subclass *EnumType* -- see :ref:`Subclassing EnumType <enumtype-examples>`
   for details.

   *EnumType* is responsible for setting the correct :meth:`!__repr__`,
   :meth:`!__str__`, :meth:`!__format__`, and :meth:`!__reduce__` methods on the
   final *enum*, as well as creating the enum members, properly handling
   duplicates, providing iteration over the enum class, etc.

   .. method:: EnumType.__call__(cls, value, names=None, *, module=None, qualname=None, type=None, start=1, boundary=None)

      This method is called in two different ways:

      * to look up an existing member:

         :cls:   The enum class being called.
         :value: The value to lookup.

      * to use the ``cls`` enum to create a new enum (only if the existing enum
        does not have any members):

         :cls:   The enum class being called.
         :value: The name of the new Enum to create.
         :names: The names/values of the members for the new Enum.
         :module:    The name of the module the new Enum is created in.
         :qualname:  The actual location in the module where this Enum can be found.
         :type:  A mix-in type for the new Enum.
         :start: The first integer value for the Enum (used by :class:`auto`).
         :boundary:  How to handle out-of-range values from bit operations (:class:`Flag` only).

   .. method:: EnumType.__contains__(cls, member)

      Returns ``True`` if member belongs to the ``cls``::

        >>> some_var = Color.RED
        >>> some_var in Color
        True
        >>> Color.RED.value in Color
        True

   .. versionchanged:: 3.12

         Before Python 3.12, a ``TypeError`` is raised if a
         non-Enum-member is used in a containment check.

   .. method:: EnumType.__dir__(cls)

      Returns ``['__class__', '__doc__', '__members__', '__module__']`` and the
      names of the members in *cls*::

        >>> dir(Color)
        ['BLUE', 'GREEN', 'RED', '__class__', '__contains__', '__doc__', '__getitem__', '__init_subclass__', '__iter__', '__len__', '__members__', '__module__', '__name__', '__qualname__']

   .. method:: EnumType.__getitem__(cls, name)

      Returns the Enum member in *cls* matching *name*, or raises a :exc:`KeyError`::

        >>> Color['BLUE']
        <Color.BLUE: 3>

   .. method:: EnumType.__iter__(cls)

      Returns each member in *cls* in definition order::

        >>> list(Color)
        [<Color.RED: 1>, <Color.GREEN: 2>, <Color.BLUE: 3>]

   .. method:: EnumType.__len__(cls)

      Returns the number of member in *cls*::

        >>> len(Color)
        3

   .. attribute:: EnumType.__members__

      Returns a mapping of every enum name to its member, including aliases

   .. method:: EnumType.__reversed__(cls)

      Returns each member in *cls* in reverse definition order::

        >>> list(reversed(Color))
        [<Color.BLUE: 3>, <Color.GREEN: 2>, <Color.RED: 1>]

   .. versionadded:: 3.11

      Before 3.11 ``enum`` used ``EnumMeta`` type, which is kept as an alias.


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

      Value of the member, can be set in :meth:`~Enum.__new__`.

      .. note:: Enum member values

         Member values can be anything: :class:`int`, :class:`str`, etc.  If
         the exact value is unimportant you may use :class:`auto` instances and an
         appropriate value will be chosen for you.  See :class:`auto` for the
         details.

         While mutable/unhashable values, such as :class:`dict`, :class:`list` or
         a mutable :class:`~dataclasses.dataclass`, can be used, they will have a
         quadratic performance impact during creation relative to the
         total number of mutable/unhashable values in the enum.

   .. attribute:: Enum._name_

      Name of the member.

   .. attribute:: Enum._value_

      Value of the member, can be set in :meth:`~Enum.__new__`.

   .. attribute:: Enum._order_

      No longer used, kept for backward compatibility.
      (class attribute, removed during class creation).

   .. attribute:: Enum._ignore_

      ``_ignore_`` is only used during creation and is removed from the
      enumeration once creation is complete.

      ``_ignore_`` is a list of names that will not become members, and whose
      names will also be removed from the completed enumeration.  See
      :ref:`TimePeriod <enum-time-period>` for an example.

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
         ...         print('today is %s' % cls(date.today().isoweekday()).name)
         ...
         >>> dir(Weekday.SATURDAY)
         ['__class__', '__doc__', '__eq__', '__hash__', '__module__', 'name', 'today', 'value']

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
         ...         return 3 ** (count + 1)
         ...     FIRST = auto()
         ...     SECOND = auto()
         ...
         >>> PowersOfThree.SECOND.value
         9

   .. method:: Enum.__init__(self, *args, **kwds)

      By default, does nothing.  If multiple values are given in the member
      assignment, those values become separate arguments to ``__init__``; e.g.

         >>> from enum import Enum
         >>> class Weekday(Enum):
         ...     MONDAY = 1, 'Mon'

      ``Weekday.__init__()`` would be called as ``Weekday.__init__(self, 1, 'Mon')``

   .. method:: Enum.__init_subclass__(cls, **kwds)

      A *classmethod* that is used to further configure subsequent subclasses.
      By default, does nothing.

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
         ...
         >>> Build.DEBUG.value
         'debug'
         >>> Build('deBUG')
         <Build.DEBUG: 'debug'>

   .. method:: Enum.__new__(cls, *args, **kwds)

      By default, doesn't exist.  If specified, either in the enum class
      definition or in a mixin class (such as ``int``), all values given
      in the member assignment will be passed; e.g.

         >>> from enum import Enum
         >>> class MyIntEnum(int, Enum):
         ...     TWENTYSIX = '1a', 16

      results in the call ``int('1a', 16)`` and a value of ``26`` for the member.

      .. note::

         When writing a custom ``__new__``, do not use ``super().__new__`` --
         call the appropriate ``__new__`` instead.

   .. method:: Enum.__repr__(self)

      Returns the string used for *repr()* calls.  By default, returns the
      *Enum* name, member name, and value, but can be overridden::

         >>> class OtherStyle(Enum):
         ...     ALTERNATE = auto()
         ...     OTHER = auto()
         ...     SOMETHING_ELSE = auto()
         ...     def __repr__(self):
         ...         cls_name = self.__class__.__name__
         ...         return f'{cls_name}.{self.name}'
         ...
         >>> OtherStyle.ALTERNATE, str(OtherStyle.ALTERNATE), f"{OtherStyle.ALTERNATE}"
         (OtherStyle.ALTERNATE, 'OtherStyle.ALTERNATE', 'OtherStyle.ALTERNATE')

   .. method:: Enum.__str__(self)

      Returns the string used for *str()* calls.  By default, returns the
      *Enum* name and member name, but can be overridden::

         >>> class OtherStyle(Enum):
         ...     ALTERNATE = auto()
         ...     OTHER = auto()
         ...     SOMETHING_ELSE = auto()
         ...     def __str__(self):
         ...         return f'{self.name}'
         ...
         >>> OtherStyle.ALTERNATE, str(OtherStyle.ALTERNATE), f"{OtherStyle.ALTERNATE}"
         (<OtherStyle.ALTERNATE: 1>, 'ALTERNATE', 'ALTERNATE')

   .. method:: Enum.__format__(self)

      Returns the string used for *format()* and *f-string* calls.  By default,
      returns :meth:`__str__` return value, but can be overridden::

         >>> class OtherStyle(Enum):
         ...     ALTERNATE = auto()
         ...     OTHER = auto()
         ...     SOMETHING_ELSE = auto()
         ...     def __format__(self, spec):
         ...         return f'{self.name}'
         ...
         >>> OtherStyle.ALTERNATE, str(OtherStyle.ALTERNATE), f"{OtherStyle.ALTERNATE}"
         (<OtherStyle.ALTERNATE: 1>, 'OtherStyle.ALTERNATE', 'ALTERNATE')

   .. note::

      Using :class:`auto` with :class:`Enum` results in integers of increasing value,
      starting with ``1``.

   .. versionchanged:: 3.12 Added :ref:`enum-dataclass-support`


.. class:: IntEnum

   *IntEnum* is the same as *Enum*, but its members are also integers and can be
   used anywhere that an integer can be used.  If any integer operation is performed
   with an *IntEnum* member, the resulting value loses its enumeration status.

      >>> from enum import IntEnum
      >>> class Number(IntEnum):
      ...     ONE = 1
      ...     TWO = 2
      ...     THREE = 3
      ...
      >>> Number.THREE
      <Number.THREE: 3>
      >>> Number.ONE + Number.TWO
      3
      >>> Number.THREE + 5
      8
      >>> Number.THREE == 3
      True

   .. note::

      Using :class:`auto` with :class:`IntEnum` results in integers of increasing
      value, starting with ``1``.

   .. versionchanged:: 3.11 :meth:`~object.__str__` is now :meth:`!int.__str__` to
      better support the *replacement of existing constants* use-case.
      :meth:`~object.__format__` was already :meth:`!int.__format__` for that same reason.


.. class:: StrEnum

   *StrEnum* is the same as *Enum*, but its members are also strings and can be used
   in most of the same places that a string can be used.  The result of any string
   operation performed on or with a *StrEnum* member is not part of the enumeration.

   .. note::

      There are places in the stdlib that check for an exact :class:`str`
      instead of a :class:`str` subclass (i.e. ``type(unknown) == str``
      instead of ``isinstance(unknown, str)``), and in those locations you
      will need to use ``str(StrEnum.member)``.

   .. note::

      Using :class:`auto` with :class:`StrEnum` results in the lower-cased member
      name as the value.

   .. note::

      :meth:`~object.__str__` is :meth:`!str.__str__` to better support the
      *replacement of existing constants* use-case.  :meth:`~object.__format__` is likewise
      :meth:`!str.__format__` for that same reason.

   .. versionadded:: 3.11

.. class:: Flag

   ``Flag`` is the same as :class:`Enum`, but its members support the bitwise
   operators ``&`` (*AND*), ``|`` (*OR*), ``^`` (*XOR*), and ``~`` (*INVERT*);
   the results of those operators are members of the enumeration.

   .. method:: __contains__(self, value)

      Returns *True* if value is in self::

         >>> from enum import Flag, auto
         >>> class Color(Flag):
         ...     RED = auto()
         ...     GREEN = auto()
         ...     BLUE = auto()
         ...
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

      Returns all contained non-alias members::

         >>> list(Color.RED)
         [<Color.RED: 1>]
         >>> list(purple)
         [<Color.RED: 1>, <Color.BLUE: 4>]

      .. versionadded:: 3.11

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
         <Color.RED|GREEN: 3>

   .. method:: __and__(self, other)

      Returns current flag binary and'ed with other::

         >>> purple & white
         <Color.RED|BLUE: 5>
         >>> purple & Color.GREEN
         <Color: 0>

   .. method:: __xor__(self, other)

      Returns current flag binary xor'ed with other::

         >>> purple ^ white
         <Color.GREEN: 2>
         >>> purple ^ Color.GREEN
         <Color.RED|GREEN|BLUE: 7>

   .. method:: __invert__(self):

      Returns all the flags in *type(self)* that are not in self::

         >>> ~white
         <Color: 0>
         >>> ~purple
         <Color.GREEN: 2>
         >>> ~Color.RED
         <Color.GREEN|BLUE: 6>

   .. method:: _numeric_repr_

      Function used to format any remaining unnamed numeric values.  Default is
      the value's repr; common choices are :func:`hex` and :func:`oct`.

   .. note::

      Using :class:`auto` with :class:`Flag` results in integers that are powers
      of two, starting with ``1``.

   .. versionchanged:: 3.11 The *repr()* of zero-valued flags has changed.  It
      is now::

         >>> Color(0) # doctest: +SKIP
         <Color: 0>

.. class:: IntFlag

   *IntFlag* is the same as *Flag*, but its members are also integers and can be
   used anywhere that an integer can be used.

      >>> from enum import IntFlag, auto
      >>> class Color(IntFlag):
      ...     RED = auto()
      ...     GREEN = auto()
      ...     BLUE = auto()
      ...
      >>> Color.RED & 2
      <Color: 0>
      >>> Color.RED | 2
      <Color.RED|GREEN: 3>

   If any integer operation is performed with an *IntFlag* member, the result is
   not an *IntFlag*::

        >>> Color.RED + 2
        3

   If a *Flag* operation is performed with an *IntFlag* member and:

   * the result is a valid *IntFlag*: an *IntFlag* is returned
   * the result is not a valid *IntFlag*: the result depends on the *FlagBoundary* setting

   The *repr()* of unnamed zero-valued flags has changed.  It is now:

      >>> Color(0)
      <Color: 0>

   .. note::

      Using :class:`auto` with :class:`IntFlag` results in integers that are powers
      of two, starting with ``1``.

   .. versionchanged:: 3.11

      :meth:`~object.__str__` is now :meth:`!int.__str__` to better support the
      *replacement of existing constants* use-case.  :meth:`~object.__format__` was
      already :meth:`!int.__format__` for that same reason.

      Inversion of an :class:`!IntFlag` now returns a positive value that is the
      union of all flags not in the given flag, rather than a negative value.
      This matches the existing :class:`Flag` behavior.

.. class:: ReprEnum

   :class:`!ReprEnum` uses the :meth:`repr() <Enum.__repr__>` of :class:`Enum`,
   but the :class:`str() <str>` of the mixed-in data type:

   * :meth:`!int.__str__` for :class:`IntEnum` and :class:`IntFlag`
   * :meth:`!str.__str__` for :class:`StrEnum`

   Inherit from :class:`!ReprEnum` to keep the :class:`str() <str>` / :func:`format`
   of the mixed-in data type instead of using the
   :class:`Enum`-default :meth:`str() <Enum.__str__>`.


   .. versionadded:: 3.11

.. class:: EnumCheck

   *EnumCheck* contains the options used by the :func:`verify` decorator to ensure
   various constraints; failed constraints result in a :exc:`ValueError`.

   .. attribute:: UNIQUE

      Ensure that each value has only one name::

         >>> from enum import Enum, verify, UNIQUE
         >>> @verify(UNIQUE)
         ... class Color(Enum):
         ...     RED = 1
         ...     GREEN = 2
         ...     BLUE = 3
         ...     CRIMSON = 1
         Traceback (most recent call last):
         ...
         ValueError: aliases found in <enum 'Color'>: CRIMSON -> RED


   .. attribute:: CONTINUOUS

      Ensure that there are no missing values between the lowest-valued member
      and the highest-valued member::

         >>> from enum import Enum, verify, CONTINUOUS
         >>> @verify(CONTINUOUS)
         ... class Color(Enum):
         ...     RED = 1
         ...     GREEN = 2
         ...     BLUE = 5
         Traceback (most recent call last):
         ...
         ValueError: invalid enum 'Color': missing values 3, 4

   .. attribute:: NAMED_FLAGS

      Ensure that any flag groups/masks contain only named flags -- useful when
      values are specified instead of being generated by :func:`auto`::

         >>> from enum import Flag, verify, NAMED_FLAGS
         >>> @verify(NAMED_FLAGS)
         ... class Color(Flag):
         ...     RED = 1
         ...     GREEN = 2
         ...     BLUE = 4
         ...     WHITE = 15
         ...     NEON = 31
         Traceback (most recent call last):
         ...
         ValueError: invalid Flag 'Color': aliases WHITE and NEON are missing combined values of 0x18 [use enum.show_flag_values(value) for details]

   .. note::

      CONTINUOUS and NAMED_FLAGS are designed to work with integer-valued members.

   .. versionadded:: 3.11

.. class:: FlagBoundary

   *FlagBoundary* controls how out-of-range values are handled in *Flag* and its
   subclasses.

   .. attribute:: STRICT

      Out-of-range values cause a :exc:`ValueError` to be raised. This is the
      default for :class:`Flag`::

         >>> from enum import Flag, STRICT, auto
         >>> class StrictFlag(Flag, boundary=STRICT):
         ...     RED = auto()
         ...     GREEN = auto()
         ...     BLUE = auto()
         ...
         >>> StrictFlag(2**2 + 2**4)
         Traceback (most recent call last):
         ...
         ValueError: <flag 'StrictFlag'> invalid value 20
             given 0b0 10100
           allowed 0b0 00111

   .. attribute:: CONFORM

      Out-of-range values have invalid values removed, leaving a valid *Flag*
      value::

         >>> from enum import Flag, CONFORM, auto
         >>> class ConformFlag(Flag, boundary=CONFORM):
         ...     RED = auto()
         ...     GREEN = auto()
         ...     BLUE = auto()
         ...
         >>> ConformFlag(2**2 + 2**4)
         <ConformFlag.BLUE: 4>

   .. attribute:: EJECT

      Out-of-range values lose their *Flag* membership and revert to :class:`int`.

         >>> from enum import Flag, EJECT, auto
         >>> class EjectFlag(Flag, boundary=EJECT):
         ...     RED = auto()
         ...     GREEN = auto()
         ...     BLUE = auto()
         ...
         >>> EjectFlag(2**2 + 2**4)
         20

   .. attribute:: KEEP

      Out-of-range values are kept, and the *Flag* membership is kept.
      This is the default for :class:`IntFlag`::

         >>> from enum import Flag, KEEP, auto
         >>> class KeepFlag(Flag, boundary=KEEP):
         ...     RED = auto()
         ...     GREEN = auto()
         ...     BLUE = auto()
         ...
         >>> KeepFlag(2**2 + 2**4)
         <KeepFlag.BLUE|16: 20>

.. versionadded:: 3.11

---------------

Supported ``__dunder__`` names
""""""""""""""""""""""""""""""

:attr:`~EnumType.__members__` is a read-only ordered mapping of ``member_name``:``member``
items.  It is only available on the class.

:meth:`~Enum.__new__`, if specified, must create and return the enum members; it is
also a very good idea to set the member's :attr:`!_value_` appropriately.  Once
all the members are created it is no longer used.


Supported ``_sunder_`` names
""""""""""""""""""""""""""""

- :attr:`~Enum._name_` -- name of the member
- :attr:`~Enum._value_` -- value of the member; can be set in ``__new__``
- :meth:`~Enum._missing_` -- a lookup function used when a value is not found;
  may be overridden
- :attr:`~Enum._ignore_` -- a list of names, either as a :class:`list` or a
  :class:`str`, that will not be transformed into members, and will be removed
  from the final class
- :attr:`~Enum._order_` -- no longer used, kept for backward
  compatibility (class attribute, removed during class creation)
- :meth:`~Enum._generate_next_value_` -- used to get an appropriate value for
  an enum member; may be overridden

  .. note::

     For standard :class:`Enum` classes the next value chosen is the last value seen
     incremented by one.

     For :class:`Flag` classes the next value chosen will be the next highest
     power-of-two, regardless of the last value seen.

.. versionadded:: 3.6 ``_missing_``, ``_order_``, ``_generate_next_value_``
.. versionadded:: 3.7 ``_ignore_``

---------------

Utilities and Decorators
------------------------

.. class:: auto

   *auto* can be used in place of a value.  If used, the *Enum* machinery will
   call an *Enum*'s :meth:`~Enum._generate_next_value_` to get an appropriate value.
   For *Enum* and *IntEnum* that appropriate value will be the last value plus
   one; for *Flag* and *IntFlag* it will be the first power-of-two greater
   than the highest value; for *StrEnum* it will be the lower-cased version of
   the member's name.  Care must be taken if mixing *auto()* with manually
   specified values.

   *auto* instances are only resolved when at the top level of an assignment:

   * ``FIRST = auto()`` will work (auto() is replaced with ``1``);
   * ``SECOND = auto(), -2`` will work (auto is replaced with ``2``, so ``2, -2`` is
     used to create the ``SECOND`` enum member;
   * ``THREE = [auto(), -3]`` will *not* work (``<auto instance>, -3`` is used to
     create the ``THREE`` enum member)

   .. versionchanged:: 3.11.1

      In prior versions, ``auto()`` had to be the only thing
      on the assignment line to work properly.

   ``_generate_next_value_`` can be overridden to customize the values used by
   *auto*.

   .. note:: in 3.13 the default ``_generate_next_value_`` will always return
             the highest member value incremented by 1, and will fail if any
             member is an incompatible type.

.. decorator:: property

   A decorator similar to the built-in *property*, but specifically for
   enumerations.  It allows member attributes to have the same names as members
   themselves.

   .. note:: the *property* and the member must be defined in separate classes;
             for example, the *value* and *name* attributes are defined in the
             *Enum* class, and *Enum* subclasses can define members with the
             names ``value`` and ``name``.

   .. versionadded:: 3.11

.. decorator:: unique

   A :keyword:`class` decorator specifically for enumerations.  It searches an
   enumeration's :attr:`~EnumType.__members__`, gathering any aliases it finds; if any are
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

.. decorator:: verify

   A :keyword:`class` decorator specifically for enumerations.  Members from
   :class:`EnumCheck` are used to specify which constraints should be checked
   on the decorated enumeration.

   .. versionadded:: 3.11

.. decorator:: member

   A decorator for use in enums: its target will become a member.

   .. versionadded:: 3.11

.. decorator:: nonmember

   A decorator for use in enums: its target will not become a member.

   .. versionadded:: 3.11

.. decorator:: global_enum

   A decorator to change the :class:`str() <str>` and :func:`repr` of an enum
   to show its members as belonging to the module instead of its class.
   Should only be used when the enum members are exported
   to the module global namespace (see :class:`re.RegexFlag` for an example).


   .. versionadded:: 3.11

.. function:: show_flag_values(value)

   Return a list of all power-of-two integers contained in a flag *value*.

   .. versionadded:: 3.11

---------------

Notes
-----

:class:`IntEnum`, :class:`StrEnum`, and :class:`IntFlag`

   These three enum types are designed to be drop-in replacements for existing
   integer- and string-based values; as such, they have extra limitations:

   - ``__str__`` uses the value and not the name of the enum member

   - ``__format__``, because it uses ``__str__``, will also use the value of
     the enum member instead of its name

   If you do not need/want those limitations, you can either create your own
   base class by mixing in the ``int`` or ``str`` type yourself::

       >>> from enum import Enum
       >>> class MyIntEnum(int, Enum):
       ...     pass

   or you can reassign the appropriate :meth:`str`, etc., in your enum::

       >>> from enum import Enum, IntEnum
       >>> class MyIntEnum(IntEnum):
       ...     __str__ = Enum.__str__
