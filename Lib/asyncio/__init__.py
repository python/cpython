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


__all__ += ('StreamReader', 'StreamWriter', 'StreamReaderProtocol')  # deprecated


def __getattr__(name):
    global StreamReader, StreamWriter, StreamReaderProtocol
    if name == 'StreamReader':
        warnings.warn("StreamReader is deprecated since Python 3.8 "
                      "in favor of Stream, and scheduled for removal "
                      "in Python 3.10",
                      DeprecationWarning,
                      stacklevel=2)
        from .streams import StreamReader as sr
        StreamReader = sr
        return StreamReader
    if name == 'StreamWriter':
        warnings.warn("StreamWriter is deprecated since Python 3.8 "
                      "in favor of Stream, and scheduled for removal "
                      "in Python 3.10",
                      DeprecationWarning,
                      stacklevel=2)
        from .streams import StreamWriter as sw
        StreamWriter = sw
        return StreamWriter
    if name == 'StreamReaderProtocol':
        warnings.warn("Using asyncio internal class StreamReaderProtocol "
                      "is deprecated since Python 3.8 "
                      " and scheduled for removal "
                      "in Python 3.10",
                      DeprecationWarning,
                      stacklevel=2)
        from .streams import StreamReaderProtocol as srp
        StreamReaderProtocol = srp
        return StreamReaderProtocol

    raise AttributeError(f"module {__name__} has no attribute {name}")
