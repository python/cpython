"""
Protocols for supporting classes in pathlib.
"""
from typing import Protocol, runtime_checkable


@runtime_checkable
class _PathParser(Protocol):
    """Protocol for path parsers, which do low-level path manipulation.

    Path parsers provide a subset of the os.path API, specifically those
    functions needed to provide JoinablePath functionality. Each JoinablePath
    subclass references its path parser via a 'parser' class attribute.
    """

    sep: str
    def split(self, path: str) -> tuple[str, str]: ...
    def splitext(self, path: str) -> tuple[str, str]: ...
    def normcase(self, path: str) -> str: ...


@runtime_checkable
class PathInfo(Protocol):
    """Protocol for path info objects, which support querying the file type.
    Methods may return cached results.
    """
    def exists(self, *, follow_symlinks: bool = True) -> bool: ...
    def is_dir(self, *, follow_symlinks: bool = True) -> bool: ...
    def is_file(self, *, follow_symlinks: bool = True) -> bool: ...
    def is_symlink(self) -> bool: ...
