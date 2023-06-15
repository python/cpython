============
Typing HOWTO
============

.. _typing-features-guide:

.. currentmodule:: typing

This page contains in-depth guides and tutorials for specific features in the
:mod:`typing` module.

.. note::

   This page is a work in progress.

.. seealso::

   Module :mod:`typing`
      API reference for the typing module.

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


Typing basics
=============

The function below takes and returns a string and is annotated as follows::

   def greeting(name: str) -> str:
       return 'Hello ' + name

In the function ``greeting``, the argument ``name`` is expected to be of type
:class:`str` and the return type :class:`str`. Subtypes are accepted as
arguments.

.. note::

   The Python runtime does not enforce function and variable type annotations.
   They can be used by third party tools such as type checkers, IDEs, linters,
   etc.

.. _type-aliases:

Type aliases
============

A type alias is defined using the :keyword:`type` statement, which creates
an instance of :class:`TypeAliasType`. In this example,
``Vector`` and ``list[float]`` will be treated equivalently by static type
checkers::

   type Vector = list[float]

   def scale(scalar: float, vector: Vector) -> Vector:
       return [scalar * num for num in vector]

   # passes type checking; a list of floats qualifies as a Vector.
   new_vector = scale(2.0, [1.0, -4.2, 5.4])

Type aliases are useful for simplifying complex type signatures. For example::

   from collections.abc import Sequence

   type ConnectionOptions = dict[str, str]
   type Address = tuple[str, int]
   type Server = tuple[Address, ConnectionOptions]

   def broadcast_message(message: str, servers: Sequence[Server]) -> None:
       ...

   # The static type checker will treat the previous type signature as
   # being exactly equivalent to this one.
   def broadcast_message(
           message: str,
           servers: Sequence[tuple[tuple[str, int], dict[str, str]]]) -> None:
       ...

The :keyword:`type` statement is new in Python 3.12. For backwards
compatibility, type aliases can also be created through simple assignment::

   Vector = list[float]

Or marked with :data:`TypeAlias` to make it explicit that this is a type alias,
not a normal variable assignment::

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
   one another. Doing ``type Alias = Original`` will make the static type checker
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


Callable
========

Frameworks expecting callback functions of specific signatures might be
type hinted using ``Callable[[Arg1Type, Arg2Type], ReturnType]``.

For example:

.. testcode::

   from collections.abc import Callable

   def feeder(get_next_item: Callable[[], str]) -> None:
       ...  # Body

   def async_query(on_success: Callable[[int], None],
                   on_error: Callable[[int, Exception], None]) -> None:
       ...  # Body

   async def on_update(value: str) -> None:
       ...  # Body

   callback: Callable[[str], Awaitable[None]] = on_update

It is possible to declare the return type of a callable without specifying
the call signature by substituting a literal ellipsis
for the list of arguments in the type hint: ``Callable[..., ReturnType]``.

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

Generic functions and classes can be parameterized by using
:ref:`type parameter syntax <type-params>`::

   from collections.abc import Sequence

   def first[T](l: Sequence[T]) -> T:  # Function is generic over the TypeVar "T"
       return l[0]

Or by using the :class:`TypeVar` factory directly::

   from collections.abc import Sequence
   from typing import TypeVar

   U = TypeVar('U')                  # Declare type variable "U"

   def second(l: Sequence[U]) -> U:  # Function is generic over the TypeVar "U"
       return l[1]

.. versionchanged:: 3.12
   Syntactic support for generics is new in Python 3.12.

.. _user-defined-generics:

User-defined generic types
==========================

A user-defined class can be defined as a generic class.

::

   from logging import Logger

   class LoggedVar[T]:
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

This syntax indicates that the class ``LoggedVar`` is parameterised around a
single :class:`type variable <TypeVar>` ``T`` . This also makes ``T`` valid as
a type within the class body.

Generic classes implicitly inherit from :class:`Generic`. For compatibility
with Python 3.11 and lower, it is also possible to inherit explicitly from
:class:`Generic` to indicate a generic class::

   from typing import TypeVar, Generic

   T = TypeVar('T')

   class LoggedVar(Generic[T]):
       ...

Generic classes have :meth:`~object.__class_getitem__` methods, meaning they
can be parameterised at runtime (e.g. ``LoggedVar[int]`` below)::

   from collections.abc import Iterable

   def zero_all_vars(vars: Iterable[LoggedVar[int]]) -> None:
       for var in vars:
           var.set(0)

A generic type can have any number of type variables. All varieties of
:class:`TypeVar` are permissible as parameters for a generic type::

   from typing import TypeVar, Generic, Sequence

   class WeirdTrio[T, B: Sequence[bytes], S: (int, str)]:
       ...

   OldT = TypeVar('OldT', contravariant=True)
   OldB = TypeVar('OldB', bound=Sequence[bytes], covariant=True)
   OldS = TypeVar('OldS', int, str)

   class OldWeirdTrio(Generic[OldT, OldB, OldS]):
       ...

Each type variable argument to :class:`Generic` must be distinct.
This is thus invalid::

   from typing import TypeVar, Generic
   ...

   class Pair[M, M]:  # SyntaxError
       ...

   T = TypeVar('T')

   class Pair(Generic[T, T]):   # INVALID
       ...

Generic classes can also inherit from other classes::

   from collections.abc import Sized

   class LinkedList[T](Sized):
       ...

When inheriting from generic classes, some type parameters could be fixed::

    from collections.abc import Mapping

    class MyDict[T](Mapping[str, T]):
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

   type Response[S] = Iterable[S] | int

   # Return type here is same as Iterable[str] | int
   def response(query: str) -> Response[str]:
       ...

   type Vec[T] = Iterable[tuple[T, T]]

   def inproduct[T: (int, float, complex)](v: Vec[T]) -> T: # Same as Iterable[tuple[T, T]]
       return sum(x*y for x, y in v)

For backward compatibility, generic type aliases can also be created
through a simple assignment::

   from collections.abc import Iterable
   from typing import TypeVar

   S = TypeVar("S")
   Response = Iterable[S] | int

.. versionchanged:: 3.7
    :class:`Generic` no longer has a custom metaclass.

.. versionchanged:: 3.12
   Syntactic support for generics and type aliases is new in version 3.12.
   Previously, generic classes had to explicitly inherit from :class:`Generic`
   or contain a type variable in one of their bases.

User-defined generics for parameter expressions are also supported via parameter
specification variables in the form ``[**P]``.  The behavior is consistent
with type variables' described above as parameter specification variables are
treated by the typing module as a specialized type variable.  The one exception
to this is that a list of types can be used to substitute a :class:`ParamSpec`::

   >>> class Z[T, **P]: ...  # T is a TypeVar; P is a ParamSpec
   ...
   >>> Z[int, [dict, float]]
   __main__.Z[int, [dict, float]]

Classes generic over a :class:`ParamSpec` can also be created using explicit
inheritance from :class:`Generic`. In this case, ``**`` is not used::

   from typing import ParamSpec, Generic

   P = ParamSpec('P')

   class Z(Generic[P]):
       ...

Another difference between :class:`TypeVar` and :class:`ParamSpec` is that a
generic with only one parameter specification variable will accept
parameter lists in the forms ``X[[Type1, Type2, ...]]`` and also
``X[Type1, Type2, ...]`` for aesthetic reasons.  Internally, the latter is converted
to the former, so the following are equivalent::

   >>> class X[**P]: ...
   ...
   >>> X[int, str]
   __main__.X[[int, str]]
   >>> X[[int, str]]
   __main__.X[[int, str]]

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
can define new custom protocols to fully enjoy structural subtyping.
