"""CancelScope — level-triggered cancellation for asyncio."""

__all__ = ('CancelScope', 'cancel_scope', 'cancel_scope_at')

from . import events
from . import exceptions
from . import tasks


class CancelScope:
    """Async context manager providing level-triggered cancellation.

    Once cancelled (via :meth:`cancel` or deadline expiry), every subsequent
    ``await`` inside the scope raises :exc:`~asyncio.CancelledError` until the
    scope exits.  The coroutine cannot simply catch-and-ignore the error.

    Parameters
    ----------
    deadline : float or None
        Absolute event-loop time after which the scope auto-cancels.
    shield : bool
        If ``True``, the level-triggered re-injection is suppressed while
        the scope is the current scope.
    """

    def __init__(self, *, deadline=None, shield=False):
        self._deadline = deadline
        self._shield = shield
        self._cancel_called = False
        self._task = None
        self._parent_scope = None
        self._timeout_handle = None
        self._host_task_cancelling = 0
        self._cancelled_caught = False

    # -- public properties ---------------------------------------------------

    @property
    def deadline(self):
        """Absolute event-loop time of the deadline, or *None*."""
        return self._deadline

    @deadline.setter
    def deadline(self, value):
        self._deadline = value
        if self._task is not None and not self._task.done():
            self._reschedule()

    @property
    def shield(self):
        """Whether level-triggered re-injection is suppressed."""
        return self._shield

    @shield.setter
    def shield(self, value):
        self._shield = value

    @property
    def cancel_called(self):
        """``True`` after :meth:`cancel` has been called."""
        return self._cancel_called

    @property
    def cancelled_caught(self):
        """``True`` if the scope caught the :exc:`CancelledError` on exit."""
        return self._cancelled_caught

    # -- control methods -----------------------------------------------------

    def cancel(self):
        """Cancel this scope.

        All subsequent awaits inside the scope will raise
        :exc:`~asyncio.CancelledError`.
        """
        if not self._cancel_called:
            self._cancel_called = True
            if self._task is not None and not self._task.done():
                self._task.cancel()

    def reschedule(self, deadline):
        """Change the deadline.

        If *deadline* is ``None`` the timeout is removed.
        """
        self._deadline = deadline
        if self._task is not None:
            self._reschedule()

    # -- async context manager -----------------------------------------------

    async def __aenter__(self):
        task = tasks.current_task()
        if task is None:
            # Fallback: _PyTask uses Python-level tracking that the
            # C current_task() does not see.
            task = tasks._py_current_task()
        if task is None:
            raise RuntimeError("CancelScope requires a running task")
        self._task = task
        self._host_task_cancelling = task.cancelling()
        self._parent_scope = task._current_cancel_scope
        task._current_cancel_scope = self
        if self._deadline is not None:
            loop = events.get_running_loop()
            self._timeout_handle = loop.call_at(
                self._deadline, self._on_deadline)
        return self

    async def __aexit__(self, exc_type, exc_val, exc_tb):
        if self._timeout_handle is not None:
            self._timeout_handle.cancel()
            self._timeout_handle = None

        # Pop scope stack
        self._task._current_cancel_scope = self._parent_scope

        if self._cancel_called:
            # Consume the one cancel() we injected, bringing the task's
            # cancellation counter back to where it was on __aenter__.
            if self._task.cancelling() > self._host_task_cancelling:
                self._task.uncancel()
            if exc_type is not None and issubclass(
                    exc_type, exceptions.CancelledError):
                self._cancelled_caught = True
                return True  # suppress the CancelledError

        return False

    # -- internal helpers ----------------------------------------------------

    def _reschedule(self):
        if self._timeout_handle is not None:
            self._timeout_handle.cancel()
            self._timeout_handle = None
        if self._deadline is not None and not self._task.done():
            loop = events.get_running_loop()
            self._timeout_handle = loop.call_at(
                self._deadline, self._on_deadline)

    def _on_deadline(self):
        self._timeout_handle = None
        self.cancel()


def cancel_scope(delay, *, shield=False):
    """Return a :class:`CancelScope` that expires *delay* seconds from now."""
    loop = events.get_running_loop()
    return CancelScope(deadline=loop.time() + delay, shield=shield)


def cancel_scope_at(when, *, shield=False):
    """Return a :class:`CancelScope` that expires at absolute time *when*."""
    return CancelScope(deadline=when, shield=shield)
