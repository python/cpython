"""
A script that replaces an old file with a new one, only if the contents
actually changed.  If not, the new file is simply deleted.

This avoids wholesale rebuilds when a code (re)generation phase does not
actually change the in-tree generated code.
"""

import os
import sys


def main(old_path, new_path):
    with open(old_path, 'rb') as f:
        old_contents = f.read()
    with open(new_path, 'rb') as f:
        new_contents = f.read()
    if old_contents != new_contents:
        os.replace(new_path, old_path)
    else:
        os.unlink(new_path)


if __name__ == '__main__':
    if len(sys.argv) != 3:
        print("Usage: %s <path to be updated> <path with new contents>" % (sys.argv[0],))
        sys.exit(1)
    main(sys.argv[1], sys.argv[2])
