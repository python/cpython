# This is a variant of the very old (early 90's) file
# Demo/threads/bug.py.  It simply provokes a number of threads into
# trying to import the same module "at the same time".
# There are no pleasant failure modes -- most likely is that Python
# complains several times about module random having no attribute
# randrange, and then Python hangs.

import _imp as imp
import collections
import faulthandler
import os
import importlib
import importlib.machinery
import sys
import time
import shutil
import threading
import types
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

    # Helpers for the deterministic two-thread tests below.  Rather than
    # relying on sleeps, a package __init__ calls back into the test and is
    # held there until the second thread is observed waiting on a specific
    # module lock, so the problematic interleaving happens on every run.

    def _setup_package_tree(self, files):
        # These tests are about what happens when one module's body imports
        # another.  Under -X lazy_imports=all those imports would never run.
        self.addCleanup(sys.set_lazy_imports, sys.get_lazy_imports())
        sys.set_lazy_imports("normal")
        os.makedirs(TESTFN, exist_ok=True)
        self.addCleanup(rmtree, TESTFN)
        for relpath, source in files.items():
            path = os.path.join(TESTFN, *relpath.split("/"))
            os.makedirs(os.path.dirname(path), exist_ok=True)
            with open(path, "w", encoding="utf-8") as f:
                f.write(source)
        sys.path.insert(0, TESTFN)
        self.addCleanup(sys.path.remove, TESTFN)
        # Forget by top-level name, not by file name: a submodule may come
        # from a directory added to __path__ or have no file at all.
        tops = {relpath.partition("/")[0].removesuffix(".py")
                for relpath in files}
        self.addCleanup(self._forget_trees, tops)
        importlib.invalidate_caches()

    @staticmethod
    def _forget_trees(tops):
        for name in list(sys.modules):
            if name.partition(".")[0] in tops:
                forget(name)

    def _wait_until_blocked_on(self, thread, modname):
        # Return once *thread* is waiting to acquire *modname*'s module lock
        # (any module lock if *modname* is None) and another thread owns it.
        # The owner check matters: a lock is listed in _blocking_on for a
        # moment during every acquire(), contended or not.
        blocking_on = importlib._bootstrap._blocking_on
        for _ in support.sleeping_retry(
                support.SHORT_TIMEOUT,
                f"thread never waited for the module lock of {modname!r}"):
            for lock in list(blocking_on.get(thread.ident, ())):
                if (modname in (None, lock.name)
                        and lock.owner not in (None, thread.ident)):
                    return
            if not thread.is_alive():
                self.fail("thread finished without ever waiting for the "
                          f"module lock of {modname!r}")

    def _join_import_threads(self, threads):
        # Every wait made by the test's own code inside these threads is
        # bounded by SHORT_TIMEOUT and reports what it was waiting for, so
        # wait longer than their sum here: only a real import deadlock gets
        # this far.  The threads are daemons so that one cannot also block
        # interpreter shutdown.
        deadline = time.monotonic() + support.LONG_TIMEOUT
        for t in threads:
            if t.ident is not None:
                t.join(max(deadline - time.monotonic(), 0))
        hung = [t for t in threads if t.is_alive()]
        if hung:
            faulthandler.dump_traceback(sys.__stderr__)
            self.fail(f"import deadlock: unable to join {hung}")

    def _run_two_first_imports(self, files, stmt1, stmt2, *,
                               sync_name, t2_waits_for):
        """Thread 1 runs *stmt1*.  Something it executes calls
        ``<sync_name>.hold()``, which starts thread 2 on *stmt2* and returns
        only once thread 2 is blocked on the module lock of *t2_waits_for*
        (any module lock if None).  Module code may also call
        ``<sync_name>.join_t2()`` to wait for thread 2 to finish.
        Returns a list of (what, problem) pairs; empty means both imports
        succeeded and no module body ran more than once.
        """
        # list.append() is atomic, on free-threaded builds too.
        count = f"import {sync_name}; {sync_name}.executed.append(__name__)\n"
        self._setup_package_tree({relpath: count + source
                                  for relpath, source in files.items()})
        sync = types.ModuleType(sync_name)
        sync.executed = []
        sys.modules[sync_name] = sync
        self.addCleanup(sys.modules.pop, sync_name, None)
        errors = []

        def run(stmt):
            try:
                exec(stmt, {})
            except Exception as e:
                errors.append((stmt, f"{type(e).__name__}: {e}"))

        t1 = threading.Thread(target=run, args=(stmt1,), daemon=True)
        t2 = threading.Thread(target=run, args=(stmt2,), daemon=True)

        def hold():
            # One-shot: if thread 1's import fails, thread 2 may re-run the
            # same code; it must not wait on itself.
            if t2.ident is not None:
                return
            t2.start()
            self._wait_until_blocked_on(t2, t2_waits_for)
        sync.hold = hold

        def join_t2():
            t2.join(support.SHORT_TIMEOUT)
            if t2.is_alive():
                errors.append((stmt2, "still blocked while thread 1's module "
                                      "code waited for it"))
        sync.join_t2 = join_t2

        t1.start()
        self._join_import_threads([t1, t2])
        if t2.ident is None:
            errors.append((stmt2, "was never started"))
        for name, n in collections.Counter(sync.executed).items():
            if n != 1:
                errors.append((name, f"executed {n} times"))
        return errors

    def _hold_in_create_module(self, modname, sync_name):
        # Have the import of *modname* call <sync_name>.hold() from its
        # loader's create_module(): the importing thread then owns modname's
        # module lock but modname is not in sys.modules yet.  (A finder's
        # find_spec() would be too early a place to stop: it runs under the
        # global import lock, which every other import needs briefly.)
        class Loader(importlib.machinery.SourceFileLoader):
            def create_module(self, spec):
                sys.modules[sync_name].hold()
                return super().create_module(spec)

        class Finder:
            def find_spec(self, name, path=None, target=None):
                if name != modname:
                    return None
                spec = importlib.machinery.PathFinder.find_spec(
                    name, path, target)
                spec.loader = Loader(spec.loader.name, spec.loader.path)
                return spec

        finder = Finder()
        sys.meta_path.insert(0, finder)
        self.addCleanup(sys.meta_path.remove, finder)

    # A first import of {pkg}.mod.  The statements call back into C
    # (import_ensure_initialized()) for a parent that is in sys.modules;
    # import_module() stays in _find_and_load().
    SUBMODULE_IMPORTS = (
        "from {pkg}.mod import X",
        "import {pkg}.mod",
        "import importlib; importlib.import_module('{pkg}.mod')",
    )

    def test_parent_lock_not_held_while_submodule_executes(self):
        # gh-159963: the first `import pkg.mod` in a process also imports pkg.
        # Once pkg/__init__.py has finished, pkg's module lock must not stay
        # held while mod's body runs; otherwise every thread that began
        # waiting for pkg is blocked for the whole of mod's body as well.
        self._setup_package_tree({
            "unheld_pkg/__init__.py": "",
            "unheld_pkg/mod.py": (
                "import sys, importlib._bootstrap as _b\n"
                "def held(name):\n"
                "    ref = _b._module_locks.get(name)\n"
                "    lock = ref() if ref is not None else None\n"
                "    return lock is not None and lock.locked()\n"
                "own_lock_held = held(__name__)\n"
                "parent_lock_held = held('unheld_pkg')\n"
                "parent_initializing = "
                "sys.modules['unheld_pkg'].__spec__._initializing\n"),
        })
        import unheld_pkg.mod
        self.assertTrue(unheld_pkg.mod.own_lock_held)
        self.assertFalse(unheld_pkg.mod.parent_initializing)
        self.assertFalse(unheld_pkg.mod.parent_lock_held)

    def test_package_init_imports_submodule_deterministic(self):
        # gh-83065 (bpo-38884) with the losing interleaving forced: thread 1
        # is inside pkg/__init__.py, about to import pkg.mod, at the moment
        # thread 2's first import of pkg.mod has got as far as waiting for
        # pkg.  Thread 2 must be waiting *without* already owning pkg.mod's
        # lock, or thread 1's import of pkg.mod deadlocks against it.
        for i, stmt in enumerate(self.SUBMODULE_IMPORTS):
            pkg = f"detinit{i}_pkg"
            stmt = stmt.format(pkg=pkg)
            with self.subTest(stmt=stmt):
                errors = self._run_two_first_imports(
                    {
                        f"{pkg}/__init__.py": (
                            "import detinit_sync; detinit_sync.hold()\n"
                            f"import {pkg}.mod\n"),
                        f"{pkg}/mod.py": "X = 1\n",
                    },
                    f"import {pkg}",
                    stmt,
                    sync_name="detinit_sync",
                    t2_waits_for=pkg,
                )
                self.assertEqual(errors, [])
                self.assertEqual(sys.modules[f"{pkg}.mod"].X, 1)

    def test_submodule_import_while_package_not_yet_in_sys_modules(self):
        # gh-83065, the earlier window: thread 1 owns pkg's lock but has not
        # put pkg in sys.modules yet when thread 2 first imports pkg.mod.
        for i, stmt in enumerate(self.SUBMODULE_IMPORTS):
            pkg = f"early{i}_pkg"
            stmt = stmt.format(pkg=pkg)
            with self.subTest(stmt=stmt):
                self._hold_in_create_module(pkg, "early_sync")
                errors = self._run_two_first_imports(
                    {
                        f"{pkg}/__init__.py": "from . import mod\n",
                        f"{pkg}/mod.py": "X = 1\n",
                    },
                    f"import {pkg}",
                    stmt,
                    sync_name="early_sync",
                    t2_waits_for=pkg,
                )
                self.assertEqual(errors, [])
                self.assertEqual(sys.modules[f"{pkg}.mod"].X, 1)

    def test_submodule_found_via_path_set_late_in_package_init(self):
        # pkg.mod can only be found once pkg/__init__.py has replaced
        # __path__, pkgutil.extend_path() style.  Thread 2 must look at
        # pkg.__path__ after it has waited for pkg, not before.
        extra = os.path.abspath(os.path.join(TESTFN, "latepath_extra"))
        errors = self._run_two_first_imports(
            {
                "latepath_pkg/__init__.py": (
                    "import latepath_sync; latepath_sync.hold()\n"
                    f"__path__ = [*__path__, {extra!r}]\n"),
                "latepath_extra/mod.py": "X = 1\n",
            },
            "import latepath_pkg",
            "from latepath_pkg.mod import X",
            sync_name="latepath_sync",
            t2_waits_for="latepath_pkg",
        )
        self.assertEqual(errors, [])
        self.assertEqual(sys.modules["latepath_pkg.mod"].X, 1)

    def test_submodule_put_in_sys_modules_late_in_package_init(self):
        # gh-130094: there is no pkg/mod.py; pkg/__init__.py puts pkg.mod in
        # sys.modules itself, as collections does for collections.abc.
        errors = self._run_two_first_imports(
            {
                "inject_pkg/__init__.py": (
                    "import inject_sync; inject_sync.hold()\n"
                    "import sys, types\n"
                    "mod = types.ModuleType('inject_pkg.mod'); mod.X = 1\n"
                    "sys.modules['inject_pkg.mod'] = mod\n"),
            },
            "import inject_pkg",
            "from inject_pkg.mod import X",
            sync_name="inject_sync",
            t2_waits_for="inject_pkg",
        )
        self.assertEqual(errors, [])

    def test_concurrent_first_imports_of_sibling_packages(self):
        # gh-159963.  Thread 1: `import a.leaf`, where a/leaf.py needs b.leaf.
        # Thread 2: `import b.leaf`, where b/__init__.py needs a, started
        # while thread 1 is still inside a/__init__.py.
        # Dependencies are a.leaf -> b.leaf -> b -> a; `a` needs nothing, so
        # there is no cycle and either order works in one thread.  If the
        # importer of a.leaf kept a's lock for the duration of a/leaf.py,
        # this would raise _DeadlockError.
        errors = self._run_two_first_imports(
            {
                "sib_a/__init__.py": "import sib_sync; sib_sync.hold()\n",
                "sib_a/leaf.py": "from sib_b.leaf import X\n",
                "sib_b/__init__.py": "import sib_a\n",
                "sib_b/leaf.py": "X = 1\n",
            },
            "import sib_a.leaf",
            "import sib_b.leaf",
            sync_name="sib_sync",
            t2_waits_for="sib_a",
        )
        self.assertEqual(errors, [])
        self.assertEqual(sys.modules["sib_a.leaf"].X, 1)
        self.assertEqual(sys.modules["sib_b.leaf"].X, 1)

    def test_concurrent_first_imports_no_partial_module(self):
        # Dependencies are a.leaf -> b -> a, again with no cycle.  Thread 1
        # finds b in sys.modules and waits for it, so if it kept a's lock
        # during a/leaf.py it would get not _DeadlockError but "cannot
        # import name 'X' from partially initialized module 'b'".
        errors = self._run_two_first_imports(
            {
                "part_a/__init__.py": "import part_sync; part_sync.hold()\n",
                "part_a/leaf.py": "from part_b import X\n",
                "part_b.py": "import part_a\nX = 1\n",
            },
            "import part_a.leaf",
            "import part_b",
            sync_name="part_sync",
            t2_waits_for="part_a",
        )
        self.assertEqual(errors, [])
        self.assertEqual(sys.modules["part_a.leaf"].X, 1)

    def test_finished_package_import_not_blocked_by_submodule_body(self):
        # Thread 2 starts waiting for pkg while pkg/__init__.py runs.  It
        # needs only the finished pkg, so it must be released when that
        # __init__ returns and not when thread 1's pkg/main.py does.  Here
        # main.py waits for thread 2, which no deadlock detector can see.
        for i, stmt in enumerate((
                "import {pkg}",
                "import {pkg}.other",
                "from {pkg}.other import X")):
            pkg = f"body{i}_pkg"
            stmt = stmt.format(pkg=pkg)
            with self.subTest(stmt=stmt):
                errors = self._run_two_first_imports(
                    {
                        f"{pkg}/__init__.py": (
                            "import body_sync; body_sync.hold()\n"),
                        f"{pkg}/main.py": (
                            "import body_sync; body_sync.join_t2()\n"),
                        f"{pkg}/other.py": "X = 1\n",
                    },
                    f"import {pkg}.main",
                    stmt,
                    sync_name="body_sync",
                    t2_waits_for=pkg,
                )
                self.assertEqual(errors, [])

    def test_concurrent_first_imports_of_nested_packages(self):
        # The same one level down: thread 1 does `import g.sub.leaf`;
        # g/sub/__init__.py needs module c; c needs g.  Thread 2 imports c
        # while thread 1 is still inside g/__init__.py.  No cycle.  If
        # thread 1 still held g's lock while it ran g/sub/__init__.py,
        # thread 2 (holding c, waiting for g) would look deadlocked to it
        # and it would get "cannot import name 'X' from partially
        # initialized module".  So g's lock must be released before any
        # package beneath it executes, not only before the leaf's body.
        errors = self._run_two_first_imports(
            {
                "nest_g/__init__.py": "import nest_sync; nest_sync.hold()\n",
                "nest_g/sub/__init__.py": "from nest_c import X\n",
                "nest_g/sub/leaf.py": "Y = 2\n",
                "nest_c.py": "import nest_g\nX = 1\n",
            },
            "import nest_g.sub.leaf",
            "import nest_c",
            sync_name="nest_sync",
            t2_waits_for="nest_g",
        )
        self.assertEqual(errors, [])
        self.assertEqual(sys.modules["nest_g.sub"].X, 1)
        self.assertEqual(sys.modules["nest_g.sub.leaf"].Y, 2)

    def test_submodule_import_waits_for_initializing_top_package(self):
        # Thread 1 runs g/__init__.py, which has imported subpackage g.a and
        # will next need g.a.other.  Thread 2's first import of g.a.other
        # starts at that point; other.py needs an attribute of g that is
        # already set.  Thread 2 must wait for g *before* owning g.a.other:
        # if it loaded g.a.other first and only then blocked on g (inside
        # other.py), thread 1 would find g.a.other held by a thread waiting
        # on itself and be handed the half-executed module ("cannot import
        # name 'X'").  Nothing here can fail in a single thread.
        errors = self._run_two_first_imports(
            {
                "topw_g/__init__.py": (
                    "Z = 1\n"
                    "import topw_g.a\n"
                    "import topw_sync; topw_sync.hold()\n"
                    "from topw_g.a.other import X\n"),
                "topw_g/a/__init__.py": "",
                "topw_g/a/other.py": "from topw_g import Z\nX = 1\n",
            },
            "import topw_g",
            "from topw_g.a.other import X",
            sync_name="topw_sync",
            t2_waits_for="topw_g",
        )
        self.assertEqual(errors, [])
        self.assertEqual(sys.modules["topw_g"].X, 1)

    def test_import_module_of_initializing_submodule_waits_for_it_only(self):
        # The converse of waiting for an initializing ancestor before
        # loading a submodule: pkg/__init__.py has *already*
        # imported pkg.leaf and then blocks on thread 2 (any non-import
        # wait).  Thread 2, started while pkg/leaf.py is still executing,
        # asks for pkg.leaf via importlib.  It must get it as soon as leaf.py
        # finishes, exactly as the import statement would, and not be made
        # to wait for pkg/__init__.py -- which is waiting for thread 2.
        errors = self._run_two_first_imports(
            {
                "lfo_pkg/__init__.py": (
                    "import lfo_pkg.leaf\n"
                    "import lfo_sync; lfo_sync.join_t2()\n"),
                "lfo_pkg/leaf.py": "import lfo_sync; lfo_sync.hold()\nX = 1\n",
            },
            "import lfo_pkg",
            "import importlib; importlib.import_module('lfo_pkg.leaf')",
            sync_name="lfo_sync",
            t2_waits_for=None,
        )
        self.assertEqual(errors, [])

    def test_import_module_in_cross_thread_cycle_returns_partial_module(self):
        # ax/x.py sets X, then imports by.y; by/y.py does `from ax.x import
        # X`.  Thread 1 is inside x.py (holding ax.x's lock) when thread 2's
        # `import by.y` reaches y.py and blocks on ax.x.  Thread 1 then asks
        # for by.y and is the one to detect the cycle.  The import statement
        # resolves this by handing thread 1 the partially
        # initialized by.y (import.c -> _lock_unlock_module());
        # importlib.import_module() must do the same rather than raise
        # _DeadlockError into x.py.
        for i, form in enumerate((
                "import by{i}_cyc.y",
                "import importlib; importlib.import_module('by{i}_cyc.y')")):
            form = form.format(i=i)
            with self.subTest(form=form):
                errors = self._run_two_first_imports(
                    {
                        f"ax{i}_cyc/__init__.py": "",
                        f"ax{i}_cyc/x.py": (
                            "X = 1\n"
                            "import cyc_sync; cyc_sync.hold()\n"
                            f"{form}\n"),
                        f"by{i}_cyc/__init__.py": "",
                        f"by{i}_cyc/y.py": (
                            f"from ax{i}_cyc.x import X\nassert X == 1\n"),
                    },
                    f"import ax{i}_cyc.x",
                    f"import by{i}_cyc.y",
                    sync_name="cyc_sync",
                    t2_waits_for=f"ax{i}_cyc.x",
                )
                self.assertEqual(errors, [])

    def test_submodule_enters_cross_thread_cycle_after_parents_resolved(self):
        # Thread 1 is inside m.py and has found pkg.leaf missing, but only
        # asks for its lock once thread 2 is inside leaf.py, waiting for m.
        # Thread 1 is the one to detect the cycle, and pkg.leaf is in
        # sys.modules by now, so it must get the partial module, as if it had
        # found it there in the first place.
        resolve_parents = importlib._bootstrap._resolve_parents

        def stalling_resolve_parents(name, import_):
            resolve_parents(name, import_)
            if name == "stall_pkg.leaf":
                sys.modules["stall_sync"].hold()

        with support.swap_attr(importlib._bootstrap, "_resolve_parents",
                               stalling_resolve_parents):
            errors = self._run_two_first_imports(
                {
                    "stall_pkg/__init__.py": "",
                    "stall_pkg/leaf.py": "import stall_m\nX = 1\n",
                    "stall_m.py": "import stall_pkg.leaf\n",
                },
                "import stall_m",
                "import stall_pkg.leaf",
                sync_name="stall_sync",
                t2_waits_for="stall_m",
            )
        self.assertEqual(errors, [])
        self.assertEqual(sys.modules["stall_pkg.leaf"].X, 1)

    def test_waiter_retries_after_failed_submodule_import(self):
        # Thread 2 waits for thread 1's import of pkg.mod, whose first
        # execution then fails.  Thread 2 must load pkg.mod itself and not
        # return the discarded module.
        for i, stmt in enumerate(self.SUBMODULE_IMPORTS):
            pkg = f"retry{i}_pkg"
            stmt = stmt.format(pkg=pkg)
            with self.subTest(stmt=stmt):
                errors = self._run_two_first_imports(
                    {
                        f"{pkg}/__init__.py": "",
                        f"{pkg}/mod.py": (
                            "import retry_sync; retry_sync.hold()\n"
                            "if retry_sync.executed.count(__name__) == 1:\n"
                            "    raise RuntimeError('first attempt fails')\n"
                            "X = 1\n"),
                    },
                    f"import {pkg}.mod",
                    stmt,
                    sync_name="retry_sync",
                    t2_waits_for=f"{pkg}.mod",
                )
                self.assertEqual(errors, [
                    (f"import {pkg}.mod",
                     "RuntimeError: first attempt fails"),
                    (f"{pkg}.mod", "executed 2 times"),
                ])
                self.assertEqual(sys.modules[f"{pkg}.mod"].X, 1)

    def test_waiters_recover_after_failed_package_init(self):
        # pkg/__init__.py imports pkg.a and pkg.b, but its first execution
        # (thread 0) raises once two other threads -- importing pkg.a and
        # pkg.b -- are already waiting for pkg.  One of the waiters then
        # re-runs pkg/__init__.py successfully; both waiters must succeed,
        # and neither may end up importing the parent while holding its
        # submodule's lock (the gh-83065 lock order) on that retry path.
        self._setup_package_tree({
            "flaky_pkg/__init__.py": (
                "import flaky_sync; flaky_sync.executed.append(__name__)\n"
                "flaky_sync.gate()\n"
                "import flaky_pkg.a\n"
                "import flaky_pkg.b\n"),
            "flaky_pkg/a.py": (
                "import flaky_sync; flaky_sync.executed.append(__name__)\n"
                "X = 1\n"),
            "flaky_pkg/b.py": (
                "import flaky_sync; flaky_sync.executed.append(__name__)\n"
                "import flaky_pkg.a\nX = 1\n"),
        })
        sync = types.ModuleType("flaky_sync")
        sync.executed = []
        sys.modules["flaky_sync"] = sync
        self.addCleanup(sys.modules.pop, "flaky_sync", None)
        errors = []

        def run(stmt):
            try:
                exec(stmt, {})
            except Exception as e:
                errors.append((stmt, f"{type(e).__name__}: {e}"))

        waiters = [threading.Thread(target=run, daemon=True,
                                    args=(f"import flaky_pkg.{m}",))
                   for m in ("a", "b")]
        first = True

        def gate():
            nonlocal first
            if not first:
                return
            first = False
            for t in waiters:
                t.start()
            for t in waiters:
                self._wait_until_blocked_on(t, "flaky_pkg")
            raise RuntimeError("first attempt fails")
        sync.gate = gate

        t0 = threading.Thread(target=run, args=("import flaky_pkg",),
                              daemon=True)
        t0.start()
        self._join_import_threads([t0, *waiters])
        self.assertEqual(
            errors,
            [("import flaky_pkg", "RuntimeError: first attempt fails")])
        self.assertEqual(sys.modules["flaky_pkg.b"].X, 1)
        self.assertEqual(
            sync.executed,
            ["flaky_pkg", "flaky_pkg", "flaky_pkg.a", "flaky_pkg.b"])


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
