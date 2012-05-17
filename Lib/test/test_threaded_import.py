# This is a variant of the very old (early 90's) file
# Demo/threads/bug.py.  It simply provokes a number of threads into
# trying to import the same module "at the same time".
# There are no pleasant failure modes -- most likely is that Python
# complains several times about module random having no attribute
# randrange, and then Python hangs.

import os
import imp
import sys
import time
import shutil
import unittest
from test.support import (
    verbose, import_module, run_unittest, TESTFN, reap_threads, forget, unlink)
threading = import_module('threading')

def task(N, done, done_tasks, errors):
    try:
        # We don't use modulefinder but still import it in order to stress
        # importing of different modules from several threads.
        if len(done_tasks) % 2:
            import modulefinder
            import random
        else:
            import random
            import modulefinder
        # This will fail if random is not completely initialized
        x = random.randrange(1, 3)
    except Exception as e:
        errors.append(e.with_traceback(None))
    finally:
        done_tasks.append(threading.get_ident())
        finished = len(done_tasks) == N
        if finished:
            done.set()

# Create a circular import structure: A -> C -> B -> D -> A
# NOTE: `time` is already loaded and therefore doesn't threaten to deadlock.

circular_imports_modules = {
    'A': """if 1:
        import time
        time.sleep(%(delay)s)
        x = 'a'
        import C
        """,
    'B': """if 1:
        import time
        time.sleep(%(delay)s)
        x = 'b'
        import D
        """,
    'C': """import B""",
    'D': """import A""",
}

class Finder:
    """A dummy finder to detect concurrent access to its find_module()
    method."""

    def __init__(self):
        self.numcalls = 0
        self.x = 0
        self.lock = threading.Lock()

    def find_module(self, name, path=None):
        # Simulate some thread-unsafe behaviour. If calls to find_module()
        # are properly serialized, `x` will end up the same as `numcalls`.
        # Otherwise not.
        with self.lock:
            self.numcalls += 1
        x = self.x
        time.sleep(0.1)
        self.x = x + 1

class FlushingFinder:
    """A dummy finder which flushes sys.path_importer_cache when it gets
    called."""

    def find_module(self, name, path=None):
        sys.path_importer_cache.clear()


class ThreadedImportTests(unittest.TestCase):

    def setUp(self):
        self.old_random = sys.modules.pop('random', None)

    def tearDown(self):
        # If the `random` module was already initialized, we restore the
        # old module at the end so that pickling tests don't fail.
        # See http://bugs.python.org/issue3657#msg110461
        if self.old_random is not None:
            sys.modules['random'] = self.old_random

    def check_parallel_module_init(self):
        if imp.lock_held():
            # This triggers on, e.g., from test import autotest.
            raise unittest.SkipTest("can't run when import lock is held")

        done = threading.Event()
        for N in (20, 50) * 3:
            if verbose:
                print("Trying", N, "threads ...", end=' ')
            # Make sure that random and modulefinder get reimported freshly
            for modname in ['random', 'modulefinder']:
                try:
                    del sys.modules[modname]
                except KeyError:
                    pass
            errors = []
            done_tasks = []
            done.clear()
            for i in range(N):
                t = threading.Thread(target=task,
                                     args=(N, done, done_tasks, errors,))
                t.start()
            done.wait(60)
            self.assertFalse(errors)
            if verbose:
                print("OK.")

    def test_parallel_module_init(self):
        self.check_parallel_module_init()

    def test_parallel_meta_path(self):
        finder = Finder()
        sys.meta_path.insert(0, finder)
        try:
            self.check_parallel_module_init()
            self.assertGreater(finder.numcalls, 0)
            self.assertEqual(finder.x, finder.numcalls)
        finally:
            sys.meta_path.remove(finder)

    def test_parallel_path_hooks(self):
        # Here the Finder instance is only used to check concurrent calls
        # to path_hook().
        finder = Finder()
        # In order for our path hook to be called at each import, we need
        # to flush the path_importer_cache, which we do by registering a
        # dedicated meta_path entry.
        flushing_finder = FlushingFinder()
        def path_hook(path):
            finder.find_module('')
            raise ImportError
        sys.path_hooks.insert(0, path_hook)
        sys.meta_path.append(flushing_finder)
        try:
            # Flush the cache a first time
            flushing_finder.find_module('')
            numtests = self.check_parallel_module_init()
            self.assertGreater(finder.numcalls, 0)
            self.assertEqual(finder.x, finder.numcalls)
        finally:
            sys.meta_path.remove(flushing_finder)
            sys.path_hooks.remove(path_hook)

    def test_import_hangers(self):
        # In case this test is run again, make sure the helper module
        # gets loaded from scratch again.
        try:
            del sys.modules['test.threaded_import_hangers']
        except KeyError:
            pass
        import test.threaded_import_hangers
        self.assertFalse(test.threaded_import_hangers.errors)

    def test_circular_imports(self):
        # The goal of this test is to exercise implementations of the import
        # lock which use a per-module lock, rather than a global lock.
        # In these implementations, there is a possible deadlock with
        # circular imports, for example:
        # - thread 1 imports A (grabbing the lock for A) which imports B
        # - thread 2 imports B (grabbing the lock for B) which imports A
        # Such implementations should be able to detect such situations and
        # resolve them one way or the other, without freezing.
        # NOTE: our test constructs a slightly less trivial import cycle,
        # in order to better stress the deadlock avoidance mechanism.
        delay = 0.5
        os.mkdir(TESTFN)
        self.addCleanup(shutil.rmtree, TESTFN)
        sys.path.insert(0, TESTFN)
        self.addCleanup(sys.path.remove, TESTFN)
        for name, contents in circular_imports_modules.items():
            contents = contents % {'delay': delay}
            with open(os.path.join(TESTFN, name + ".py"), "wb") as f:
                f.write(contents.encode('utf-8'))
            self.addCleanup(forget, name)

        results = []
        def import_ab():
            import A
            results.append(getattr(A, 'x', None))
        def import_ba():
            import B
            results.append(getattr(B, 'x', None))
        t1 = threading.Thread(target=import_ab)
        t2 = threading.Thread(target=import_ba)
        t1.start()
        t2.start()
        t1.join()
        t2.join()
        self.assertEqual(set(results), {'a', 'b'})

    def test_side_effect_import(self):
        code = """if 1:
            import threading
            def target():
                import random
            t = threading.Thread(target=target)
            t.start()
            t.join()"""
        sys.path.insert(0, os.curdir)
        self.addCleanup(sys.path.remove, os.curdir)
        filename = TESTFN + ".py"
        with open(filename, "wb") as f:
            f.write(code.encode('utf-8'))
        self.addCleanup(unlink, filename)
        self.addCleanup(forget, TESTFN)
        __import__(TESTFN)


@reap_threads
def test_main():
    run_unittest(ThreadedImportTests)

if __name__ == "__main__":
    test_main()
