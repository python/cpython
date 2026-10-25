from dataclasses import dataclass

from test.test_dataclasses.dataclass_lazy_initvar_other import (
    AliasedInitVar, DeferredClassVar, DeferredField, DirectInitVar,
    QualifiedInitVar,
)

__all__ = [
    'A', 'AliasedInitVar', 'DeferredClassVar', 'DeferredField',
    'DirectInitVar', 'QualifiedInitVar',
]


@dataclass
class A:
    pass
