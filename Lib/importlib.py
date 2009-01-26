"""Backport of importlib.import_module from 3.x."""
import sys

def _resolve_name(name, package, level):
    """Return the absolute name of the module to be imported."""
    level -= 1
    try:
        if package.count('.') < level:
            raise ValueError("attempted relative import beyond top-level "
                              "package")
    except AttributeError:
        raise ValueError("__package__ not set to a string")
    base = package.rsplit('.', level)[0]
    if name:
        return "{0}.{1}".format(base, name)
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
    __import__(name)
    return sys.modules[name]
