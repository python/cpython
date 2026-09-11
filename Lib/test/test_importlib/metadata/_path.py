# from jaraco.path 3.7.2

from __future__ import annotations

import functools
import pathlib
from collections.abc import Mapping
from typing import TYPE_CHECKING, Protocol, Union, runtime_checkable

if TYPE_CHECKING:
    from typing_extensions import Self


class Symlink(str):
    """
    A string indicating the target of a symlink.
    """


FilesSpec = Mapping[str, Union[str, bytes, Symlink, 'FilesSpec']]


@runtime_checkable
class TreeMaker(Protocol):
    def __truediv__(self, other, /) -> Self: ...
    def mkdir(self, *, exist_ok) -> object: ...
    def write_text(self, content, /, *, encoding) -> object: ...
    def write_bytes(self, content, /) -> object: ...
    def symlink_to(self, target, /) -> object: ...


def _ensure_tree_maker(obj: str | TreeMaker) -> TreeMaker:
    return obj if isinstance(obj, TreeMaker) else pathlib.Path(obj)


def build(
    spec: FilesSpec,
    prefix: str | TreeMaker = pathlib.Path(),
):
    """
    Build a set of files/directories, as described by the spec.

    Each key represents a pathname, and the value represents
    the content. Content may be a nested directory.

    >>> spec = {
    ...     'README.txt': "A README file",
    ...     "foo": {
    ...         "__init__.py": "",
    ...         "bar": {
    ...             "__init__.py": "",
    ...         },
    ...         "baz.py": "# Some code",
    ...         "bar.py": Symlink("baz.py"),
    ...     },
    ...     "bing": Symlink("foo"),
    ... }
    >>> target = getfixture('tmp_path')
    >>> build(spec, target)
    >>> target.joinpath('foo/baz.py').read_text(encoding='utf-8')
    '# Some code'
    >>> target.joinpath('bing/bar.py').read_text(encoding='utf-8')
    '# Some code'
    """
    for name, contents in spec.items():
        create(contents, _ensure_tree_maker(prefix) / name)


@functools.singledispatch
def create(content: str | bytes | FilesSpec, path: TreeMaker) -> None:
    path.mkdir(exist_ok=True)
    # Mypy only looks at the signature of the main singledispatch method. So it must contain the complete Union
    build(content, prefix=path)  # type: ignore[arg-type] # python/mypy#11727


@create.register
def _(content: bytes, path: TreeMaker) -> None:
    path.write_bytes(content)


@create.register
def _(content: str, path: TreeMaker) -> None:
    path.write_text(content, encoding='utf-8')


@create.register
def _(content: Symlink, path: TreeMaker) -> None:
    path.symlink_to(content)


class Recording:
    """
    A TreeMaker object that records everything that would be written.

    >>> r = Recording()
    >>> build({'foo': {'foo1.txt': 'yes'}, 'bar.txt': 'abc'}, r)
    >>> r.record
    ['foo/foo1.txt', 'bar.txt']
    """

    def __init__(self, loc=pathlib.PurePosixPath(), record=None):
        self.loc = loc
        self.record = record if record is not None else []

    def __truediv__(self, other):
        return Recording(self.loc / other, self.record)

    def write_text(self, content, **kwargs):
        self.record.append(str(self.loc))

    write_bytes = write_text

    def mkdir(self, **kwargs):
        return

    def symlink_to(self, target):
        pass
