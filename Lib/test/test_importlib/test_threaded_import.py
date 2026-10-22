# This is a variant of the very old (early 90's) file
# Demo/threads/bug.py.  It simply provokes a number of threads into
# trying to import the same module "at the same time".
# There are no pleasant failure modes -- most likely is that Python
# complains several times about module random having no attribute
# randrange, and then Python hangs.

import _imp as imp
import os
import importlib
import sys
import time
import shutil
import threading
import unittest
from test import support
from test.support import verbose
from test.support.import_helper import forget, mock_register_at_fork
from test.support.os_helper import (TESTFN, unlink, rmtree)
from test.support import script_helper, threading_helper

threading_helper.requires_working_threading(module=True)

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
    """A dummy finder to detect concurrent access to its find_spec()
    method."""

    def __init__(self):
        self.numcalls = 0
        self.x = 0
        self.lock = threading.Lock()

    def find_spec(self, name, path=None, target=None):
        # Simulate some thread-unsafe behaviour. If calls to find_spec()
        # are properly serialized, `x` will end up the same as `numcalls`.
        # Otherwise not.
        assert imp.lock_held()
        with self.lock:
            self.numcalls += 1
        x = self.x
        time.sleep(0.01)
        self.x = x + 1

class FlushingFinder:
    """A dummy finder which flushes sys.path_importer_cache when it gets
    called."""

    def find_spec(self, name, path=None, target=None):
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

    @mock_register_at_fork
    def check_parallel_module_init(self, mock_os):
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
            t0 = time.monotonic()
            with threading_helper.start_threads(
                    threading.Thread(target=task, args=(N, done, done_tasks, errors,))
                    for i in range(N)):
                pass
            completed = done.wait(10 * 60)
            dt = time.monotonic() - t0
            if verbose:
                print("%.1f ms" % (dt*1e3), flush=True, end=" ")
            dbg_info = 'done: %s/%s' % (len(done_tasks), N)
            self.assertFalse(errors, dbg_info)
            self.assertTrue(completed, dbg_info)
            if verbose:
                print("OK.")

    @support.bigmemtest(size=50, memuse=76*2**20, dry_run=False)
    def test_parallel_module_init(self, size):
        self.check_parallel_module_init()

    @support.bigmemtest(size=50, memuse=76*2**20, dry_run=False)
    def test_parallel_meta_path(self, size):
        finder = Finder()
        sys.meta_path.insert(0, finder)
        try:
            self.check_parallel_module_init()
            self.assertGreater(finder.numcalls, 0)
            self.assertEqual(finder.x, finder.numcalls)
        finally:
            sys.meta_path.remove(finder)

    @support.bigmemtest(size=50, memuse=76*2**20, dry_run=False)
    def test_parallel_path_hooks(self, size):
        # Here the Finder instance is only used to check concurrent calls
        # to path_hook().
        finder = Finder()
        # In order for our path hook to be called at each import, we need
        # to flush the path_importer_cache, which we do by registering a
        # dedicated meta_path entry.
        flushing_finder = FlushingFinder()
        def path_hook(path):
            finder.find_spec('')
            raise ImportError
        sys.path_hooks.insert(0, path_hook)
        sys.meta_path.append(flushing_finder)
        try:
            # Flush the cache a first time
            flushing_finder.find_spec('')
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
            del sys.modules['test.test_importlib.threaded_import_hangers']
        except KeyError:
            pass
        import test.test_importlib.threaded_import_hangers
        self.assertFalse(test.test_importlib.threaded_import_hangers.errors)

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

        importlib.invalidate_caches()
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

    @mock_register_at_fork
    def test_side_effect_import(self, mock_os):
        code = """if 1:
            import threading
            def target():
                import random
            t = threading.Thread(target=target)
            t.start()
            t.join()
            t = None"""
        sys.path.insert(0, os.curdir)
        self.addCleanup(sys.path.remove, os.curdir)
        filename = TESTFN + ".py"
        with open(filename, "wb") as f:
            f.write(code.encode('utf-8'))
        self.addCleanup(unlink, filename)
        self.addCleanup(forget, TESTFN)
        self.addCleanup(rmtree, '__pycache__')
        importlib.invalidate_caches()
        with threading_helper.wait_threads_exit():
            __import__(TESTFN)
        del sys.modules[TESTFN]

    @support.bigmemtest(size=1, memuse=1.8*2**30, dry_run=False)
    def test_concurrent_futures_circular_import(self, size):
        # Regression test for bpo-43515
        fn = os.path.join(os.path.dirname(__file__),
                          'partial', 'cfimport.py')
        script_helper.assert_python_ok(fn)

    @support.bigmemtest(size=1, memuse=1.8*2**30, dry_run=False)
    def test_multiprocessing_pool_circular_import(self, size):
        # Regression test for bpo-41567
        fn = os.path.join(os.path.dirname(__file__),
                          'partial', 'pool_in_threads.py')
        script_helper.assert_python_ok(fn)

    def test_import_failure_race_condition(self):
        # Regression test for race condition where a thread could receive
        # a partially-initialized module when another thread's import fails.
        # The race occurs when:
        # 1. Thread 1 starts importing, adds module to sys.modules
        # 2. Thread 2 sees the module in sys.modules
        # 3. Thread 1's import fails, removes module from sys.modules
        # 4. Thread 2 should NOT return the stale module reference
        os.mkdir(TESTFN)
        self.addCleanup(shutil.rmtree, TESTFN)
        sys.path.insert(0, TESTFN)
        self.addCleanup(sys.path.remove, TESTFN)

        # Create a module that partially initializes then fails
        modname = 'failing_import_module'
        with open(os.path.join(TESTFN, modname + '.py'), 'w') as f:
            f.write('''
import time
PARTIAL_ATTR = 'initialized'
time.sleep(0.05)  # Widen race window
raise RuntimeError("Intentional import failure")
''')
        self.addCleanup(forget, modname)
        importlib.invalidate_caches()

        errors = []
        results = []

        def do_import(delay=0):
            time.sleep(delay)
            try:
                mod = __import__(modname)
                # If we got a module, verify it's in sys.modules
                if modname not in sys.modules:
                    errors.append(
                        f"Got module {mod!r} but {modname!r} not in sys.modules"
                    )
                elif sys.modules[modname] is not mod:
                    errors.append(
                        f"Got different module than sys.modules[{modname!r}]"
                    )
                else:
                    results.append(('success', mod))
            except RuntimeError:
                results.append(('RuntimeError',))
            except Exception as e:
                errors.append(f"Unexpected exception: {e}")

        # Run multiple iterations to increase chance of hitting the race
        for _ in range(10):
            errors.clear()
            results.clear()
            if modname in sys.modules:
                del sys.modules[modname]

            t1 = threading.Thread(target=do_import, args=(0,))
            t2 = threading.Thread(target=do_import, args=(0.01,))
            t1.start()
            t2.start()
            t1.join()
            t2.join()

            # Neither thread should have errors about stale modules
            self.assertEqual(errors, [], f"Race condition detected: {errors}")

    def test_hierarchical_import_deadlock(self):
        # Regression test for bpo-38884 / gh-83065
        # Tests that concurrent imports at different hierarchy levels
        # don't deadlock when parent imports child in __init__.py

        # Create package structure:
        # package/__init__.py: from package import subpackage
        # package/subpackage/__init__.py: from package.subpackage.module import *
        # package/subpackage/module.py: class SomeClass: pass

        pkg_dir = os.path.join(TESTFN, 'hier_deadlock_pkg')
        os.makedirs(pkg_dir)
        self.addCleanup(shutil.rmtree, TESTFN)

        subpkg_dir = os.path.join(pkg_dir, 'subpackage')
        os.makedirs(subpkg_dir)

        with open(os.path.join(pkg_dir, "__init__.py"), "w") as f:
            f.write("from hier_deadlock_pkg import subpackage\n")

        with open(os.path.join(subpkg_dir, "__init__.py"), "w") as f:
            f.write("from hier_deadlock_pkg.subpackage.module import *\n")

        with open(os.path.join(subpkg_dir, "module.py"), "w") as f:
            f.write("class SomeClass:\n    pass\n")

        sys.path.insert(0, TESTFN)
        self.addCleanup(sys.path.remove, TESTFN)
        self.addCleanup(forget, 'hier_deadlock_pkg')
        self.addCleanup(forget, 'hier_deadlock_pkg.subpackage')
        self.addCleanup(forget, 'hier_deadlock_pkg.subpackage.module')

        importlib.invalidate_caches()

        errors = []
        results = []
        barrier = threading.Barrier(2)

        def t1():
            barrier.wait()
            try:
                import hier_deadlock_pkg.subpackage
                results.append('t1_success')
            except Exception as e:
                errors.append(('t1', type(e).__name__, str(e)))

        def t2():
            barrier.wait()
            try:
                import hier_deadlock_pkg.subpackage.module
                results.append('t2_success')
            except Exception as e:
                errors.append(('t2', type(e).__name__, str(e)))

        # Run multiple times to increase chance of hitting race condition
        for i in range(10):
            for mod in ['hier_deadlock_pkg', 'hier_deadlock_pkg.subpackage',
                       'hier_deadlock_pkg.subpackage.module']:
                sys.modules.pop(mod, None)

            errors.clear()
            results.clear()
            barrier.reset()

            thread1 = threading.Thread(target=t1)
            thread2 = threading.Thread(target=t2)

            thread1.start()
            thread2.start()

            thread1.join(timeout=5)
            thread2.join(timeout=5)

            if thread1.is_alive() or thread2.is_alive():
                self.fail(f"Threads deadlocked on iteration {i}")

            self.assertEqual(
                errors, [],
                f"Import(s) failed on iteration {i}: {errors}")
            self.assertEqual(
                sorted(results), ['t1_success', 't2_success'],
                f"Not all imports succeeded on iteration {i}: {results}")

    def test_cross_package_circular_import(self):
        # Two packages whose __init__.py each import a submodule of the
        # other. Concurrent imports of submodules of each must not raise
        # _DeadlockError; the import system accepts a partially-initialised
        # parent in this case (see _lock_unlock_module).
        os.makedirs(os.path.join(TESTFN, "circ_a"))
        os.makedirs(os.path.join(TESTFN, "circ_b"))
        self.addCleanup(shutil.rmtree, TESTFN)
        with open(os.path.join(TESTFN, "circ_a", "__init__.py"), "w") as f:
            f.write("import time; time.sleep(0.03)\nimport circ_b.other\n")
        with open(os.path.join(TESTFN, "circ_b", "__init__.py"), "w") as f:
            f.write("import time; time.sleep(0.03)\nimport circ_a.other\n")
        for pkg in ("circ_a", "circ_b"):
            for mod in ("sub.py", "other.py"):
                with open(os.path.join(TESTFN, pkg, mod), "w") as f:
                    f.write("X = 1\n")

        sys.path.insert(0, TESTFN)
        self.addCleanup(sys.path.remove, TESTFN)
        for mod in ("circ_a", "circ_a.sub", "circ_a.other",
                    "circ_b", "circ_b.sub", "circ_b.other"):
            self.addCleanup(forget, mod)
        importlib.invalidate_caches()

        errors = []
        barrier = threading.Barrier(2)

        def do_import(name):
            barrier.wait()
            try:
                importlib.import_module(name)
            except Exception as e:
                errors.append((name, type(e).__name__, str(e)))

        for i in range(10):
            for mod in ("circ_a", "circ_a.sub", "circ_a.other",
                        "circ_b", "circ_b.sub", "circ_b.other"):
                sys.modules.pop(mod, None)
            errors.clear()
            barrier.reset()

            thread1 = threading.Thread(target=do_import, args=("circ_a.sub",))
            thread2 = threading.Thread(target=do_import, args=("circ_b.sub",))
            thread1.start()
            thread2.start()
            thread1.join(timeout=5)
            thread2.join(timeout=5)

            if thread1.is_alive() or thread2.is_alive():
                self.fail(f"Threads deadlocked on iteration {i}")
            self.assertEqual(
                errors, [],
                f"Import(s) failed on iteration {i}: {errors}")


def setUpModule():
    thread_info = threading_helper.threading_setup()
    unittest.addModuleCleanup(threading_helper.threading_cleanup, *thread_info)
    try:
        old_switchinterval = sys.getswitchinterval()
        unittest.addModuleCleanup(sys.setswitchinterval, old_switchinterval)
        support.setswitchinterval(1e-5)
    except AttributeError:
        pass


if __name__ == "__main__":
    unittest.main()
