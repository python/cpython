from __future__ import annotations
from typing import Callable


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


class D:
    Foo = int
    Bar = str

    def generic_method[Foo, **Bar](
        self, x: Foo, y: Bar
    ) -> None: ...
