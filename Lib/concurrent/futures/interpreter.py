"""Implements InterpreterPoolExecutor."""

import contextlib
import pickle
import textwrap
from . import thread as _thread
import _interpreters
import _interpqueues


class ExecutionFailed(_interpreters.InterpreterError):
    """An unhandled exception happened during execution."""

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
            return textwrap.dedent(f"""
{super().__str__()}

Uncaught in the interpreter:

{formatted}
                """.strip())


class WorkerContext(_thread.WorkerContext):

    @classmethod
    def prepare(cls, initializer, initargs, shared):
        def resolve_task(fn, args, kwargs):
            if isinstance(fn, str):
                # XXX Circle back to this later.
                raise TypeError('scripts not supported')
            else:
                task = (fn, args, kwargs)
            return task

        if initializer is not None:
            try:
                initdata = resolve_task(initializer, initargs, {})
            except ValueError:
                if isinstance(initializer, str) and initargs:
                    raise ValueError(f'an initializer script does not take args, got {initargs!r}')
                raise  # re-raise
        else:
            initdata = None
        def create_context():
            return cls(initdata, shared)
        return create_context, resolve_task

    def __init__(self, initdata, shared=None):
        self.initdata = initdata
        self.shared = dict(shared) if shared else None
        self.interpid = None
        self.resultsid = None

    def __del__(self):
        if self.interpid is not None:
            self.finalize()

    def _call(self, fn, args, kwargs):
        def do_call(resultsid, func, *args, **kwargs):
            try:
                return func(*args, **kwargs)
            except BaseException as exc:
                # Avoid relying on globals.
                import _interpreters
                import _interpqueues
                # Send the captured exception out on the results queue,
                # but still leave it unhandled for the interpreter to handle.
                try:
                    _interpqueues.put(resultsid, exc)
                except _interpreters.NotShareableError:
                    # The exception is not shareable.
                    import sys
                    import traceback
                    print('exception is not shareable:', file=sys.stderr)
                    traceback.print_exception(exc)
                    _interpqueues.put(resultsid, None)
                raise  # re-raise

        args = (self.resultsid, fn, *args)
        res, excinfo = _interpreters.call(self.interpid, do_call, args, kwargs)
        if excinfo is not None:
            raise ExecutionFailed(excinfo)
        return res

    def _get_exception(self):
        # Wait for the exception data to show up.
        while True:
            try:
                excdata = _interpqueues.get(self.resultsid)
            except _interpqueues.QueueNotFoundError:
                raise  # re-raise
            except _interpqueues.QueueError as exc:
                if exc.__cause__ is not None or exc.__context__ is not None:
                    raise  # re-raise
                if str(exc).endswith(' is empty'):
                    continue
                else:
                    raise  # re-raise
            except ModuleNotFoundError:
                # interpreters.queues doesn't exist, which means
                # QueueEmpty doesn't.  Act as though it does.
                continue
            else:
                break
        exc, unboundop = excdata
        assert unboundop is None, unboundop
        return exc

    def initialize(self):
        assert self.interpid is None, self.interpid
        self.interpid = _interpreters.create(reqrefs=True)
        try:
            _interpreters.incref(self.interpid)

            maxsize = 0
            self.resultsid = _interpqueues.create(maxsize)

            if self.shared:
                _interpreters.set___main___attrs(
                                    self.interpid, self.shared, restrict=True)

            if self.initdata:
                self.run(self.initdata)
        except BaseException:
            self.finalize()
            raise  # re-raise

    def finalize(self):
        interpid = self.interpid
        resultsid = self.resultsid
        self.resultsid = None
        self.interpid = None
        if resultsid is not None:
            try:
                _interpqueues.destroy(resultsid)
            except _interpqueues.QueueNotFoundError:
                pass
        if interpid is not None:
            try:
                _interpreters.decref(interpid)
            except _interpreters.InterpreterNotFoundError:
                pass

    def run(self, task):
        fn, args, kwargs = task
        try:
            return self._call(fn, args, kwargs)
        except ExecutionFailed as wrapper:
            exc = self._get_exception()
            if exc is None:
                # The exception must have been not shareable.
                raise  # re-raise
            raise exc from wrapper


class BrokenInterpreterPool(_thread.BrokenThreadPool):
    """
    Raised when a worker thread in an InterpreterPoolExecutor failed initializing.
    """


class InterpreterPoolExecutor(_thread.ThreadPoolExecutor):

    BROKEN = BrokenInterpreterPool

    @classmethod
    def prepare_context(cls, initializer, initargs, shared):
        return WorkerContext.prepare(initializer, initargs, shared)

    def __init__(self, max_workers=None, thread_name_prefix='',
                 initializer=None, initargs=(), shared=None):
        """Initializes a new InterpreterPoolExecutor instance.

        Args:
            max_workers: The maximum number of interpreters that can be used to
                execute the given calls.
            thread_name_prefix: An optional name prefix to give our threads.
            initializer: A callable or script used to initialize
                each worker interpreter.
            initargs: A tuple of arguments to pass to the initializer.
            shared: A mapping of shareabled objects to be inserted into
                each worker interpreter.
        """
        super().__init__(max_workers, thread_name_prefix,
                         initializer, initargs, shared=shared)
