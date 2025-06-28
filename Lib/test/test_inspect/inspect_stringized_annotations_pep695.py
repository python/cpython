from __future__ import annotations
from typing import Callable, Unpack


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
    x: T, *y: Unpack[Ts], z: P.args, zz: P.kwargs
) -> None: ...


def generic_function_2[Eggs, **Spam](x: Eggs, y: Spam): pass


class D:
    Foo = int
    Bar = str

    def generic_method[Foo, **Bar](
        self, x: Foo, y: Bar
    ) -> None: ...

    def generic_method_2[Eggs, **Spam](self, x: Eggs, y: Spam): pass


# Eggs is `int` in globals, a TypeVar in type_params, and `str` in locals:
class E[Eggs]:
    Eggs = str
    x: Eggs



def nested():
    from types import SimpleNamespace
    from inspect import get_annotations

    Eggs = bytes
    Spam = memoryview


    class F[Eggs, **Spam]:
        x: Eggs
        y: Spam

        def generic_method[Eggs, **Spam](self, x: Eggs, y: Spam): pass


    def generic_function[Eggs, **Spam](x: Eggs, y: Spam): pass


    # Eggs is `int` in globals, `bytes` in the function scope,
    # a TypeVar in the type_params, and `str` in locals:
    class G[Eggs]:
        Eggs = str
        x: Eggs


    return SimpleNamespace(
        F=F,
        F_annotations=get_annotations(F, eval_str=True),
        F_meth_annotations=get_annotations(F.generic_method, eval_str=True),
        G_annotations=get_annotations(G, eval_str=True),
        generic_func=generic_function,
        generic_func_annotations=get_annotations(generic_function, eval_str=True)
    )
