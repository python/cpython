from dataclasses import *

class D:
    """A descriptor class that knows its name."""
    def __set_name__(self, owner, name):
        self.name = name
    def __get__(self, instance, owner):
        if instance is not None:
            return 1
        return self

from dataclasses import *

@dataclass
class C:
    d: int = field(default=D(), init=False)

@dataclass
class E(C):
    e: int = field(default=D(), init=False)
