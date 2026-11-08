from __future__ import annotations

import dataclasses
lazy import typing as t
lazy from _dataclass_test_missing_module import Missing
lazy from test.test_dataclasses.dataclass_lazy_broken import Broken

@dataclasses.dataclass
class C:
    x: Missing | None = None
    y: Broken | None = None
    cv: t.ClassVar[int] = 0
