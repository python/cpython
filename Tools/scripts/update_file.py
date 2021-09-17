"""
A script that replaces an old file with a new one, only if the contents
actually changed.  If not, the new file is simply deleted.

This avoids wholesale rebuilds when a code (re)generation phase does not
actually change the in-tree generated code.
"""

import contextlib
import os
import os.path
import sys


@contextlib.contextmanager
def updating_file_with_tmpfile(filename, tmpfile=None):
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


def update_file_with_tmpfile(filename, tmpfile):
    with open(filename, 'rb') as f:
        old_contents = f.read()
    with open(tmpfile, 'rb') as f:
        new_contents = f.read()
    if old_contents != new_contents:
        os.replace(tmpfile, filename)
    else:
        os.unlink(tmpfile)


if __name__ == '__main__':
    if len(sys.argv) != 3:
        print("Usage: %s <path to be updated> <path with new contents>" % (sys.argv[0],))
        sys.exit(1)
    update_file_with_tmpfile(sys.argv[1], sys.argv[2])
