"""This module provides the components needed to build your own __import__
function.  Undocumented functions are obsolete.

In most cases it is preferred you consider using the importlib module's
functionality over this module.

"""
# (Probably) need to stay in _imp
from _imp import (lock_held, acquire_lock, release_lock,
                  load_dynamic, get_frozen_object, is_frozen_package,
                  init_builtin, init_frozen, is_builtin, is_frozen,
                  _fix_co_filename, extension_suffixes)
# Could move out of _imp, but not worth the code
from _imp import get_magic, get_tag

from importlib._bootstrap import new_module
from importlib._bootstrap import cache_from_source

from importlib import _bootstrap
import os
import sys
import tokenize


# XXX "deprecate" once find_module(), load_module(), and get_suffixes() are
#     deprecated.
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


def get_suffixes():
    extensions = [(s, 'rb', C_EXTENSION) for s in extension_suffixes()]
    source = [(s, 'U', PY_SOURCE) for s in _bootstrap._SOURCE_SUFFIXES]
    bytecode = [(_bootstrap._BYTECODE_SUFFIX, 'rb', PY_COMPILED)]

    return extensions + source + bytecode


def source_from_cache(path):
    """Given the path to a .pyc./.pyo file, return the path to its .py file.

    The .pyc/.pyo file does not need to exist; this simply returns the path to
    the .py file calculated to correspond to the .pyc/.pyo file.  If path does
    not conform to PEP 3147 format, ValueError will be raised.

    """
    head, pycache_filename = os.path.split(path)
    head, pycache = os.path.split(head)
    if pycache != _bootstrap._PYCACHE:
        raise ValueError('{} not bottom-level directory in '
                         '{!r}'.format(_bootstrap._PYCACHE, path))
    if pycache_filename.count('.') != 2:
        raise ValueError('expected only 2 dots in '
                         '{!r}'.format(pycache_filename))
    base_filename = pycache_filename.partition('.')[0]
    return os.path.join(head, base_filename + _bootstrap._SOURCE_SUFFIXES[0])


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


# XXX deprecate after better API exposed in importlib
def load_source(name, pathname, file=None):
    return _LoadSourceCompatibility(name, pathname, file).load_module(name)


class _LoadCompiledCompatibility(_HackedGetData,
        _bootstrap.SourcelessFileLoader):

    """Compatibility support for implementing load_compiled()."""


# XXX deprecate
def load_compiled(name, pathname, file=None):
    return _LoadCompiledCompatibility(name, pathname, file).load_module(name)


# XXX deprecate
def load_package(name, path):
    if os.path.isdir(path):
        extensions = _bootstrap._SOURCE_SUFFIXES
        extensions += [_bootstrap._BYTECODE_SUFFIX]
        for extension in extensions:
            path = os.path.join(path, '__init__'+extension)
            if os.path.exists(path):
                break
        else:
            raise ValueError('{!r} is not a package'.format(path))
    return _bootstrap.SourceFileLoader(name, path).load_module(name)


# XXX deprecate
def load_module(name, file, filename, details):
    """Load a module, given information returned by find_module().

    The module name must include the full package name, if any.

    """
    suffix, mode, type_ = details
    if mode and (not mode.startswith(('r', 'U')) or '+' in mode):
        raise ValueError('invalid file open mode {!r}'.format(mode))
    elif file is None and type_ in {PY_SOURCE, PY_COMPILED}:
        msg = 'file object required for import (type code {})'.format(type_)
        raise ValueError(msg)
    elif type_ == PY_SOURCE:
        return load_source(name, filename, file)
    elif type_ == PY_COMPILED:
        return load_compiled(name, filename, file)
    elif type_ == PKG_DIRECTORY:
        return load_package(name, filename)
    elif type_ == C_BUILTIN:
        return init_builtin(name)
    elif type_ == PY_FROZEN:
        return init_frozen(name)
    else:
        msg =  "Don't know how to import {} (type code {}".format(name, type_)
        raise ImportError(msg, name=name)


def find_module(name, path=None):
    """Search for a module.

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
        for suffix in ['.py', _bootstrap._BYTECODE_SUFFIX]:
            package_file_name = '__init__' + suffix
            file_path = os.path.join(package_directory, package_file_name)
            if os.path.isfile(file_path):
                return None, package_directory, ('', '', PKG_DIRECTORY)
        for suffix, mode, type_ in get_suffixes():
            file_name = name + suffix
            file_path = os.path.join(entry, file_name)
            if os.path.isfile(file_path):
                break
        else:
            continue
        break  # Break out of outer loop when breaking out of inner loop.
    else:
        raise ImportError('No module name {!r}'.format(name), name=name)

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
