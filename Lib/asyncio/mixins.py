"""Event loop mixins."""

__all__ = ('LoopBoundedMixin',)

import threading
from . import events

global_lock = threading.Lock()


class LoopBoundedMixin:
    _loop = None

    def _get_loop(self):
        loop = events._get_running_loop()

        if self._loop is None:
            with global_lock:
                if self._loop is None:
                    self._loop = loop
        if loop is not self._loop:
            raise RuntimeError
        return loop
