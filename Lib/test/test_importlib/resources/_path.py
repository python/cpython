import pathlib
import functools


####
# from jaraco.path 3.4


def build(spec, prefix=pathlib.Path()):
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
    >>> tmpdir = getfixture('tmpdir')
    >>> build(spec, tmpdir)
    """
    for name, contents in spec.items():
        create(contents, pathlib.Path(prefix) / name)


@functools.singledispatch
def create(content, path):
    path.mkdir(exist_ok=True)
    build(content, prefix=path)  # type: ignore


@create.register
def _(content: bytes, path):
    path.write_bytes(content)


@create.register
def _(content: str, path):
    path.write_text(content)


# end from jaraco.path
####
