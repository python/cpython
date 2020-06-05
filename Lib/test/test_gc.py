import unittest
import unittest.mock
from test.support import (verbose, refcount_test, run_unittest,
                          cpython_only, temp_dir, TESTFN, unlink,
                          import_module)
from test.support.script_helper import assert_python_ok, make_script
from test.support import threading_helper

import gc
import sys
import sysconfig
import textwrap
import threading
import time
import weakref

try:
    from _testcapi import with_tp_del
except ImportError:
    def with_tp_del(cls):
        class C(object):
            def __new__(cls, *args, **kwargs):
                raise TypeError('requires _testcapi.with_tp_del')
        return C

try:
    from _testcapi import ContainerNoGC
except ImportError:
    ContainerNoGC = None

### Support code
###############################################################################

# Bug 1055820 has several tests of longstanding bugs involving weakrefs and
# cyclic gc.

# An instance of C1055820 has a self-loop, so becomes cyclic trash when
# unreachable.
class C1055820(object):
    def __init__(self, i):
        self.i = i
        self.loop = self

class GC_Detector(object):
    # Create an instance I.  Then gc hasn't happened again so long as
    # I.gc_happened is false.

    def __init__(self):
        self.gc_happened = False

        def it_happened(ignored):
            self.gc_happened = True

        # Create a piece of cyclic trash that triggers it_happened when
        # gc collects it.
        self.wr = weakref.ref(C1055820(666), it_happened)

@with_tp_del
class Uncollectable(object):
    """Create a reference cycle with multiple __del__ methods.

    An object in a reference cycle will never have zero references,
    and so must be garbage collected.  If one or more objects in the
    cycle have __del__ methods, the gc refuses to guess an order,
    and leaves the cycle uncollected."""
    def __init__(self, partner=None):
        if partner is None:
            self.partner = Uncollectable(partner=self)
        else:
            self.partner = partner
    def __tp_del__(self):
        pass

if sysconfig.get_config_vars().get('PY_CFLAGS', ''):
    BUILD_WITH_NDEBUG = ('-DNDEBUG' in sysconfig.get_config_vars()['PY_CFLAGS'])
else:
    # Usually, sys.gettotalrefcount() is only present if Python has been
    # compiled in debug mode. If it's missing, expect that Python has
    # been released in release mode: with NDEBUG defined.
    BUILD_WITH_NDEBUG = (not hasattr(sys, 'gettotalrefcount'))

### Tests
###############################################################################

class GCTests(unittest.TestCase):
    def test_list(self):
        l = []
        l.append(l)
        gc.collect()
        del l
        self.assertEqual(gc.collect(), 1)

    def test_dict(self):
        d = {}
        d[1] = d
        gc.collect()
        del d
        self.assertEqual(gc.collect(), 1)

    def test_tuple(self):
        # since tuples are immutable we close the loop with a list
        l = []
        t = (l,)
        l.append(t)
        gc.collect()
        del t
        del l
        self.assertEqual(gc.collect(), 2)

    def test_class(self):
        class A:
            pass
        A.a = A
        gc.collect()
        del A
        self.assertNotEqual(gc.collect(), 0)

    def test_newstyleclass(self):
        class A(object):
            pass
        gc.collect()
        del A
        self.assertNotEqual(gc.collect(), 0)

    def test_instance(self):
        class A:
            pass
        a = A()
        a.a = a
        gc.collect()
        del a
        self.assertNotEqual(gc.collect(), 0)

    def test_newinstance(self):
        class A(object):
            pass
        a = A()
        a.a = a
        gc.collect()
        del a
        self.assertNotEqual(gc.collect(), 0)
        class B(list):
            pass
        class C(B, A):
            pass
        a = C()
        a.a = a
        gc.collect()
        del a
        self.assertNotEqual(gc.collect(), 0)
        del B, C
        self.assertNotEqual(gc.collect(), 0)
        A.a = A()
        del A
        self.assertNotEqual(gc.collect(), 0)
        self.assertEqual(gc.collect(), 0)

    def test_method(self):
        # Tricky: self.__init__ is a bound method, it references the instance.
        class A:
            def __init__(self):
                self.init = self.__init__
        a = A()
        gc.collect()
        del a
        self.assertNotEqual(gc.collect(), 0)

    @cpython_only
    def test_legacy_finalizer(self):
        # A() is uncollectable if it is part of a cycle, make sure it shows up
        # in gc.garbage.
        @with_tp_del
        class A:
            def __tp_del__(self): pass
        class B:
            pass
        a = A()
        a.a = a
        id_a = id(a)
        b = B()
        b.b = b
        gc.collect()
        del a
        del b
        self.assertNotEqual(gc.collect(), 0)
        for obj in gc.garbage:
            if id(obj) == id_a:
                del obj.a
                break
        else:
            self.fail("didn't find obj in garbage (finalizer)")
        gc.garbage.remove(obj)

    @cpython_only
    def test_legacy_finalizer_newclass(self):
        # A() is uncollectable if it is part of a cycle, make sure it shows up
        # in gc.garbage.
        @with_tp_del
        class A(object):
            def __tp_del__(self): pass
        class B(object):
            pass
        a = A()
        a.a = a
        id_a = id(a)
        b = B()
        b.b = b
        gc.collect()
        del a
        del b
        self.assertNotEqual(gc.collect(), 0)
        for obj in gc.garbage:
            if id(obj) == id_a:
                del obj.a
                break
        else:
            self.fail("didn't find obj in garbage (finalizer)")
        gc.garbage.remove(obj)

    def test_function(self):
        # Tricky: f -> d -> f, code should call d.clear() after the exec to
        # break the cycle.
        d = {}
        exec("def f(): pass\n", d)
        gc.collect()
        del d
        self.assertEqual(gc.collect(), 2)

    @refcount_test
    def test_frame(self):
        def f():
            frame = sys._getframe()
        gc.collect()
        f()
        self.assertEqual(gc.collect(), 1)

    def test_saveall(self):
        # Verify that cyclic garbage like lists show up in gc.garbage if the
        # SAVEALL option is enabled.

        # First make sure we don't save away other stuff that just happens to
        # be waiting for collection.
        gc.collect()
        # if this fails, someone else created immortal trash
        self.assertEqual(gc.garbage, [])

        L = []
        L.append(L)
        id_L = id(L)

        debug = gc.get_debug()
        gc.set_debug(debug | gc.DEBUG_SAVEALL)
        del L
        gc.collect()
        gc.set_debug(debug)

        self.assertEqual(len(gc.garbage), 1)
        obj = gc.garbage.pop()
        self.assertEqual(id(obj), id_L)

    def test_del(self):
        # __del__ methods can trigger collection, make this to happen
        thresholds = gc.get_threshold()
        gc.enable()
        gc.set_threshold(1)

        class A:
            def __del__(self):
                dir(self)
        a = A()
        del a

        gc.disable()
        gc.set_threshold(*thresholds)

    def test_del_newclass(self):
        # __del__ methods can trigger collection, make this to happen
        thresholds = gc.get_threshold()
        gc.enable()
        gc.set_threshold(1)

        class A(object):
            def __del__(self):
                dir(self)
        a = A()
        del a

        gc.disable()
        gc.set_threshold(*thresholds)

    # The following two tests are fragile:
    # They precisely count the number of allocations,
    # which is highly implementation-dependent.
    # For example, disposed tuples are not freed, but reused.
    # To minimize variations, though, we first store the get_count() results
    # and check them at the end.
    @refcount_test
    def test_get_count(self):
        gc.collect()
        a, b, c = gc.get_count()
        x = []
        d, e, f = gc.get_count()
        self.assertEqual((b, c), (0, 0))
        self.assertEqual((e, f), (0, 0))
        # This is less fragile than asserting that a equals 0.
        self.assertLess(a, 5)
        # Between the two calls to get_count(), at least one object was
        # created (the list).
        self.assertGreater(d, a)

    @refcount_test
    def test_collect_generations(self):
        gc.collect()
        # This object will "trickle" into generation N + 1 after
        # each call to collect(N)
        x = []
        gc.collect(0)
        # x is now in gen 1
        a, b, c = gc.get_count()
        gc.collect(1)
        # x is now in gen 2
        d, e, f = gc.get_count()
        gc.collect(2)
        # x is now in gen 3
        g, h, i = gc.get_count()
        # We don't check a, d, g since their exact values depends on
        # internal implementation details of the interpreter.
        self.assertEqual((b, c), (1, 0))
        self.assertEqual((e, f), (0, 1))
        self.assertEqual((h, i), (0, 0))

    def test_trashcan(self):
        class Ouch:
            n = 0
            def __del__(self):
                Ouch.n = Ouch.n + 1
                if Ouch.n % 17 == 0:
                    gc.collect()

        # "trashcan" is a hack to prevent stack overflow when deallocating
        # very deeply nested tuples etc.  It works in part by abusing the
        # type pointer and refcount fields, and that can yield horrible
        # problems when gc tries to traverse the structures.
        # If this test fails (as it does in 2.0, 2.1 and 2.2), it will
        # most likely die via segfault.

        # Note:  In 2.3 the possibility for compiling without cyclic gc was
        # removed, and that in turn allows the trashcan mechanism to work
        # via much simpler means (e.g., it never abuses the type pointer or
        # refcount fields anymore).  Since it's much less likely to cause a
        # problem now, the various constants in this expensive (we force a lot
        # of full collections) test are cut back from the 2.2 version.
        gc.enable()
        N = 150
        for count in range(2):
            t = []
            for i in range(N):
                t = [t, Ouch()]
            u = []
            for i in range(N):
                u = [u, Ouch()]
            v = {}
            for i in range(N):
                v = {1: v, 2: Ouch()}
        gc.disable()

    def test_trashcan_threads(self):
        # Issue #13992: trashcan mechanism should be thread-safe
        NESTING = 60
        N_THREADS = 2

        def sleeper_gen():
            """A generator that releases the GIL when closed or dealloc'ed."""
            try:
                yield
            finally:
                time.sleep(0.000001)

        class C(list):
            # Appending to a list is atomic, which avoids the use of a lock.
            inits = []
            dels = []
            def __init__(self, alist):
                self[:] = alist
                C.inits.append(None)
            def __del__(self):
                # This __del__ is called by subtype_dealloc().
                C.dels.append(None)
                # `g` will release the GIL when garbage-collected.  This
                # helps assert subtype_dealloc's behaviour when threads
                # switch in the middle of it.
                g = sleeper_gen()
                next(g)
                # Now that __del__ is finished, subtype_dealloc will proceed
                # to call list_dealloc, which also uses the trashcan mechanism.

        def make_nested():
            """Create a sufficiently nested container object so that the
            trashcan mechanism is invoked when deallocating it."""
            x = C([])
            for i in range(NESTING):
                x = [C([x])]
            del x

        def run_thread():
            """Exercise make_nested() in a loop."""
            while not exit:
                make_nested()

        old_switchinterval = sys.getswitchinterval()
        sys.setswitchinterval(1e-5)
        try:
            exit = []
            threads = []
            for i in range(N_THREADS):
                t = threading.Thread(target=run_thread)
                threads.append(t)
            with threading_helper.start_threads(threads, lambda: exit.append(1)):
                time.sleep(1.0)
        finally:
            sys.setswitchinterval(old_switchinterval)
        gc.collect()
        self.assertEqual(len(C.inits), len(C.dels))

    def test_boom(self):
        class Boom:
            def __getattr__(self, someattribute):
                del self.attr
                raise AttributeError

        a = Boom()
        b = Boom()
        a.attr = b
        b.attr = a

        gc.collect()
        garbagelen = len(gc.garbage)
        del a, b
        # a<->b are in a trash cycle now.  Collection will invoke
        # Boom.__getattr__ (to see whether a and b have __del__ methods), and
        # __getattr__ deletes the internal "attr" attributes as a side effect.
        # That causes the trash cycle to get reclaimed via refcounts falling to
        # 0, thus mutating the trash graph as a side effect of merely asking
        # whether __del__ exists.  This used to (before 2.3b1) crash Python.
        # Now __getattr__ isn't called.
        self.assertEqual(gc.collect(), 4)
        self.assertEqual(len(gc.garbage), garbagelen)

    def test_boom2(self):
        class Boom2:
            def __init__(self):
                self.x = 0

            def __getattr__(self, someattribute):
                self.x += 1
                if self.x > 1:
                    del self.attr
                raise AttributeError

        a = Boom2()
        b = Boom2()
        a.attr = b
        b.attr = a

        gc.collect()
        garbagelen = len(gc.garbage)
        del a, b
        # Much like test_boom(), except that __getattr__ doesn't break the
        # cycle until the second time gc checks for __del__.  As of 2.3b1,
        # there isn't a second time, so this simply cleans up the trash cycle.
        # We expect a, b, a.__dict__ and b.__dict__ (4 objects) to get
        # reclaimed this way.
        self.assertEqual(gc.collect(), 4)
        self.assertEqual(len(gc.garbage), garbagelen)

    def test_boom_new(self):
        # boom__new and boom2_new are exactly like boom and boom2, except use
        # new-style classes.

        class Boom_New(object):
            def __getattr__(self, someattribute):
                del self.attr
                raise AttributeError

        a = Boom_New()
        b = Boom_New()
        a.attr = b
        b.attr = a

        gc.collect()
        garbagelen = len(gc.garbage)
        del a, b
        self.assertEqual(gc.collect(), 4)
        self.assertEqual(len(gc.garbage), garbagelen)

    def test_boom2_new(self):
        class Boom2_New(object):
            def __init__(self):
                self.x = 0

            def __getattr__(self, someattribute):
                self.x += 1
                if self.x > 1:
                    del self.attr
                raise AttributeError

        a = Boom2_New()
        b = Boom2_New()
        a.attr = b
        b.attr = a

        gc.collect()
        garbagelen = len(gc.garbage)
        del a, b
        self.assertEqual(gc.collect(), 4)
        self.assertEqual(len(gc.garbage), garbagelen)

    def test_get_referents(self):
        alist = [1, 3, 5]
        got = gc.get_referents(alist)
        got.sort()
        self.assertEqual(got, alist)

        atuple = tuple(alist)
        got = gc.get_referents(atuple)
        got.sort()
        self.assertEqual(got, alist)

        adict = {1: 3, 5: 7}
        expected = [1, 3, 5, 7]
        got = gc.get_referents(adict)
        got.sort()
        self.assertEqual(got, expected)

        got = gc.get_referents([1, 2], {3: 4}, (0, 0, 0))
        got.sort()
        self.assertEqual(got, [0, 0] + list(range(5)))

        self.assertEqual(gc.get_referents(1, 'a', 4j), [])

    def test_is_tracked(self):
        # Atomic built-in types are not tracked, user-defined objects and
        # mutable containers are.
        # NOTE: types with special optimizations (e.g. tuple) have tests
        # in their own test files instead.
        self.assertFalse(gc.is_tracked(None))
        self.assertFalse(gc.is_tracked(1))
        self.assertFalse(gc.is_tracked(1.0))
        self.assertFalse(gc.is_tracked(1.0 + 5.0j))
        self.assertFalse(gc.is_tracked(True))
        self.assertFalse(gc.is_tracked(False))
        self.assertFalse(gc.is_tracked(b"a"))
        self.assertFalse(gc.is_tracked("a"))
        self.assertFalse(gc.is_tracked(bytearray(b"a")))
        self.assertFalse(gc.is_tracked(type))
        self.assertFalse(gc.is_tracked(int))
        self.assertFalse(gc.is_tracked(object))
        self.assertFalse(gc.is_tracked(object()))

        class UserClass:
            pass

        class UserInt(int):
            pass

        # Base class is object; no extra fields.
        class UserClassSlots:
            __slots__ = ()

        # Base class is fixed size larger than object; no extra fields.
        class UserFloatSlots(float):
            __slots__ = ()

        # Base class is variable size; no extra fields.
        class UserIntSlots(int):
            __slots__ = ()

        self.assertTrue(gc.is_tracked(gc))
        self.assertTrue(gc.is_tracked(UserClass))
        self.assertTrue(gc.is_tracked(UserClass()))
        self.assertTrue(gc.is_tracked(UserInt()))
        self.assertTrue(gc.is_tracked([]))
        self.assertTrue(gc.is_tracked(set()))
        self.assertFalse(gc.is_tracked(UserClassSlots()))
        self.assertFalse(gc.is_tracked(UserFloatSlots()))
        self.assertFalse(gc.is_tracked(UserIntSlots()))

    def test_is_finalized(self):
        # Objects not tracked by the always gc return false
        self.assertFalse(gc.is_finalized(3))

        storage = []
        class Lazarus:
            def __del__(self):
                storage.append(self)

        lazarus = Lazarus()
        self.assertFalse(gc.is_finalized(lazarus))

        del lazarus
        gc.collect()

        lazarus = storage.pop()
        self.assertTrue(gc.is_finalized(lazarus))

    def test_bug1055820b(self):
        # Corresponds to temp2b.py in the bug report.

        ouch = []
        def callback(ignored):
            ouch[:] = [wr() for wr in WRs]

        Cs = [C1055820(i) for i in range(2)]
        WRs = [weakref.ref(c, callback) for c in Cs]
        c = None

        gc.collect()
        self.assertEqual(len(ouch), 0)
        # Make the two instances trash, and collect again.  The bug was that
        # the callback materialized a strong reference to an instance, but gc
        # cleared the instance's dict anyway.
        Cs = None
        gc.collect()
        self.assertEqual(len(ouch), 2)  # else the callbacks didn't run
        for x in ouch:
            # If the callback resurrected one of these guys, the instance
            # would be damaged, with an empty __dict__.
            self.assertEqual(x, None)

    def test_bug21435(self):
        # This is a poor test - its only virtue is that it happened to
        # segfault on Tim's Windows box before the patch for 21435 was
        # applied.  That's a nasty bug relying on specific pieces of cyclic
        # trash appearing in exactly the right order in finalize_garbage()'s
        # input list.
        # But there's no reliable way to force that order from Python code,
        # so over time chances are good this test won't really be testing much
        # of anything anymore.  Still, if it blows up, there's _some_
        # problem ;-)
        gc.collect()

        class A:
            pass

        class B:
            def __init__(self, x):
                self.x = x

            def __del__(self):
                self.attr = None

        def do_work():
            a = A()
            b = B(A())

            a.attr = b
            b.attr = a

        do_work()
        gc.collect() # this blows up (bad C pointer) when it fails

    @cpython_only
    def test_garbage_at_shutdown(self):
        import subprocess
        code = """if 1:
            import gc
            import _testcapi
            @_testcapi.with_tp_del
            class X:
                def __init__(self, name):
                    self.name = name
                def __repr__(self):
                    return "<X %%r>" %% self.name
                def __tp_del__(self):
                    pass

            x = X('first')
            x.x = x
            x.y = X('second')
            del x
            gc.set_debug(%s)
        """
        def run_command(code):
            p = subprocess.Popen([sys.executable, "-Wd", "-c", code],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE)
            stdout, stderr = p.communicate()
            p.stdout.close()
            p.stderr.close()
            self.assertEqual(p.returncode, 0)
            self.assertEqual(stdout, b"")
            return stderr

        stderr = run_command(code % "0")
        self.assertIn(b"ResourceWarning: gc: 2 uncollectable objects at "
                      b"shutdown; use", stderr)
        self.assertNotIn(b"<X 'first'>", stderr)
        # With DEBUG_UNCOLLECTABLE, the garbage list gets printed
        stderr = run_command(code % "gc.DEBUG_UNCOLLECTABLE")
        self.assertIn(b"ResourceWarning: gc: 2 uncollectable objects at "
                      b"shutdown", stderr)
        self.assertTrue(
            (b"[<X 'first'>, <X 'second'>]" in stderr) or
            (b"[<X 'second'>, <X 'first'>]" in stderr), stderr)
        # With DEBUG_SAVEALL, no additional message should get printed
        # (because gc.garbage also contains normally reclaimable cyclic
        # references, and its elements get printed at runtime anyway).
        stderr = run_command(code % "gc.DEBUG_SAVEALL")
        self.assertNotIn(b"uncollectable objects at shutdown", stderr)

    def test_gc_main_module_at_shutdown(self):
        # Create a reference cycle through the __main__ module and check
        # it gets collected at interpreter shutdown.
        code = """if 1:
            class C:
                def __del__(self):
                    print('__del__ called')
            l = [C()]
            l.append(l)
            """
        rc, out, err = assert_python_ok('-c', code)
        self.assertEqual(out.strip(), b'__del__ called')

    def test_gc_ordinary_module_at_shutdown(self):
        # Same as above, but with a non-__main__ module.
        with temp_dir() as script_dir:
            module = """if 1:
                class C:
                    def __del__(self):
                        print('__del__ called')
                l = [C()]
                l.append(l)
                """
            code = """if 1:
                import sys
                sys.path.insert(0, %r)
                import gctest
                """ % (script_dir,)
            make_script(script_dir, 'gctest', module)
            rc, out, err = assert_python_ok('-c', code)
            self.assertEqual(out.strip(), b'__del__ called')

    def test_global_del_SystemExit(self):
        code = """if 1:
            class ClassWithDel:
                def __del__(self):
                    print('__del__ called')
            a = ClassWithDel()
            a.link = a
            raise SystemExit(0)"""
        self.addCleanup(unlink, TESTFN)
        with open(TESTFN, 'w') as script:
            script.write(code)
        rc, out, err = assert_python_ok(TESTFN)
        self.assertEqual(out.strip(), b'__del__ called')

    def test_get_stats(self):
        stats = gc.get_stats()
        self.assertEqual(len(stats), 3)
        for st in stats:
            self.assertIsInstance(st, dict)
            self.assertEqual(set(st),
                             {"collected", "collections", "uncollectable"})
            self.assertGreaterEqual(st["collected"], 0)
            self.assertGreaterEqual(st["collections"], 0)
            self.assertGreaterEqual(st["uncollectable"], 0)
        # Check that collection counts are incremented correctly
        if gc.isenabled():
            self.addCleanup(gc.enable)
            gc.disable()
        old = gc.get_stats()
        gc.collect(0)
        new = gc.get_stats()
        self.assertEqual(new[0]["collections"], old[0]["collections"] + 1)
        self.assertEqual(new[1]["collections"], old[1]["collections"])
        self.assertEqual(new[2]["collections"], old[2]["collections"])
        gc.collect(2)
        new = gc.get_stats()
        self.assertEqual(new[0]["collections"], old[0]["collections"] + 1)
        self.assertEqual(new[1]["collections"], old[1]["collections"])
        self.assertEqual(new[2]["collections"], old[2]["collections"] + 1)

    def test_freeze(self):
        gc.freeze()
        self.assertGreater(gc.get_freeze_count(), 0)
        gc.unfreeze()
        self.assertEqual(gc.get_freeze_count(), 0)

    def test_get_objects(self):
        gc.collect()
        l = []
        l.append(l)
        self.assertTrue(
                any(l is element for element in gc.get_objects(generation=0))
        )
        self.assertFalse(
                any(l is element for element in  gc.get_objects(generation=1))
        )
        self.assertFalse(
                any(l is element for element in gc.get_objects(generation=2))
        )
        gc.collect(generation=0)
        self.assertFalse(
                any(l is element for element in gc.get_objects(generation=0))
        )
        self.assertTrue(
                any(l is element for element in  gc.get_objects(generation=1))
        )
        self.assertFalse(
                any(l is element for element in gc.get_objects(generation=2))
        )
        gc.collect(generation=1)
        self.assertFalse(
                any(l is element for element in gc.get_objects(generation=0))
        )
        self.assertFalse(
                any(l is element for element in  gc.get_objects(generation=1))
        )
        self.assertTrue(
                any(l is element for element in gc.get_objects(generation=2))
        )
        gc.collect(generation=2)
        self.assertFalse(
                any(l is element for element in gc.get_objects(generation=0))
        )
        self.assertFalse(
                any(l is element for element in  gc.get_objects(generation=1))
        )
        self.assertTrue(
                any(l is element for element in gc.get_objects(generation=2))
        )
        del l
        gc.collect()

    def test_get_objects_arguments(self):
        gc.collect()
        self.assertEqual(len(gc.get_objects()),
                         len(gc.get_objects(generation=None)))

        self.assertRaises(ValueError, gc.get_objects, 1000)
        self.assertRaises(ValueError, gc.get_objects, -1000)
        self.assertRaises(TypeError, gc.get_objects, "1")
        self.assertRaises(TypeError, gc.get_objects, 1.234)

    def test_resurrection_only_happens_once_per_object(self):
        class A:  # simple self-loop
            def __init__(self):
                self.me = self

        class Lazarus(A):
            resurrected = 0
            resurrected_instances = []

            def __del__(self):
                Lazarus.resurrected += 1
                Lazarus.resurrected_instances.append(self)

        gc.collect()
        gc.disable()

        # We start with 0 resurrections
        laz = Lazarus()
        self.assertEqual(Lazarus.resurrected, 0)

        # Deleting the instance and triggering a collection
        # resurrects the object
        del laz
        gc.collect()
        self.assertEqual(Lazarus.resurrected, 1)
        self.assertEqual(len(Lazarus.resurrected_instances), 1)

        # Clearing the references and forcing a collection
        # should not resurrect the object again.
        Lazarus.resurrected_instances.clear()
        self.assertEqual(Lazarus.resurrected, 1)
        gc.collect()
        self.assertEqual(Lazarus.resurrected, 1)

        gc.enable()

    def test_resurrection_is_transitive(self):
        class Cargo:
            def __init__(self):
                self.me = self

        class Lazarus:
            resurrected_instances = []

            def __del__(self):
                Lazarus.resurrected_instances.append(self)

        gc.collect()
        gc.disable()

        laz = Lazarus()
        cargo = Cargo()
        cargo_id = id(cargo)

        # Create a cycle between cargo and laz
        laz.cargo = cargo
        cargo.laz = laz

        # Drop the references, force a collection and check that
        # everything was resurrected.
        del laz, cargo
        gc.collect()
        self.assertEqual(len(Lazarus.resurrected_instances), 1)
        instance = Lazarus.resurrected_instances.pop()
        self.assertTrue(hasattr(instance, "cargo"))
        self.assertEqual(id(instance.cargo), cargo_id)

        gc.collect()
        gc.enable()

    def test_resurrection_does_not_block_cleanup_of_other_objects(self):

        # When a finalizer resurrects objects, stats were reporting them as
        # having been collected.  This affected both collect()'s return
        # value and the dicts returned by get_stats().
        N = 100

        class A:  # simple self-loop
            def __init__(self):
                self.me = self

        class Z(A):  # resurrecting __del__
            def __del__(self):
                zs.append(self)

        zs = []

        def getstats():
            d = gc.get_stats()[-1]
            return d['collected'], d['uncollectable']

        gc.collect()
        gc.disable()

        # No problems if just collecting A() instances.
        oldc, oldnc = getstats()
        for i in range(N):
            A()
        t = gc.collect()
        c, nc = getstats()
        self.assertEqual(t, 2*N) # instance object & its dict
        self.assertEqual(c - oldc, 2*N)
        self.assertEqual(nc - oldnc, 0)

        # But Z() is not actually collected.
        oldc, oldnc = c, nc
        Z()
        # Nothing is collected - Z() is merely resurrected.
        t = gc.collect()
        c, nc = getstats()
        self.assertEqual(t, 0)
        self.assertEqual(c - oldc, 0)
        self.assertEqual(nc - oldnc, 0)

        # Z() should not prevent anything else from being collected.
        oldc, oldnc = c, nc
        for i in range(N):
            A()
        Z()
        t = gc.collect()
        c, nc = getstats()
        self.assertEqual(t, 2*N)
        self.assertEqual(c - oldc, 2*N)
        self.assertEqual(nc - oldnc, 0)

        # The A() trash should have been reclaimed already but the
        # 2 copies of Z are still in zs (and the associated dicts).
        oldc, oldnc = c, nc
        zs.clear()
        t = gc.collect()
        c, nc = getstats()
        self.assertEqual(t, 4)
        self.assertEqual(c - oldc, 4)
        self.assertEqual(nc - oldnc, 0)

        gc.enable()

    @unittest.skipIf(ContainerNoGC is None,
                     'requires ContainerNoGC extension type')
    def test_trash_weakref_clear(self):
        # Test that trash weakrefs are properly cleared (bpo-38006).
        #
        # Structure we are creating:
        #
        #   Z <- Y <- A--+--> WZ -> C
        #             ^  |
        #             +--+
        # where:
        #   WZ is a weakref to Z with callback C
        #   Y doesn't implement tp_traverse
        #   A contains a reference to itself, Y and WZ
        #
        # A, Y, Z, WZ are all trash.  The GC doesn't know that Z is trash
        # because Y does not implement tp_traverse.  To show the bug, WZ needs
        # to live long enough so that Z is deallocated before it.  Then, if
        # gcmodule is buggy, when Z is being deallocated, C will run.
        #
        # To ensure WZ lives long enough, we put it in a second reference
        # cycle.  That trick only works due to the ordering of the GC prev/next
        # linked lists.  So, this test is a bit fragile.
        #
        # The bug reported in bpo-38006 is caused because the GC did not
        # clear WZ before starting the process of calling tp_clear on the
        # trash.  Normally, handle_weakrefs() would find the weakref via Z and
        # clear it.  However, since the GC cannot find Z, WR is not cleared and
        # it can execute during delete_garbage().  That can lead to disaster
        # since the callback might tinker with objects that have already had
        # tp_clear called on them (leaving them in possibly invalid states).

        callback = unittest.mock.Mock()

        class A:
            __slots__ = ['a', 'y', 'wz']

        class Z:
            pass

        # setup required object graph, as described above
        a = A()
        a.a = a
        a.y = ContainerNoGC(Z())
        a.wz = weakref.ref(a.y.value, callback)
        # create second cycle to keep WZ alive longer
        wr_cycle = [a.wz]
        wr_cycle.append(wr_cycle)
        # ensure trash unrelated to this test is gone
        gc.collect()
        gc.disable()
        # release references and create trash
        del a, wr_cycle
        gc.collect()
        # if called, it means there is a bug in the GC.  The weakref should be
        # cleared before Z dies.
        callback.assert_not_called()
        gc.enable()


class GCCallbackTests(unittest.TestCase):
    def setUp(self):
        # Save gc state and disable it.
        self.enabled = gc.isenabled()
        gc.disable()
        self.debug = gc.get_debug()
        gc.set_debug(0)
        gc.callbacks.append(self.cb1)
        gc.callbacks.append(self.cb2)
        self.othergarbage = []

    def tearDown(self):
        # Restore gc state
        del self.visit
        gc.callbacks.remove(self.cb1)
        gc.callbacks.remove(self.cb2)
        gc.set_debug(self.debug)
        if self.enabled:
            gc.enable()
        # destroy any uncollectables
        gc.collect()
        for obj in gc.garbage:
            if isinstance(obj, Uncollectable):
                obj.partner = None
        del gc.garbage[:]
        del self.othergarbage
        gc.collect()

    def preclean(self):
        # Remove all fluff from the system.  Invoke this function
        # manually rather than through self.setUp() for maximum
        # safety.
        self.visit = []
        gc.collect()
        garbage, gc.garbage[:] = gc.garbage[:], []
        self.othergarbage.append(garbage)
        self.visit = []

    def cb1(self, phase, info):
        self.visit.append((1, phase, dict(info)))

    def cb2(self, phase, info):
        self.visit.append((2, phase, dict(info)))
        if phase == "stop" and hasattr(self, "cleanup"):
            # Clean Uncollectable from garbage
            uc = [e for e in gc.garbage if isinstance(e, Uncollectable)]
            gc.garbage[:] = [e for e in gc.garbage
                             if not isinstance(e, Uncollectable)]
            for e in uc:
                e.partner = None

    def test_collect(self):
        self.preclean()
        gc.collect()
        # Algorithmically verify the contents of self.visit
        # because it is long and tortuous.

        # Count the number of visits to each callback
        n = [v[0] for v in self.visit]
        n1 = [i for i in n if i == 1]
        n2 = [i for i in n if i == 2]
        self.assertEqual(n1, [1]*2)
        self.assertEqual(n2, [2]*2)

        # Count that we got the right number of start and stop callbacks.
        n = [v[1] for v in self.visit]
        n1 = [i for i in n if i == "start"]
        n2 = [i for i in n if i == "stop"]
        self.assertEqual(n1, ["start"]*2)
        self.assertEqual(n2, ["stop"]*2)

        # Check that we got the right info dict for all callbacks
        for v in self.visit:
            info = v[2]
            self.assertTrue("generation" in info)
            self.assertTrue("collected" in info)
            self.assertTrue("uncollectable" in info)

    def test_collect_generation(self):
        self.preclean()
        gc.collect(2)
        for v in self.visit:
            info = v[2]
            self.assertEqual(info["generation"], 2)

    @cpython_only
    def test_collect_garbage(self):
        self.preclean()
        # Each of these cause four objects to be garbage: Two
        # Uncollectables and their instance dicts.
        Uncollectable()
        Uncollectable()
        C1055820(666)
        gc.collect()
        for v in self.visit:
            if v[1] != "stop":
                continue
            info = v[2]
            self.assertEqual(info["collected"], 2)
            self.assertEqual(info["uncollectable"], 8)

        # We should now have the Uncollectables in gc.garbage
        self.assertEqual(len(gc.garbage), 4)
        for e in gc.garbage:
            self.assertIsInstance(e, Uncollectable)

        # Now, let our callback handle the Uncollectable instances
        self.cleanup=True
        self.visit = []
        gc.garbage[:] = []
        gc.collect()
        for v in self.visit:
            if v[1] != "stop":
                continue
            info = v[2]
            self.assertEqual(info["collected"], 0)
            self.assertEqual(info["uncollectable"], 4)

        # Uncollectables should be gone
        self.assertEqual(len(gc.garbage), 0)


    @unittest.skipIf(BUILD_WITH_NDEBUG,
                     'built with -NDEBUG')
    def test_refcount_errors(self):
        self.preclean()
        # Verify the "handling" of objects with broken refcounts

        # Skip the test if ctypes is not available
        import_module("ctypes")

        import subprocess
        code = textwrap.dedent('''
            from test.support import gc_collect, SuppressCrashReport

            a = [1, 2, 3]
            b = [a]

            # Avoid coredump when Py_FatalError() calls abort()
            SuppressCrashReport().__enter__()

            # Simulate the refcount of "a" being too low (compared to the
            # references held on it by live data), but keeping it above zero
            # (to avoid deallocating it):
            import ctypes
            ctypes.pythonapi.Py_DecRef(ctypes.py_object(a))

            # The garbage collector should now have a fatal error
            # when it reaches the broken object
            gc_collect()
        ''')
        p = subprocess.Popen([sys.executable, "-c", code],
                             stdout=subprocess.PIPE,
                             stderr=subprocess.PIPE)
        stdout, stderr = p.communicate()
        p.stdout.close()
        p.stderr.close()
        # Verify that stderr has a useful error message:
        self.assertRegex(stderr,
            br'gcmodule\.c:[0-9]+: gc_decref: Assertion "gc_get_refs\(g\) > 0" failed.')
        self.assertRegex(stderr,
            br'refcount is too small')
        # "address : 0x7fb5062efc18"
        # "address : 7FB5062EFC18"
        address_regex = br'[0-9a-fA-Fx]+'
        self.assertRegex(stderr,
            br'object address  : ' + address_regex)
        self.assertRegex(stderr,
            br'object refcount : 1')
        self.assertRegex(stderr,
            br'object type     : ' + address_regex)
        self.assertRegex(stderr,
            br'object type name: list')
        self.assertRegex(stderr,
            br'object repr     : \[1, 2, 3\]')


class GCTogglingTests(unittest.TestCase):
    def setUp(self):
        gc.enable()

    def tearDown(self):
        gc.disable()

    def test_bug1055820c(self):
        # Corresponds to temp2c.py in the bug report.  This is pretty
        # elaborate.

        c0 = C1055820(0)
        # Move c0 into generation 2.
        gc.collect()

        c1 = C1055820(1)
        c1.keep_c0_alive = c0
        del c0.loop # now only c1 keeps c0 alive

        c2 = C1055820(2)
        c2wr = weakref.ref(c2) # no callback!

        ouch = []
        def callback(ignored):
            ouch[:] = [c2wr()]

        # The callback gets associated with a wr on an object in generation 2.
        c0wr = weakref.ref(c0, callback)

        c0 = c1 = c2 = None

        # What we've set up:  c0, c1, and c2 are all trash now.  c0 is in
        # generation 2.  The only thing keeping it alive is that c1 points to
        # it. c1 and c2 are in generation 0, and are in self-loops.  There's a
        # global weakref to c2 (c2wr), but that weakref has no callback.
        # There's also a global weakref to c0 (c0wr), and that does have a
        # callback, and that callback references c2 via c2wr().
        #
        #               c0 has a wr with callback, which references c2wr
        #               ^
        #               |
        #               |     Generation 2 above dots
        #. . . . . . . .|. . . . . . . . . . . . . . . . . . . . . . . .
        #               |     Generation 0 below dots
        #               |
        #               |
        #            ^->c1   ^->c2 has a wr but no callback
        #            |  |    |  |
        #            <--v    <--v
        #
        # So this is the nightmare:  when generation 0 gets collected, we see
        # that c2 has a callback-free weakref, and c1 doesn't even have a
        # weakref.  Collecting generation 0 doesn't see c0 at all, and c0 is
        # the only object that has a weakref with a callback.  gc clears c1
        # and c2.  Clearing c1 has the side effect of dropping the refcount on
        # c0 to 0, so c0 goes away (despite that it's in an older generation)
        # and c0's wr callback triggers.  That in turn materializes a reference
        # to c2 via c2wr(), but c2 gets cleared anyway by gc.

        # We want to let gc happen "naturally", to preserve the distinction
        # between generations.
        junk = []
        i = 0
        detector = GC_Detector()
        while not detector.gc_happened:
            i += 1
            if i > 10000:
                self.fail("gc didn't happen after 10000 iterations")
            self.assertEqual(len(ouch), 0)
            junk.append([])  # this will eventually trigger gc

        self.assertEqual(len(ouch), 1)  # else the callback wasn't invoked
        for x in ouch:
            # If the callback resurrected c2, the instance would be damaged,
            # with an empty __dict__.
            self.assertEqual(x, None)

    def test_bug1055820d(self):
        # Corresponds to temp2d.py in the bug report.  This is very much like
        # test_bug1055820c, but uses a __del__ method instead of a weakref
        # callback to sneak in a resurrection of cyclic trash.

        ouch = []
        class D(C1055820):
            def __del__(self):
                ouch[:] = [c2wr()]

        d0 = D(0)
        # Move all the above into generation 2.
        gc.collect()

        c1 = C1055820(1)
        c1.keep_d0_alive = d0
        del d0.loop # now only c1 keeps d0 alive

        c2 = C1055820(2)
        c2wr = weakref.ref(c2) # no callback!

        d0 = c1 = c2 = None

        # What we've set up:  d0, c1, and c2 are all trash now.  d0 is in
        # generation 2.  The only thing keeping it alive is that c1 points to
        # it.  c1 and c2 are in generation 0, and are in self-loops.  There's
        # a global weakref to c2 (c2wr), but that weakref has no callback.
        # There are no other weakrefs.
        #
        #               d0 has a __del__ method that references c2wr
        #               ^
        #               |
        #               |     Generation 2 above dots
        #. . . . . . . .|. . . . . . . . . . . . . . . . . . . . . . . .
        #               |     Generation 0 below dots
        #               |
        #               |
        #            ^->c1   ^->c2 has a wr but no callback
        #            |  |    |  |
        #            <--v    <--v
        #
        # So this is the nightmare:  when generation 0 gets collected, we see
        # that c2 has a callback-free weakref, and c1 doesn't even have a
        # weakref.  Collecting generation 0 doesn't see d0 at all.  gc clears
        # c1 and c2.  Clearing c1 has the side effect of dropping the refcount
        # on d0 to 0, so d0 goes away (despite that it's in an older
        # generation) and d0's __del__ triggers.  That in turn materializes
        # a reference to c2 via c2wr(), but c2 gets cleared anyway by gc.

        # We want to let gc happen "naturally", to preserve the distinction
        # between generations.
        detector = GC_Detector()
        junk = []
        i = 0
        while not detector.gc_happened:
            i += 1
            if i > 10000:
                self.fail("gc didn't happen after 10000 iterations")
            self.assertEqual(len(ouch), 0)
            junk.append([])  # this will eventually trigger gc

        self.assertEqual(len(ouch), 1)  # else __del__ wasn't invoked
        for x in ouch:
            # If __del__ resurrected c2, the instance would be damaged, with an
            # empty __dict__.
            self.assertEqual(x, None)

def test_main():
    enabled = gc.isenabled()
    gc.disable()
    assert not gc.isenabled()
    debug = gc.get_debug()
    gc.set_debug(debug & ~gc.DEBUG_LEAK) # this test is supposed to leak

    try:
        gc.collect() # Delete 2nd generation garbage
        run_unittest(GCTests, GCTogglingTests, GCCallbackTests)
    finally:
        gc.set_debug(debug)
        # test gc.enable() even if GC is disabled by default
        if verbose:
            print("restoring automatic collection")
        # make sure to always test gc.enable()
        gc.enable()
        assert gc.isenabled()
        if not enabled:
            gc.disable()

if __name__ == "__main__":
    test_main()
