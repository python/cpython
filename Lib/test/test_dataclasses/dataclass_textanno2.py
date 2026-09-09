from __future__ import annotations

import dataclasses

# We need to be sure that `Foo` is not in scope
from test.test_dataclasses import dataclass_textanno


class Custom:
    pass


@dataclasses.dataclass
class Child(dataclass_textanno.Bar):
    custom: Custom


class Foo:  # matching name with `dataclass_testanno.Foo`
    pass


@dataclasses.dataclass
class WithMatchingNameOverride(dataclass_textanno.Bar):
    foo: Foo  # Existing `foo` annotation should be overridden


@dataclasses.dataclass(init=False)
class WithFutureInit(Child):
    def __init__(self, foo: dataclass_textanno.Foo, custom: Custom) -> None:
        pass
