"""Implements InterpreterPoolExecutor."""

import pickle
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
        if isinstance(initializer, str):
            if initargs:
                raise ValueError(f'an initializer script does not take args, got {initargs!r}')
            initscript = initializer
            # Make sure the script compiles.
            # XXX Keep the compiled code object?
            compile(initscript, '<string>', 'exec')
        elif initializer is not None:
            pickled = pickle.dumps((initializer, initargs))
            initscript = f'''if True:
                import pickle
                initializer, initargs = pickle.loads({pickled!r})
                initializer(*initargs)
                '''
        else:
            initscript = None
        def create_context():
            return cls(initscript, shared)
        def resolve_task(cls, fn, args, kwargs):
            if isinstance(fn, str):
                if args or kwargs:
                    raise ValueError(f'a script does not take args or kwargs, got {args!r} and {kwargs!r}')
                data = fn
                kind = 'script'
            else:
                data = pickle.dumps((fn, args, kwargs))
                kind = 'function'
            return (data, kind)
        return create_context, resolve_task

    @classmethod
    def _run_pickled_func(cls, data, resultsid):
        fn, args, kwargs = pickle.loads(data)
        res = fn(*args, **kwargs)
        # Send the result back.
        try:
            _interpqueues.put(resultsid, res, 0, UNBOUND)
        except _interpreters.NotShareableError:
            res = pickle.dumps(res)
            _interpqueues.put(resultsid, res, 1, UNBOUND)

    def __init__(self, initscript, shared=None):
        self.initscript = initscript or ''
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

            initscript = f"""if True:
                from {__name__} import WorkerContext
                """
            initscript += LINESEP + self.initscript
            self._exec(initscript)

            if self.shared:
                _interpreters.set___main___attrs(
                                    self.interpid, self.shared, restrict=True)

            maxsize = 0
            fmt = 0
            self.resultsid = _interpqueues.create(maxsize, fmt, UNBOUND)
        except _interpreters.InterpreterNotFoundError:
            raise  # re-raise
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
            self._exec(data)
            return None
        elif kind == 'function':
            self._exec(
                f'WorkerContext._run_pickled_func({data!r}, {self.resultsid})')
            obj, pickled, unboundop = _interpqueues.get(self.resultsid)
            assert unboundop is None, unboundop
            return pickle.loads(obj) if pickled else obj
        else:
            raise NotImplementedError(kind)


class InterpreterPoolExecutor(_thread.ThreadPoolExecutor):

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
