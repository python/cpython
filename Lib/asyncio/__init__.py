"""The asyncio package, tracking PEP 3156."""

# flake8: noqa

import sys

# This relies on each of the submodules having an __all__ variable.
from .base_events import *
from .coroutines import *
from .events import *
from .exceptions import *
from .futures import *
from .graph import *
from .locks import *
from .protocols import *
from .runners import *
from .queues import *
from .streams import *
from .subprocess import *
from .tasks import *
from .taskgroups import *
from .timeouts import *
from .threads import *
from .transports import *

__all__ = (base_events.__all__ +
           coroutines.__all__ +
           events.__all__ +
           exceptions.__all__ +
           futures.__all__ +
           graph.__all__ +
           locks.__all__ +
           protocols.__all__ +
           runners.__all__ +
           queues.__all__ +
           streams.__all__ +
           subprocess.__all__ +
           tasks.__all__ +
           taskgroups.__all__ +
           threads.__all__ +
           timeouts.__all__ +
           transports.__all__)

if sys.platform == 'win32':  # pragma: no cover
    from .windows_events import *
    __all__ += windows_events.__all__
else:
    from .unix_events import *  # pragma: no cover
    __all__ += unix_events.__all__

def __getattr__(name: str):
    import warnings

    match name:
        case "AbstractEventLoopPolicy":
            warnings._deprecated(f"asyncio.{name}", remove=(3, 16))
            return events._AbstractEventLoopPolicy
        case "DefaultEventLoopPolicy":
            warnings._deprecated(f"asyncio.{name}", remove=(3, 16))
            if sys.platform == 'win32':
                return windows_events._DefaultEventLoopPolicy
            return unix_events._DefaultEventLoopPolicy
        case "WindowsSelectorEventLoopPolicy":
            if sys.platform == 'win32':
                warnings._deprecated(f"asyncio.{name}", remove=(3, 16))
                return windows_events._WindowsSelectorEventLoopPolicy
            # Else fall through to the AttributeError below.
        case "WindowsProactorEventLoopPolicy":
            if sys.platform == 'win32':
                warnings._deprecated(f"asyncio.{name}", remove=(3, 16))
                return windows_events._WindowsProactorEventLoopPolicy
            # Else fall through to the AttributeError below.

    raise AttributeError(f"module {__name__!r} has no attribute {name!r}")
