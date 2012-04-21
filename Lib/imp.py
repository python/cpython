"""This module provides the components needed to build your own __import__
function.  Undocumented functions are obsolete.

In most cases it is preferred you consider using the importlib module's
functionality over this module.

"""
# (Probably) need to stay in _imp
from _imp import (lock_held, acquire_lock, release_lock, reload,
                  load_dynamic, get_frozen_object, is_frozen_package,
                  init_builtin, init_frozen, is_builtin, is_frozen,
                  _fix_co_filename)
# Could move out of _imp, but not worth the code
from _imp import get_magic
# Can (probably) move to importlib
from _imp import (get_tag, get_suffixes)
# Should be re-implemented here (and mostly deprecated)
from _imp import (find_module, NullImporter,
                  SEARCH_ERROR, PY_SOURCE, PY_COMPILED, C_EXTENSION,
                  PY_RESOURCE, PKG_DIRECTORY, C_BUILTIN, PY_FROZEN,
                  PY_CODERESOURCE, IMP_HOOK)

from importlib._bootstrap import _new_module as new_module
from importlib._bootstrap import _cache_from_source as cache_from_source

from importlib import _bootstrap
import os


def source_from_cache(path):
    """Given the path to a .pyc./.pyo file, return the path to its .py file.

    The .pyc/.pyo file does not need to exist; this simply returns the path to
    the .py file calculated to correspond to the .pyc/.pyo file.  If path does
    not conform to PEP 3147 format, ValueError will be raised.

    """
    head, pycache_filename = os.path.split(path)
    head, pycache = os.path.split(head)
    if pycache != _bootstrap.PYCACHE:
        raise ValueError('{} not bottom-level directory in '
                         '{!r}'.format(_bootstrap.PYCACHE, path))
    if pycache_filename.count('.') != 2:
        raise ValueError('expected only 2 dots in '
                         '{!r}'.format(pycache_filename))
    base_filename = pycache_filename.partition('.')[0]
    return os.path.join(head, base_filename + _bootstrap.SOURCE_SUFFIXES[0])


class _HackedGetData:

    """Compatibiilty support for 'file' arguments of various load_*()
    functions."""

    def __init__(self, fullname, path, file=None):
        super().__init__(fullname, path)
        self.file = file

    def get_data(self, path):
        """Gross hack to contort loader to deal w/ load_*()'s bad API."""
        if self.file and path == self._path:
            with self.file:
                # Technically should be returning bytes, but
                # SourceLoader.get_code() just passed what is returned to
                # compile() which can handle str. And converting to bytes would
                # require figuring out the encoding to decode to and
                # tokenize.detect_encoding() only accepts bytes.
                return self.file.read()
        else:
            return super().get_data(path)


class _LoadSourceCompatibility(_HackedGetData, _bootstrap._SourceFileLoader):

    """Compatibility support for implementing load_source()."""


# XXX deprecate after better API exposed in importlib
def load_source(name, pathname, file=None):
    return _LoadSourceCompatibility(name, pathname, file).load_module(name)


class _LoadCompiledCompatibility(_HackedGetData,
        _bootstrap._SourcelessFileLoader):

    """Compatibility support for implementing load_compiled()."""


# XXX deprecate
def load_compiled(name, pathname, file=None):
    return _LoadCompiledCompatibility(name, pathname, file).load_module(name)


# XXX deprecate
def load_package(name, path):
    if os.path.isdir(path):
        extensions = _bootstrap._suffix_list(PY_SOURCE)
        extensions += _bootstrap._suffix_list(PY_COMPILED)
        for extension in extensions:
            path = os.path.join(path, '__init__'+extension)
            if os.path.exists(path):
                break
        else:
            raise ValueError('{!r} is not a package'.format(path))
    return _bootstrap._SourceFileLoader(name, path).load_module(name)


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
