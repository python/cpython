"""
A script that replaces an old file with a new one, only if the contents
actually changed.  If not, the new file is simply deleted.

This avoids wholesale rebuilds when a code (re)generation phase does not
actually change the in-tree generated code.
"""

from __future__ import annotations

import contextlib
import os
import os.path
import sys

TYPE_CHECKING = False
if TYPE_CHECKING:
    import typing
    from collections.abc import Iterator
    from io import TextIOWrapper

    _Outcome: typing.TypeAlias = typing.Literal['created', 'updated', 'same']


@contextlib.contextmanager
def updating_file_with_tmpfile(
    filename: str,
    tmpfile: str | None = None,
) -> Iterator[tuple[TextIOWrapper, TextIOWrapper]]:
    """A context manager for updating a file via a temp file.

    The context manager provides two open files: the source file open
    for reading, and the temp file, open for writing.

    Upon exiting: both files are closed, and the source file is replaced
    with the temp file.
    """
    # XXX Optionally use tempfile.TemporaryFile?
    if not tmpfile:
        tmpfile = filename + '.tmp'
    elif os.path.isdir(tmpfile):
        tmpfile = os.path.join(tmpfile, filename + '.tmp')

    with open(filename, 'rb') as infile:
        line = infile.readline()

    if line.endswith(b'\r\n'):
        newline = "\r\n"
    elif line.endswith(b'\r'):
        newline = "\r"
    elif line.endswith(b'\n'):
        newline = "\n"
    else:
        raise ValueError(f"unknown end of line: {filename}: {line!a}")

    with open(tmpfile, 'w', newline=newline) as outfile:
        with open(filename) as infile:
            yield infile, outfile
    update_file_with_tmpfile(filename, tmpfile)


def update_file_with_tmpfile(
    filename: str,
    tmpfile: str,
    *,
    create: bool = False,
) -> _Outcome:
    try:
        targetfile = open(filename, 'rb')
    except FileNotFoundError:
        if not create:
            raise  # re-raise
        outcome: _Outcome = 'created'
        os.replace(tmpfile, filename)
    else:
        with targetfile:
            old_contents = targetfile.read()
        with open(tmpfile, 'rb') as f:
            new_contents = f.read()
        # Now compare!
        if old_contents != new_contents:
            outcome = 'updated'
            os.replace(tmpfile, filename)
        else:
            outcome = 'same'
            os.unlink(tmpfile)
    return outcome


if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument('--create', action='store_true')
    parser.add_argument('--exitcode', action='store_true')
    parser.add_argument('filename', help='path to be updated')
    parser.add_argument('tmpfile', help='path with new contents')
    args = parser.parse_args()
    kwargs = vars(args)
    setexitcode = kwargs.pop('exitcode')

    outcome = update_file_with_tmpfile(**kwargs)
    if setexitcode:
        if outcome == 'same':
            sys.exit(0)
        elif outcome == 'updated':
            sys.exit(1)
        elif outcome == 'created':
            sys.exit(2)
        else:
            raise NotImplementedError
