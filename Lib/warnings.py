import sys

__all__ = [
    "warn",
    "warn_explicit",
    "showwarning",
    "formatwarning",
    "filterwarnings",
    "simplefilter",
    "resetwarnings",
    "catch_warnings",
    "deprecated",
]

from _py_warnings import (
    _filters_mutated_lock_held,
    _lock,
    _processoptions,
    _set_module,
    _setup_defaults,
    _warnings_context,
    catch_warnings,
    defaultaction,
    deprecated,
    filters,
    filterwarnings,
    formatwarning,
    onceregistry,
    resetwarnings,
    showwarning,
    simplefilter,
    warn,
    warn_explicit,
)

try:
    # Try to use the C extension, this will replace some parts of the
    # _py_warnings implementation imported above.
    from _warnings import (
        _acquire_lock,
        _filters_mutated_lock_held,
        _release_lock,
        _warnings_context,
        filters,
        warn,
        warn_explicit,
    )
    from _warnings import (
        _defaultaction as defaultaction,
    )
    from _warnings import (
        _onceregistry as onceregistry,
    )

    _warnings_defaults = True

    class _Lock:
        def __enter__(self):
            _acquire_lock()
            return self

        def __exit__(self, *args):
            _release_lock()

    _lock = _Lock()
except ImportError:
    _warnings_defaults = False


# Module initialization
_set_module(sys.modules[__name__])
_processoptions(sys.warnoptions)
if not _warnings_defaults:
    _setup_defaults()

del _warnings_defaults
del _setup_defaults
