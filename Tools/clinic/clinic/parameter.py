from __future__ import annotations

import copy
import dataclasses as dc
import inspect
from typing import Any, TYPE_CHECKING

from .utils import text_accumulator, unspecified, VersionTuple
if TYPE_CHECKING:
    from .function import Function
    from .converter import CConverter


@dc.dataclass(repr=False, slots=True)
class Parameter:
    """
    Mutable duck type of inspect.Parameter.
    """
    name: str
    kind: inspect._ParameterKind
    _: dc.KW_ONLY
    default: object = inspect.Parameter.empty
    function: Function
    converter: CConverter
    annotation: object = inspect.Parameter.empty
    docstring: str = ''
    group: int = 0
    # (`None` signifies that there is no deprecation)
    deprecated_positional: VersionTuple | None = None
    deprecated_keyword: VersionTuple | None = None
    right_bracket_count: int = dc.field(init=False, default=0)

    def __repr__(self) -> str:
        return f'<clinic.Parameter {self.name!r}>'

    def is_keyword_only(self) -> bool:
        return self.kind == inspect.Parameter.KEYWORD_ONLY

    def is_positional_only(self) -> bool:
        return self.kind == inspect.Parameter.POSITIONAL_ONLY

    def is_vararg(self) -> bool:
        return self.kind == inspect.Parameter.VAR_POSITIONAL

    def is_optional(self) -> bool:
        return not self.is_vararg() and (self.default is not unspecified)

    def copy(
        self,
        /,
        *,
        converter: CConverter | None = None,
        function: Function | None = None,
        **overrides: Any
    ) -> Parameter:
        function = function or self.function
        if not converter:
            converter = copy.copy(self.converter)
            converter.function = function
        return dc.replace(self, **overrides, function=function, converter=converter)

    def get_displayname(self, i: int) -> str:
        if i == 0:
            return 'argument'
        if not self.is_positional_only():
            return f'argument {self.name!r}'
        else:
            return f'argument {i}'

    def render_docstring(self) -> str:
        add, out = text_accumulator()
        add(f"  {self.name}\n")
        for line in self.docstring.split("\n"):
            add(f"    {line}\n")
        return out().rstrip()


ParamTuple = tuple[Parameter, ...]
ParamDict = dict[str, Parameter]
