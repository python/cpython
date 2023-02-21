import contextlib
import time


class DeadlineExceeded(Exception):
    pass


class TimedContext(contextlib.ContextDecorator):
    """
    A context that will raise DeadlineExceeded if the
    max duration is reached during the execution.

    >>> TimedContext(1)(time.sleep)(.1)
    >>> TimedContext(0)(time.sleep)(.1)
    Traceback (most recent call last):
    ...
    tests._context.DeadlineExceeded: (..., 0)
    """

    def __init__(self, max_duration: int):
        self.max_duration = max_duration

    def __enter__(self):
        self.start = time.monotonic()

    def __exit__(self, *err):
        duration = time.monotonic() - self.start
        if duration > self.max_duration:
            raise DeadlineExceeded(duration, self.max_duration)
