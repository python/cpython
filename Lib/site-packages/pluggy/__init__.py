try:
    from ._version import version as __version__
except ImportError:
    # broken installation, we don't even try
    # unknown only works because we do poor mans version compare
    __version__ = "unknown"

__all__ = [
    "PluginManager",
    "PluginValidationError",
    "HookCallError",
    "HookspecMarker",
    "HookimplMarker",
]

from .manager import PluginManager, PluginValidationError
from .callers import HookCallError
from .hooks import HookspecMarker, HookimplMarker
