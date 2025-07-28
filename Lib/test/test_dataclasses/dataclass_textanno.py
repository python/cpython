from __future__ import annotations

import dataclasses


class Foo:
    pass


@dataclasses.dataclass
class Bar:
    foo: Foo


@dataclasses.dataclass(init=False)
class WithFutureInit(Bar):
    def __init__(self, foo: Foo) -> None:
        pass
