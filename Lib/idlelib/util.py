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
from os import path

# .pyw is for Windows; .pyi is for stub files.
PYTHON_EXTENSIONS = frozenset(('.py', '.pyw', '.pyi'))


def is_python_extension(extension, valid_extensions=PYTHON_EXTENSIONS):
    "Identify whether a given string is a valid Python file extension."
    return extension in valid_extensions


def is_python_source(filepath, firstline=None):
    """Identify whether a given path is valid as a Python module."""
    if not filepath or path.isdir(filepath):
        return True
    base, ext = path.splitext(path.basename(filepath))
    return is_python_extension(path.normcase(ext)) or (
        firstline is not None
        and firstline.startswith('#!')
        and 'python' in firstline
    )


if __name__ == '__main__':
    from unittest import main
    main('idlelib.idle_test.test_util', verbosity=2)
