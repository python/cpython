"""The asyncio package, tracking PEP 3156."""

# flake8: noqa

import sys

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
from .taskgroups import *
from .timeouts import *
from .threads import *
from .transports import *

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
           threads.__all__ +
           timeouts.__all__ +
           transports.__all__ + (
            'create_eager_task_factory',
            'eager_task_factory',
           ))

# throwing things here temporarily to defer premature dir layout bikeshedding

def create_eager_task_factory(custom_task_constructor):

    def factory(loop, coro, *, name=None, context=None):
        loop._check_closed()
        if not loop.is_running():
            return custom_task_constructor(coro, loop=loop, name=name, context=context)

        try:
            result = coro.send(None)
        except StopIteration as si:
            fut = loop.create_future()
            fut.set_result(si.value)
            return fut
        except Exception as ex:
            fut = loop.create_future()
            fut.set_exception(ex)
            return fut
        else:
            return custom_task_constructor(
                coro, loop=loop, name=name, context=context, yield_result=result)

    return factory

eager_task_factory = create_eager_task_factory(Task)

if sys.platform == 'win32':  # pragma: no cover
    from .windows_events import *
    __all__ += windows_events.__all__
else:
    from .unix_events import *  # pragma: no cover
    __all__ += unix_events.__all__
