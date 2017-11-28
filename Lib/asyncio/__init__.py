"""The asyncio package, tracking PEP 3156."""

import sys

# This relies on each of the submodules having an __all__ variable.
from .base_events import *
from .coroutines import *
from .events import *
from .futures import *
from .locks import *
from .protocols import *
from .queues import *
from .streams import *
from .subprocess import *
from .tasks import *
from .transports import *

__all__ = (base_events.__all__ +
           coroutines.__all__ +
           events.__all__ +
           futures.__all__ +
           locks.__all__ +
           protocols.__all__ +
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
