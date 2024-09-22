"""asyncio exceptions."""


__all__ = ('BrokenBarrierError',
           'CancelledError', 'InvalidStateError', 'TimeoutError',
           'IncompleteReadError', 'LimitOverrunError',
           'SendfileNotAvailableError')


class CancelledError(BaseException):
    """The Future or Task was cancelled."""


# GH-124308, BPO-32413: Originally, asyncio.TimeoutError was it's
# own unique exception. This was a bit of a gotcha because catching TimeoutError
# didn't catch asyncio.TimeoutError. So, it was turned into an alias
# of TimeoutError directly. Unfortunately, this made it effectively
# impossible to differentiate between asyncio.TimeoutError or a TimeoutError
# raised by a third party, so this is now a unique exception again (that inherits
# from TimeoutError to address the first problem).
class TimeoutError(TimeoutError):
    """Operation timed out."""


class InvalidStateError(Exception):
    """The operation is not allowed in this state."""


class SendfileNotAvailableError(RuntimeError):
    """Sendfile syscall is not available.

    Raised if OS does not support sendfile syscall for given socket or
    file type.
    """


class IncompleteReadError(EOFError):
    """
    Incomplete read error. Attributes:

    - partial: read bytes string before the end of stream was reached
    - expected: total number of expected bytes (or None if unknown)
    """
    def __init__(self, partial, expected):
        r_expected = 'undefined' if expected is None else repr(expected)
        super().__init__(f'{len(partial)} bytes read on a total of '
                         f'{r_expected} expected bytes')
        self.partial = partial
        self.expected = expected

    def __reduce__(self):
        return type(self), (self.partial, self.expected)


class LimitOverrunError(Exception):
    """Reached the buffer limit while looking for a separator.

    Attributes:
    - consumed: total number of to be consumed bytes.
    """
    def __init__(self, message, consumed):
        super().__init__(message)
        self.consumed = consumed

    def __reduce__(self):
        return type(self), (self.args[0], self.consumed)


class BrokenBarrierError(RuntimeError):
    """Barrier is broken by barrier.abort() call."""
