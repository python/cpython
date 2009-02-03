"""Backport of importlib.import_module from 3.x."""
# While not critical (and in no way guaranteed!), it would be nice to keep this
# code compatible with Python 2.3.
import sys

def _resolve_name(name, package, level):
    """Return the absolute name of the module to be imported."""
    level -= 1
    try:
        if package.count('.') < level:
            raise ValueError("attempted relative import beyond top-level "
                              "package")
    except AttributeError:
        raise ValueError("'package' not set to a string")
    try:
        # rpartition is more "correct" and rfind is just as easy to use, but
        # neither are in Python 2.3.
        dot_rindex = package.rindex('.', level)[0]
        base = package[:dot_rindex]
    except ValueError:
        base = package
    if name:
        return "%s.%s" % (base, name)
    else:
        return base


def import_module(name, package=None):
    """Import a module.

    The 'package' argument is required when performing a relative import. It
    specifies the package to use as the anchor point from which to resolve the
    relative import to an absolute import.

    """
    if name.startswith('.'):
        if not package:
            raise TypeError("relative imports require the 'package' argument")
        level = 0
        for character in name:
            if character != '.':
                break
            level += 1
        name = _resolve_name(name[level:], package, level)
    # Try to import specifying the level to be as accurate as possible, but
    # realize that keyword arguments are not found in Python 2.3.
    try:
        __import__(name, level=0)
    except TypeError:
        __import__(name)
    return sys.modules[name]
