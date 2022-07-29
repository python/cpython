"""
Idlelib objects with no external idlelib dependencies
which are needed in more than one idlelib module.

They are included here because
    a) they don't particularly belong elsewhere; or
    b) because inclusion here simplifies the idlelib dependency graph.

TODO:
    * Python versions (editor and help_about),
    * tk version and patchlevel (pyshell, help_about, maxos?, editor?),
    * std streams (pyshell, run),
    * warning stuff (pyshell, run).
"""

import os


# .pyw is for Windows; .pyi is for stub files.
py_extensions = ('.py', '.pyw', '.pyi')  # Order needed for open/save dialogs.

# The browser depends on pyclbr and importlib which do not support .pyi files.
browseable_extension_blocklist = ('.pyi',)


def _get_ext(path):
    _, ext = os.path.splitext(path)
    return os.path.normcase(ext)


def is_supported_extension(path):
    ext = _get_ext(path)
    return ext in py_extensions


def is_browseable_extension(path):
    ext = _get_ext(path)
    return ext in py_extensions and ext not in browseable_extension_blocklist


if __name__ == '__main__':
    from unittest import main
    main('idlelib.idle_test.test_util', verbosity=2)
