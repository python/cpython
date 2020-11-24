"""Event loop mixins."""

import threading
from . import events

_global_lock = threading.Lock()


class _LoopBoundedMixin:
    _loop = None

    def _get_loop(self):
        loop = events._get_running_loop()

        if self._loop is None:
            with _global_lock:
                if self._loop is None:
                    self._loop = loop
        if loop is not self._loop:
            raise RuntimeError(f'{type(self).__name__} have already bounded to another loop')
        return loop
