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
from _imp import (find_module, load_compiled,
                  load_package, load_source, NullImporter,
                  SEARCH_ERROR, PY_SOURCE, PY_COMPILED, C_EXTENSION,
                  PY_RESOURCE, PKG_DIRECTORY, C_BUILTIN, PY_FROZEN,
                  PY_CODERESOURCE, IMP_HOOK)

from importlib._bootstrap import _new_module as new_module


def load_module(name, file, filename, details):
    """Load a module, given information returned by find_module().

    The module name must include the full package name, if any.

    """
    suffix, mode, type_ = details
    if mode and (not mode.startswith(('r', 'U'))) or '+' in mode:
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
