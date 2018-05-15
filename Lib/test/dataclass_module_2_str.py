from __future__ import annotations

# dataclass_module_2.py and dataclass_module_2_str.py are identical
# except only the latter uses string annotations.

from typing import ClassVar

class C:
    x: ClassVar[int] = 20
