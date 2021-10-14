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
    * warning stuff (ditto).
"""
from os import path

# PYTHON_EXTENSIONS is used in editor, browser, and iomenu.
# .pyw is for Windows; .pyi is for stub files.
PYTHON_EXTENSIONS = frozenset({'.py', '.pyw', '.pyi'})


def is_python_extension(extension, is_valid=PYTHON_EXTENSIONS.__contains__):
    """Identify whether a given string is a valid Python file extension"""
    return is_valid(extension)


def is_python_source(filepath, firstline=None):
    """Identify whether a given path is valid as a Python module."""
    if not filepath or path.isdir(filepath):
        return True
    base, ext = path.splitext(path.basename(filepath))
    return is_python_extension(path.normcase(ext)) or all((
        firstline is not None,
        firstline.startswith('#!'),
        'python' in firstline
    ))
