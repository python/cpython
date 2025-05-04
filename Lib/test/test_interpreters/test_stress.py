import threading
import unittest

from test import support
from test.support import import_helper
from test.support import threading_helper
# Raise SkipTest if subinterpreters not supported.
import_helper.import_module('_interpreters')
from test.support import interpreters
from .utils import TestBase


class StressTests(TestBase):

    # In these tests we generally want a lot of interpreters,
    # but not so many that any test takes too long.

    @support.requires_resource('cpu')
    def test_create_many_sequential(self):
        alive = []
        for _ in range(100):
            interp = interpreters.create()
            alive.append(interp)
        del alive
        support.gc_collect()

    @support.bigmemtest(size=200, memuse=32*2**20, dry_run=False)
    def test_create_many_threaded(self, size):
        alive = []
        start = threading.Event()
        def task():
            # try to create all interpreters simultaneously
            if not start.wait(support.SHORT_TIMEOUT):
                raise TimeoutError
            interp = interpreters.create()
            alive.append(interp)
        threads = [threading.Thread(target=task) for _ in range(size)]
        with threading_helper.start_threads(threads):
            start.set()
        del alive
        support.gc_collect()

    @threading_helper.requires_working_threading()
    @support.bigmemtest(size=200, memuse=34*2**20, dry_run=False)
    def test_many_threads_running_interp_in_other_interp(self, size):
        start = threading.Event()
        interp = interpreters.create()

        script = f"""if True:
            import _interpreters
            _interpreters.run_string({interp.id}, '1')
            """

        def run():
            interp = interpreters.create()
            alreadyrunning = (f'{interpreters.InterpreterError}: '
                              'interpreter already running')
            # try to run all interpreters simultaneously
            if not start.wait(support.SHORT_TIMEOUT):
                raise TimeoutError
            success = False
            while not success:
                try:
                    interp.exec(script)
                except interpreters.ExecutionFailed as exc:
                    if exc.excinfo.msg != 'interpreter already running':
                        raise  # re-raise
                    assert exc.excinfo.type.__name__ == 'InterpreterError'
                else:
                    success = True

        threads = [threading.Thread(target=run) for _ in range(size)]
        with threading_helper.start_threads(threads):
            start.set()
        support.gc_collect()


if __name__ == '__main__':
    # Test needs to be a package, so we can do relative imports.
    unittest.main()
