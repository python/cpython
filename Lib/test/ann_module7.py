# Ensures that top-level ``ClassVar`` is not allowed.
# We test this explicitly without ``from __future__ import annotations``

from typing import ClassVar, Final

wrong: ClassVar[int] = 1
