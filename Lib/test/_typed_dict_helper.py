"""Used to test `get_type_hints()` on a cross-module inherited `TypedDict` class

This script uses future annotations to postpone a type that won't be available
on the module inheriting from to `Foo`. The subclass in the other module should
look something like this:

    class Bar(_typed_dict_helper.Foo, total=False):
        b: int
"""

from __future__ import annotations

from typing import Optional, TypedDict

OptionalIntType = Optional[int]

class Foo(TypedDict):
    a: OptionalIntType
