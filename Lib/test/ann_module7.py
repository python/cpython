# Ensures that top-level ``ClassVar`` is not allowed.
# We test this explicitly without ``from __future__ import annotations``

from typing import ClassVar, Final

wrong: ClassVar[int] = 1
def final_var_arg(arg: Final): pass
def final_var_return_type() -> Final: pass
