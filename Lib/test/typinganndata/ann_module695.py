from __future__ import annotations
from typing import Callable, TypedDict


class A[T, *Ts, **P]:
    x: T
    y: tuple[*Ts]
    z: Callable[P, str]


class B[T, *Ts, **P]:
    T = int
    Ts = str
    P = bytes
    x: T
    y: Ts
    z: P


Eggs = int
Spam = str


class C[Eggs, **Spam]:
    x: Eggs
    y: Spam


def generic_function[T, *Ts, **P](
    x: T, *y: *Ts, z: P.args, zz: P.kwargs
) -> None: ...


def generic_function_2[Eggs, **Spam](x: Eggs, y: Spam): pass


class D:
    Foo = int
    Bar = str

    def generic_method[Foo, **Bar](
        self, x: Foo, y: Bar
    ) -> None: ...

    def generic_method_2[Eggs, **Spam](self, x: Eggs, y: Spam): pass


def nested():
    from types import SimpleNamespace
    from typing import get_type_hints

    Eggs = bytes
    Spam = memoryview


    class E[Eggs, **Spam]:
        x: Eggs
        y: Spam

        def generic_method[Eggs, **Spam](self, x: Eggs, y: Spam): pass


    def generic_function[Eggs, **Spam](x: Eggs, y: Spam): pass


    return SimpleNamespace(
        E=E,
        hints_for_E=get_type_hints(E),
        hints_for_E_meth=get_type_hints(E.generic_method),
        generic_func=generic_function,
        hints_for_generic_func=get_type_hints(generic_function)
    )

# gh-138949
class TD1[T](TypedDict):
    a: T

class TD2(TD1):
    b: int

class TD3[CT](TD2):
    c: CT

class TD4[T, E](TD3):
    d: T
    e: E

class TD1_2(TD1):
    # This must raise a `NameError`, because `T` is only defined for a parent
    # keys scope.
    b: T
