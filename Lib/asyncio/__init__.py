"""The asyncio package, tracking PEP 3156."""

# flake8: noqa

import sys
import warnings

# This relies on each of the submodules having an __all__ variable.
from .base_events import *
from .coroutines import *
from .events import *
from .exceptions import *
from .futures import *
from .locks import *
from .protocols import *
from .runners import *
from .queues import *
from .streams import *
from .subprocess import *
from .tasks import *
from .transports import *

# Exposed for _asynciomodule.c to implement now deprecated
# Task.all_tasks() method.  This function will be removed in 3.9.
from .tasks import _all_tasks_compat  # NoQA

__all__ = (base_events.__all__ +
           coroutines.__all__ +
           events.__all__ +
           exceptions.__all__ +
           futures.__all__ +
           locks.__all__ +
           protocols.__all__ +
           runners.__all__ +
           queues.__all__ +
           streams.__all__ +
           subprocess.__all__ +
           tasks.__all__ +
           transports.__all__)

if sys.platform == 'win32':  # pragma: no cover
    from .windows_events import *
    __all__ += windows_events.__all__
else:
    from .unix_events import *  # pragma: no cover
    __all__ += unix_events.__all__


__all__ += ('StreamReader', 'StreamWriter')  # deprecated


def __getattr__(name):
    if name == 'StreamReader':
        warnings.warn("StreamReader is deprecated, use asyncio.Stream instead",
                      DeprecationWarning,
                      stacklevel=2)
        return Stream
    if name == 'StreamWriter':
        warnings.warn("StreamWriter is deprecated, use asyncio.Stream instead",
                      DeprecationWarning,
                      stacklevel=2)
        return Stream

    if name == 'StreamReaderProtocol':
        raise AttributeError("StreamReaderProtocol is a private API, "
                              "don't use the class in user code")

    raise AttributeError(f"module {__name__} has no attribute {name}")
