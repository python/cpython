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
from _imp import (find_module, load_module, load_compiled,
                  load_package, load_source, NullImporter,
                  SEARCH_ERROR, PY_SOURCE, PY_COMPILED, C_EXTENSION,
                  PY_RESOURCE, PKG_DIRECTORY, C_BUILTIN, PY_FROZEN,
                  PY_CODERESOURCE, IMP_HOOK)

from importlib._bootstrap import _new_module as new_module
