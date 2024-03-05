========================================
:mod:`typing` --- Support for type hints
========================================

.. testsetup:: *

   import typing
   from dataclasses import dataclass
   from typing import *

.. module:: typing
   :synopsis: Support for type hints (see :pep:`484`).

.. versionadded:: 3.5

**Source code:** :source:`Lib/typing.py`

.. note::

   The Python runtime does not enforce function and variable type annotations.
   They can be used by third party tools such as :term:`type checkers <static type checker>`,
   IDEs, linters, etc.

--------------

This module provides runtime support for type hints. For the original
specification of the typing system, see :pep:`484`. For a simplified
introduction to type hints, see :pep:`483`.


The function below takes and returns a string and is annotated as follows::

   def greeting(name: str) -> str:
       return 'Hello ' + name

In the function ``greeting``, the argument ``name`` is expected to be of type
:class:`str` and the return type :class:`str`. Subtypes are accepted as
arguments.

New features are frequently added to the ``typing`` module.
The `typing_extensions <https://pypi.org/project/typing-extensions/>`_ package
provides backports of these new features to older versions of Python.

For a summary of deprecated features and a deprecation timeline, please see
`Deprecation Timeline of Major Features`_.

.. seealso::

   `"Typing cheat sheet" <https://mypy.readthedocs.io/en/stable/cheat_sheet_py3.html>`_
       A quick overview of type hints (hosted at the mypy docs)

   "Type System Reference" section of `the mypy docs <https://mypy.readthedocs.io/en/stable/index.html>`_
      The Python typing system is standardised via PEPs, so this reference
      should broadly apply to most Python type checkers. (Some parts may still
      be specific to mypy.)

   `"Static Typing with Python" <https://typing.readthedocs.io/en/latest/>`_
      Type-checker-agnostic documentation written by the community detailing
      type system features, useful typing related tools and typing best
      practices.

.. _relevant-peps:

Relevant PEPs
=============

Since the initial introduction of type hints in :pep:`484` and :pep:`483`, a
number of PEPs have modified and enhanced Python's framework for type
annotations:

.. raw:: html

   <details>
   <summary><a style="cursor:pointer;">The full list of PEPs</a></summary>

* :pep:`526`: Syntax for Variable Annotations
     *Introducing* syntax for annotating variables outside of function
     definitions, and :data:`ClassVar`
* :pep:`544`: Protocols: Structural subtyping (static duck typing)
     *Introducing* :class:`Protocol` and the
     :func:`@runtime_checkable<runtime_checkable>` decorator
* :pep:`585`: Type Hinting Generics In Standard Collections
     *Introducing* :class:`types.GenericAlias` and the ability to use standard
     library classes as :ref:`generic types<types-genericalias>`
* :pep:`586`: Literal Types
     *Introducing* :data:`Literal`
* :pep:`589`: TypedDict: Type Hints for Dictionaries with a Fixed Set of Keys
     *Introducing* :class:`TypedDict`
* :pep:`591`: Adding a final qualifier to typing
     *Introducing* :data:`Final` and the :func:`@final<final>` decorator
* :pep:`593`: Flexible function and variable annotations
     *Introducing* :data:`Annotated`
* :pep:`604`: Allow writing union types as ``X | Y``
     *Introducing* :data:`types.UnionType` and the ability to use
     the binary-or operator ``|`` to signify a
     :ref:`union of types<types-union>`
* :pep:`612`: Parameter Specification Variables
     *Introducing* :class:`ParamSpec` and :data:`Concatenate`
* :pep:`613`: Explicit Type Aliases
     *Introducing* :data:`TypeAlias`
* :pep:`646`: Variadic Generics
     *Introducing* :data:`TypeVarTuple`
* :pep:`647`: User-Defined Type Guards
     *Introducing* :data:`TypeGuard`
* :pep:`655`: Marking individual TypedDict items as required or potentially missing
     *Introducing* :data:`Required` and :data:`NotRequired`
* :pep:`673`: Self type
    *Introducing* :data:`Self`
* :pep:`675`: Arbitrary Literal String Type
    *Introducing* :data:`LiteralString`
* :pep:`681`: Data Class Transforms
    *Introducing* the :func:`@dataclass_transform<dataclass_transform>` decorator

.. raw:: html

   </details>
   <br>

.. _type-aliases:

Type aliases
============

A type alias is defined by assigning the type to the alias. In this example,
``Vector`` and ``list[float]`` will be treated as interchangeable synonyms::

   Vector = list[float]

   def scale(scalar: float, vector: Vector) -> Vector:
       return [scalar * num for num in vector]

   # passes type checking; a list of floats qualifies as a Vector.
   new_vector = scale(2.0, [1.0, -4.2, 5.4])

Type aliases are useful for simplifying complex type signatures. For example::

   from collections.abc import Sequence

   ConnectionOptions = dict[str, str]
   Address = tuple[str, int]
   Server = tuple[Address, ConnectionOptions]

   def broadcast_message(message: str, servers: Sequence[Server]) -> None:
       ...

   # The static type checker will treat the previous type signature as
   # being exactly equivalent to this one.
   def broadcast_message(
           message: str,
           servers: Sequence[tuple[tuple[str, int], dict[str, str]]]) -> None:
       ...

Type aliases may be marked with :data:`TypeAlias` to make it explicit that
the statement is a type alias declaration, not a normal variable assignment::

   from typing import TypeAlias

   Vector: TypeAlias = list[float]

.. _distinct:

NewType
=======

Use the :class:`NewType` helper to create distinct types::

   from typing import NewType

   UserId = NewType('UserId', int)
   some_id = UserId(524313)

The static type checker will treat the new type as if it were a subclass
of the original type. This is useful in helping catch logical errors::

   def get_user_name(user_id: UserId) -> str:
       ...

   # passes type checking
   user_a = get_user_name(UserId(42351))

   # fails type checking; an int is not a UserId
   user_b = get_user_name(-1)

You may still perform all ``int`` operations on a variable of type ``UserId``,
but the result will always be of type ``int``. This lets you pass in a
``UserId`` wherever an ``int`` might be expected, but will prevent you from
accidentally creating a ``UserId`` in an invalid way::

   # 'output' is of type 'int', not 'UserId'
   output = UserId(23413) + UserId(54341)

Note that these checks are enforced only by the static type checker. At runtime,
the statement ``Derived = NewType('Derived', Base)`` will make ``Derived`` a
callable that immediately returns whatever parameter you pass it. That means
the expression ``Derived(some_value)`` does not create a new class or introduce
much overhead beyond that of a regular function call.

More precisely, the expression ``some_value is Derived(some_value)`` is always
true at runtime.

It is invalid to create a subtype of ``Derived``::

   from typing import NewType

   UserId = NewType('UserId', int)

   # Fails at runtime and does not pass type checking
   class AdminUserId(UserId): pass

However, it is possible to create a :class:`NewType` based on a 'derived' ``NewType``::

   from typing import NewType

   UserId = NewType('UserId', int)

   ProUserId = NewType('ProUserId', UserId)

and typechecking for ``ProUserId`` will work as expected.

See :pep:`484` for more details.

.. note::

   Recall that the use of a type alias declares two types to be *equivalent* to
   one another. Doing ``Alias = Original`` will make the static type checker
   treat ``Alias`` as being *exactly equivalent* to ``Original`` in all cases.
   This is useful when you want to simplify complex type signatures.

   In contrast, ``NewType`` declares one type to be a *subtype* of another.
   Doing ``Derived = NewType('Derived', Original)`` will make the static type
   checker treat ``Derived`` as a *subclass* of ``Original``, which means a
   value of type ``Original`` cannot be used in places where a value of type
   ``Derived`` is expected. This is useful when you want to prevent logic
   errors with minimal runtime cost.

.. versionadded:: 3.5.2

.. versionchanged:: 3.10
   ``NewType`` is now a class rather than a function.  As a result, there is
   some additional runtime cost when calling ``NewType`` over a regular
   function.

.. versionchanged:: 3.11
   The performance of calling ``NewType`` has been restored to its level in
   Python 3.9.

.. _annotating-callables:

Annotating callable objects
===========================

Functions -- or other :term:`callable` objects -- can be annotated using
:class:`collections.abc.Callable` or :data:`typing.Callable`.
``Callable[[int], str]`` signifies a function that takes a single parameter
of type :class:`int` and returns a :class:`str`.

For example:

.. testcode::

   from collections.abc import Callable, Awaitable

   def feeder(get_next_item: Callable[[], str]) -> None:
       ...  # Body

   def async_query(on_success: Callable[[int], None],
                   on_error: Callable[[int, Exception], None]) -> None:
       ...  # Body

   async def on_update(value: str) -> None:
       ...  # Body

   callback: Callable[[str], Awaitable[None]] = on_update

The subscription syntax must always be used with exactly two values: the
argument list and the return type.  The argument list must be a list of types,
a :class:`ParamSpec`, :data:`Concatenate`, or an ellipsis. The return type must
be a single type.

If a literal ellipsis ``...`` is given as the argument list, it indicates that
a callable with any arbitrary parameter list would be acceptable:

.. testcode::

   def concat(x: str, y: str) -> str:
       return x + y

   x: Callable[..., str]
   x = str     # OK
   x = concat  # Also OK

``Callable`` cannot express complex signatures such as functions that take a
variadic number of arguments, :ref:`overloaded functions <overload>`, or
functions that have keyword-only parameters. However, these signatures can be
expressed by defining a :class:`Protocol` class with a
:meth:`~object.__call__` method:

.. testcode::

   from collections.abc import Iterable
   from typing import Protocol

   class Combiner(Protocol):
       def __call__(self, *vals: bytes, maxlen: int | None = None) -> list[bytes]: ...

   def batch_proc(data: Iterable[bytes], cb_results: Combiner) -> bytes:
       for item in data:
           ...

   def good_cb(*vals: bytes, maxlen: int | None = None) -> list[bytes]:
       ...
   def bad_cb(*vals: bytes, maxitems: int | None) -> list[bytes]:
       ...

   batch_proc([], good_cb)  # OK
   batch_proc([], bad_cb)   # Error! Argument 2 has incompatible type because of
                            # different name and kind in the callback

Callables which take other callables as arguments may indicate that their
parameter types are dependent on each other using :class:`ParamSpec`.
Additionally, if that callable adds or removes arguments from other
callables, the :data:`Concatenate` operator may be used.  They
take the form ``Callable[ParamSpecVariable, ReturnType]`` and
``Callable[Concatenate[Arg1Type, Arg2Type, ..., ParamSpecVariable], ReturnType]``
respectively.

.. versionchanged:: 3.10
   ``Callable`` now supports :class:`ParamSpec` and :data:`Concatenate`.
   See :pep:`612` for more details.

.. seealso::
   The documentation for :class:`ParamSpec` and :class:`Concatenate` provides
   examples of usage in ``Callable``.

.. _generics:

Generics
========

Since type information about objects kept in containers cannot be statically
inferred in a generic way, many container classes in the standard library support
subscription to denote the expected types of container elements.

.. testcode::

   from collections.abc import Mapping, Sequence

   class Employee: ...

   # Sequence[Employee] indicates that all elements in the sequence
   # must be instances of "Employee".
   # Mapping[str, str] indicates that all keys and all values in the mapping
   # must be strings.
   def notify_by_email(employees: Sequence[Employee],
                       overrides: Mapping[str, str]) -> None: ...

Generics can be parameterized by using a factory available in typing
called :class:`TypeVar`.

::

   from collections.abc import Sequence
   from typing import TypeVar

   T = TypeVar('T')                  # Declare type variable "T"

   def first(l: Sequence[T]) -> T:   # Function is generic over the TypeVar "T"
       return l[0]

.. _annotating-tuples:

Annotating tuples
=================

For most containers in Python, the typing system assumes that all elements in
the container will be of the same type. For example::

   from collections.abc import Mapping

   # Type checker will infer that all elements in ``x`` are meant to be ints
   x: list[int] = []

   # Type checker error: ``list`` only accepts a single type argument:
   y: list[int, str] = [1, 'foo']

   # Type checker will infer that all keys in ``z`` are meant to be strings,
   # and that all values in ``z`` are meant to be either strings or ints
   z: Mapping[str, str | int] = {}

:class:`list` only accepts one type argument, so a type checker would emit an
error on the ``y`` assignment above. Similarly,
:class:`~collections.abc.Mapping` only accepts two type arguments: the first
indicates the type of the keys, and the second indicates the type of the
values.

Unlike most other Python containers, however, it is common in idiomatic Python
code for tuples to have elements which are not all of the same type. For this
reason, tuples are special-cased in Python's typing system. :class:`tuple`
accepts *any number* of type arguments::

   # OK: ``x`` is assigned to a tuple of length 1 where the sole element is an int
   x: tuple[int] = (5,)

   # OK: ``y`` is assigned to a tuple of length 2;
   # element 1 is an int, element 2 is a str
   y: tuple[int, str] = (5, "foo")

   # Error: the type annotation indicates a tuple of length 1,
   # but ``z`` has been assigned to a tuple of length 3
   z: tuple[int] = (1, 2, 3)

To denote a tuple which could be of *any* length, and in which all elements are
of the same type ``T``, use ``tuple[T, ...]``. To denote an empty tuple, use
``tuple[()]``. Using plain ``tuple`` as an annotation is equivalent to using
``tuple[Any, ...]``::

   x: tuple[int, ...] = (1, 2)
   # These reassignments are OK: ``tuple[int, ...]`` indicates x can be of any length
   x = (1, 2, 3)
   x = ()
   # This reassignment is an error: all elements in ``x`` must be ints
   x = ("foo", "bar")

   # ``y`` can only ever be assigned to an empty tuple
   y: tuple[()] = ()

   z: tuple = ("foo", "bar")
   # These reassignments are OK: plain ``tuple`` is equivalent to ``tuple[Any, ...]``
   z = (1, 2, 3)
   z = ()

.. _type-of-class-objects:

The type of class objects
=========================

A variable annotated with ``C`` may accept a value of type ``C``. In
contrast, a variable annotated with ``type[C]`` (or
:class:`typing.Type[C] <Type>`) may accept values that are classes
themselves -- specifically, it will accept the *class object* of ``C``. For
example::

   a = 3         # Has type ``int``
   b = int       # Has type ``type[int]``
   c = type(a)   # Also has type ``type[int]``

Note that ``type[C]`` is covariant::

   class User: ...
   class ProUser(User): ...
   class TeamUser(User): ...

   def make_new_user(user_class: type[User]) -> User:
       # ...
       return user_class()

   make_new_user(User)      # OK
   make_new_user(ProUser)   # Also OK: ``type[ProUser]`` is a subtype of ``type[User]``
   make_new_user(TeamUser)  # Still fine
   make_new_user(User())    # Error: expected ``type[User]`` but got ``User``
   make_new_user(int)       # Error: ``type[int]`` is not a subtype of ``type[User]``

The only legal parameters for :class:`type` are classes, :data:`Any`,
:ref:`type variables <generics>`, and unions of any of these types.
For example::

   def new_non_team_user(user_class: type[BasicUser | ProUser]): ...

   new_non_team_user(BasicUser)  # OK
   new_non_team_user(ProUser)    # OK
   new_non_team_user(TeamUser)   # Error: ``type[TeamUser]`` is not a subtype
                                 # of ``type[BasicUser | ProUser]``
   new_non_team_user(User)       # Also an error

``type[Any]`` is equivalent to :class:`type`, which is the root of Python's
:ref:`metaclass hierarchy <metaclasses>`.

.. _user-defined-generics:

User-defined generic types
==========================

A user-defined class can be defined as a generic class.

::

   from typing import TypeVar, Generic
   from logging import Logger

   T = TypeVar('T')

   class LoggedVar(Generic[T]):
       def __init__(self, value: T, name: str, logger: Logger) -> None:
           self.name = name
           self.logger = logger
           self.value = value

       def set(self, new: T) -> None:
           self.log('Set ' + repr(self.value))
           self.value = new

       def get(self) -> T:
           self.log('Get ' + repr(self.value))
           return self.value

       def log(self, message: str) -> None:
           self.logger.info('%s: %s', self.name, message)

``Generic[T]`` as a base class defines that the class ``LoggedVar`` takes a
single type parameter ``T`` . This also makes ``T`` valid as a type within the
class body.

The :class:`Generic` base class defines :meth:`~object.__class_getitem__` so
that ``LoggedVar[T]`` is valid as a type::

   from collections.abc import Iterable

   def zero_all_vars(vars: Iterable[LoggedVar[int]]) -> None:
       for var in vars:
           var.set(0)

A generic type can have any number of type variables. All varieties of
:class:`TypeVar` are permissible as parameters for a generic type::

   from typing import TypeVar, Generic, Sequence

   T = TypeVar('T', contravariant=True)
   B = TypeVar('B', bound=Sequence[bytes], covariant=True)
   S = TypeVar('S', int, str)

   class WeirdTrio(Generic[T, B, S]):
       ...

Each type variable argument to :class:`Generic` must be distinct.
This is thus invalid::

   from typing import TypeVar, Generic
   ...

   T = TypeVar('T')

   class Pair(Generic[T, T]):   # INVALID
       ...

You can use multiple inheritance with :class:`Generic`::

   from collections.abc import Sized
   from typing import TypeVar, Generic

   T = TypeVar('T')

   class LinkedList(Sized, Generic[T]):
       ...

When inheriting from generic classes, some type parameters could be fixed::

   from collections.abc import Mapping
   from typing import TypeVar

   T = TypeVar('T')

   class MyDict(Mapping[str, T]):
       ...

In this case ``MyDict`` has a single parameter, ``T``.

Using a generic class without specifying type parameters assumes
:data:`Any` for each position. In the following example, ``MyIterable`` is
not generic but implicitly inherits from ``Iterable[Any]``:

.. testcode::

   from collections.abc import Iterable

   class MyIterable(Iterable): # Same as Iterable[Any]
       ...

User-defined generic type aliases are also supported. Examples::

   from collections.abc import Iterable
   from typing import TypeVar
   S = TypeVar('S')
   Response = Iterable[S] | int

   # Return type here is same as Iterable[str] | int
   def response(query: str) -> Response[str]:
       ...

   T = TypeVar('T', int, float, complex)
   Vec = Iterable[tuple[T, T]]

   def inproduct(v: Vec[T]) -> T: # Same as Iterable[tuple[T, T]]
       return sum(x*y for x, y in v)

.. versionchanged:: 3.7
    :class:`Generic` no longer has a custom metaclass.

User-defined generics for parameter expressions are also supported via parameter
specification variables in the form ``Generic[P]``.  The behavior is consistent
with type variables' described above as parameter specification variables are
treated by the typing module as a specialized type variable.  The one exception
to this is that a list of types can be used to substitute a :class:`ParamSpec`::

   >>> from typing import Generic, ParamSpec, TypeVar

   >>> T = TypeVar('T')
   >>> P = ParamSpec('P')

   >>> class Z(Generic[T, P]): ...
   ...
   >>> Z[int, [dict, float]]
   __main__.Z[int, (<class 'dict'>, <class 'float'>)]

Furthermore, a generic with only one parameter specification variable will accept
parameter lists in the forms ``X[[Type1, Type2, ...]]`` and also
``X[Type1, Type2, ...]`` for aesthetic reasons.  Internally, the latter is converted
to the former, so the following are equivalent::

   >>> class X(Generic[P]): ...
   ...
   >>> X[int, str]
   __main__.X[(<class 'int'>, <class 'str'>)]
   >>> X[[int, str]]
   __main__.X[(<class 'int'>, <class 'str'>)]

Note that generics with :class:`ParamSpec` may not have correct
``__parameters__`` after substitution in some cases because they
are intended primarily for static type checking.

.. versionchanged:: 3.10
   :class:`Generic` can now be parameterized over parameter expressions.
   See :class:`ParamSpec` and :pep:`612` for more details.

A user-defined generic class can have ABCs as base classes without a metaclass
conflict. Generic metaclasses are not supported. The outcome of parameterizing
generics is cached, and most types in the typing module are :term:`hashable` and
comparable for equality.


The :data:`Any` type
====================

A special kind of type is :data:`Any`. A static type checker will treat
every type as being compatible with :data:`Any` and :data:`Any` as being
compatible with every type.

This means that it is possible to perform any operation or method call on a
value of type :data:`Any` and assign it to any variable::

   from typing import Any

   a: Any = None
   a = []          # OK
   a = 2           # OK

   s: str = ''
   s = a           # OK

   def foo(item: Any) -> int:
       # Passes type checking; 'item' could be any type,
       # and that type might have a 'bar' method
       item.bar()
       ...

Notice that no type checking is performed when assigning a value of type
:data:`Any` to a more precise type. For example, the static type checker did
not report an error when assigning ``a`` to ``s`` even though ``s`` was
declared to be of type :class:`str` and receives an :class:`int` value at
runtime!

Furthermore, all functions without a return type or parameter types will
implicitly default to using :data:`Any`::

   def legacy_parser(text):
       ...
       return data

   # A static type checker will treat the above
   # as having the same signature as:
   def legacy_parser(text: Any) -> Any:
       ...
       return data

This behavior allows :data:`Any` to be used as an *escape hatch* when you
need to mix dynamically and statically typed code.

Contrast the behavior of :data:`Any` with the behavior of :class:`object`.
Similar to :data:`Any`, every type is a subtype of :class:`object`. However,
unlike :data:`Any`, the reverse is not true: :class:`object` is *not* a
subtype of every other type.

That means when the type of a value is :class:`object`, a type checker will
reject almost all operations on it, and assigning it to a variable (or using
it as a return value) of a more specialized type is a type error. For example::

   def hash_a(item: object) -> int:
       # Fails type checking; an object does not have a 'magic' method.
       item.magic()
       ...

   def hash_b(item: Any) -> int:
       # Passes type checking
       item.magic()
       ...

   # Passes type checking, since ints and strs are subclasses of object
   hash_a(42)
   hash_a("foo")

   # Passes type checking, since Any is compatible with all types
   hash_b(42)
   hash_b("foo")

Use :class:`object` to indicate that a value could be any type in a typesafe
manner. Use :data:`Any` to indicate that a value is dynamically typed.


Nominal vs structural subtyping
===============================

Initially :pep:`484` defined the Python static type system as using
*nominal subtyping*. This means that a class ``A`` is allowed where
a class ``B`` is expected if and only if ``A`` is a subclass of ``B``.

This requirement previously also applied to abstract base classes, such as
:class:`~collections.abc.Iterable`. The problem with this approach is that a class had
to be explicitly marked to support them, which is unpythonic and unlike
what one would normally do in idiomatic dynamically typed Python code.
For example, this conforms to :pep:`484`::

   from collections.abc import Sized, Iterable, Iterator

   class Bucket(Sized, Iterable[int]):
       ...
       def __len__(self) -> int: ...
       def __iter__(self) -> Iterator[int]: ...

:pep:`544` allows to solve this problem by allowing users to write
the above code without explicit base classes in the class definition,
allowing ``Bucket`` to be implicitly considered a subtype of both ``Sized``
and ``Iterable[int]`` by static type checkers. This is known as
*structural subtyping* (or static duck-typing)::

   from collections.abc import Iterator, Iterable

   class Bucket:  # Note: no base classes
       ...
       def __len__(self) -> int: ...
       def __iter__(self) -> Iterator[int]: ...

   def collect(items: Iterable[int]) -> int: ...
   result = collect(Bucket())  # Passes type check

Moreover, by subclassing a special class :class:`Protocol`, a user
can define new custom protocols to fully enjoy structural subtyping
(see examples below).

Module contents
===============

The ``typing`` module defines the following classes, functions and decorators.

Special typing primitives
-------------------------

Special types
"""""""""""""

These can be used as types in annotations. They do not support subscription
using ``[]``.

.. data:: Any

   Special type indicating an unconstrained type.

   * Every type is compatible with :data:`Any`.
   * :data:`Any` is compatible with every type.

   .. versionchanged:: 3.11
      :data:`Any` can now be used as a base class. This can be useful for
      avoiding type checker errors with classes that can duck type anywhere or
      are highly dynamic.

.. data:: AnyStr

   A :ref:`constrained type variable <typing-constrained-typevar>`.

   Definition::

      AnyStr = TypeVar('AnyStr', str, bytes)

   ``AnyStr`` is meant to be used for functions that may accept :class:`str` or
   :class:`bytes` arguments but cannot allow the two to mix.

   For example::

      def concat(a: AnyStr, b: AnyStr) -> AnyStr:
          return a + b

      concat("foo", "bar")    # OK, output has type 'str'
      concat(b"foo", b"bar")  # OK, output has type 'bytes'
      concat("foo", b"bar")   # Error, cannot mix str and bytes

   Note that, despite its name, ``AnyStr`` has nothing to do with the
   :class:`Any` type, nor does it mean "any string". In particular, ``AnyStr``
   and ``str | bytes`` are different from each other and have different use
   cases::

      # Invalid use of AnyStr:
      # The type variable is used only once in the function signature,
      # so cannot be "solved" by the type checker
      def greet_bad(cond: bool) -> AnyStr:
          return "hi there!" if cond else b"greetings!"

      # The better way of annotating this function:
      def greet_proper(cond: bool) -> str | bytes:
          return "hi there!" if cond else b"greetings!"

.. data:: LiteralString

   Special type that includes only literal strings.

   Any string
   literal is compatible with ``LiteralString``, as is another
   ``LiteralString``. However, an object typed as just ``str`` is not.
   A string created by composing ``LiteralString``-typed objects
   is also acceptable as a ``LiteralString``.

   Example:

   .. testcode::

      def run_query(sql: LiteralString) -> None:
          ...

      def caller(arbitrary_string: str, literal_string: LiteralString) -> None:
          run_query("SELECT * FROM students")  # OK
          run_query(literal_string)  # OK
          run_query("SELECT * FROM " + literal_string)  # OK
          run_query(arbitrary_string)  # type checker error
          run_query(  # type checker error
              f"SELECT * FROM students WHERE name = {arbitrary_string}"
          )

   ``LiteralString`` is useful for sensitive APIs where arbitrary user-generated
   strings could generate problems. For example, the two cases above
   that generate type checker errors could be vulnerable to an SQL
   injection attack.

   See :pep:`675` for more details.

   .. versionadded:: 3.11

.. data:: Never

   The `bottom type <https://en.wikipedia.org/wiki/Bottom_type>`_,
   a type that has no members.

   This can be used to define a function that should never be
   called, or a function that never returns::

      from typing import Never

      def never_call_me(arg: Never) -> None:
          pass

      def int_or_str(arg: int | str) -> None:
          never_call_me(arg)  # type checker error
          match arg:
              case int():
                  print("It's an int")
              case str():
                  print("It's a str")
              case _:
                  never_call_me(arg)  # OK, arg is of type Never

   .. versionadded:: 3.11

      On older Python versions, :data:`NoReturn` may be used to express the
      same concept. ``Never`` was added to make the intended meaning more explicit.

.. data:: NoReturn

   Special type indicating that a function never returns.

   For example::

      from typing import NoReturn

      def stop() -> NoReturn:
          raise RuntimeError('no way')

   ``NoReturn`` can also be used as a
   `bottom type <https://en.wikipedia.org/wiki/Bottom_type>`_, a type that
   has no values. Starting in Python 3.11, the :data:`Never` type should
   be used for this concept instead. Type checkers should treat the two
   equivalently.

   .. versionadded:: 3.6.2

.. data:: Self

   Special type to represent the current enclosed class.

   For example::

      from typing import Self, reveal_type

      class Foo:
          def return_self(self) -> Self:
              ...
              return self

      class SubclassOfFoo(Foo): pass

      reveal_type(Foo().return_self())  # Revealed type is "Foo"
      reveal_type(SubclassOfFoo().return_self())  # Revealed type is "SubclassOfFoo"

   This annotation is semantically equivalent to the following,
   albeit in a more succinct fashion::

      from typing import TypeVar

      Self = TypeVar("Self", bound="Foo")

      class Foo:
          def return_self(self: Self) -> Self:
              ...
              return self

   In general, if something returns ``self``, as in the above examples, you
   should use ``Self`` as the return annotation. If ``Foo.return_self`` was
   annotated as returning ``"Foo"``, then the type checker would infer the
   object returned from ``SubclassOfFoo.return_self`` as being of type ``Foo``
   rather than ``SubclassOfFoo``.

   Other common use cases include:

   - :class:`classmethod`\s that are used as alternative constructors and return instances
     of the ``cls`` parameter.
   - Annotating an :meth:`~object.__enter__` method which returns self.

   You should not use ``Self`` as the return annotation if the method is not
   guaranteed to return an instance of a subclass when the class is
   subclassed::

      class Eggs:
          # Self would be an incorrect return annotation here,
          # as the object returned is always an instance of Eggs,
          # even in subclasses
          def returns_eggs(self) -> "Eggs":
              return Eggs()

   See :pep:`673` for more details.

   .. versionadded:: 3.11

.. data:: TypeAlias

   Special annotation for explicitly declaring a :ref:`type alias <type-aliases>`.

   For example::

      from typing import TypeAlias

      Factors: TypeAlias = list[int]

   ``TypeAlias`` is particularly useful for annotating
   aliases that make use of forward references, as it can be hard for type
   checkers to distinguish these from normal variable assignments:

   .. testcode::

      from typing import Generic, TypeAlias, TypeVar

      T = TypeVar("T")

      # "Box" does not exist yet,
      # so we have to use quotes for the forward reference.
      # Using ``TypeAlias`` tells the type checker that this is a type alias declaration,
      # not a variable assignment to a string.
      BoxOfStrings: TypeAlias = "Box[str]"

      class Box(Generic[T]):
          @classmethod
          def make_box_of_strings(cls) -> BoxOfStrings: ...

   See :pep:`613` for more details.

   .. versionadded:: 3.10

Special forms
"""""""""""""

These can be used as types in annotations. They all support subscription using
``[]``, but each has a unique syntax.

.. data:: Union

   Union type; ``Union[X, Y]`` is equivalent to ``X | Y`` and means either X or Y.

   To define a union, use e.g. ``Union[int, str]`` or the shorthand ``int | str``. Using that shorthand is recommended. Details:

   * The arguments must be types and there must be at least one.

   * Unions of unions are flattened, e.g.::

       Union[Union[int, str], float] == Union[int, str, float]

   * Unions of a single argument vanish, e.g.::

       Union[int] == int  # The constructor actually returns int

   * Redundant arguments are skipped, e.g.::

       Union[int, str, int] == Union[int, str] == int | str

   * When comparing unions, the argument order is ignored, e.g.::

       Union[int, str] == Union[str, int]

   * You cannot subclass or instantiate a ``Union``.

   * You cannot write ``Union[X][Y]``.

   .. versionchanged:: 3.7
      Don't remove explicit subclasses from unions at runtime.

   .. versionchanged:: 3.10
      Unions can now be written as ``X | Y``. See
      :ref:`union type expressions<types-union>`.

.. data:: Optional

   ``Optional[X]`` is equivalent to ``X | None`` (or ``Union[X, None]``).

   Note that this is not the same concept as an optional argument,
   which is one that has a default.  An optional argument with a
   default does not require the ``Optional`` qualifier on its type
   annotation just because it is optional. For example::

      def foo(arg: int = 0) -> None:
          ...

   On the other hand, if an explicit value of ``None`` is allowed, the
   use of ``Optional`` is appropriate, whether the argument is optional
   or not. For example::

      def foo(arg: Optional[int] = None) -> None:
          ...

   .. versionchanged:: 3.10
      Optional can now be written as ``X | None``. See
      :ref:`union type expressions<types-union>`.

.. data:: Concatenate

   Special form for annotating higher-order functions.

   ``Concatenate`` can be used in conjunction with :ref:`Callable <annotating-callables>` and
   :class:`ParamSpec` to annotate a higher-order callable which adds, removes,
   or transforms parameters of another
   callable.  Usage is in the form
   ``Concatenate[Arg1Type, Arg2Type, ..., ParamSpecVariable]``. ``Concatenate``
   is currently only valid when used as the first argument to a :ref:`Callable <annotating-callables>`.
   The last parameter to ``Concatenate`` must be a :class:`ParamSpec` or
   ellipsis (``...``).

   For example, to annotate a decorator ``with_lock`` which provides a
   :class:`threading.Lock` to the decorated function,  ``Concatenate`` can be
   used to indicate that ``with_lock`` expects a callable which takes in a
   ``Lock`` as the first argument, and returns a callable with a different type
   signature.  In this case, the :class:`ParamSpec` indicates that the returned
   callable's parameter types are dependent on the parameter types of the
   callable being passed in::

      from collections.abc import Callable
      from threading import Lock
      from typing import Concatenate, ParamSpec, TypeVar

      P = ParamSpec('P')
      R = TypeVar('R')

      # Use this lock to ensure that only one thread is executing a function
      # at any time.
      my_lock = Lock()

      def with_lock(f: Callable[Concatenate[Lock, P], R]) -> Callable[P, R]:
          '''A type-safe decorator which provides a lock.'''
          def inner(*args: P.args, **kwargs: P.kwargs) -> R:
              # Provide the lock as the first argument.
              return f(my_lock, *args, **kwargs)
          return inner

      @with_lock
      def sum_threadsafe(lock: Lock, numbers: list[float]) -> float:
          '''Add a list of numbers together in a thread-safe manner.'''
          with lock:
              return sum(numbers)

      # We don't need to pass in the lock ourselves thanks to the decorator.
      sum_threadsafe([1.1, 2.2, 3.3])

   .. versionadded:: 3.10

   .. seealso::

      * :pep:`612` -- Parameter Specification Variables (the PEP which introduced
        ``ParamSpec`` and ``Concatenate``)
      * :class:`ParamSpec`
      * :ref:`annotating-callables`

.. data:: Literal

   Special typing form to define "literal types".

   ``Literal`` can be used to indicate to type checkers that the
   annotated object has a value equivalent to one of the
   provided literals.

   For example::

      def validate_simple(data: Any) -> Literal[True]:  # always returns True
          ...

      Mode: TypeAlias = Literal['r', 'rb', 'w', 'wb']
      def open_helper(file: str, mode: Mode) -> str:
          ...

      open_helper('/some/path', 'r')      # Passes type check
      open_helper('/other/path', 'typo')  # Error in type checker

   ``Literal[...]`` cannot be subclassed. At runtime, an arbitrary value
   is allowed as type argument to ``Literal[...]``, but type checkers may
   impose restrictions. See :pep:`586` for more details about literal types.

   .. versionadded:: 3.8

   .. versionchanged:: 3.9.1
      ``Literal`` now de-duplicates parameters.  Equality comparisons of
      ``Literal`` objects are no longer order dependent. ``Literal`` objects
      will now raise a :exc:`TypeError` exception during equality comparisons
      if one of their parameters are not :term:`hashable`.

.. data:: ClassVar

   Special type construct to mark class variables.

   As introduced in :pep:`526`, a variable annotation wrapped in ClassVar
   indicates that a given attribute is intended to be used as a class variable
   and should not be set on instances of that class. Usage::

      class Starship:
          stats: ClassVar[dict[str, int]] = {} # class variable
          damage: int = 10                     # instance variable

   :data:`ClassVar` accepts only types and cannot be further subscribed.

   :data:`ClassVar` is not a class itself, and should not
   be used with :func:`isinstance` or :func:`issubclass`.
   :data:`ClassVar` does not change Python runtime behavior, but
   it can be used by third-party type checkers. For example, a type checker
   might flag the following code as an error::

      enterprise_d = Starship(3000)
      enterprise_d.stats = {} # Error, setting class variable on instance
      Starship.stats = {}     # This is OK

   .. versionadded:: 3.5.3

.. data:: Final

   Special typing construct to indicate final names to type checkers.

   Final names cannot be reassigned in any scope. Final names declared in class
   scopes cannot be overridden in subclasses.

   For example::

      MAX_SIZE: Final = 9000
      MAX_SIZE += 1  # Error reported by type checker

      class Connection:
          TIMEOUT: Final[int] = 10

      class FastConnector(Connection):
          TIMEOUT = 1  # Error reported by type checker

   There is no runtime checking of these properties. See :pep:`591` for
   more details.

   .. versionadded:: 3.8

.. data:: Required

   Special typing construct to mark a :class:`TypedDict` key as required.

   This is mainly useful for ``total=False`` TypedDicts. See :class:`TypedDict`
   and :pep:`655` for more details.

   .. versionadded:: 3.11

.. data:: NotRequired

   Special typing construct to mark a :class:`TypedDict` key as potentially
   missing.

   See :class:`TypedDict` and :pep:`655` for more details.

   .. versionadded:: 3.11

.. data:: Annotated

   Special typing form to add context-specific metadata to an annotation.

   Add metadata ``x`` to a given type ``T`` by using the annotation
   ``Annotated[T, x]``. Metadata added using ``Annotated`` can be used by
   static analysis tools or at runtime. At runtime, the metadata is stored
   in a :attr:`!__metadata__` attribute.

   If a library or tool encounters an annotation ``Annotated[T, x]`` and has
   no special logic for the metadata, it should ignore the metadata and simply
   treat the annotation as ``T``. As such, ``Annotated`` can be useful for code
   that wants to use annotations for purposes outside Python's static typing
   system.

   Using ``Annotated[T, x]`` as an annotation still allows for static
   typechecking of ``T``, as type checkers will simply ignore the metadata ``x``.
   In this way, ``Annotated`` differs from the
   :func:`@no_type_check <no_type_check>` decorator, which can also be used for
   adding annotations outside the scope of the typing system, but
   completely disables typechecking for a function or class.

   The responsibility of how to interpret the metadata
   lies with the tool or library encountering an
   ``Annotated`` annotation. A tool or library encountering an ``Annotated`` type
   can scan through the metadata elements to determine if they are of interest
   (e.g., using :func:`isinstance`).

   .. describe:: Annotated[<type>, <metadata>]

   Here is an example of how you might use ``Annotated`` to add metadata to
   type annotations if you were doing range analysis:

   .. testcode::

      @dataclass
      class ValueRange:
          lo: int
          hi: int

      T1 = Annotated[int, ValueRange(-10, 5)]
      T2 = Annotated[T1, ValueRange(-20, 3)]

   Details of the syntax:

   * The first argument to ``Annotated`` must be a valid type

   * Multiple metadata elements can be supplied (``Annotated`` supports variadic
     arguments)::

        @dataclass
        class ctype:
            kind: str

        Annotated[int, ValueRange(3, 10), ctype("char")]

     It is up to the tool consuming the annotations to decide whether the
     client is allowed to add multiple metadata elements to one annotation and how to
     merge those annotations.

   * ``Annotated`` must be subscripted with at least two arguments (
     ``Annotated[int]`` is not valid)

   * The order of the metadata elements is preserved and matters for equality
     checks::

        assert Annotated[int, ValueRange(3, 10), ctype("char")] != Annotated[
            int, ctype("char"), ValueRange(3, 10)
        ]

   * Nested ``Annotated`` types are flattened. The order of the metadata elements
     starts with the innermost annotation::

        assert Annotated[Annotated[int, ValueRange(3, 10)], ctype("char")] == Annotated[
            int, ValueRange(3, 10), ctype("char")
        ]

   * Duplicated metadata elements are not removed::

        assert Annotated[int, ValueRange(3, 10)] != Annotated[
            int, ValueRange(3, 10), ValueRange(3, 10)
        ]

   * ``Annotated`` can be used with nested and generic aliases:

     .. testcode::

        @dataclass
        class MaxLen:
            value: int

        T = TypeVar("T")
        Vec: TypeAlias = Annotated[list[tuple[T, T]], MaxLen(10)]

        assert Vec[int] == Annotated[list[tuple[int, int]], MaxLen(10)]

   * ``Annotated`` cannot be used with an unpacked :class:`TypeVarTuple`::

        Variadic: TypeAlias = Annotated[*Ts, Ann1]  # NOT valid

     This would be equivalent to::

        Annotated[T1, T2, T3, ..., Ann1]

     where ``T1``, ``T2``, etc. are :class:`TypeVars <TypeVar>`. This would be
     invalid: only one type should be passed to Annotated.

   * By default, :func:`get_type_hints` strips the metadata from annotations.
     Pass ``include_extras=True`` to have the metadata preserved:

     .. doctest::

        >>> from typing import Annotated, get_type_hints
        >>> def func(x: Annotated[int, "metadata"]) -> None: pass
        ...
        >>> get_type_hints(func)
        {'x': <class 'int'>, 'return': <class 'NoneType'>}
        >>> get_type_hints(func, include_extras=True)
        {'x': typing.Annotated[int, 'metadata'], 'return': <class 'NoneType'>}

   * At runtime, the metadata associated with an ``Annotated`` type can be
     retrieved via the :attr:`!__metadata__` attribute:

     .. doctest::

        >>> from typing import Annotated
        >>> X = Annotated[int, "very", "important", "metadata"]
        >>> X
        typing.Annotated[int, 'very', 'important', 'metadata']
        >>> X.__metadata__
        ('very', 'important', 'metadata')

   .. seealso::

      :pep:`593` - Flexible function and variable annotations
         The PEP introducing ``Annotated`` to the standard library.

   .. versionadded:: 3.9


.. data:: TypeGuard

   Special typing construct for marking user-defined type guard functions.

   ``TypeGuard`` can be used to annotate the return type of a user-defined
   type guard function.  ``TypeGuard`` only accepts a single type argument.
   At runtime, functions marked this way should return a boolean.

   ``TypeGuard`` aims to benefit *type narrowing* -- a technique used by static
   type checkers to determine a more precise type of an expression within a
   program's code flow.  Usually type narrowing is done by analyzing
   conditional code flow and applying the narrowing to a block of code.  The
   conditional expression here is sometimes referred to as a "type guard"::

      def is_str(val: str | float):
          # "isinstance" type guard
          if isinstance(val, str):
              # Type of ``val`` is narrowed to ``str``
              ...
          else:
              # Else, type of ``val`` is narrowed to ``float``.
              ...

   Sometimes it would be convenient to use a user-defined boolean function
   as a type guard.  Such a function should use ``TypeGuard[...]`` as its
   return type to alert static type checkers to this intention.

   Using  ``-> TypeGuard`` tells the static type checker that for a given
   function:

   1. The return value is a boolean.
   2. If the return value is ``True``, the type of its argument
      is the type inside ``TypeGuard``.

   For example::

         def is_str_list(val: list[object]) -> TypeGuard[list[str]]:
             '''Determines whether all objects in the list are strings'''
             return all(isinstance(x, str) for x in val)

         def func1(val: list[object]):
             if is_str_list(val):
                 # Type of ``val`` is narrowed to ``list[str]``.
                 print(" ".join(val))
             else:
                 # Type of ``val`` remains as ``list[object]``.
                 print("Not a list of strings!")

   If ``is_str_list`` is a class or instance method, then the type in
   ``TypeGuard`` maps to the type of the second parameter after ``cls`` or
   ``self``.

   In short, the form ``def foo(arg: TypeA) -> TypeGuard[TypeB]: ...``,
   means that if ``foo(arg)`` returns ``True``, then ``arg`` narrows from
   ``TypeA`` to ``TypeB``.

   .. note::

      ``TypeB`` need not be a narrower form of ``TypeA`` -- it can even be a
      wider form. The main reason is to allow for things like
      narrowing ``list[object]`` to ``list[str]`` even though the latter
      is not a subtype of the former, since ``list`` is invariant.
      The responsibility of writing type-safe type guards is left to the user.

   ``TypeGuard`` also works with type variables.  See :pep:`647` for more details.

   .. versionadded:: 3.10


.. data:: Unpack

   Typing operator to conceptually mark an object as having been unpacked.

   For example, using the unpack operator ``*`` on a
   :ref:`type variable tuple <typevartuple>` is equivalent to using ``Unpack``
   to mark the type variable tuple as having been unpacked::

      Ts = TypeVarTuple('Ts')
      tup: tuple[*Ts]
      # Effectively does:
      tup: tuple[Unpack[Ts]]

   In fact, ``Unpack`` can be used interchangeably with ``*`` in the context
   of :class:`typing.TypeVarTuple <TypeVarTuple>` and
   :class:`builtins.tuple <tuple>` types. You might see ``Unpack`` being used
   explicitly in older versions of Python, where ``*`` couldn't be used in
   certain places::

      # In older versions of Python, TypeVarTuple and Unpack
      # are located in the `typing_extensions` backports package.
      from typing_extensions import TypeVarTuple, Unpack

      Ts = TypeVarTuple('Ts')
      tup: tuple[*Ts]         # Syntax error on Python <= 3.10!
      tup: tuple[Unpack[Ts]]  # Semantically equivalent, and backwards-compatible

   .. versionadded:: 3.11

Building generic types
""""""""""""""""""""""

The following classes should not be used directly as annotations.
Their intended purpose is to be building blocks
for creating generic types.

.. class:: Generic

   Abstract base class for generic types.

   A generic type is typically declared by inheriting from an
   instantiation of this class with one or more type variables.
   For example, a generic mapping type might be defined as::

      class Mapping(Generic[KT, VT]):
          def __getitem__(self, key: KT) -> VT:
              ...
              # Etc.

   This class can then be used as follows::

      X = TypeVar('X')
      Y = TypeVar('Y')

      def lookup_name(mapping: Mapping[X, Y], key: X, default: Y) -> Y:
          try:
              return mapping[key]
          except KeyError:
              return default

.. _typevar:

.. class:: TypeVar(name, *constraints, bound=None, covariant=False, contravariant=False)

   Type variable.

   Usage::

      T = TypeVar('T')  # Can be anything
      S = TypeVar('S', bound=str)  # Can be any subtype of str
      A = TypeVar('A', str, bytes)  # Must be exactly str or bytes

   Type variables exist primarily for the benefit of static type
   checkers.  They serve as the parameters for generic types as well
   as for generic function and type alias definitions.
   See :class:`Generic` for more
   information on generic types.  Generic functions work as follows::

      def repeat(x: T, n: int) -> Sequence[T]:
          """Return a list containing n references to x."""
          return [x]*n


      def print_capitalized(x: S) -> S:
          """Print x capitalized, and return x."""
          print(x.capitalize())
          return x


      def concatenate(x: A, y: A) -> A:
          """Add two strings or bytes objects together."""
          return x + y

   Note that type variables can be *bound*, *constrained*, or neither, but
   cannot be both bound *and* constrained.

   Type variables may be marked covariant or contravariant by passing
   ``covariant=True`` or ``contravariant=True``.  See :pep:`484` for more
   details.  By default, type variables are invariant.

   Bound type variables and constrained type variables have different
   semantics in several important ways. Using a *bound* type variable means
   that the ``TypeVar`` will be solved using the most specific type possible::

      x = print_capitalized('a string')
      reveal_type(x)  # revealed type is str

      class StringSubclass(str):
          pass

      y = print_capitalized(StringSubclass('another string'))
      reveal_type(y)  # revealed type is StringSubclass

      z = print_capitalized(45)  # error: int is not a subtype of str

   Type variables can be bound to concrete types, abstract types (ABCs or
   protocols), and even unions of types::

      U = TypeVar('U', bound=str|bytes)  # Can be any subtype of the union str|bytes
      V = TypeVar('V', bound=SupportsAbs)  # Can be anything with an __abs__ method

   .. _typing-constrained-typevar:

   Using a *constrained* type variable, however, means that the ``TypeVar``
   can only ever be solved as being exactly one of the constraints given::

      a = concatenate('one', 'two')
      reveal_type(a)  # revealed type is str

      b = concatenate(StringSubclass('one'), StringSubclass('two'))
      reveal_type(b)  # revealed type is str, despite StringSubclass being passed in

      c = concatenate('one', b'two')  # error: type variable 'A' can be either str or bytes in a function call, but not both

   At runtime, ``isinstance(x, T)`` will raise :exc:`TypeError`.

   .. attribute:: __name__

      The name of the type variable.

   .. attribute:: __covariant__

      Whether the type var has been marked as covariant.

   .. attribute:: __contravariant__

      Whether the type var has been marked as contravariant.

   .. attribute:: __bound__

      The bound of the type variable, if any.

   .. attribute:: __constraints__

      A tuple containing the constraints of the type variable, if any.

.. _typevartuple:

.. class:: TypeVarTuple(name)

   Type variable tuple. A specialized form of :ref:`type variable <typevar>`
   that enables *variadic* generics.

   Usage::

      T = TypeVar("T")
      Ts = TypeVarTuple("Ts")

      def move_first_element_to_last(tup: tuple[T, *Ts]) -> tuple[*Ts, T]:
          return (*tup[1:], tup[0])

   A normal type variable enables parameterization with a single type. A type
   variable tuple, in contrast, allows parameterization with an
   *arbitrary* number of types by acting like an *arbitrary* number of type
   variables wrapped in a tuple. For example::

      # T is bound to int, Ts is bound to ()
      # Return value is (1,), which has type tuple[int]
      move_first_element_to_last(tup=(1,))

      # T is bound to int, Ts is bound to (str,)
      # Return value is ('spam', 1), which has type tuple[str, int]
      move_first_element_to_last(tup=(1, 'spam'))

      # T is bound to int, Ts is bound to (str, float)
      # Return value is ('spam', 3.0, 1), which has type tuple[str, float, int]
      move_first_element_to_last(tup=(1, 'spam', 3.0))

      # This fails to type check (and fails at runtime)
      # because tuple[()] is not compatible with tuple[T, *Ts]
      # (at least one element is required)
      move_first_element_to_last(tup=())

   Note the use of the unpacking operator ``*`` in ``tuple[T, *Ts]``.
   Conceptually, you can think of ``Ts`` as a tuple of type variables
   ``(T1, T2, ...)``. ``tuple[T, *Ts]`` would then become
   ``tuple[T, *(T1, T2, ...)]``, which is equivalent to
   ``tuple[T, T1, T2, ...]``. (Note that in older versions of Python, you might
   see this written using :data:`Unpack <Unpack>` instead, as
   ``Unpack[Ts]``.)

   Type variable tuples must *always* be unpacked. This helps distinguish type
   variable tuples from normal type variables::

      x: Ts          # Not valid
      x: tuple[Ts]   # Not valid
      x: tuple[*Ts]  # The correct way to do it

   Type variable tuples can be used in the same contexts as normal type
   variables. For example, in class definitions, arguments, and return types::

      Shape = TypeVarTuple("Shape")
      class Array(Generic[*Shape]):
          def __getitem__(self, key: tuple[*Shape]) -> float: ...
          def __abs__(self) -> "Array[*Shape]": ...
          def get_shape(self) -> tuple[*Shape]: ...

   Type variable tuples can be happily combined with normal type variables:

   .. testcode::

      DType = TypeVar('DType')
      Shape = TypeVarTuple('Shape')

      class Array(Generic[DType, *Shape]):  # This is fine
          pass

      class Array2(Generic[*Shape, DType]):  # This would also be fine
          pass

      class Height: ...
      class Width: ...

      float_array_1d: Array[float, Height] = Array()     # Totally fine
      int_array_2d: Array[int, Height, Width] = Array()  # Yup, fine too

   However, note that at most one type variable tuple may appear in a single
   list of type arguments or type parameters::

      x: tuple[*Ts, *Ts]                     # Not valid
      class Array(Generic[*Shape, *Shape]):  # Not valid
          pass

   Finally, an unpacked type variable tuple can be used as the type annotation
   of ``*args``::

      def call_soon(
               callback: Callable[[*Ts], None],
               *args: *Ts
      ) -> None:
          ...
          callback(*args)

   In contrast to non-unpacked annotations of ``*args`` - e.g. ``*args: int``,
   which would specify that *all* arguments are ``int`` - ``*args: *Ts``
   enables reference to the types of the *individual* arguments in ``*args``.
   Here, this allows us to ensure the types of the ``*args`` passed
   to ``call_soon`` match the types of the (positional) arguments of
   ``callback``.

   See :pep:`646` for more details on type variable tuples.

   .. attribute:: __name__

      The name of the type variable tuple.

   .. versionadded:: 3.11

.. class:: ParamSpec(name, *, bound=None, covariant=False, contravariant=False)

   Parameter specification variable.  A specialized version of
   :ref:`type variables <typevar>`.

   Usage::

      P = ParamSpec('P')

   Parameter specification variables exist primarily for the benefit of static
   type checkers.  They are used to forward the parameter types of one
   callable to another callable -- a pattern commonly found in higher order
   functions and decorators.  They are only valid when used in ``Concatenate``,
   or as the first argument to ``Callable``, or as parameters for user-defined
   Generics.  See :class:`Generic` for more information on generic types.

   For example, to add basic logging to a function, one can create a decorator
   ``add_logging`` to log function calls.  The parameter specification variable
   tells the type checker that the callable passed into the decorator and the
   new callable returned by it have inter-dependent type parameters::

      from collections.abc import Callable
      from typing import TypeVar, ParamSpec
      import logging

      T = TypeVar('T')
      P = ParamSpec('P')

      def add_logging(f: Callable[P, T]) -> Callable[P, T]:
          '''A type-safe decorator to add logging to a function.'''
          def inner(*args: P.args, **kwargs: P.kwargs) -> T:
              logging.info(f'{f.__name__} was called')
              return f(*args, **kwargs)
          return inner

      @add_logging
      def add_two(x: float, y: float) -> float:
          '''Add two numbers together.'''
          return x + y

   Without ``ParamSpec``, the simplest way to annotate this previously was to
   use a :class:`TypeVar` with bound ``Callable[..., Any]``.  However this
   causes two problems:

   1. The type checker can't type check the ``inner`` function because
      ``*args`` and ``**kwargs`` have to be typed :data:`Any`.
   2. :func:`~cast` may be required in the body of the ``add_logging``
      decorator when returning the ``inner`` function, or the static type
      checker must be told to ignore the ``return inner``.

   .. attribute:: args
   .. attribute:: kwargs

      Since ``ParamSpec`` captures both positional and keyword parameters,
      ``P.args`` and ``P.kwargs`` can be used to split a ``ParamSpec`` into its
      components.  ``P.args`` represents the tuple of positional parameters in a
      given call and should only be used to annotate ``*args``.  ``P.kwargs``
      represents the mapping of keyword parameters to their values in a given call,
      and should be only be used to annotate ``**kwargs``.  Both
      attributes require the annotated parameter to be in scope. At runtime,
      ``P.args`` and ``P.kwargs`` are instances respectively of
      :class:`ParamSpecArgs` and :class:`ParamSpecKwargs`.

   .. attribute:: __name__

      The name of the parameter specification.

   Parameter specification variables created with ``covariant=True`` or
   ``contravariant=True`` can be used to declare covariant or contravariant
   generic types.  The ``bound`` argument is also accepted, similar to
   :class:`TypeVar`.  However the actual semantics of these keywords are yet to
   be decided.

   .. versionadded:: 3.10

   .. note::
      Only parameter specification variables defined in global scope can
      be pickled.

   .. seealso::
      * :pep:`612` -- Parameter Specification Variables (the PEP which introduced
        ``ParamSpec`` and ``Concatenate``)
      * :data:`Concatenate`
      * :ref:`annotating-callables`

.. data:: ParamSpecArgs
.. data:: ParamSpecKwargs

   Arguments and keyword arguments attributes of a :class:`ParamSpec`. The
   ``P.args`` attribute of a ``ParamSpec`` is an instance of ``ParamSpecArgs``,
   and ``P.kwargs`` is an instance of ``ParamSpecKwargs``. They are intended
   for runtime introspection and have no special meaning to static type checkers.

   Calling :func:`get_origin` on either of these objects will return the
   original ``ParamSpec``:

   .. doctest::

      >>> from typing import ParamSpec, get_origin
      >>> P = ParamSpec("P")
      >>> get_origin(P.args) is P
      True
      >>> get_origin(P.kwargs) is P
      True

   .. versionadded:: 3.10


Other special directives
""""""""""""""""""""""""

These functions and classes should not be used directly as annotations.
Their intended purpose is to be building blocks for creating and declaring
types.

.. class:: NamedTuple

   Typed version of :func:`collections.namedtuple`.

   Usage::

       class Employee(NamedTuple):
           name: str
           id: int

   This is equivalent to::

       Employee = collections.namedtuple('Employee', ['name', 'id'])

   To give a field a default value, you can assign to it in the class body::

      class Employee(NamedTuple):
          name: str
          id: int = 3

      employee = Employee('Guido')
      assert employee.id == 3

   Fields with a default value must come after any fields without a default.

   The resulting class has an extra attribute ``__annotations__`` giving a
   dict that maps the field names to the field types.  (The field names are in
   the ``_fields`` attribute and the default values are in the
   ``_field_defaults`` attribute, both of which are part of the :func:`~collections.namedtuple`
   API.)

   ``NamedTuple`` subclasses can also have docstrings and methods::

      class Employee(NamedTuple):
          """Represents an employee."""
          name: str
          id: int = 3

          def __repr__(self) -> str:
              return f'<Employee {self.name}, id={self.id}>'

   ``NamedTuple`` subclasses can be generic::

      class Group(NamedTuple, Generic[T]):
          key: T
          group: list[T]

   Backward-compatible usage::

       Employee = NamedTuple('Employee', [('name', str), ('id', int)])

   .. versionchanged:: 3.6
      Added support for :pep:`526` variable annotation syntax.

   .. versionchanged:: 3.6.1
      Added support for default values, methods, and docstrings.

   .. versionchanged:: 3.8
      The ``_field_types`` and ``__annotations__`` attributes are
      now regular dictionaries instead of instances of ``OrderedDict``.

   .. versionchanged:: 3.9
      Removed the ``_field_types`` attribute in favor of the more
      standard ``__annotations__`` attribute which has the same information.

   .. versionchanged:: 3.11
      Added support for generic namedtuples.

.. class:: NewType(name, tp)

   Helper class to create low-overhead :ref:`distinct types <distinct>`.

   A ``NewType`` is considered a distinct type by a typechecker. At runtime,
   however, calling a ``NewType`` returns its argument unchanged.

   Usage::

      UserId = NewType('UserId', int)  # Declare the NewType "UserId"
      first_user = UserId(1)  # "UserId" returns the argument unchanged at runtime

   .. attribute:: __module__

      The module in which the new type is defined.

   .. attribute:: __name__

      The name of the new type.

   .. attribute:: __supertype__

      The type that the new type is based on.

   .. versionadded:: 3.5.2

   .. versionchanged:: 3.10
      ``NewType`` is now a class rather than a function.

.. class:: Protocol(Generic)

   Base class for protocol classes.

   Protocol classes are defined like this::

      class Proto(Protocol):
          def meth(self) -> int:
              ...

   Such classes are primarily used with static type checkers that recognize
   structural subtyping (static duck-typing), for example::

      class C:
          def meth(self) -> int:
              return 0

      def func(x: Proto) -> int:
          return x.meth()

      func(C())  # Passes static type check

   See :pep:`544` for more details. Protocol classes decorated with
   :func:`runtime_checkable` (described later) act as simple-minded runtime
   protocols that check only the presence of given attributes, ignoring their
   type signatures.

   Protocol classes can be generic, for example::

      T = TypeVar("T")

      class GenProto(Protocol[T]):
          def meth(self) -> T:
              ...

   .. versionadded:: 3.8

.. decorator:: runtime_checkable

   Mark a protocol class as a runtime protocol.

   Such a protocol can be used with :func:`isinstance` and :func:`issubclass`.
   This raises :exc:`TypeError` when applied to a non-protocol class.  This
   allows a simple-minded structural check, very similar to "one trick ponies"
   in :mod:`collections.abc` such as :class:`~collections.abc.Iterable`.  For example::

      @runtime_checkable
      class Closable(Protocol):
          def close(self): ...

      assert isinstance(open('/some/file'), Closable)

      @runtime_checkable
      class Named(Protocol):
          name: str

      import threading
      assert isinstance(threading.Thread(name='Bob'), Named)

   .. note::

        :func:`!runtime_checkable` will check only the presence of the required
        methods or attributes, not their type signatures or types.
        For example, :class:`ssl.SSLObject`
        is a class, therefore it passes an :func:`issubclass`
        check against :ref:`Callable <annotating-callables>`. However, the
        ``ssl.SSLObject.__init__`` method exists only to raise a
        :exc:`TypeError` with a more informative message, therefore making
        it impossible to call (instantiate) :class:`ssl.SSLObject`.

   .. note::

        An :func:`isinstance` check against a runtime-checkable protocol can be
        surprisingly slow compared to an ``isinstance()`` check against
        a non-protocol class. Consider using alternative idioms such as
        :func:`hasattr` calls for structural checks in performance-sensitive
        code.

   .. versionadded:: 3.8


.. class:: TypedDict(dict)

   Special construct to add type hints to a dictionary.
   At runtime it is a plain :class:`dict`.

   ``TypedDict`` declares a dictionary type that expects all of its
   instances to have a certain set of keys, where each key is
   associated with a value of a consistent type. This expectation
   is not checked at runtime but is only enforced by type checkers.
   Usage::

      class Point2D(TypedDict):
          x: int
          y: int
          label: str

      a: Point2D = {'x': 1, 'y': 2, 'label': 'good'}  # OK
      b: Point2D = {'z': 3, 'label': 'bad'}           # Fails type check

      assert Point2D(x=1, y=2, label='first') == dict(x=1, y=2, label='first')

   To allow using this feature with older versions of Python that do not
   support :pep:`526`, ``TypedDict`` supports two additional equivalent
   syntactic forms:

   * Using a literal :class:`dict` as the second argument::

      Point2D = TypedDict('Point2D', {'x': int, 'y': int, 'label': str})

   * Using keyword arguments::

      Point2D = TypedDict('Point2D', x=int, y=int, label=str)

   .. deprecated-removed:: 3.11 3.13
      The keyword-argument syntax is deprecated in 3.11 and will be removed
      in 3.13. It may also be unsupported by static type checkers.

   The functional syntax should also be used when any of the keys are not valid
   :ref:`identifiers <identifiers>`, for example because they are keywords or contain hyphens.
   Example::

      # raises SyntaxError
      class Point2D(TypedDict):
          in: int  # 'in' is a keyword
          x-y: int  # name with hyphens

      # OK, functional syntax
      Point2D = TypedDict('Point2D', {'in': int, 'x-y': int})

   By default, all keys must be present in a ``TypedDict``. It is possible to
   mark individual keys as non-required using :data:`NotRequired`::

      class Point2D(TypedDict):
          x: int
          y: int
          label: NotRequired[str]

      # Alternative syntax
      Point2D = TypedDict('Point2D', {'x': int, 'y': int, 'label': NotRequired[str]})

   This means that a ``Point2D`` ``TypedDict`` can have the ``label``
   key omitted.

   It is also possible to mark all keys as non-required by default
   by specifying a totality of ``False``::

      class Point2D(TypedDict, total=False):
          x: int
          y: int

      # Alternative syntax
      Point2D = TypedDict('Point2D', {'x': int, 'y': int}, total=False)

   This means that a ``Point2D`` ``TypedDict`` can have any of the keys
   omitted. A type checker is only expected to support a literal ``False`` or
   ``True`` as the value of the ``total`` argument. ``True`` is the default,
   and makes all items defined in the class body required.

   Individual keys of a ``total=False`` ``TypedDict`` can be marked as
   required using :data:`Required`::

      class Point2D(TypedDict, total=False):
          x: Required[int]
          y: Required[int]
          label: str

      # Alternative syntax
      Point2D = TypedDict('Point2D', {
          'x': Required[int],
          'y': Required[int],
          'label': str
      }, total=False)

   It is possible for a ``TypedDict`` type to inherit from one or more other ``TypedDict`` types
   using the class-based syntax.
   Usage::

      class Point3D(Point2D):
          z: int

   ``Point3D`` has three items: ``x``, ``y`` and ``z``. It is equivalent to this
   definition::

      class Point3D(TypedDict):
          x: int
          y: int
          z: int

   A ``TypedDict`` cannot inherit from a non-\ ``TypedDict`` class,
   except for :class:`Generic`. For example::

      class X(TypedDict):
          x: int

      class Y(TypedDict):
          y: int

      class Z(object): pass  # A non-TypedDict class

      class XY(X, Y): pass  # OK

      class XZ(X, Z): pass  # raises TypeError

   A ``TypedDict`` can be generic:

   .. testcode::

      T = TypeVar("T")

      class Group(TypedDict, Generic[T]):
          key: T
          group: list[T]

   A ``TypedDict`` can be introspected via annotations dicts
   (see :ref:`annotations-howto` for more information on annotations best practices),
   :attr:`__total__`, :attr:`__required_keys__`, and :attr:`__optional_keys__`.

   .. attribute:: __total__

      ``Point2D.__total__`` gives the value of the ``total`` argument.
      Example:

      .. doctest::

         >>> from typing import TypedDict
         >>> class Point2D(TypedDict): pass
         >>> Point2D.__total__
         True
         >>> class Point2D(TypedDict, total=False): pass
         >>> Point2D.__total__
         False
         >>> class Point3D(Point2D): pass
         >>> Point3D.__total__
         True

      This attribute reflects *only* the value of the ``total`` argument
      to the current ``TypedDict`` class, not whether the class is semantically
      total. For example, a ``TypedDict`` with ``__total__`` set to True may
      have keys marked with :data:`NotRequired`, or it may inherit from another
      ``TypedDict`` with ``total=False``. Therefore, it is generally better to use
      :attr:`__required_keys__` and :attr:`__optional_keys__` for introspection.

   .. attribute:: __required_keys__

      .. versionadded:: 3.9

   .. attribute:: __optional_keys__

      ``Point2D.__required_keys__`` and ``Point2D.__optional_keys__`` return
      :class:`frozenset` objects containing required and non-required keys, respectively.

      Keys marked with :data:`Required` will always appear in ``__required_keys__``
      and keys marked with :data:`NotRequired` will always appear in ``__optional_keys__``.

      For backwards compatibility with Python 3.10 and below,
      it is also possible to use inheritance to declare both required and
      non-required keys in the same ``TypedDict`` . This is done by declaring a
      ``TypedDict`` with one value for the ``total`` argument and then
      inheriting from it in another ``TypedDict`` with a different value for
      ``total``:

      .. doctest::

         >>> class Point2D(TypedDict, total=False):
         ...     x: int
         ...     y: int
         ...
         >>> class Point3D(Point2D):
         ...     z: int
         ...
         >>> Point3D.__required_keys__ == frozenset({'z'})
         True
         >>> Point3D.__optional_keys__ == frozenset({'x', 'y'})
         True

      .. versionadded:: 3.9

      .. note::

         If ``from __future__ import annotations`` is used or if annotations
         are given as strings, annotations are not evaluated when the
         ``TypedDict`` is defined. Therefore, the runtime introspection that
         ``__required_keys__`` and ``__optional_keys__`` rely on may not work
         properly, and the values of the attributes may be incorrect.

   See :pep:`589` for more examples and detailed rules of using ``TypedDict``.

   .. versionadded:: 3.8

   .. versionchanged:: 3.11
      Added support for marking individual keys as :data:`Required` or :data:`NotRequired`.
      See :pep:`655`.

   .. versionchanged:: 3.11
      Added support for generic ``TypedDict``\ s.

Protocols
---------

The following protocols are provided by the typing module. All are decorated
with :func:`@runtime_checkable <runtime_checkable>`.

.. class:: SupportsAbs

    An ABC with one abstract method ``__abs__`` that is covariant
    in its return type.

.. class:: SupportsBytes

    An ABC with one abstract method ``__bytes__``.

.. class:: SupportsComplex

    An ABC with one abstract method ``__complex__``.

.. class:: SupportsFloat

    An ABC with one abstract method ``__float__``.

.. class:: SupportsIndex

    An ABC with one abstract method ``__index__``.

    .. versionadded:: 3.8

.. class:: SupportsInt

    An ABC with one abstract method ``__int__``.

.. class:: SupportsRound

    An ABC with one abstract method ``__round__``
    that is covariant in its return type.

ABCs for working with IO
------------------------

.. class:: IO
           TextIO
           BinaryIO

   Generic type ``IO[AnyStr]`` and its subclasses ``TextIO(IO[str])``
   and ``BinaryIO(IO[bytes])``
   represent the types of I/O streams such as returned by
   :func:`open`.

Functions and decorators
------------------------

.. function:: cast(typ, val)

   Cast a value to a type.

   This returns the value unchanged.  To the type checker this
   signals that the return value has the designated type, but at
   runtime we intentionally don't check anything (we want this
   to be as fast as possible).

.. function:: assert_type(val, typ, /)

   Ask a static type checker to confirm that *val* has an inferred type of *typ*.

   At runtime this does nothing: it returns the first argument unchanged with no
   checks or side effects, no matter the actual type of the argument.

   When a static type checker encounters a call to ``assert_type()``, it
   emits an error if the value is not of the specified type::

       def greet(name: str) -> None:
           assert_type(name, str)  # OK, inferred type of `name` is `str`
           assert_type(name, int)  # type checker error

   This function is useful for ensuring the type checker's understanding of a
   script is in line with the developer's intentions::

       def complex_function(arg: object):
           # Do some complex type-narrowing logic,
           # after which we hope the inferred type will be `int`
           ...
           # Test whether the type checker correctly understands our function
           assert_type(arg, int)

   .. versionadded:: 3.11

.. function:: assert_never(arg, /)

   Ask a static type checker to confirm that a line of code is unreachable.

   Example::

       def int_or_str(arg: int | str) -> None:
           match arg:
               case int():
                   print("It's an int")
               case str():
                   print("It's a str")
               case _ as unreachable:
                   assert_never(unreachable)

   Here, the annotations allow the type checker to infer that the
   last case can never execute, because ``arg`` is either
   an :class:`int` or a :class:`str`, and both options are covered by
   earlier cases.

   If a type checker finds that a call to ``assert_never()`` is
   reachable, it will emit an error. For example, if the type annotation
   for ``arg`` was instead ``int | str | float``, the type checker would
   emit an error pointing out that ``unreachable`` is of type :class:`float`.
   For a call to ``assert_never`` to pass type checking, the inferred type of
   the argument passed in must be the bottom type, :data:`Never`, and nothing
   else.

   At runtime, this throws an exception when called.

   .. seealso::
      `Unreachable Code and Exhaustiveness Checking
      <https://typing.readthedocs.io/en/latest/source/unreachable.html>`__ has more
      information about exhaustiveness checking with static typing.

   .. versionadded:: 3.11

.. function:: reveal_type(obj, /)

   Ask a static type checker to reveal the inferred type of an expression.

   When a static type checker encounters a call to this function,
   it emits a diagnostic with the inferred type of the argument. For example::

      x: int = 1
      reveal_type(x)  # Revealed type is "builtins.int"

   This can be useful when you want to debug how your type checker
   handles a particular piece of code.

   At runtime, this function prints the runtime type of its argument to
   :data:`sys.stderr` and returns the argument unchanged (allowing the call to
   be used within an expression)::

      x = reveal_type(1)  # prints "Runtime type is int"
      print(x)  # prints "1"

   Note that the runtime type may be different from (more or less specific
   than) the type statically inferred by a type checker.

   Most type checkers support ``reveal_type()`` anywhere, even if the
   name is not imported from ``typing``. Importing the name from
   ``typing``, however, allows your code to run without runtime errors and
   communicates intent more clearly.

   .. versionadded:: 3.11

.. decorator:: dataclass_transform(*, eq_default=True, order_default=False, \
                                   kw_only_default=False, \
                                   field_specifiers=(), **kwargs)

   Decorator to mark an object as providing
   :func:`dataclass <dataclasses.dataclass>`-like behavior.

   ``dataclass_transform`` may be used to
   decorate a class, metaclass, or a function that is itself a decorator.
   The presence of ``@dataclass_transform()`` tells a static type checker that the
   decorated object performs runtime "magic" that
   transforms a class in a similar way to
   :func:`@dataclasses.dataclass <dataclasses.dataclass>`.

   Example usage with a decorator function:

   .. testcode::

      T = TypeVar("T")

      @dataclass_transform()
      def create_model(cls: type[T]) -> type[T]:
          ...
          return cls

      @create_model
      class CustomerModel:
          id: int
          name: str

   On a base class::

      @dataclass_transform()
      class ModelBase: ...

      class CustomerModel(ModelBase):
          id: int
          name: str

   On a metaclass::

      @dataclass_transform()
      class ModelMeta(type): ...

      class ModelBase(metaclass=ModelMeta): ...

      class CustomerModel(ModelBase):
          id: int
          name: str

   The ``CustomerModel`` classes defined above will
   be treated by type checkers similarly to classes created with
   :func:`@dataclasses.dataclass <dataclasses.dataclass>`.
   For example, type checkers will assume these classes have
   ``__init__`` methods that accept ``id`` and ``name``.

   The decorated class, metaclass, or function may accept the following bool
   arguments which type checkers will assume have the same effect as they
   would have on the
   :func:`@dataclasses.dataclass<dataclasses.dataclass>` decorator: ``init``,
   ``eq``, ``order``, ``unsafe_hash``, ``frozen``, ``match_args``,
   ``kw_only``, and ``slots``. It must be possible for the value of these
   arguments (``True`` or ``False``) to be statically evaluated.

   The arguments to the ``dataclass_transform`` decorator can be used to
   customize the default behaviors of the decorated class, metaclass, or
   function:

   :param bool eq_default:
       Indicates whether the ``eq`` parameter is assumed to be
       ``True`` or ``False`` if it is omitted by the caller.
       Defaults to ``True``.

   :param bool order_default:
       Indicates whether the ``order`` parameter is
       assumed to be ``True`` or ``False`` if it is omitted by the caller.
       Defaults to ``False``.

   :param bool kw_only_default:
       Indicates whether the ``kw_only`` parameter is
       assumed to be ``True`` or ``False`` if it is omitted by the caller.
       Defaults to ``False``.

   :param field_specifiers:
       Specifies a static list of supported classes
       or functions that describe fields, similar to :func:`dataclasses.field`.
       Defaults to ``()``.
   :type field_specifiers: tuple[Callable[..., Any], ...]

   :param Any \**kwargs:
       Arbitrary other keyword arguments are accepted in order to allow for
       possible future extensions.

   Type checkers recognize the following optional parameters on field
   specifiers:

   .. list-table:: **Recognised parameters for field specifiers**
      :header-rows: 1
      :widths: 20 80

      * - Parameter name
        - Description
      * - ``init``
        - Indicates whether the field should be included in the
          synthesized ``__init__`` method. If unspecified, ``init`` defaults to
          ``True``.
      * - ``default``
        - Provides the default value for the field.
      * - ``default_factory``
        - Provides a runtime callback that returns the
          default value for the field. If neither ``default`` nor
          ``default_factory`` are specified, the field is assumed to have no
          default value and must be provided a value when the class is
          instantiated.
      * - ``factory``
        - An alias for the ``default_factory`` parameter on field specifiers.
      * - ``kw_only``
        - Indicates whether the field should be marked as
          keyword-only. If ``True``, the field will be keyword-only. If
          ``False``, it will not be keyword-only. If unspecified, the value of
          the ``kw_only`` parameter on the object decorated with
          ``dataclass_transform`` will be used, or if that is unspecified, the
          value of ``kw_only_default`` on ``dataclass_transform`` will be used.
      * - ``alias``
        - Provides an alternative name for the field. This alternative
          name is used in the synthesized ``__init__`` method.

   At runtime, this decorator records its arguments in the
   ``__dataclass_transform__`` attribute on the decorated object.
   It has no other runtime effect.

   See :pep:`681` for more details.

   .. versionadded:: 3.11

.. _overload:

.. decorator:: overload

   Decorator for creating overloaded functions and methods.

   The ``@overload`` decorator allows describing functions and methods
   that support multiple different combinations of argument types. A series
   of ``@overload``-decorated definitions must be followed by exactly one
   non-``@overload``-decorated definition (for the same function/method).

   ``@overload``-decorated definitions are for the benefit of the
   type checker only, since they will be overwritten by the
   non-``@overload``-decorated definition. The non-``@overload``-decorated
   definition, meanwhile, will be used at
   runtime but should be ignored by a type checker.  At runtime, calling
   an ``@overload``-decorated function directly will raise
   :exc:`NotImplementedError`.

   An example of overload that gives a more
   precise type than can be expressed using a union or a type variable:

   .. testcode::

      @overload
      def process(response: None) -> None:
          ...
      @overload
      def process(response: int) -> tuple[int, str]:
          ...
      @overload
      def process(response: bytes) -> str:
          ...
      def process(response):
          ...  # actual implementation goes here

   See :pep:`484` for more details and comparison with other typing semantics.

   .. versionchanged:: 3.11
      Overloaded functions can now be introspected at runtime using
      :func:`get_overloads`.


.. function:: get_overloads(func)

   Return a sequence of :func:`@overload <overload>`-decorated definitions for
   *func*.

   *func* is the function object for the implementation of the
   overloaded function. For example, given the definition of ``process`` in
   the documentation for :func:`@overload <overload>`,
   ``get_overloads(process)`` will return a sequence of three function objects
   for the three defined overloads. If called on a function with no overloads,
   ``get_overloads()`` returns an empty sequence.

   ``get_overloads()`` can be used for introspecting an overloaded function at
   runtime.

   .. versionadded:: 3.11


.. function:: clear_overloads()

   Clear all registered overloads in the internal registry.

   This can be used to reclaim the memory used by the registry.

   .. versionadded:: 3.11


.. decorator:: final

   Decorator to indicate final methods and final classes.

   Decorating a method with ``@final`` indicates to a type checker that the
   method cannot be overridden in a subclass. Decorating a class with ``@final``
   indicates that it cannot be subclassed.

   For example::

      class Base:
          @final
          def done(self) -> None:
              ...
      class Sub(Base):
          def done(self) -> None:  # Error reported by type checker
              ...

      @final
      class Leaf:
          ...
      class Other(Leaf):  # Error reported by type checker
          ...

   There is no runtime checking of these properties. See :pep:`591` for
   more details.

   .. versionadded:: 3.8

   .. versionchanged:: 3.11
      The decorator will now attempt to set a ``__final__`` attribute to ``True``
      on the decorated object. Thus, a check like
      ``if getattr(obj, "__final__", False)`` can be used at runtime
      to determine whether an object ``obj`` has been marked as final.
      If the decorated object does not support setting attributes,
      the decorator returns the object unchanged without raising an exception.


.. decorator:: no_type_check

   Decorator to indicate that annotations are not type hints.

   This works as a class or function :term:`decorator`.  With a class, it
   applies recursively to all methods and classes defined in that class
   (but not to methods defined in its superclasses or subclasses). Type
   checkers will ignore all annotations in a function or class with this
   decorator.

   ``@no_type_check`` mutates the decorated object in place.

.. decorator:: no_type_check_decorator

   Decorator to give another decorator the :func:`no_type_check` effect.

   This wraps the decorator with something that wraps the decorated
   function in :func:`no_type_check`.

.. decorator:: type_check_only

   Decorator to mark a class or function as unavailable at runtime.

   This decorator is itself not available at runtime. It is mainly
   intended to mark classes that are defined in type stub files if
   an implementation returns an instance of a private class::

      @type_check_only
      class Response:  # private or not available at runtime
          code: int
          def get_header(self, name: str) -> str: ...

      def fetch_response() -> Response: ...

   Note that returning instances of private classes is not recommended.
   It is usually preferable to make such classes public.

Introspection helpers
---------------------

.. function:: get_type_hints(obj, globalns=None, localns=None, include_extras=False)

   Return a dictionary containing type hints for a function, method, module
   or class object.

   This is often the same as ``obj.__annotations__``. In addition,
   forward references encoded as string literals are handled by evaluating
   them in ``globals`` and ``locals`` namespaces. For a class ``C``, return
   a dictionary constructed by merging all the ``__annotations__`` along
   ``C.__mro__`` in reverse order.

   The function recursively replaces all ``Annotated[T, ...]`` with ``T``,
   unless ``include_extras`` is set to ``True`` (see :class:`Annotated` for
   more information). For example:

   .. testcode::

       class Student(NamedTuple):
           name: Annotated[str, 'some marker']

       assert get_type_hints(Student) == {'name': str}
       assert get_type_hints(Student, include_extras=False) == {'name': str}
       assert get_type_hints(Student, include_extras=True) == {
           'name': Annotated[str, 'some marker']
       }

   .. note::

      :func:`get_type_hints` does not work with imported
      :ref:`type aliases <type-aliases>` that include forward references.
      Enabling postponed evaluation of annotations (:pep:`563`) may remove
      the need for most forward references.

   .. versionchanged:: 3.9
      Added ``include_extras`` parameter as part of :pep:`593`.
      See the documentation on :data:`Annotated` for more information.

   .. versionchanged:: 3.11
      Previously, ``Optional[t]`` was added for function and method annotations
      if a default value equal to ``None`` was set.
      Now the annotation is returned unchanged.

.. function:: get_origin(tp)

   Get the unsubscripted version of a type: for a typing object of the form
   ``X[Y, Z, ...]`` return ``X``.

   If ``X`` is a typing-module alias for a builtin or
   :mod:`collections` class, it will be normalized to the original class.
   If ``X`` is an instance of :class:`ParamSpecArgs` or :class:`ParamSpecKwargs`,
   return the underlying :class:`ParamSpec`.
   Return ``None`` for unsupported objects.

   Examples:

   .. testcode::

      assert get_origin(str) is None
      assert get_origin(Dict[str, int]) is dict
      assert get_origin(Union[int, str]) is Union
      P = ParamSpec('P')
      assert get_origin(P.args) is P
      assert get_origin(P.kwargs) is P

   .. versionadded:: 3.8

.. function:: get_args(tp)

   Get type arguments with all substitutions performed: for a typing object
   of the form ``X[Y, Z, ...]`` return ``(Y, Z, ...)``.

   If ``X`` is a union or :class:`Literal` contained in another
   generic type, the order of ``(Y, Z, ...)`` may be different from the order
   of the original arguments ``[Y, Z, ...]`` due to type caching.
   Return ``()`` for unsupported objects.

   Examples:

   .. testcode::

      assert get_args(int) == ()
      assert get_args(Dict[int, str]) == (int, str)
      assert get_args(Union[int, str]) == (int, str)

   .. versionadded:: 3.8

.. function:: is_typeddict(tp)

   Check if a type is a :class:`TypedDict`.

   For example:

   .. testcode::

      class Film(TypedDict):
          title: str
          year: int

      assert is_typeddict(Film)
      assert not is_typeddict(list | str)

      # TypedDict is a factory for creating typed dicts,
      # not a typed dict itself
      assert not is_typeddict(TypedDict)

   .. versionadded:: 3.10

.. class:: ForwardRef

   Class used for internal typing representation of string forward references.

   For example, ``List["SomeClass"]`` is implicitly transformed into
   ``List[ForwardRef("SomeClass")]``.  ``ForwardRef`` should not be instantiated by
   a user, but may be used by introspection tools.

   .. note::
      :pep:`585` generic types such as ``list["SomeClass"]`` will not be
      implicitly transformed into ``list[ForwardRef("SomeClass")]`` and thus
      will not automatically resolve to ``list[SomeClass]``.

   .. versionadded:: 3.7.4

Constant
--------

.. data:: TYPE_CHECKING

   A special constant that is assumed to be ``True`` by 3rd party static
   type checkers. It is ``False`` at runtime.

   Usage::

      if TYPE_CHECKING:
          import expensive_mod

      def fun(arg: 'expensive_mod.SomeType') -> None:
          local_var: expensive_mod.AnotherType = other_fun()

   The first type annotation must be enclosed in quotes, making it a
   "forward reference", to hide the ``expensive_mod`` reference from the
   interpreter runtime.  Type annotations for local variables are not
   evaluated, so the second annotation does not need to be enclosed in quotes.

   .. note::

      If ``from __future__ import annotations`` is used,
      annotations are not evaluated at function definition time.
      Instead, they are stored as strings in ``__annotations__``.
      This makes it unnecessary to use quotes around the annotation
      (see :pep:`563`).

   .. versionadded:: 3.5.2

.. _generic-concrete-collections:
.. _deprecated-typing-aliases:

Deprecated aliases
------------------

This module defines several deprecated aliases to pre-existing
standard library classes. These were originally included in the typing
module in order to support parameterizing these generic classes using ``[]``.
However, the aliases became redundant in Python 3.9 when the
corresponding pre-existing classes were enhanced to support ``[]`` (see
:pep:`585`).

The redundant types are deprecated as of Python 3.9. However, while the aliases
may be removed at some point, removal of these aliases is not currently
planned. As such, no deprecation warnings are currently issued by the
interpreter for these aliases.

If at some point it is decided to remove these deprecated aliases, a
deprecation warning will be issued by the interpreter for at least two releases
prior to removal. The aliases are guaranteed to remain in the typing module
without deprecation warnings until at least Python 3.14.

Type checkers are encouraged to flag uses of the deprecated types if the
program they are checking targets a minimum Python version of 3.9 or newer.

.. _corresponding-to-built-in-types:

Aliases to built-in types
"""""""""""""""""""""""""

.. class:: Dict(dict, MutableMapping[KT, VT])

   Deprecated alias to :class:`dict`.

   Note that to annotate arguments, it is preferred
   to use an abstract collection type such as :class:`Mapping`
   rather than to use :class:`dict` or :class:`!typing.Dict`.

   This type can be used as follows::

      def count_words(text: str) -> Dict[str, int]:
          ...

   .. deprecated:: 3.9
      :class:`builtins.dict <dict>` now supports subscripting (``[]``).
      See :pep:`585` and :ref:`types-genericalias`.

.. class:: List(list, MutableSequence[T])

   Deprecated alias to :class:`list`.

   Note that to annotate arguments, it is preferred
   to use an abstract collection type such as :class:`Sequence` or
   :class:`Iterable` rather than to use :class:`list` or :class:`!typing.List`.

   This type may be used as follows::

      T = TypeVar('T', int, float)

      def vec2(x: T, y: T) -> List[T]:
          return [x, y]

      def keep_positives(vector: Sequence[T]) -> List[T]:
          return [item for item in vector if item > 0]

   .. deprecated:: 3.9
      :class:`builtins.list <list>` now supports subscripting (``[]``).
      See :pep:`585` and :ref:`types-genericalias`.

.. class:: Set(set, MutableSet[T])

   Deprecated alias to :class:`builtins.set <set>`.

   Note that to annotate arguments, it is preferred
   to use an abstract collection type such as :class:`AbstractSet`
   rather than to use :class:`set` or :class:`!typing.Set`.

   .. deprecated:: 3.9
      :class:`builtins.set <set>` now supports subscripting (``[]``).
      See :pep:`585` and :ref:`types-genericalias`.

.. class:: FrozenSet(frozenset, AbstractSet[T_co])

   Deprecated alias to :class:`builtins.frozenset <frozenset>`.

   .. deprecated:: 3.9
      :class:`builtins.frozenset <frozenset>`
      now supports subscripting (``[]``).
      See :pep:`585` and :ref:`types-genericalias`.

.. data:: Tuple

   Deprecated alias for :class:`tuple`.

   :class:`tuple` and ``Tuple`` are special-cased in the type system; see
   :ref:`annotating-tuples` for more details.

   .. deprecated:: 3.9
      :class:`builtins.tuple <tuple>` now supports subscripting (``[]``).
      See :pep:`585` and :ref:`types-genericalias`.

.. class:: Type(Generic[CT_co])

   Deprecated alias to :class:`type`.

   See :ref:`type-of-class-objects` for details on using :class:`type` or
   ``typing.Type`` in type annotations.

   .. versionadded:: 3.5.2

   .. deprecated:: 3.9
      :class:`builtins.type <type>` now supports subscripting (``[]``).
      See :pep:`585` and :ref:`types-genericalias`.

.. _corresponding-to-types-in-collections:

Aliases to types in :mod:`collections`
""""""""""""""""""""""""""""""""""""""

.. class:: DefaultDict(collections.defaultdict, MutableMapping[KT, VT])

   Deprecated alias to :class:`collections.defaultdict`.

   .. versionadded:: 3.5.2

   .. deprecated:: 3.9
      :class:`collections.defaultdict` now supports subscripting (``[]``).
      See :pep:`585` and :ref:`types-genericalias`.

.. class:: OrderedDict(collections.OrderedDict, MutableMapping[KT, VT])

   Deprecated alias to :class:`collections.OrderedDict`.

   .. versionadded:: 3.7.2

   .. deprecated:: 3.9
      :class:`collections.OrderedDict` now supports subscripting (``[]``).
      See :pep:`585` and :ref:`types-genericalias`.

.. class:: ChainMap(collections.ChainMap, MutableMapping[KT, VT])

   Deprecated alias to :class:`collections.ChainMap`.

   .. versionadded:: 3.6.1

   .. deprecated:: 3.9
      :class:`collections.ChainMap` now supports subscripting (``[]``).
      See :pep:`585` and :ref:`types-genericalias`.

.. class:: Counter(collections.Counter, Dict[T, int])

   Deprecated alias to :class:`collections.Counter`.

   .. versionadded:: 3.6.1

   .. deprecated:: 3.9
      :class:`collections.Counter` now supports subscripting (``[]``).
      See :pep:`585` and :ref:`types-genericalias`.

.. class:: Deque(deque, MutableSequence[T])

   Deprecated alias to :class:`collections.deque`.

   .. versionadded:: 3.6.1

   .. deprecated:: 3.9
      :class:`collections.deque` now supports subscripting (``[]``).
      See :pep:`585` and :ref:`types-genericalias`.

.. _other-concrete-types:

Aliases to other concrete types
"""""""""""""""""""""""""""""""

.. class:: Pattern
           Match

   Deprecated aliases corresponding to the return types from
   :func:`re.compile` and :func:`re.match`.

   These types (and the corresponding functions) are generic over
   :data:`AnyStr`. ``Pattern`` can be specialised as ``Pattern[str]`` or
   ``Pattern[bytes]``; ``Match`` can be specialised as ``Match[str]`` or
   ``Match[bytes]``.

   .. deprecated-removed:: 3.8 3.13
      The ``typing.re`` namespace is deprecated and will be removed.
      These types should be directly imported from ``typing`` instead.

   .. deprecated:: 3.9
      Classes ``Pattern`` and ``Match`` from :mod:`re` now support ``[]``.
      See :pep:`585` and :ref:`types-genericalias`.

.. class:: Text

   Deprecated alias for :class:`str`.

   ``Text`` is provided to supply a forward
   compatible path for Python 2 code: in Python 2, ``Text`` is an alias for
   ``unicode``.

   Use ``Text`` to indicate that a value must contain a unicode string in
   a manner that is compatible with both Python 2 and Python 3::

       def add_unicode_checkmark(text: Text) -> Text:
           return text + u' \u2713'

   .. versionadded:: 3.5.2

   .. deprecated:: 3.11
      Python 2 is no longer supported, and most type checkers also no longer
      support type checking Python 2 code. Removal of the alias is not
      currently planned, but users are encouraged to use
      :class:`str` instead of ``Text``.

.. _abstract-base-classes:
.. _corresponding-to-collections-in-collections-abc:

Aliases to container ABCs in :mod:`collections.abc`
"""""""""""""""""""""""""""""""""""""""""""""""""""

.. class:: AbstractSet(Collection[T_co])

   Deprecated alias to :class:`collections.abc.Set`.

   .. deprecated:: 3.9
      :class:`collections.abc.Set` now supports subscripting (``[]``).
      See :pep:`585` and :ref:`types-genericalias`.

.. class:: ByteString(Sequence[int])

   This type represents the types :class:`bytes`, :class:`bytearray`,
   and :class:`memoryview` of byte sequences.

   .. deprecated-removed:: 3.9 3.14
      Prefer ``typing_extensions.Buffer``, or a union like ``bytes | bytearray | memoryview``.

.. class:: Collection(Sized, Iterable[T_co], Container[T_co])

   Deprecated alias to :class:`collections.abc.Collection`.

   .. versionadded:: 3.6

   .. deprecated:: 3.9
      :class:`collections.abc.Collection` now supports subscripting (``[]``).
      See :pep:`585` and :ref:`types-genericalias`.

.. class:: Container(Generic[T_co])

   Deprecated alias to :class:`collections.abc.Container`.

   .. deprecated:: 3.9
      :class:`collections.abc.Container` now supports subscripting (``[]``).
      See :pep:`585` and :ref:`types-genericalias`.

.. class:: ItemsView(MappingView, AbstractSet[tuple[KT_co, VT_co]])

   Deprecated alias to :class:`collections.abc.ItemsView`.

   .. deprecated:: 3.9
      :class:`collections.abc.ItemsView` now supports subscripting (``[]``).
      See :pep:`585` and :ref:`types-genericalias`.

.. class:: KeysView(MappingView, AbstractSet[KT_co])

   Deprecated alias to :class:`collections.abc.KeysView`.

   .. deprecated:: 3.9
      :class:`collections.abc.KeysView` now supports subscripting (``[]``).
      See :pep:`585` and :ref:`types-genericalias`.

.. class:: Mapping(Collection[KT], Generic[KT, VT_co])

   Deprecated alias to :class:`collections.abc.Mapping`.

   This type can be used as follows::

      def get_position_in_index(word_list: Mapping[str, int], word: str) -> int:
          return word_list[word]

   .. deprecated:: 3.9
      :class:`collections.abc.Mapping` now supports subscripting (``[]``).
      See :pep:`585` and :ref:`types-genericalias`.

.. class:: MappingView(Sized)

   Deprecated alias to :class:`collections.abc.MappingView`.

   .. deprecated:: 3.9
      :class:`collections.abc.MappingView` now supports subscripting (``[]``).
      See :pep:`585` and :ref:`types-genericalias`.

.. class:: MutableMapping(Mapping[KT, VT])

   Deprecated alias to :class:`collections.abc.MutableMapping`.

   .. deprecated:: 3.9
      :class:`collections.abc.MutableMapping`
      now supports subscripting (``[]``).
      See :pep:`585` and :ref:`types-genericalias`.

.. class:: MutableSequence(Sequence[T])

   Deprecated alias to :class:`collections.abc.MutableSequence`.

   .. deprecated:: 3.9
      :class:`collections.abc.MutableSequence`
      now supports subscripting (``[]``).
      See :pep:`585` and :ref:`types-genericalias`.

.. class:: MutableSet(AbstractSet[T])

   Deprecated alias to :class:`collections.abc.MutableSet`.

   .. deprecated:: 3.9
      :class:`collections.abc.MutableSet` now supports subscripting (``[]``).
      See :pep:`585` and :ref:`types-genericalias`.

.. class:: Sequence(Reversible[T_co], Collection[T_co])

   Deprecated alias to :class:`collections.abc.Sequence`.

   .. deprecated:: 3.9
      :class:`collections.abc.Sequence` now supports subscripting (``[]``).
      See :pep:`585` and :ref:`types-genericalias`.

.. class:: ValuesView(MappingView, Collection[_VT_co])

   Deprecated alias to :class:`collections.abc.ValuesView`.

   .. deprecated:: 3.9
      :class:`collections.abc.ValuesView` now supports subscripting (``[]``).
      See :pep:`585` and :ref:`types-genericalias`.

.. _asynchronous-programming:

Aliases to asynchronous ABCs in :mod:`collections.abc`
""""""""""""""""""""""""""""""""""""""""""""""""""""""

.. class:: Coroutine(Awaitable[ReturnType], Generic[YieldType, SendType, ReturnType])

   Deprecated alias to :class:`collections.abc.Coroutine`.

   The variance and order of type variables
   correspond to those of :class:`Generator`, for example::

      from collections.abc import Coroutine
      c: Coroutine[list[str], str, int]  # Some coroutine defined elsewhere
      x = c.send('hi')                   # Inferred type of 'x' is list[str]
      async def bar() -> None:
          y = await c                    # Inferred type of 'y' is int

   .. versionadded:: 3.5.3

   .. deprecated:: 3.9
      :class:`collections.abc.Coroutine` now supports subscripting (``[]``).
      See :pep:`585` and :ref:`types-genericalias`.

.. class:: AsyncGenerator(AsyncIterator[YieldType], Generic[YieldType, SendType])

   Deprecated alias to :class:`collections.abc.AsyncGenerator`.

   An async generator can be annotated by the generic type
   ``AsyncGenerator[YieldType, SendType]``. For example::

      async def echo_round() -> AsyncGenerator[int, float]:
          sent = yield 0
          while sent >= 0.0:
              rounded = await round(sent)
              sent = yield rounded

   Unlike normal generators, async generators cannot return a value, so there
   is no ``ReturnType`` type parameter. As with :class:`Generator`, the
   ``SendType`` behaves contravariantly.

   If your generator will only yield values, set the ``SendType`` to
   ``None``::

      async def infinite_stream(start: int) -> AsyncGenerator[int, None]:
          while True:
              yield start
              start = await increment(start)

   Alternatively, annotate your generator as having a return type of
   either ``AsyncIterable[YieldType]`` or ``AsyncIterator[YieldType]``::

      async def infinite_stream(start: int) -> AsyncIterator[int]:
          while True:
              yield start
              start = await increment(start)

   .. versionadded:: 3.6.1

   .. deprecated:: 3.9
      :class:`collections.abc.AsyncGenerator`
      now supports subscripting (``[]``).
      See :pep:`585` and :ref:`types-genericalias`.

.. class:: AsyncIterable(Generic[T_co])

   Deprecated alias to :class:`collections.abc.AsyncIterable`.

   .. versionadded:: 3.5.2

   .. deprecated:: 3.9
      :class:`collections.abc.AsyncIterable` now supports subscripting (``[]``).
      See :pep:`585` and :ref:`types-genericalias`.

.. class:: AsyncIterator(AsyncIterable[T_co])

   Deprecated alias to :class:`collections.abc.AsyncIterator`.

   .. versionadded:: 3.5.2

   .. deprecated:: 3.9
      :class:`collections.abc.AsyncIterator` now supports subscripting (``[]``).
      See :pep:`585` and :ref:`types-genericalias`.

.. class:: Awaitable(Generic[T_co])

   Deprecated alias to :class:`collections.abc.Awaitable`.

   .. versionadded:: 3.5.2

   .. deprecated:: 3.9
      :class:`collections.abc.Awaitable` now supports subscripting (``[]``).
      See :pep:`585` and :ref:`types-genericalias`.

.. _corresponding-to-other-types-in-collections-abc:

Aliases to other ABCs in :mod:`collections.abc`
"""""""""""""""""""""""""""""""""""""""""""""""

.. class:: Iterable(Generic[T_co])

   Deprecated alias to :class:`collections.abc.Iterable`.

   .. deprecated:: 3.9
      :class:`collections.abc.Iterable` now supports subscripting (``[]``).
      See :pep:`585` and :ref:`types-genericalias`.

.. class:: Iterator(Iterable[T_co])

   Deprecated alias to :class:`collections.abc.Iterator`.

   .. deprecated:: 3.9
      :class:`collections.abc.Iterator` now supports subscripting (``[]``).
      See :pep:`585` and :ref:`types-genericalias`.

.. data:: Callable

   Deprecated alias to :class:`collections.abc.Callable`.

   See :ref:`annotating-callables` for details on how to use
   :class:`collections.abc.Callable` and ``typing.Callable`` in type annotations.

   .. deprecated:: 3.9
      :class:`collections.abc.Callable` now supports subscripting (``[]``).
      See :pep:`585` and :ref:`types-genericalias`.

   .. versionchanged:: 3.10
      ``Callable`` now supports :class:`ParamSpec` and :data:`Concatenate`.
      See :pep:`612` for more details.

.. class:: Generator(Iterator[YieldType], Generic[YieldType, SendType, ReturnType])

   Deprecated alias to :class:`collections.abc.Generator`.

   A generator can be annotated by the generic type
   ``Generator[YieldType, SendType, ReturnType]``. For example::

      def echo_round() -> Generator[int, float, str]:
          sent = yield 0
          while sent >= 0:
              sent = yield round(sent)
          return 'Done'

   Note that unlike many other generics in the typing module, the ``SendType``
   of :class:`Generator` behaves contravariantly, not covariantly or
   invariantly.

   If your generator will only yield values, set the ``SendType`` and
   ``ReturnType`` to ``None``::

      def infinite_stream(start: int) -> Generator[int, None, None]:
          while True:
              yield start
              start += 1

   Alternatively, annotate your generator as having a return type of
   either ``Iterable[YieldType]`` or ``Iterator[YieldType]``::

      def infinite_stream(start: int) -> Iterator[int]:
          while True:
              yield start
              start += 1

   .. deprecated:: 3.9
      :class:`collections.abc.Generator` now supports subscripting (``[]``).
      See :pep:`585` and :ref:`types-genericalias`.

.. class:: Hashable

   Alias to :class:`collections.abc.Hashable`.

.. class:: Reversible(Iterable[T_co])

   Deprecated alias to :class:`collections.abc.Reversible`.

   .. deprecated:: 3.9
      :class:`collections.abc.Reversible` now supports subscripting (``[]``).
      See :pep:`585` and :ref:`types-genericalias`.

.. class:: Sized

   Alias to :class:`collections.abc.Sized`.

.. _context-manager-types:

Aliases to :mod:`contextlib` ABCs
"""""""""""""""""""""""""""""""""

.. class:: ContextManager(Generic[T_co])

   Deprecated alias to :class:`contextlib.AbstractContextManager`.

   .. versionadded:: 3.5.4

   .. deprecated:: 3.9
      :class:`contextlib.AbstractContextManager`
      now supports subscripting (``[]``).
      See :pep:`585` and :ref:`types-genericalias`.

.. class:: AsyncContextManager(Generic[T_co])

   Deprecated alias to :class:`contextlib.AbstractAsyncContextManager`.

   .. versionadded:: 3.6.2

   .. deprecated:: 3.9
      :class:`contextlib.AbstractAsyncContextManager`
      now supports subscripting (``[]``).
      See :pep:`585` and :ref:`types-genericalias`.

Deprecation Timeline of Major Features
======================================

Certain features in ``typing`` are deprecated and may be removed in a future
version of Python. The following table summarizes major deprecations for your
convenience. This is subject to change, and not all deprecations are listed.

.. list-table::
   :header-rows: 1

   * - Feature
     - Deprecated in
     - Projected removal
     - PEP/issue
   * - ``typing.io`` and ``typing.re`` submodules
     - 3.8
     - 3.13
     - :issue:`38291`
   * - ``typing`` versions of standard collections
     - 3.9
     - Undecided (see :ref:`deprecated-typing-aliases` for more information)
     - :pep:`585`
   * - :class:`typing.ByteString`
     - 3.9
     - 3.14
     - :gh:`91896`
   * - :data:`typing.Text`
     - 3.11
     - Undecided
     - :gh:`92332`
