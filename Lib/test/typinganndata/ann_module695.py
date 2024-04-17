from __future__ import annotations


class A[T, *Ts, **P]:
    x: T
    y: Ts
    z: P


class B[T, *Ts, **P]:
    T = int
    Ts = str
    P = bytes
    x: T
    y: Ts
    z: P


def generic_function[T, *Ts, **P](
    x: T, *y: *Ts, z: P.args, zz: P.kwargs
) -> None: ...
