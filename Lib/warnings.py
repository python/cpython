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
    WarningMessage,
    _DEPRECATED_MSG,
    _OptionError,
    _add_filter,
    _deprecated,
    _filters_mutated,
    _filters_mutated_lock_held,
    _filters_version,
    _formatwarning_orig,
    _formatwarnmsg,
    _formatwarnmsg_impl,
    _get_context,
    _get_filters,
    _getaction,
    _getcategory,
    _is_filename_to_skip,
    _is_internal_filename,
    _is_internal_frame,
    _lock,
    _new_context,
    _next_external_frame,
    _processoptions,
    _set_context,
    _set_module,
    _setoption,
    _setup_defaults,
    _showwarning_orig,
    _showwarnmsg,
    _showwarnmsg_impl,
    _use_context,
    _warn_unawaited_coroutine,
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
        _defaultaction as defaultaction,
        _filters_mutated_lock_held,
        _onceregistry as onceregistry,
        _release_lock,
        _warnings_context,
        filters,
        warn,
        warn_explicit,
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
