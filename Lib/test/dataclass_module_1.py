#from __future__ import annotations

# dataclass_module_1.py and dataclass_module_1_str.py are identical
# except only the latter uses string annotations.

import typing

class C:
    x: typing.ClassVar[int] = 20
