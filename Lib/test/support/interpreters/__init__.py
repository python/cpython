"""Subinterpreters High Level Module."""

import threading
import weakref
import _xxsubinterpreters as _interpreters

# aliases:
from _xxsubinterpreters import (
    InterpreterError, InterpreterNotFoundError,
    is_shareable,
)


__all__ = [
    'get_current', 'get_main', 'create', 'list_all', 'is_shareable',
    'Interpreter',
    'InterpreterError', 'InterpreterNotFoundError', 'ExecFailure',
    'create_queue', 'Queue', 'QueueEmpty', 'QueueFull',
]


_queuemod = None

def __getattr__(name):
    if name in ('Queue', 'QueueEmpty', 'QueueFull', 'create_queue'):
        global create_queue, Queue, QueueEmpty, QueueFull
        ns = globals()
        from .queues import (
            create as create_queue,
            Queue, QueueEmpty, QueueFull,
        )
        return ns[name]
    else:
        raise AttributeError(name)


class ExecFailure(RuntimeError):

    def __init__(self, excinfo):
        msg = excinfo.formatted
        if not msg:
            if excinfo.type and snapshot.msg:
                msg = f'{snapshot.type.__name__}: {snapshot.msg}'
            else:
                msg = snapshot.type.__name__ or snapshot.msg
        super().__init__(msg)
        self.snapshot = excinfo


def create():
    """Return a new (idle) Python interpreter."""
    id = _interpreters.create(isolated=True)
    return Interpreter(id)


def list_all():
    """Return all existing interpreters."""
    return [Interpreter(id) for id in _interpreters.list_all()]


def get_current():
    """Return the currently running interpreter."""
    id = _interpreters.get_current()
    return Interpreter(id)


def get_main():
    """Return the main interpreter."""
    id = _interpreters.get_main()
    return Interpreter(id)


_known = weakref.WeakValueDictionary()

class Interpreter:
    """A single Python interpreter."""

    def __new__(cls, id, /):
        # There is only one instance for any given ID.
        if not isinstance(id, int):
            raise TypeError(f'id must be an int, got {id!r}')
        id = int(id)
        try:
            self = _known[id]
            assert hasattr(self, '_ownsref')
        except KeyError:
            # This may raise InterpreterNotFoundError:
            _interpreters._incref(id)
            try:
                self = super().__new__(cls)
                self._id = id
                self._ownsref = True
            except BaseException:
                _interpreters._deccref(id)
                raise
            _known[id] = self
        return self

    def __repr__(self):
        return f'{type(self).__name__}({self.id})'

    def __hash__(self):
        return hash(self._id)

    def __del__(self):
        self._decref()

    def _decref(self):
        if not self._ownsref:
            return
        self._ownsref = False
        try:
            _interpreters._decref(self.id)
        except InterpreterNotFoundError:
            pass

    @property
    def id(self):
        return self._id

    def is_running(self):
        """Return whether or not the identified interpreter is running."""
        return _interpreters.is_running(self._id)

    def close(self):
        """Finalize and destroy the interpreter.

        Attempting to destroy the current interpreter results
        in a RuntimeError.
        """
        return _interpreters.destroy(self._id)

    def exec_sync(self, code, /, channels=None):
        """Run the given source code in the interpreter.

        This is essentially the same as calling the builtin "exec"
        with this interpreter, using the __dict__ of its __main__
        module as both globals and locals.

        There is no return value.

        If the code raises an unhandled exception then an ExecFailure
        is raised, which summarizes the unhandled exception.  The actual
        exception is discarded because objects cannot be shared between
        interpreters.

        This blocks the current Python thread until done.  During
        that time, the previous interpreter is allowed to run
        in other threads.
        """
        excinfo = _interpreters.exec(self._id, code, channels)
        if excinfo is not None:
            raise ExecFailure(excinfo)

    def run(self, code, /, channels=None):
        def task():
            self.exec_sync(code, channels=channels)
        t = threading.Thread(target=task)
        t.start()
        return t
