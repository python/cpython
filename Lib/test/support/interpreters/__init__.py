"""Subinterpreters High Level Module."""

import threading
import weakref
import _xxsubinterpreters as _interpreters

# aliases:
from _xxsubinterpreters import (
    InterpreterError, InterpreterNotFoundError, NotShareableError,
    is_shareable,
)


__all__ = [
    'get_current', 'get_main', 'create', 'list_all', 'is_shareable',
    'Interpreter',
    'InterpreterError', 'InterpreterNotFoundError', 'ExecutionFailed',
    'NotShareableError',
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


_EXEC_FAILURE_STR = """
{superstr}

Uncaught in the interpreter:

{formatted}
""".strip()

class ExecutionFailed(InterpreterError):
    """An unhandled exception happened during execution.

    This is raised from Interpreter.exec() and Interpreter.call().
    """

    def __init__(self, excinfo):
        msg = excinfo.formatted
        if not msg:
            if excinfo.type and excinfo.msg:
                msg = f'{excinfo.type.__name__}: {excinfo.msg}'
            else:
                msg = excinfo.type.__name__ or excinfo.msg
        super().__init__(msg)
        self.excinfo = excinfo

    def __str__(self):
        try:
            formatted = self.excinfo.errdisplay
        except Exception:
            return super().__str__()
        else:
            return _EXEC_FAILURE_STR.format(
                superstr=super().__str__(),
                formatted=formatted,
            )


def create():
    """Return a new (idle) Python interpreter."""
    id = _interpreters.create(reqrefs=True)
    return Interpreter(id, _owned=True)


def list_all():
    """Return all existing interpreters."""
    return [Interpreter(id, _owned=owned)
            for id, owned in _interpreters.list_all()]


def get_current():
    """Return the currently running interpreter."""
    id, owned = _interpreters.get_current()
    return Interpreter(id, _owned=owned)


def get_main():
    """Return the main interpreter."""
    id, owned = _interpreters.get_main()
    assert owned is False
    return Interpreter(id, _owned=owned)


_known = weakref.WeakValueDictionary()

class Interpreter:
    """A single Python interpreter.

    Attributes:

    "id" - the unique process-global ID number for the interpreter
    "owned" - indicates whether or not the interpreter was created
              by interpreters.create()

    If interp.owned is false then any method that modifies the
    interpreter will fail, i.e. .close(), .prepare_main(), .exec(),
    and .call()
    """

    def __new__(cls, id, /, _owned=None):
        # There is only one instance for any given ID.
        if not isinstance(id, int):
            raise TypeError(f'id must be an int, got {id!r}')
        id = int(id)
        if _owned is None:
            _owned = _interpreters.is_owned(id)
        try:
            self = _known[id]
            assert hasattr(self, '_ownsref')
        except KeyError:
            self = super().__new__(cls)
            _known[id] = self
            self._id = id
            self._owned = _owned
            self._ownsref = _owned
            if _owned:
                # This may raise InterpreterNotFoundError:
                _interpreters.incref(id)
        return self

    def __repr__(self):
        return f'{type(self).__name__}({self.id})'

    def __hash__(self):
        return hash(self._id)

    def __del__(self):
        self._decref()

    # for pickling:
    def __getnewargs__(self):
        return (self._id,)

    # for pickling:
    def __getstate__(self):
        return None

    def _decref(self):
        if not self._ownsref:
            return
        self._ownsref = False
        try:
            _interpreters.decref(self._id)
        except InterpreterNotFoundError:
            pass

    @property
    def id(self):
        return self._id

    @property
    def owned(self):
        return self._owned

    def is_running(self):
        """Return whether or not the identified interpreter is running."""
        # require_owned is okay since this doesn't modify the interpreter.
        return _interpreters.is_running(self._id, require_owned=False)

    # Everything past here is available only to interpreters created by
    # interpreters.create().

    def close(self):
        """Finalize and destroy the interpreter.

        Attempting to destroy the current interpreter results
        in an InterpreterError.
        """
        return _interpreters.destroy(self._id, require_owned=True)

    def prepare_main(self, ns=None, /, **kwargs):
        """Bind the given values into the interpreter's __main__.

        The values must be shareable.
        """
        ns = dict(ns, **kwargs) if ns is not None else kwargs
        _interpreters.set___main___attrs(self._id, ns, require_owned=True)

    def exec(self, code, /):
        """Run the given source code in the interpreter.

        This is essentially the same as calling the builtin "exec"
        with this interpreter, using the __dict__ of its __main__
        module as both globals and locals.

        There is no return value.

        If the code raises an unhandled exception then an ExecutionFailed
        exception is raised, which summarizes the unhandled exception.
        The actual exception is discarded because objects cannot be
        shared between interpreters.

        This blocks the current Python thread until done.  During
        that time, the previous interpreter is allowed to run
        in other threads.
        """
        excinfo = _interpreters.exec(self._id, code, require_owned=True)
        if excinfo is not None:
            raise ExecutionFailed(excinfo)

    def call(self, callable, /):
        """Call the object in the interpreter with given args/kwargs.

        Only functions that take no arguments and have no closure
        are supported.

        The return value is discarded.

        If the callable raises an exception then the error display
        (including full traceback) is send back between the interpreters
        and an ExecutionFailed exception is raised, much like what
        happens with Interpreter.exec().
        """
        # XXX Support args and kwargs.
        # XXX Support arbitrary callables.
        # XXX Support returning the return value (e.g. via pickle).
        excinfo = _interpreters.call(self._id, callable, require_owned=True)
        if excinfo is not None:
            raise ExecutionFailed(excinfo)

    def call_in_thread(self, callable, /):
        """Return a new thread that calls the object in the interpreter.

        The return value and any raised exception are discarded.
        """
        def task():
            self.call(callable)
        t = threading.Thread(target=task)
        t.start()
        return t
