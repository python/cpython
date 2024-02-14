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
    'InterpreterError', 'InterpreterNotFoundError', 'ExecFailure',
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

class ExecFailure(RuntimeError):

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

    def prepare_main(self, ns=None, /, **kwargs):
        """Bind the given values into the interpreter's __main__.

        The values must be shareable.
        """
        ns = dict(ns, **kwargs) if ns is not None else kwargs
        _interpreters.set___main___attrs(self._id, ns)

    def exec(self, code, /):
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
        excinfo = _interpreters.exec(self._id, code)
        if excinfo is not None:
            raise ExecFailure(excinfo)

    def call(self, callable, /, args=None, kwargs=None):
        """Call the object in the interpreter with given args/kwargs.

        Return the function's return value.  If it raises an exception,
        raise it in the calling interpreter.  This contrasts with
        Interpreter.exec(), which discards the return value and only
        propagates the exception as ExecFailure.

        Unlike Interpreter.exec() and prepare_main(), all objects are
        supported, at the expense of some performance.
        """
        pickled_callable = pickle.dumps(callable)
        pickled_args = pickle.dumps(args)
        pickled_kwargs = pickle.dumps(kwargs)

        results = create_queue(sharedonly=False)
        self.prepare_main(_call_results=results)
        self.exec(f"""
            def _call_impl():
                try:
                    import pickle
                    callable = pickle.loads({pickled_callable!r})
                    if {pickled_args!r} is None:
                        args = ()
                    else:
                        args = pickle.loads({pickled_args!r})
                    if {pickled_kwargs!r} is None:
                        kwargs = {}
                    else:
                        kwargs = pickle.loads({pickled_kwargs!r})

                    res = callable(*args, **kwargs)
                except Exception as exc:
                    res = pickle.dumps((None, exc))
                else:
                    res = pickle.dumps((res, None))
                _call_results.put(res)
            _call_impl()
            del _call_impl
            del _call_results
            """)
        res, exc = results.get()
        if exc is None:
            raise exc
        else:
            return res

    def call_in_thread(self, callable, /, args=None, kwargs=None):
        """Return a new thread that calls the object in the interpreter.

        The return value and any raised exception are discarded.
        """
        def task():
            self.call(callable, args, kwargs)
        t = threading.Thread(target=task)
        t.start()
        return t
