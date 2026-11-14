from __future__ import annotations

import os
from collections.abc import Iterable, Iterator
from typing import (
    Any,
    Protocol,
    TypeVar,
    overload,
    runtime_checkable,
)

_T = TypeVar("_T")


@runtime_checkable
class PackageMetadata(Protocol):
    def __len__(self) -> int: ...  # pragma: no cover

    def __contains__(self, item: str) -> bool: ...  # pragma: no cover

    def __getitem__(self, key: str) -> str: ...  # pragma: no cover

    def __iter__(self) -> Iterator[str]: ...  # pragma: no cover

    @overload
    def get(
        self, name: str, failobj: None = None
    ) -> str | None: ...  # pragma: no cover

    @overload
    def get(self, name: str, failobj: _T) -> str | _T: ...  # pragma: no cover

    # overload per python/importlib_metadata#435
    @overload
    def get_all(
        self, name: str, failobj: None = None
    ) -> list[Any] | None: ...  # pragma: no cover

    @overload
    def get_all(self, name: str, failobj: _T) -> list[Any] | _T:
        """
        Return all values associated with a possibly multi-valued key.
        """

    @property
    def json(self) -> dict[str, str | list[str]]:
        """
        A JSON-compatible form of the metadata.
        """


class SimplePath(Protocol):
    """
    A minimal subset of pathlib.Path required by Distribution.
    """

    def joinpath(
        self, other: str | os.PathLike[str], /
    ) -> SimplePath: ...  # pragma: no cover

    def __truediv__(
        self, other: str | os.PathLike[str], /
    ) -> SimplePath: ...  # pragma: no cover

    @property
    def parent(self) -> SimplePath: ...  # pragma: no cover

    def read_text(self, encoding=None) -> str: ...  # pragma: no cover

    def read_bytes(self) -> bytes: ...  # pragma: no cover

    def exists(self) -> bool: ...  # pragma: no cover


@runtime_checkable
class IPackagePath(Protocol):
    """
    The interface of the paths yielded by ``IDistribution.files``.

    Consumers should prefer this protocol over ``PackagePath``,
    as it is also satisfied by the objects that ``importlib.metadata``
    supplies.
    """

    hash: Any | None
    size: int | None
    dist: IDistribution

    def read_text(self, encoding: str = 'utf-8') -> str: ...  # pragma: no cover

    def read_binary(self) -> bytes: ...  # pragma: no cover

    def locate(self) -> SimplePath: ...  # pragma: no cover

    @property
    def parts(self) -> tuple[str, ...]: ...  # pragma: no cover

    def __fspath__(self) -> str: ...  # pragma: no cover


@runtime_checkable
class IDistribution(Protocol):
    """
    The interface of the distributions yielded by ``distributions()``
    and friends.

    Consumers should prefer this protocol over ``Distribution``,
    as it is also satisfied by the objects that ``importlib.metadata``
    supplies.
    """

    def read_text(
        self, filename: str | os.PathLike[str]
    ) -> str | None: ...  # pragma: no cover

    def locate_file(
        self, path: str | os.PathLike[str]
    ) -> SimplePath: ...  # pragma: no cover

    @property
    def metadata(self) -> PackageMetadata: ...  # pragma: no cover

    @property
    def name(self) -> str: ...  # pragma: no cover

    @property
    def version(self) -> str: ...  # pragma: no cover

    @property
    def entry_points(self) -> Any: ...  # pragma: no cover

    @property
    def files(self) -> list[IPackagePath] | None: ...  # pragma: no cover

    @property
    def requires(self) -> list[str] | None: ...  # pragma: no cover

    @property
    def origin(self) -> Any: ...  # pragma: no cover

    @classmethod
    def discover(
        cls, *, context: Any | None = None, **kwargs: Any
    ) -> Iterable[IDistribution]: ...  # pragma: no cover

    @classmethod
    def from_name(cls, name: str) -> IDistribution: ...  # pragma: no cover

    @staticmethod
    def at(path: str | os.PathLike[str]) -> IDistribution: ...  # pragma: no cover
