"""This module provides the components needed to build your own __import__
function.  Undocumented functions are obsolete.

In most cases it is preferred you consider using the importlib module's
functionality over this module.

"""
# (Probably) need to stay in _imp
from _imp import (lock_held, acquire_lock, release_lock,
                  get_frozen_object, is_frozen_package,
                  init_builtin, init_frozen, is_builtin, is_frozen,
                  _fix_co_filename)
try:
    from _imp import load_dynamic
except ImportError:
    # Platform doesn't support dynamic loading.
    load_dynamic = None

# Directly exposed by this module
from importlib._bootstrap import new_module
from importlib._bootstrap import cache_from_source, source_from_cache


from importlib import _bootstrap
from importlib import machinery
import os
import sys
import tokenize
import warnings


# DEPRECATED
SEARCH_ERROR = 0
PY_SOURCE = 1
PY_COMPILED = 2
C_EXTENSION = 3
PY_RESOURCE = 4
PKG_DIRECTORY = 5
C_BUILTIN = 6
PY_FROZEN = 7
PY_CODERESOURCE = 8
IMP_HOOK = 9


def get_magic():
    """Return the magic number for .pyc or .pyo files."""
    return _bootstrap._MAGIC_BYTES


def get_tag():
    """Return the magic tag for .pyc or .pyo files."""
    return sys.implementation.cache_tag


def get_suffixes():
    warnings.warn('imp.get_suffixes() is deprecated; use the constants '
                  'defined on importlib.machinery instead',
                  DeprecationWarning, 2)
    extensions = [(s, 'rb', C_EXTENSION) for s in machinery.EXTENSION_SUFFIXES]
    source = [(s, 'U', PY_SOURCE) for s in machinery.SOURCE_SUFFIXES]
    bytecode = [(s, 'rb', PY_COMPILED) for s in machinery.BYTECODE_SUFFIXES]

    return extensions + source + bytecode


class NullImporter:

    """Null import object."""

    def __init__(self, path):
        if path == '':
            raise ImportError('empty pathname', path='')
        elif os.path.isdir(path):
            raise ImportError('existing directory', path=path)

    def find_module(self, fullname):
        """Always returns None."""
        return None


class _HackedGetData:

    """Compatibiilty support for 'file' arguments of various load_*()
    functions."""

    def __init__(self, fullname, path, file=None):
        super().__init__(fullname, path)
        self.file = file

    def get_data(self, path):
        """Gross hack to contort loader to deal w/ load_*()'s bad API."""
        if self.file and path == self.path:
            with self.file:
                # Technically should be returning bytes, but
                # SourceLoader.get_code() just passed what is returned to
                # compile() which can handle str. And converting to bytes would
                # require figuring out the encoding to decode to and
                # tokenize.detect_encoding() only accepts bytes.
                return self.file.read()
        else:
            return super().get_data(path)


class _LoadSourceCompatibility(_HackedGetData, _bootstrap.SourceFileLoader):

    """Compatibility support for implementing load_source()."""


def load_source(name, pathname, file=None):
    msg = ('imp.load_source() is deprecated; use '
           'importlib.machinery.SourceFileLoader(name, pathname).load_module()'
           ' instead')
    warnings.warn(msg, DeprecationWarning, 2)
    _LoadSourceCompatibility(name, pathname, file).load_module(name)
    module = sys.modules[name]
    # To allow reloading to potentially work, use a non-hacked loader which
    # won't rely on a now-closed file object.
    module.__loader__ = _bootstrap.SourceFileLoader(name, pathname)
    return module


class _LoadCompiledCompatibility(_HackedGetData,
        _bootstrap.SourcelessFileLoader):

    """Compatibility support for implementing load_compiled()."""


def load_compiled(name, pathname, file=None):
    msg = ('imp.load_compiled() is deprecated; use '
           'importlib.machinery.SourcelessFileLoader(name, pathname).'
           'load_module() instead ')
    warnings.warn(msg, DeprecationWarning, 2)
    _LoadCompiledCompatibility(name, pathname, file).load_module(name)
    module = sys.modules[name]
    # To allow reloading to potentially work, use a non-hacked loader which
    # won't rely on a now-closed file object.
    module.__loader__ = _bootstrap.SourcelessFileLoader(name, pathname)
    return module


def load_package(name, path):
    msg = ('imp.load_package() is deprecated; use either '
           'importlib.machinery.SourceFileLoader() or '
           'importlib.machinery.SourcelessFileLoader() instead')
    warnings.warn(msg, DeprecationWarning, 2)
    if os.path.isdir(path):
        extensions = (machinery.SOURCE_SUFFIXES[:] +
                      machinery.BYTECODE_SUFFIXES[:])
        for extension in extensions:
            path = os.path.join(path, '__init__'+extension)
            if os.path.exists(path):
                break
        else:
            raise ValueError('{!r} is not a package'.format(path))
    return _bootstrap.SourceFileLoader(name, path).load_module(name)


def load_module(name, file, filename, details):
    """**DEPRECATED**

    Load a module, given information returned by find_module().

    The module name must include the full package name, if any.

    """
    suffix, mode, type_ = details
    with warnings.catch_warnings():
        warnings.simplefilter('ignore')
        if mode and (not mode.startswith(('r', 'U')) or '+' in mode):
            raise ValueError('invalid file open mode {!r}'.format(mode))
        elif file is None and type_ in {PY_SOURCE, PY_COMPILED}:
            msg = 'file object required for import (type code {})'.format(type_)
            raise ValueError(msg)
        elif type_ == PY_SOURCE:
            return load_source(name, filename, file)
        elif type_ == PY_COMPILED:
            return load_compiled(name, filename, file)
        elif type_ == C_EXTENSION and load_dynamic is not None:
            if file is None:
                with open(filename, 'rb') as opened_file:
                    return load_dynamic(name, filename, opened_file)
            else:
                return load_dynamic(name, filename, file)
        elif type_ == PKG_DIRECTORY:
            return load_package(name, filename)
        elif type_ == C_BUILTIN:
            return init_builtin(name)
        elif type_ == PY_FROZEN:
            return init_frozen(name)
        else:
            msg =  "Don't know how to import {} (type code {})".format(name, type_)
            raise ImportError(msg, name=name)


def find_module(name, path=None):
    """**DEPRECATED**

    Search for a module.

    If path is omitted or None, search for a built-in, frozen or special
    module and continue search in sys.path. The module name cannot
    contain '.'; to search for a submodule of a package, pass the
    submodule name and the package's __path__.

    """
    if not isinstance(name, str):
        raise TypeError("'name' must be a str, not {}".format(type(name)))
    elif not isinstance(path, (type(None), list)):
        # Backwards-compatibility
        raise RuntimeError("'list' must be None or a list, "
                           "not {}".format(type(name)))

    if path is None:
        if is_builtin(name):
            return None, None, ('', '', C_BUILTIN)
        elif is_frozen(name):
            return None, None, ('', '', PY_FROZEN)
        else:
            path = sys.path

    for entry in path:
        package_directory = os.path.join(entry, name)
        for suffix in ['.py', machinery.BYTECODE_SUFFIXES[0]]:
            package_file_name = '__init__' + suffix
            file_path = os.path.join(package_directory, package_file_name)
            if os.path.isfile(file_path):
                return None, package_directory, ('', '', PKG_DIRECTORY)
        with warnings.catch_warnings():
            warnings.simplefilter('ignore')
            for suffix, mode, type_ in get_suffixes():
                file_name = name + suffix
                file_path = os.path.join(entry, file_name)
                if os.path.isfile(file_path):
                    break
            else:
                continue
            break  # Break out of outer loop when breaking out of inner loop.
    else:
        raise ImportError(_bootstrap._ERR_MSG.format(name), name=name)

    encoding = None
    if mode == 'U':
        with open(file_path, 'rb') as file:
            encoding = tokenize.detect_encoding(file.readline)[0]
    file = open(file_path, mode, encoding=encoding)
    return file, file_path, (suffix, mode, type_)


_RELOADING = {}

def reload(module):
    """Reload the module and return it.

    The module must have been successfully imported before.

    """
    if not module or type(module) != type(sys):
        raise TypeError("reload() argument must be module")
    name = module.__name__
    if name not in sys.modules:
        msg = "module {} not in sys.modules"
        raise ImportError(msg.format(name), name=name)
    if name in _RELOADING:
        return _RELOADING[name]
    _RELOADING[name] = module
    try:
        parent_name = name.rpartition('.')[0]
        if parent_name and parent_name not in sys.modules:
            msg = "parent {!r} not in sys.modules"
            raise ImportError(msg.format(parentname), name=parent_name)
        return module.__loader__.load_module(name)
    finally:
        try:
            del _RELOADING[name]
        except KeyError:
            pass
