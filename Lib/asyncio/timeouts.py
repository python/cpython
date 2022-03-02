import enum

from types import TracebackType
from typing import final, Any, Dict, Optional, Type

from . import events
from . import tasks


__all__ = (
    "Timeout",
    "timeout",
    "timeout_at",
)


class _State(str, enum.Enum):
    CREATED = "created"
    ENTERED = "active"
    EXPIRING = "expiring"
    EXPIRED = "expired"
    EXITED = "exited"


@final
class Timeout:

    def __init__(self, deadline: Optional[float]) -> None:
        self._state = _State.CREATED

        self._timeout_handler: Optional[events.TimerHandle] = None
        self._task: Optional[tasks.Task[Any]] = None
        self._deadline = deadline

    def when(self) -> Optional[float]:
        return self._deadline

    def reschedule(self, deadline: Optional[float]) -> None:
        assert self._state != _State.CREATED
        if self._state != _State.ENTERED:
            raise RuntimeError(
                f"Cannot change state of {self._state} CancelScope",
            )

        self._deadline = deadline

        if self._timeout_handler is not None:
            self._timeout_handler.cancel()

        if deadline is None:
            self._timeout_handler = None
        else:
            loop = events.get_running_loop()
            self._timeout_handler = loop.call_at(
                deadline,
                self._on_timeout,
            )

    def expired(self) -> bool:
        """Is timeout expired during execution?"""
        return self._state in (_State.EXPIRING, _State.EXPIRED)

    def __repr__(self) -> str:
        info = [str(self._state)]
        if self._state is _State.ENTERED and self._deadline is not None:
            info.append(f"deadline={self._deadline}")
        cls_name = self.__class__.__name__
        return f"<{cls_name} at {id(self):#x}, {' '.join(info)}>"

    async def __aenter__(self) -> "Timeout":
        self._state = _State.ENTERED
        self._task = tasks.current_task()
        if self._task is None:
            raise RuntimeError("Timeout should be used inside a task")
        self.reschedule(self._deadline)
        return self

    async def __aexit__(
        self,
        exc_type: Optional[Type[BaseException]],
        exc_val: Optional[BaseException],
        exc_tb: Optional[TracebackType],
    ) -> Optional[bool]:
        assert self._state in (_State.ENTERED, _State.EXPIRING)

        if self._timeout_handler is not None:
            self._timeout_handler.cancel()
            self._timeout_handler = None

        if self._state is _State.EXPIRING:
            self._state = _State.EXPIRED

            if self._task.uncancel() == 0:
                # Since there are no outstanding cancel requests, we're
                # handling this.
                raise TimeoutError
        elif self._state is _State.ENTERED:
            self._state = _State.EXITED

        return None

    def _on_timeout(self) -> None:
        assert self._state is _State.ENTERED
        self._task.cancel()
        self._state = _State.EXPIRING
        # drop the reference early
        self._timeout_handler = None


def timeout(delay: Optional[float]) -> Timeout:
    """timeout context manager.

    Useful in cases when you want to apply timeout logic around block
    of code or in cases when asyncio.wait_for is not suitable. For example:

    >>> with timeout(10):  # 10 seconds timeout
    ...     await long_running_task()


    delay - value in seconds or None to disable timeout logic
    """
    loop = events.get_running_loop()
    return Timeout(loop.time() + delay if delay is not None else None)


def timeout_at(deadline: Optional[float]) -> Timeout:
    """Schedule the timeout at absolute time.

    deadline argument points on the time in the same clock system
    as loop.time().

    Please note: it is not POSIX time but a time with
    undefined starting base, e.g. the time of the system power on.

    >>> async with timeout_at(loop.time() + 10):
    ...     async with aiohttp.get('https://github.com') as r:
    ...         await r.text()


    """
    return Timeout(deadline)
