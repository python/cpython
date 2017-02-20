"""Module for testing the behavior of generics across different modules."""

from typing import TypeVar, Generic

T = TypeVar('T')


class A(Generic[T]):
    pass


class B(Generic[T]):
    class A(Generic[T]):
        pass
