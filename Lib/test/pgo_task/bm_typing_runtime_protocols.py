"""
Test the performance of isinstance() checks against runtime-checkable protocols.

For programmes that make extensive use of this feature,
these calls can easily become a bottleneck.
See https://github.com/python/cpython/issues/74690

The following situations all exercise different code paths
in typing._ProtocolMeta.__instancecheck__,
so each is tested in this benchmark:

  (1) Comparing an instance of a class that directly inherits
      from a protocol to that protocol.
  (2) Comparing an instance of a class that fulfils the interface
      of a protocol using instance attributes.
  (3) Comparing an instance of a class that fulfils the interface
      of a protocol using class attributes.
  (4) Comparing an instance of a class that fulfils the interface
      of a protocol using properties.

Protocols with callable and non-callable members also
exercise different code paths in _ProtocolMeta.__instancecheck__,
so are also tested separately.
"""

from typing import Protocol, runtime_checkable


##################################################
# Protocols to call isinstance() against
##################################################


@runtime_checkable
class HasX(Protocol):
    """A runtime-checkable protocol with a single non-callable member"""
    x: int

@runtime_checkable
class HasManyAttributes(Protocol):
    """A runtime-checkable protocol with many non-callable members"""
    a: int
    b: int
    c: int
    d: int
    e: int

@runtime_checkable
class SupportsInt(Protocol):
    """A runtime-checkable protocol with a single callable member"""
    def __int__(self) -> int: ...

@runtime_checkable
class SupportsManyMethods(Protocol):
    """A runtime-checkable protocol with many callable members"""
    def one(self) -> int: ...
    def two(self) -> str: ...
    def three(self) -> bytes: ...
    def four(self) -> memoryview: ...
    def five(self) -> bytearray: ...

@runtime_checkable
class SupportsIntAndX(Protocol):
    """A runtime-checkable protocol with a mix of callable and non-callable members"""
    x: int
    def __int__(self) -> int: ...


##################################################
# Classes for comparing against the protocols
##################################################


class Empty:
    """Empty class with no attributes"""

class PropertyX:
    """Class with a property x"""
    @property
    def x(self) -> int: return 42

class HasIntMethod:
    """Class with an __int__ method"""
    def __int__(self): return 42

class HasManyMethods:
    """Class with many methods"""
    def one(self): return 42
    def two(self): return "42"
    def three(self): return b"42"
    def four(self): return memoryview(b"42")
    def five(self): return bytearray(b"42")

class PropertyXWithInt:
    """Class with a property x and an __int__ method"""
    @property
    def x(self) -> int: return 42
    def __int__(self): return 42

class ClassVarX:
    """Class with a ClassVar x"""
    x = 42

class ClassVarXWithInt:
    """Class with a ClassVar x and an __int__ method"""
    x = 42
    def __int__(self): return 42

class InstanceVarX:
    """Class with an instance var x"""
    def __init__(self):
        self.x = 42

class ManyInstanceVars:
    """Class with many instance vars"""
    def __init__(self):
        for attr in 'abcde':
            setattr(self, attr, 42)

class InstanceVarXWithInt:
    """Class with an instance var x and an __int__ method"""
    def __init__(self):
        self.x = 42
    def __int__(self):
        return 42

class NominalX(HasX):
    """Class that explicitly subclasses HasX"""
    def __init__(self):
        self.x = 42

class NominalSupportsInt(SupportsInt):
    """Class that explicitly subclasses SupportsInt"""
    def __int__(self):
        return 42

class NominalXWithInt(SupportsIntAndX):
    """Class that explicitly subclasses NominalXWithInt"""
    def __init__(self):
        self.x = 42


##################################################
# Time to test the performance of isinstance()!
##################################################


def bench_protocols(loops: int) -> float:
    protocols = [
        HasX, HasManyAttributes, SupportsInt, SupportsManyMethods, SupportsIntAndX
    ]
    instances = [
        cls()
        for cls in (
            Empty, PropertyX, HasIntMethod, HasManyMethods, PropertyXWithInt,
            ClassVarX, ClassVarXWithInt, InstanceVarX, ManyInstanceVars,
            InstanceVarXWithInt, NominalX, NominalSupportsInt, NominalXWithInt
        )
    ]

    for _ in range(loops):
        for protocol in protocols:
            for instance in instances:
                isinstance(instance, protocol)


def run_pgo():
    for _ in range(10):
        bench_protocols(100)
