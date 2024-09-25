import unittest
from concurrent.futures.interpreter import ExecutionFailed
from test import support
from test.support.interpreters import queues

from .executor import ExecutorTest, mul
from .util import BaseTestCase, InterpreterPoolMixin, setup_module


class InterpreterPoolExecutorTest(InterpreterPoolMixin, ExecutorTest, BaseTestCase):

    def assertTaskRaises(self, exctype):
        return self.assertRaisesRegex(ExecutionFailed, exctype.__name__)

    def test_saturation(self):
        blocker = queues.create()
        executor = self.executor_type(4, shared=dict(blocker=blocker))

        for i in range(15 * executor._max_workers):
            executor.submit('blocker.get()')
        self.assertEqual(len(executor._threads), executor._max_workers)
        for i in range(15 * executor._max_workers):
            blocker.put_nowait(None)
        executor.shutdown(wait=True)

    @support.requires_gil_enabled("gh-117344: test is flaky without the GIL")
    def test_idle_thread_reuse(self):
        executor = self.executor_type()
        executor.submit(mul, 21, 2).result()
        executor.submit(mul, 6, 7).result()
        executor.submit(mul, 3, 14).result()
        self.assertEqual(len(executor._threads), 1)
        executor.shutdown(wait=True)

#    def test_executor_map_current_future_cancel(self):
#        blocker = queues.create()
#        log = queues.create()
#
#        script = """if True:
#            def log_n_wait({ident}):
#                blocker(f"ident {ident} started")
#                try:
#                    stop_event.wait()
#                finally:
#                    log.append(f"ident {ident} stopped")
#            """
#
#        with self.executor_type(max_workers=1) as pool:
#            # submit work to saturate the pool
#            fut = pool.submit(script.format(ident="first"))
#            gen = pool.map(log_n_wait, ["second", "third"], timeout=0)
#            try:
#                with self.assertRaises(TimeoutError):
#                    next(gen)
#            finally:
#                gen.close()
#                blocker.put
#                stop_event.set()
#            fut.result()
#        # ident='second' is cancelled as a result of raising a TimeoutError
#        # ident='third' is cancelled because it remained in the collection of futures
#        self.assertListEqual(log, ["ident='first' started", "ident='first' stopped"])


def setUpModule():
    setup_module()


if __name__ == "__main__":
    unittest.main()
