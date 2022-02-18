import contextlib
import enum

from types import TracebackType
from typing import final, Any, Iterator, Optional, Type

from . import events
from . import tasks


__all__ = (
    "cancel_scope",
    "cancel_scope_at",
    "timeout",
    "timeout_at",
    "CancelScope"
)


class _State(enum.Enum):
    CREATED = "created"
    ENTERED = "active"
    CANCELLING = "cancelling"
    CANCELLED = "cancelled"
    EXITED = "exited"


@final
class CancelScope:
    __slots__ = ("_deadline", "_loop", "_state", "_task", "_timeout_handler")

    def __init__(
        self, deadline: Optional[float], loop: events.AbstractEventLoop
    ) -> None:
        self._loop = loop
        self._state = _State.CREATED

        self._timeout_handler: Optional[events.TimerHandle] = None
        self._task: Optional[tasks.Task[Any]] = None
        self._deadline = deadline

    @property
    def deadline(self) -> Optional[float]:
        return self._deadline

    @deadline.setter
    def deadline(self, value: Optional[float]) -> None:
        self._deadline = value
        self._reschedule()

    def cancel(self, msg: Any = None) -> bool:
        return self._task.cancel(msg)

    def cancelled(self) -> bool:
        """Is timeout expired during execution?"""
        return self._state == _State.CANCELLED

    def cancelling(self) -> bool:
        # Timeout reached, cancellation scheduled
        return self._state in (_State.CANCELLING, _State.CANCELLED)

    def __repr__(self) -> str:
        info = [str(self._state)]
        if self._state == _State.ENTERED and self._deadline is not None:
            info.append("deadline={self._deadline}")
        cls_name = self.__class__.__name__
        return f"<{cls_name} at {id(self):#x}, {' '.join(info)}>"

    def __enter__(self) -> "CancelScope":
        self._state = _State.ENTERED
        self._task = tasks.current_task()
        if self._task is None:
            raise RuntimeError("CancelScope should be used inside a task")
        self._reschedule()
        return self

    def __exit__(
        self,
        exc_type: Optional[Type[BaseException]],
        exc_val: Optional[BaseException],
        exc_tb: Optional[TracebackType],
    ) -> Optional[bool]:
        # state is EXITED if not timed out previously
        assert self._state in (_State.ENTERED, _State.CANCELLING)
        if self._state == _State.CANCELLING:
            self._state = _State.CANCELLED
        elif self._state == _State.ENTERED:
            self._state = _State.EXITED

        if self._timeout_handler is not None:
            self._timeout_handler.cancel()
            self._timeout_handler = None

        return None

    def _reschedule(self) -> None:
        assert self._state != _State.CREATED
        if self._state != _State.ENTERED:
            raise RuntimeError(
                f"Cannot change state of {self._state} CancelScope",
            )

        if self._timeout_handler is not None:
            self._timeout_handler.cancel()

        if self._deadline is None:
            self._timeout_handler = None
        else:
            self._timeout_handler = self._loop.call_at(
                self._deadline,
                self._on_timeout,
            )

    def _on_timeout(self) -> None:
        assert self._state == _State.ENTERED
        self._task.cancel()
        self._state = _State.CANCELLING
        # drop the reference early
        self._timeout_handler = None


@contextlib.contextmanager
def cancel_scope(delay: Optional[float]) -> Iterator[CancelScope]:
    loop = events.get_running_loop()
    with CancelScope(
        loop.time() + delay if delay is not None else None,
        loop,
    ) as scope:
        yield scope


def cancel_scope_at(deadline: Optional[float]) -> Iterator[CancelScope]:
    loop = events.get_running_loop()
    with CancelScope(deadline, loop) as scope:
        yield scope


@contextlib.contextmanager
def timeout(delay: Optional[float]) -> Iterator[CancelScope]:
    """timeout context manager.

    Useful in cases when you want to apply timeout logic around block
    of code or in cases when asyncio.wait_for is not suitable. For example:

    >>> with timeout(10):  # 10 seconds timeout
    ...     await long_running_task()


    delay - value in seconds or None to disable timeout logic
    """
    with cancel_scope(delay) as scope:
        yield scope
    if scope.expired:
        raise TimeoutError()


def timeout_at(deadline: Optional[float]) -> Iterator[CancelScope]:
    """Schedule the timeout at absolute time.

    deadline argument points on the time in the same clock system
    as loop.time().

    Please note: it is not POSIX time but a time with
    undefined starting base, e.g. the time of the system power on.

    >>> async with timeout_at(loop.time() + 10):
    ...     async with aiohttp.get('https://github.com') as r:
    ...         await r.text()


    """
    with cancel_scope_at(deadline) as scope:
        yield scope
    if scope.expired:
        raise TimeoutError()
