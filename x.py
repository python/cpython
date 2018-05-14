#from __future__ import annotations
from typing import ClassVar, Dict, get_type_hints
from dataclasses import *

class Starship:
    stats: ClassVar[Dict[str, int]] = {}

#print(get_type_hints(Starship))

#class A:
#    a: Dict[int, C]

#print(get_type_hints(A))

cv = [ClassVar[int]]

@dataclass
class C:
    CVS = [ClassVar[str]]
    a: cv[0]
    b: 'C'
    c: 'CVS[0]'
    x: 'ClassVar["int"]'
    y: 'ClassVar[C]'

print()
print(C.__annotations__)
print(C.__dataclass_fields__)


#print(get_type_hints(C))
