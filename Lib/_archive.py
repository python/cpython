"""Internal helpers shared by tarfile and zipfile."""

import os
import sys


if sys.platform in ("win32", "emscripten"):
    # These follow a dangling symlink even with O_EXCL, so remove what is
    # there first.
    def create_or_replace(path):
        """Open path for writing as a new file, replacing what is there.

        An existing symlink or hard link is replaced, instead of written
        through to the file it refers to.
        """
        try:
            os.unlink(path)
        except FileNotFoundError:
            pass
        return open(path, "xb")
else:
    # O_EXCL refuses any existing file, including a dangling symlink.
    def create_or_replace(path):
        """Open path for writing as a new file, replacing what is there.

        An existing symlink or hard link is replaced, instead of written
        through to the file it refers to.
        """
        try:
            return open(path, "xb")
        except FileExistsError:
            os.unlink(path)
            return open(path, "xb")
