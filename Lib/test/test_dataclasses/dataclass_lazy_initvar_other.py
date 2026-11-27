from dataclasses import dataclass, InitVar
from dataclasses import InitVar as IV
from typing import ClassVar
import dataclasses

lazy from test.test_dataclasses.dataclass_lazy_initvar import A


@dataclass
class DirectInitVar:
    value: InitVar[A]

    def __post_init__(self, value):
        self.received = value


@dataclass
class QualifiedInitVar:
    value: dataclasses.InitVar[A]

    def __post_init__(self, value):
        self.received = value


@dataclass
class AliasedInitVar:
    value: IV[A]

    def __post_init__(self, value):
        self.received = value


@dataclass
class DeferredClassVar:
    value: ClassVar[A] = None


@dataclass
class DeferredField:
    value: A
