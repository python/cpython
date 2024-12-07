"""
Protocols for supporting classes in pathlib.
"""
from typing import Protocol, runtime_checkable


@runtime_checkable
class Parser(Protocol):
    """Protocol for path parsers, which do low-level path manipulation.

    Path parsers provide a subset of the os.path API, specifically those
    functions needed to provide PurePathBase functionality. Each PurePathBase
    subclass references its path parser via a 'parser' class attribute.
    """

    sep: str
    def join(self, path: str, *paths: str) -> str: ...
    def split(self, path: str) -> tuple[str, str]: ...
    def splitdrive(self, path: str) -> tuple[str, str]: ...
    def splitext(self, path: str) -> tuple[str, str]: ...
    def normcase(self, path: str) -> str: ...
    def isabs(self, path: str) -> bool: ...
