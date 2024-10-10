
"""Implements InterpreterPoolExecutor."""

import concurrent.futures.thread as _thread
import pickle
from test.support import interpreters


class InterpreterPoolExecutor(_thread.ThreadPoolExecutor):

    @classmethod
    def _normalize_initializer(cls, initializer, initargs):
        shared, initargs = initargs
        if initializer is None:
            if shared:
                def initializer(ctx, *ignored):
                    interp, _ = ctx
                    interp.prepare_main(**shared)
            return initializer, initargs

        pickled = pickle.dumps((initializer, initargs))
        def initializer(ctx):
            interp, _ = ctx
            if shared:
                interp.prepare_main(**shared)
            interp.exec(f'''if True:
                import pickle
                initializer, initargs = pickle.loads({pickled!r})
                initializer(*initargs)
                ''')
        return initializer, ()

    @classmethod
    def _normalize_task(cls, fn, args, kwargs):
        if isinstance(fn, str):
            if args or kwargs:
                raise ValueError(f'a script does not take args or kwargs, got {args!r} and {kwargs!r}')
            script = fn
            def wrapped(ctx):
                interp, _ = ctx
                interp.exec(script)
                return None
        else:
            pickled = pickle.dumps((fn, args, kwargs))
            def wrapped(ctx):
                interp, results = ctx
                interp.exec(f'''if True:
                    import pickle
                    fn, args, kwargs = pickle.loads({pickled!r})
                    res = fn(*args, **kwargs)
                    _interp_pool_executor_results.put(res)
                    ''')
                return results.get_nowait()
        return wrapped, (), {}

    @classmethod
    def _run_worker(cls, *args):
        interp = interpreters.create()
        interp.exec('import test.support.interpreters.queues')
        results = interpreters.create_queue()
        interp.prepare_main(_interp_pool_executor_results=results)
        ctx = (interp, results)
        try:
            _thread._worker(ctx, *args)
        finally:
            interp.close()

    def __init__(self, max_workers=None, thread_name_prefix='',
                 initializer=None, initargs=(), shared=None):
        initargs = (shared, initargs)
        super().__init__(max_workers, thread_name_prefix, initializer, initargs)
