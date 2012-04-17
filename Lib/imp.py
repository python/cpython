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
# Can (probably) move to importlib
from _imp import (get_magic, get_tag, get_suffixes, cache_from_source,
                  source_from_cache)
# Should be re-implemented here (and mostly deprecated)
from _imp import (find_module, NullImporter,
                  SEARCH_ERROR, PY_SOURCE, PY_COMPILED, C_EXTENSION,
                  PY_RESOURCE, PKG_DIRECTORY, C_BUILTIN, PY_FROZEN,
                  PY_CODERESOURCE, IMP_HOOK)

from importlib._bootstrap import _new_module as new_module

from importlib import _bootstrap
import os


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


def load_source(name, pathname, file=None):
    return _LoadSourceCompatibility(name, pathname, file).load_module(name)


class _LoadCompiledCompatibility(_HackedGetData,
        _bootstrap._SourcelessFileLoader):

    """Compatibility support for implementing load_compiled()."""


def load_compiled(name, pathname, file=None):
    return _LoadCompiledCompatibility(name, pathname, file).load_module(name)


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
