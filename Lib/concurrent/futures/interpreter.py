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


UNBOUND = 2  # error; this should not happen.


class WorkerContext(_thread.WorkerContext):

    @classmethod
    def prepare(cls, initializer, initargs, shared):
        def resolve_task(fn, args, kwargs):
            if isinstance(fn, str):
                # XXX Circle back to this later.
                raise TypeError('scripts not supported')
                if args or kwargs:
                    raise ValueError(f'a script does not take args or kwargs, got {args!r} and {kwargs!r}')
                data = textwrap.dedent(fn)
                kind = 'script'
                # Make sure the script compiles.
                # Ideally we wouldn't throw away the resulting code
                # object.  However, there isn't much to be done until
                # code objects are shareable and/or we do a better job
                # of supporting code objects in _interpreters.exec().
                compile(data, '<string>', 'exec')
            else:
                # Functions defined in the __main__ module can't be pickled,
                # so they can't be used here.  In the future, we could possibly
                # borrow from multiprocessing to work around this.
                data = pickle.dumps((fn, args, kwargs))
                kind = 'function'
            return (data, kind)

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

    @classmethod
    @contextlib.contextmanager
    def _capture_exc(cls, resultsid):
        try:
            yield
        except BaseException as exc:
            # Send the captured exception out on the results queue,
            # but still leave it unhandled for the interpreter to handle.
            err = pickle.dumps(exc)
            _interpqueues.put(resultsid, (None, err), 1, UNBOUND)
            raise  # re-raise

    @classmethod
    def _send_script_result(cls, resultsid):
        _interpqueues.put(resultsid, (None, None), 0, UNBOUND)

    @classmethod
    def _call(cls, func, args, kwargs, resultsid):
        with cls._capture_exc(resultsid):
            res = func(*args or (), **kwargs or {})
        # Send the result back.
        try:
            _interpqueues.put(resultsid, (res, None), 0, UNBOUND)
        except _interpreters.NotShareableError:
            res = pickle.dumps(res)
            _interpqueues.put(resultsid, (res, None), 1, UNBOUND)

    @classmethod
    def _call_pickled(cls, pickled, resultsid):
        with cls._capture_exc(resultsid):
            fn, args, kwargs = pickle.loads(pickled)
        cls._call(fn, args, kwargs, resultsid)

    def __init__(self, initdata, shared=None):
        self.initdata = initdata
        self.shared = dict(shared) if shared else None
        self.interpid = None
        self.resultsid = None

    def __del__(self):
        if self.interpid is not None:
            self.finalize()

    def _exec(self, script):
        assert self.interpid is not None
        excinfo = _interpreters.exec(self.interpid, script, restrict=True)
        if excinfo is not None:
            raise ExecutionFailed(excinfo)

    def initialize(self):
        assert self.interpid is None, self.interpid
        self.interpid = _interpreters.create(reqrefs=True)
        try:
            _interpreters.incref(self.interpid)

            maxsize = 0
            fmt = 0
            self.resultsid = _interpqueues.create(maxsize, fmt, UNBOUND)

            self._exec(f'from {__name__} import WorkerContext')

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
        data, kind = task
        if kind == 'script':
            raise NotImplementedError('script kind disabled')
            script = f"""
with WorkerContext._capture_exc({self.resultsid}):
{textwrap.indent(data, '    ')}
WorkerContext._send_script_result({self.resultsid})"""
        elif kind == 'function':
            script = f'WorkerContext._call_pickled({data!r}, {self.resultsid})'
        else:
            raise NotImplementedError(kind)

        try:
            self._exec(script)
        except ExecutionFailed as exc:
            exc_wrapper = exc
        else:
            exc_wrapper = None

        # Return the result, or raise the exception.
        while True:
            try:
                obj = _interpqueues.get(self.resultsid)
            except _interpqueues.QueueNotFoundError:
                raise  # re-raise
            except _interpqueues.QueueError:
                continue
            except ModuleNotFoundError:
                # interpreters.queues doesn't exist, which means
                # QueueEmpty doesn't.  Act as though it does.
                continue
            else:
                break
        (res, excdata), pickled, unboundop = obj
        assert unboundop is None, unboundop
        if excdata is not None:
            assert res is None, res
            assert pickled
            assert exc_wrapper is not None
            exc = pickle.loads(excdata)
            raise exc from exc_wrapper
        return pickle.loads(res) if pickled else res


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
