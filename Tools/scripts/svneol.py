#! /usr/bin/env python3

r"""
SVN helper script.

Try to set the svn:eol-style property to "native" on every .py, .txt, .c and
.h file in the directory tree rooted at the current directory.

Files with the svn:eol-style property already set (to anything) are skipped.

svn will itself refuse to set this property on a file that's not under SVN
control, or that has a binary mime-type property set.  This script inherits
that behavior, and passes on whatever warning message the failing "svn
propset" command produces.

In the Python project, it's safe to invoke this script from the root of
a checkout.

No output is produced for files that are ignored.  For a file that gets
svn:eol-style set, output looks like:

    property 'svn:eol-style' set on 'Lib\ctypes\__init__.py'

For a file not under version control:

    svn: warning: 'patch-finalizer.txt' is not under version control

and for a file with a binary mime-type property:

    svn: File 'Lib\test\test_pep263.py' has binary mime type property
"""

import re
import os
import sys
import subprocess


def propfiles(root, fn):
    default = os.path.join(root, ".svn", "props", fn + ".svn-work")
    try:
        format = int(open(os.path.join(root, ".svn", "format")).read().strip())
    except IOError:
        return []
    if format in (8, 9):
        # In version 8 and 9, committed props are stored in prop-base, local
        # modifications in props
        return [os.path.join(root, ".svn", "prop-base", fn + ".svn-base"),
                os.path.join(root, ".svn", "props", fn + ".svn-work")]
    raise ValueError("Unknown repository format")


def proplist(root, fn):
    """Return a list of property names for file fn in directory root."""
    result = []
    for path in propfiles(root, fn):
        try:
            f = open(path)
        except IOError:
            # no properties file: not under version control,
            # or no properties set
            continue
        while True:
            # key-value pairs, of the form
            # K <length>
            # <keyname>NL
            # V length
            # <value>NL
            # END
            line = f.readline()
            if line.startswith("END"):
                break
            assert line.startswith("K ")
            L = int(line.split()[1])
            key = f.read(L)
            result.append(key)
            f.readline()
            line = f.readline()
            assert line.startswith("V ")
            L = int(line.split()[1])
            value = f.read(L)
            f.readline()
        f.close()
    return result


def set_eol_native(path):
    cmd = 'svn propset svn:eol-style native "{}"'.format(path)
    propset = subprocess.Popen(cmd, shell=True)
    propset.wait()


possible_text_file = re.compile(r"\.([hc]|py|txt|sln|vcproj)$").search


def main():
    for arg in sys.argv[1:] or [os.curdir]:
        if os.path.isfile(arg):
            root, fn = os.path.split(arg)
            if 'svn:eol-style' not in proplist(root, fn):
                set_eol_native(arg)
        elif os.path.isdir(arg):
            for root, dirs, files in os.walk(arg):
                if '.svn' in dirs:
                    dirs.remove('.svn')
                for fn in files:
                    if possible_text_file(fn):
                        if 'svn:eol-style' not in proplist(root, fn):
                            path = os.path.join(root, fn)
                            set_eol_native(path)


if __name__ == '__main__':
    main()
