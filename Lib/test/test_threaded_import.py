# This is a variant of the very old (early 90's) file
# Demo/threads/bug.py.  It simply provokes a number of threads into
# trying to import the same module "at the same time".
# There are no pleasant failure modes -- most likely is that Python
# complains several times about module random having no attribute
# randrange, and then Python hangs.

import imp
import sys
import unittest
from test.support import verbose, TestFailed, import_module, run_unittest
thread = import_module('_thread')

def task(N, done, done_tasks, errors):
    try:
        import random
        # This will fail if random is not completely initialized
        x = random.randrange(1, 3)
    except Exception as e:
        errors.append(e.with_traceback(None))
    finally:
        done_tasks.append(thread.get_ident())
        finished = len(done_tasks) == N
        if finished:
            done.release()


class ThreadedImportTests(unittest.TestCase):

    def setUp(self):
        self.old_random = sys.modules.pop('random', None)

    def tearDown(self):
        # If the `random` module was already initialized, we restore the
        # old module at the end so that pickling tests don't fail.
        # See http://bugs.python.org/issue3657#msg110461
        if self.old_random is not None:
            sys.modules['random'] = self.old_random

    def test_parallel_module_init(self):
        if imp.lock_held():
            # This triggers on, e.g., from test import autotest.
            raise unittest.SkipTest("can't run when import lock is held")

        done = thread.allocate_lock()
        done.acquire()
        for N in (20, 50) * 3:
            if verbose:
                print("Trying", N, "threads ...", end=' ')
            # Make sure that random gets reimported freshly
            try:
                del sys.modules['random']
            except KeyError:
                pass
            errors = []
            done_tasks = []
            for i in range(N):
                thread.start_new_thread(task, (N, done, done_tasks, errors,))
            done.acquire()
            self.assertFalse(errors)
            if verbose:
                print("OK.")
        done.release()

    def test_import_hangers(self):
        # In case this test is run again, make sure the helper module
        # gets loaded from scratch again.
        try:
            del sys.modules['test.threaded_import_hangers']
        except KeyError:
            pass
        import test.threaded_import_hangers
        self.assertFalse(test.threaded_import_hangers.errors)


def test_main():
    run_unittest(ThreadedImportTests)


if __name__ == "__main__":
    test_main()
