from __future__ import annotations
from dataclasses import dataclass
from typing import List
from typing import ClassVar as CV

@dataclass
class A:
    a: List[str]

@dataclass
class B(A):
    b: CV[int]
