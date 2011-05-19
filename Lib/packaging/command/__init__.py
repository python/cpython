"""Subpackage containing all standard commands."""

from packaging.errors import PackagingModuleError
from packaging.util import resolve_name

__all__ = ['get_command_names', 'set_command', 'get_command_class',
           'STANDARD_COMMANDS']

_COMMANDS = {
    'check': 'packaging.command.check.check',
    'test': 'packaging.command.test.test',
    'build': 'packaging.command.build.build',
    'build_py': 'packaging.command.build_py.build_py',
    'build_ext': 'packaging.command.build_ext.build_ext',
    'build_clib': 'packaging.command.build_clib.build_clib',
    'build_scripts': 'packaging.command.build_scripts.build_scripts',
    'clean': 'packaging.command.clean.clean',
    'install_dist': 'packaging.command.install_dist.install_dist',
    'install_lib': 'packaging.command.install_lib.install_lib',
    'install_headers': 'packaging.command.install_headers.install_headers',
    'install_scripts': 'packaging.command.install_scripts.install_scripts',
    'install_data': 'packaging.command.install_data.install_data',
    'install_distinfo':
        'packaging.command.install_distinfo.install_distinfo',
    'sdist': 'packaging.command.sdist.sdist',
    'bdist': 'packaging.command.bdist.bdist',
    'bdist_dumb': 'packaging.command.bdist_dumb.bdist_dumb',
    'bdist_wininst': 'packaging.command.bdist_wininst.bdist_wininst',
    'register': 'packaging.command.register.register',
    'upload': 'packaging.command.upload.upload',
    'upload_docs': 'packaging.command.upload_docs.upload_docs'}

STANDARD_COMMANDS = set(_COMMANDS)


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
        if isinstance(cls, str):
            cls = resolve_name(cls)
            _COMMANDS[name] = cls
        return cls
    except KeyError:
        raise PackagingModuleError("Invalid command %s" % name)
