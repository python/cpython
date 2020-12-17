"""Everything made explicitly available via `__all__` can be considered as part of the tox API.

We will emit deprecation warnings for one minor release before making changes to these objects.

If objects are marked experimental they might change between minor versions.

To override/modify tox behaviour via plugins see `tox.hookspec` and its use with pluggy.
"""
import pluggy

from . import exception
from .constants import INFO, PIP, PYTHON
from .hookspecs import hookspec
from .version import __version__

__all__ = (
    "__version__",  # tox version
    "cmdline",  # run tox as part of another program/IDE (same behaviour as called standalone)
    "hookimpl",  # Hook implementation marker to be imported by plugins
    "exception",  # tox specific exceptions
    # EXPERIMENTAL CONSTANTS API
    "PYTHON",
    "INFO",
    "PIP",
    # DEPRECATED - will be removed from API in tox 4
    "hookspec",
)

hookimpl = pluggy.HookimplMarker("tox")

# NOTE: must come last due to circular import
from .session import cmdline  # isort:skip
