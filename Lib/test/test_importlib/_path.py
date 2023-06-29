# from jaraco.path 3.5

import functools
import pathlib
from typing import Dict, Union

try:
    from typing import Protocol, runtime_checkable
except ImportError:  # pragma: no cover
    # Python 3.7
    from typing_extensions import Protocol, runtime_checkable  # type: ignore


FilesSpec = Dict[str, Union[str, bytes, 'FilesSpec']]  # type: ignore


@runtime_checkable
class TreeMaker(Protocol):
    def __truediv__(self, *args, **kwargs):
        ...  # pragma: no cover

    def mkdir(self, **kwargs):
        ...  # pragma: no cover

    def write_text(self, content, **kwargs):
        ...  # pragma: no cover

    def write_bytes(self, content):
        ...  # pragma: no cover


def _ensure_tree_maker(obj: Union[str, TreeMaker]) -> TreeMaker:
    return obj if isinstance(obj, TreeMaker) else pathlib.Path(obj)  # type: ignore


def build(
    spec: FilesSpec,
    prefix: Union[str, TreeMaker] = pathlib.Path(),  # type: ignore
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
    ...     }
    ... }
    >>> target = getfixture('tmp_path')
    >>> build(spec, target)
    >>> target.joinpath('foo/baz.py').read_text(encoding='utf-8')
    '# Some code'
    """
    for name, contents in spec.items():
        create(contents, _ensure_tree_maker(prefix) / name)


@functools.singledispatch
def create(content: Union[str, bytes, FilesSpec], path):
    path.mkdir(exist_ok=True)
    build(content, prefix=path)  # type: ignore


@create.register
def _(content: bytes, path):
    path.write_bytes(content)


@create.register
def _(content: str, path):
    path.write_text(content, encoding='utf-8')


@create.register
def _(content: str, path):
    path.write_text(content, encoding='utf-8')


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
