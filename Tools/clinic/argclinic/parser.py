from __future__ import annotations
import contextlib
import functools
import io
from types import NoneType
from typing import Any, Protocol, TYPE_CHECKING

from . import unspecified
from .block_parser import Block
from .converter import CConverter, converters
from .converters import buffer, robuffer, rwbuffer
from .return_converters import CReturnConverter, return_converters
if TYPE_CHECKING:
    from .app import Clinic


class Parser(Protocol):
    def __init__(self, clinic: Clinic) -> None: ...
    def parse(self, block: Block) -> None: ...


@functools.cache
def _create_parser_base_namespace() -> dict[str, Any]:
    ns = dict(
        CConverter=CConverter,
        CReturnConverter=CReturnConverter,
        buffer=buffer,
        robuffer=robuffer,
        rwbuffer=rwbuffer,
        unspecified=unspecified,
        NoneType=NoneType,
    )
    for name, converter in converters.items():
        ns[f'{name}_converter'] = converter
    for name, return_converter in return_converters.items():
        ns[f'{name}_return_converter'] = return_converter
    return ns


def create_parser_namespace() -> dict[str, Any]:
    base_namespace = _create_parser_base_namespace()
    return base_namespace.copy()


class PythonParser:
    def __init__(self, clinic: Clinic) -> None:
        pass

    def parse(self, block: Block) -> None:
        namespace = create_parser_namespace()
        with contextlib.redirect_stdout(io.StringIO()) as s:
            exec(block.input, namespace)
            block.output = s.getvalue()
