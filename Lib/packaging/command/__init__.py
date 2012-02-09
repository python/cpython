"""Subpackage containing all standard commands."""
import os
from packaging.errors import PackagingModuleError
from packaging.util import resolve_name

__all__ = ['get_command_names', 'set_command', 'get_command_class',
           'STANDARD_COMMANDS']


STANDARD_COMMANDS = [
    # packaging
    'check', 'test',
    # building
    'build', 'build_py', 'build_ext', 'build_clib', 'build_scripts', 'clean',
    # installing
    'install_dist', 'install_lib', 'install_headers', 'install_scripts',
    'install_data', 'install_distinfo',
    # distributing
    'sdist', 'bdist', 'bdist_dumb', 'bdist_wininst',
    'register', 'upload', 'upload_docs',
    ]

if os.name == 'nt':
    STANDARD_COMMANDS.insert(STANDARD_COMMANDS.index('bdist_wininst'),
                             'bdist_msi')

# XXX maybe we need more than one registry, so that --list-comands can display
# standard, custom and overriden standard commands differently
_COMMANDS = dict((name, 'packaging.command.%s.%s' % (name, name))
                 for name in STANDARD_COMMANDS)


def get_command_names():
    """Return registered commands"""
    return sorted(_COMMANDS)


def set_command(location):
    cls = resolve_name(location)
    # XXX we want to do the duck-type checking here
    _COMMANDS[cls.get_command_name()] = cls


def get_command_class(name):
    """Return the registered command"""
    try:
        cls = _COMMANDS[name]
    except KeyError:
        raise PackagingModuleError("Invalid command %s" % name)
    if isinstance(cls, str):
        cls = resolve_name(cls)
        _COMMANDS[name] = cls
    return cls
