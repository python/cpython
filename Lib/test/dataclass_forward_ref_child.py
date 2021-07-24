from dataclasses import dataclass
from typing import Optional

from test.dataclass_forward_ref import A


@dataclass
class B(A):
    b: Optional['B']
