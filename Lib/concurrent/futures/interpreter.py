
"""Implements InterpreterPoolExecutor."""

import concurrent.futures.thread as _thread
import pickle
from test.support import interpreters


class InterpreterPoolExecutor(_thread.ThreadPoolExecutor):

    @classmethod
    def _normalize_initializer(cls, initializer, initargs):
        if initializer is None:
            return initializer, initargs

        pickled = pickle.dumps((initializer, initargs))
        def initializer(ctx):
            interp, _ = ctx
            interp.exec(f'''if True:
                import pickle
                initializer, initargs = pickle.loads({pickled!r})
                initializer(*initargs)
                ''')
        return initializer, ()

    @classmethod
    def _normalize_task(cls, fn, args, kwargs):
        pickled = pickle.dumps((fn, args, kwargs))
        def wrapped(ctx):
            interp, results = ctx
            interp.exec(f'''if True:
                import pickle
                fn, args, kwargs = pickle.loads({pickled!r})
                res = fn(*args, **kwargs)
                _results.put(res)
                ''')
            return results.get_nowait()
        return wrapped, (), {}

    @classmethod
    def _run_worker(cls, *args):
        interp = interpreters.create()
        interp.exec('import test.support.interpreters.queues')
        results = interpreters.create_queue()
        interp.prepare_main(_results=results)
        ctx = (interp, results)
        try:
            _thread._worker(ctx, *args)
        finally:
            interp.close()
