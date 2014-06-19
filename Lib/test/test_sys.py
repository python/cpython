# -*- coding: iso-8859-1 -*-
import unittest, test.test_support
from test.script_helper import assert_python_ok, assert_python_failure
import sys, os, cStringIO
import struct
import operator

class SysModuleTest(unittest.TestCase):

    def tearDown(self):
        test.test_support.reap_children()

    def test_original_displayhook(self):
        import __builtin__
        savestdout = sys.stdout
        out = cStringIO.StringIO()
        sys.stdout = out

        dh = sys.__displayhook__

        self.assertRaises(TypeError, dh)
        if hasattr(__builtin__, "_"):
            del __builtin__._

        dh(None)
        self.assertEqual(out.getvalue(), "")
        self.assertTrue(not hasattr(__builtin__, "_"))
        dh(42)
        self.assertEqual(out.getvalue(), "42\n")
        self.assertEqual(__builtin__._, 42)

        del sys.stdout
        self.assertRaises(RuntimeError, dh, 42)

        sys.stdout = savestdout

    def test_lost_displayhook(self):
        olddisplayhook = sys.displayhook
        del sys.displayhook
        code = compile("42", "<string>", "single")
        self.assertRaises(RuntimeError, eval, code)
        sys.displayhook = olddisplayhook

    def test_custom_displayhook(self):
        olddisplayhook = sys.displayhook
        def baddisplayhook(obj):
            raise ValueError
        sys.displayhook = baddisplayhook
        code = compile("42", "<string>", "single")
        self.assertRaises(ValueError, eval, code)
        sys.displayhook = olddisplayhook

    def test_original_excepthook(self):
        savestderr = sys.stderr
        err = cStringIO.StringIO()
        sys.stderr = err

        eh = sys.__excepthook__

        self.assertRaises(TypeError, eh)
        try:
            raise ValueError(42)
        except ValueError, exc:
            eh(*sys.exc_info())

        sys.stderr = savestderr
        self.assertTrue(err.getvalue().endswith("ValueError: 42\n"))

    # FIXME: testing the code for a lost or replaced excepthook in
    # Python/pythonrun.c::PyErr_PrintEx() is tricky.

    def test_exc_clear(self):
        self.assertRaises(TypeError, sys.exc_clear, 42)

        # Verify that exc_info is present and matches exc, then clear it, and
        # check that it worked.
        def clear_check(exc):
            typ, value, traceback = sys.exc_info()
            self.assertTrue(typ is not None)
            self.assertTrue(value is exc)
            self.assertTrue(traceback is not None)

            with test.test_support.check_py3k_warnings():
                sys.exc_clear()

            typ, value, traceback = sys.exc_info()
            self.assertTrue(typ is None)
            self.assertTrue(value is None)
            self.assertTrue(traceback is None)

        def clear():
            try:
                raise ValueError, 42
            except ValueError, exc:
                clear_check(exc)

        # Raise an exception and check that it can be cleared
        clear()

        # Verify that a frame currently handling an exception is
        # unaffected by calling exc_clear in a nested frame.
        try:
            raise ValueError, 13
        except ValueError, exc:
            typ1, value1, traceback1 = sys.exc_info()
            clear()
            typ2, value2, traceback2 = sys.exc_info()

            self.assertTrue(typ1 is typ2)
            self.assertTrue(value1 is exc)
            self.assertTrue(value1 is value2)
            self.assertTrue(traceback1 is traceback2)

        # Check that an exception can be cleared outside of an except block
        clear_check(exc)

    def test_exit(self):
        # call with two arguments
        self.assertRaises(TypeError, sys.exit, 42, 42)

        # call without argument
        with self.assertRaises(SystemExit) as cm:
            sys.exit()
        self.assertIsNone(cm.exception.code)

        rc, out, err = assert_python_ok('-c', 'import sys; sys.exit()')
        self.assertEqual(rc, 0)
        self.assertEqual(out, b'')
        self.assertEqual(err, b'')

        # call with integer argument
        with self.assertRaises(SystemExit) as cm:
            sys.exit(42)
        self.assertEqual(cm.exception.code, 42)

        # call with tuple argument with one entry
        # entry will be unpacked
        with self.assertRaises(SystemExit) as cm:
            sys.exit((42,))
        self.assertEqual(cm.exception.code, 42)

        # call with string argument
        with self.assertRaises(SystemExit) as cm:
            sys.exit("exit")
        self.assertEqual(cm.exception.code, "exit")

        # call with tuple argument with two entries
        with self.assertRaises(SystemExit) as cm:
            sys.exit((17, 23))
        self.assertEqual(cm.exception.code, (17, 23))

        # test that the exit machinery handles SystemExits properly
        # both unnormalized...
        rc, out, err = assert_python_failure('-c', 'raise SystemExit, 46')
        self.assertEqual(rc, 46)
        self.assertEqual(out, b'')
        self.assertEqual(err, b'')
        # ... and normalized
        rc, out, err = assert_python_failure('-c', 'raise SystemExit(47)')
        self.assertEqual(rc, 47)
        self.assertEqual(out, b'')
        self.assertEqual(err, b'')

        def check_exit_message(code, expected, **env_vars):
            rc, out, err = assert_python_failure('-c', code, **env_vars)
            self.assertEqual(rc, 1)
            self.assertEqual(out, b'')
            self.assertTrue(err.startswith(expected),
                "%s doesn't start with %s" % (repr(err), repr(expected)))

        # test that stderr buffer is flushed before the exit message is written
        # into stderr
        check_exit_message(
            r'import sys; sys.stderr.write("unflushed,"); sys.exit("message")',
            b"unflushed,message")

        # test that the unicode message is encoded to the stderr encoding
        check_exit_message(
            r'import sys; sys.exit(u"h\xe9")',
            b"h\xe9", PYTHONIOENCODING='latin-1')

    def test_getdefaultencoding(self):
        if test.test_support.have_unicode:
            self.assertRaises(TypeError, sys.getdefaultencoding, 42)
            # can't check more than the type, as the user might have changed it
            self.assertIsInstance(sys.getdefaultencoding(), str)

    # testing sys.settrace() is done in test_sys_settrace.py
    # testing sys.setprofile() is done in test_sys_setprofile.py

    def test_setcheckinterval(self):
        self.assertRaises(TypeError, sys.setcheckinterval)
        orig = sys.getcheckinterval()
        for n in 0, 100, 120, orig: # orig last to restore starting state
            sys.setcheckinterval(n)
            self.assertEqual(sys.getcheckinterval(), n)

    def test_recursionlimit(self):
        self.assertRaises(TypeError, sys.getrecursionlimit, 42)
        oldlimit = sys.getrecursionlimit()
        self.assertRaises(TypeError, sys.setrecursionlimit)
        self.assertRaises(ValueError, sys.setrecursionlimit, -42)
        sys.setrecursionlimit(10000)
        self.assertEqual(sys.getrecursionlimit(), 10000)
        sys.setrecursionlimit(oldlimit)

        self.assertRaises(OverflowError, sys.setrecursionlimit, 1 << 31)
        try:
            sys.setrecursionlimit((1 << 31) - 5)
            try:
                # issue13546: isinstance(e, ValueError) used to fail
                # when the recursion limit is close to 1<<31
                raise ValueError()
            except ValueError, e:
                pass
        finally:
            sys.setrecursionlimit(oldlimit)

    def test_getwindowsversion(self):
        # Raise SkipTest if sys doesn't have getwindowsversion attribute
        test.test_support.get_attribute(sys, "getwindowsversion")
        v = sys.getwindowsversion()
        self.assertEqual(len(v), 5)
        self.assertIsInstance(v[0], int)
        self.assertIsInstance(v[1], int)
        self.assertIsInstance(v[2], int)
        self.assertIsInstance(v[3], int)
        self.assertIsInstance(v[4], str)
        self.assertRaises(IndexError, operator.getitem, v, 5)
        self.assertIsInstance(v.major, int)
        self.assertIsInstance(v.minor, int)
        self.assertIsInstance(v.build, int)
        self.assertIsInstance(v.platform, int)
        self.assertIsInstance(v.service_pack, str)
        self.assertIsInstance(v.service_pack_minor, int)
        self.assertIsInstance(v.service_pack_major, int)
        self.assertIsInstance(v.suite_mask, int)
        self.assertIsInstance(v.product_type, int)
        self.assertEqual(v[0], v.major)
        self.assertEqual(v[1], v.minor)
        self.assertEqual(v[2], v.build)
        self.assertEqual(v[3], v.platform)
        self.assertEqual(v[4], v.service_pack)

        # This is how platform.py calls it. Make sure tuple
        #  still has 5 elements
        maj, min, buildno, plat, csd = sys.getwindowsversion()

    @unittest.skipUnless(hasattr(sys, "setdlopenflags"),
                         'test needs sys.setdlopenflags()')
    def test_dlopenflags(self):
        self.assertTrue(hasattr(sys, "getdlopenflags"))
        self.assertRaises(TypeError, sys.getdlopenflags, 42)
        oldflags = sys.getdlopenflags()
        self.assertRaises(TypeError, sys.setdlopenflags)
        sys.setdlopenflags(oldflags+1)
        self.assertEqual(sys.getdlopenflags(), oldflags+1)
        sys.setdlopenflags(oldflags)

    def test_refcount(self):
        # n here must be a global in order for this test to pass while
        # tracing with a python function.  Tracing calls PyFrame_FastToLocals
        # which will add a copy of any locals to the frame object, causing
        # the reference count to increase by 2 instead of 1.
        global n
        self.assertRaises(TypeError, sys.getrefcount)
        c = sys.getrefcount(None)
        n = None
        self.assertEqual(sys.getrefcount(None), c+1)
        del n
        self.assertEqual(sys.getrefcount(None), c)
        if hasattr(sys, "gettotalrefcount"):
            self.assertIsInstance(sys.gettotalrefcount(), int)

    def test_getframe(self):
        self.assertRaises(TypeError, sys._getframe, 42, 42)
        self.assertRaises(ValueError, sys._getframe, 2000000000)
        self.assertTrue(
            SysModuleTest.test_getframe.im_func.func_code \
            is sys._getframe().f_code
        )

    # sys._current_frames() is a CPython-only gimmick.
    def test_current_frames(self):
        have_threads = True
        try:
            import thread
        except ImportError:
            have_threads = False

        if have_threads:
            self.current_frames_with_threads()
        else:
            self.current_frames_without_threads()

    # Test sys._current_frames() in a WITH_THREADS build.
    @test.test_support.reap_threads
    def current_frames_with_threads(self):
        import threading, thread
        import traceback

        # Spawn a thread that blocks at a known place.  Then the main
        # thread does sys._current_frames(), and verifies that the frames
        # returned make sense.
        entered_g = threading.Event()
        leave_g = threading.Event()
        thread_info = []  # the thread's id

        def f123():
            g456()

        def g456():
            thread_info.append(thread.get_ident())
            entered_g.set()
            leave_g.wait()

        t = threading.Thread(target=f123)
        t.start()
        entered_g.wait()

        # At this point, t has finished its entered_g.set(), although it's
        # impossible to guess whether it's still on that line or has moved on
        # to its leave_g.wait().
        self.assertEqual(len(thread_info), 1)
        thread_id = thread_info[0]

        d = sys._current_frames()

        main_id = thread.get_ident()
        self.assertIn(main_id, d)
        self.assertIn(thread_id, d)

        # Verify that the captured main-thread frame is _this_ frame.
        frame = d.pop(main_id)
        self.assertTrue(frame is sys._getframe())

        # Verify that the captured thread frame is blocked in g456, called
        # from f123.  This is a litte tricky, since various bits of
        # threading.py are also in the thread's call stack.
        frame = d.pop(thread_id)
        stack = traceback.extract_stack(frame)
        for i, (filename, lineno, funcname, sourceline) in enumerate(stack):
            if funcname == "f123":
                break
        else:
            self.fail("didn't find f123() on thread's call stack")

        self.assertEqual(sourceline, "g456()")

        # And the next record must be for g456().
        filename, lineno, funcname, sourceline = stack[i+1]
        self.assertEqual(funcname, "g456")
        self.assertIn(sourceline, ["leave_g.wait()", "entered_g.set()"])

        # Reap the spawned thread.
        leave_g.set()
        t.join()

    # Test sys._current_frames() when thread support doesn't exist.
    def current_frames_without_threads(self):
        # Not much happens here:  there is only one thread, with artificial
        # "thread id" 0.
        d = sys._current_frames()
        self.assertEqual(len(d), 1)
        self.assertIn(0, d)
        self.assertTrue(d[0] is sys._getframe())

    def test_attributes(self):
        self.assertIsInstance(sys.api_version, int)
        self.assertIsInstance(sys.argv, list)
        self.assertIn(sys.byteorder, ("little", "big"))
        self.assertIsInstance(sys.builtin_module_names, tuple)
        self.assertIsInstance(sys.copyright, basestring)
        self.assertIsInstance(sys.exec_prefix, basestring)
        self.assertIsInstance(sys.executable, basestring)
        self.assertEqual(len(sys.float_info), 11)
        self.assertEqual(sys.float_info.radix, 2)
        self.assertEqual(len(sys.long_info), 2)
        self.assertTrue(sys.long_info.bits_per_digit % 5 == 0)
        self.assertTrue(sys.long_info.sizeof_digit >= 1)
        self.assertEqual(type(sys.long_info.bits_per_digit), int)
        self.assertEqual(type(sys.long_info.sizeof_digit), int)
        self.assertIsInstance(sys.hexversion, int)
        self.assertIsInstance(sys.maxint, int)
        if test.test_support.have_unicode:
            self.assertIsInstance(sys.maxunicode, int)
        self.assertIsInstance(sys.platform, basestring)
        self.assertIsInstance(sys.prefix, basestring)
        self.assertIsInstance(sys.version, basestring)
        vi = sys.version_info
        self.assertIsInstance(vi[:], tuple)
        self.assertEqual(len(vi), 5)
        self.assertIsInstance(vi[0], int)
        self.assertIsInstance(vi[1], int)
        self.assertIsInstance(vi[2], int)
        self.assertIn(vi[3], ("alpha", "beta", "candidate", "final"))
        self.assertIsInstance(vi[4], int)
        self.assertIsInstance(vi.major, int)
        self.assertIsInstance(vi.minor, int)
        self.assertIsInstance(vi.micro, int)
        self.assertIn(vi.releaselevel, ("alpha", "beta", "candidate", "final"))
        self.assertIsInstance(vi.serial, int)
        self.assertEqual(vi[0], vi.major)
        self.assertEqual(vi[1], vi.minor)
        self.assertEqual(vi[2], vi.micro)
        self.assertEqual(vi[3], vi.releaselevel)
        self.assertEqual(vi[4], vi.serial)
        self.assertTrue(vi > (1,0,0))
        self.assertIsInstance(sys.float_repr_style, str)
        self.assertIn(sys.float_repr_style, ('short', 'legacy'))

    def test_43581(self):
        # Can't use sys.stdout, as this is a cStringIO object when
        # the test runs under regrtest.
        self.assertTrue(sys.__stdout__.encoding == sys.__stderr__.encoding)

    def test_sys_flags(self):
        self.assertTrue(sys.flags)
        attrs = ("debug", "py3k_warning", "division_warning", "division_new",
                 "inspect", "interactive", "optimize", "dont_write_bytecode",
                 "no_site", "ignore_environment", "tabcheck", "verbose",
                 "unicode", "bytes_warning", "hash_randomization")
        for attr in attrs:
            self.assertTrue(hasattr(sys.flags, attr), attr)
            self.assertEqual(type(getattr(sys.flags, attr)), int, attr)
        self.assertTrue(repr(sys.flags))

    def test_clear_type_cache(self):
        sys._clear_type_cache()

    def test_ioencoding(self):
        import subprocess
        env = dict(os.environ)

        # Test character: cent sign, encoded as 0x4A (ASCII J) in CP424,
        # not representable in ASCII.

        env["PYTHONIOENCODING"] = "cp424"
        p = subprocess.Popen([sys.executable, "-c", 'print unichr(0xa2)'],
                             stdout = subprocess.PIPE, env=env)
        out = p.communicate()[0].strip()
        self.assertEqual(out, unichr(0xa2).encode("cp424"))

        env["PYTHONIOENCODING"] = "ascii:replace"
        p = subprocess.Popen([sys.executable, "-c", 'print unichr(0xa2)'],
                             stdout = subprocess.PIPE, env=env)
        out = p.communicate()[0].strip()
        self.assertEqual(out, '?')

    def test_call_tracing(self):
        self.assertEqual(sys.call_tracing(str, (2,)), "2")
        self.assertRaises(TypeError, sys.call_tracing, str, 2)

    def test_executable(self):
        # sys.executable should be absolute
        self.assertEqual(os.path.abspath(sys.executable), sys.executable)

        # Issue #7774: Ensure that sys.executable is an empty string if argv[0]
        # has been set to an non existent program name and Python is unable to
        # retrieve the real program name
        import subprocess
        # For a normal installation, it should work without 'cwd'
        # argument. For test runs in the build directory, see #7774.
        python_dir = os.path.dirname(os.path.realpath(sys.executable))
        p = subprocess.Popen(
            ["nonexistent", "-c", 'import sys; print repr(sys.executable)'],
            executable=sys.executable, stdout=subprocess.PIPE, cwd=python_dir)
        executable = p.communicate()[0].strip()
        p.wait()
        self.assertIn(executable, ["''", repr(sys.executable)])

@test.test_support.cpython_only
class SizeofTest(unittest.TestCase):

    def setUp(self):
        self.P = struct.calcsize('P')
        self.longdigit = sys.long_info.sizeof_digit
        import _testcapi
        self.gc_headsize = _testcapi.SIZEOF_PYGC_HEAD
        self.file = open(test.test_support.TESTFN, 'wb')

    def tearDown(self):
        self.file.close()
        test.test_support.unlink(test.test_support.TESTFN)

    check_sizeof = test.test_support.check_sizeof

    def test_gc_head_size(self):
        # Check that the gc header size is added to objects tracked by the gc.
        size = test.test_support.calcobjsize
        gc_header_size = self.gc_headsize
        # bool objects are not gc tracked
        self.assertEqual(sys.getsizeof(True), size('l'))
        # but lists are
        self.assertEqual(sys.getsizeof([]), size('P PP') + gc_header_size)

    def test_default(self):
        size = test.test_support.calcobjsize
        self.assertEqual(sys.getsizeof(True, -1), size('l'))

    def test_objecttypes(self):
        # check all types defined in Objects/
        size = test.test_support.calcobjsize
        vsize = test.test_support.calcvobjsize
        check = self.check_sizeof
        # bool
        check(True, size('l'))
        # buffer
        with test.test_support.check_py3k_warnings():
            check(buffer(''), size('2P2Pil'))
        # builtin_function_or_method
        check(len, size('3P'))
        # bytearray
        samples = ['', 'u'*100000]
        for sample in samples:
            x = bytearray(sample)
            check(x, vsize('iPP') + x.__alloc__())
        # bytearray_iterator
        check(iter(bytearray()), size('PP'))
        # cell
        def get_cell():
            x = 42
            def inner():
                return x
            return inner
        check(get_cell().func_closure[0], size('P'))
        # classobj (old-style class)
        class class_oldstyle():
            def method():
                pass
        check(class_oldstyle, size('7P'))
        # instance (old-style class)
        check(class_oldstyle(), size('3P'))
        # instancemethod (old-style class)
        check(class_oldstyle().method, size('4P'))
        # complex
        check(complex(0,1), size('2d'))
        # code
        check(get_cell().func_code, size('4i8Pi3P'))
        # BaseException
        check(BaseException(), size('3P'))
        # UnicodeEncodeError
        check(UnicodeEncodeError("", u"", 0, 0, ""), size('5P2PP'))
        # UnicodeDecodeError
        check(UnicodeDecodeError("", "", 0, 0, ""), size('5P2PP'))
        # UnicodeTranslateError
        check(UnicodeTranslateError(u"", 0, 1, ""), size('5P2PP'))
        # method_descriptor (descriptor object)
        check(str.lower, size('2PP'))
        # classmethod_descriptor (descriptor object)
        # XXX
        # member_descriptor (descriptor object)
        import datetime
        check(datetime.timedelta.days, size('2PP'))
        # getset_descriptor (descriptor object)
        import __builtin__
        check(__builtin__.file.closed, size('2PP'))
        # wrapper_descriptor (descriptor object)
        check(int.__add__, size('2P2P'))
        # dictproxy
        class C(object): pass
        check(C.__dict__, size('P'))
        # method-wrapper (descriptor object)
        check({}.__iter__, size('2P'))
        # dict
        check({}, size('3P2P' + 8*'P2P'))
        x = {1:1, 2:2, 3:3, 4:4, 5:5, 6:6, 7:7, 8:8}
        check(x, size('3P2P' + 8*'P2P') + 16*struct.calcsize('P2P'))
        # dictionary-keyiterator
        check({}.iterkeys(), size('P2PPP'))
        # dictionary-valueiterator
        check({}.itervalues(), size('P2PPP'))
        # dictionary-itemiterator
        check({}.iteritems(), size('P2PPP'))
        # ellipses
        check(Ellipsis, size(''))
        # EncodingMap
        import codecs, encodings.iso8859_3
        x = codecs.charmap_build(encodings.iso8859_3.decoding_table)
        check(x, size('32B2iB'))
        # enumerate
        check(enumerate([]), size('l3P'))
        # file
        check(self.file, size('4P2i4P3i3P3i'))
        # float
        check(float(0), size('d'))
        # sys.floatinfo
        check(sys.float_info, vsize('') + self.P * len(sys.float_info))
        # frame
        import inspect
        CO_MAXBLOCKS = 20
        x = inspect.currentframe()
        ncells = len(x.f_code.co_cellvars)
        nfrees = len(x.f_code.co_freevars)
        extras = x.f_code.co_stacksize + x.f_code.co_nlocals +\
                 ncells + nfrees - 1
        check(x, vsize('12P3i' + CO_MAXBLOCKS*'3i' + 'P' + extras*'P'))
        # function
        def func(): pass
        check(func, size('9P'))
        class c():
            @staticmethod
            def foo():
                pass
            @classmethod
            def bar(cls):
                pass
            # staticmethod
            check(foo, size('P'))
            # classmethod
            check(bar, size('P'))
        # generator
        def get_gen(): yield 1
        check(get_gen(), size('Pi2P'))
        # integer
        check(1, size('l'))
        check(100, size('l'))
        # iterator
        check(iter('abc'), size('lP'))
        # callable-iterator
        import re
        check(re.finditer('',''), size('2P'))
        # list
        samples = [[], [1,2,3], ['1', '2', '3']]
        for sample in samples:
            check(sample, vsize('PP') + len(sample)*self.P)
        # sortwrapper (list)
        # XXX
        # cmpwrapper (list)
        # XXX
        # listiterator (list)
        check(iter([]), size('lP'))
        # listreverseiterator (list)
        check(reversed([]), size('lP'))
        # long
        check(0L, vsize(''))
        check(1L, vsize('') + self.longdigit)
        check(-1L, vsize('') + self.longdigit)
        PyLong_BASE = 2**sys.long_info.bits_per_digit
        check(long(PyLong_BASE), vsize('') + 2*self.longdigit)
        check(long(PyLong_BASE**2-1), vsize('') + 2*self.longdigit)
        check(long(PyLong_BASE**2), vsize('') + 3*self.longdigit)
        # module
        check(unittest, size('P'))
        # None
        check(None, size(''))
        # object
        check(object(), size(''))
        # property (descriptor object)
        class C(object):
            def getx(self): return self.__x
            def setx(self, value): self.__x = value
            def delx(self): del self.__x
            x = property(getx, setx, delx, "")
            check(x, size('4Pi'))
        # PyCObject
        # PyCapsule
        # XXX
        # rangeiterator
        check(iter(xrange(1)), size('4l'))
        # reverse
        check(reversed(''), size('PP'))
        # set
        # frozenset
        PySet_MINSIZE = 8
        samples = [[], range(10), range(50)]
        s = size('3P2P' + PySet_MINSIZE*'lP' + 'lP')
        for sample in samples:
            minused = len(sample)
            if minused == 0: tmp = 1
            # the computation of minused is actually a bit more complicated
            # but this suffices for the sizeof test
            minused = minused*2
            newsize = PySet_MINSIZE
            while newsize <= minused:
                newsize = newsize << 1
            if newsize <= 8:
                check(set(sample), s)
                check(frozenset(sample), s)
            else:
                check(set(sample), s + newsize*struct.calcsize('lP'))
                check(frozenset(sample), s + newsize*struct.calcsize('lP'))
        # setiterator
        check(iter(set()), size('P3P'))
        # slice
        check(slice(1), size('3P'))
        # str
        vh = test.test_support._vheader
        check('', struct.calcsize(vh + 'lic'))
        check('abc', struct.calcsize(vh + 'lic') + 3)
        # super
        check(super(int), size('3P'))
        # tuple
        check((), vsize(''))
        check((1,2,3), vsize('') + 3*self.P)
        # tupleiterator
        check(iter(()), size('lP'))
        # type
        # (PyTypeObject + PyNumberMethods +  PyMappingMethods +
        #  PySequenceMethods + PyBufferProcs)
        s = vsize('P2P15Pl4PP9PP11PI') + struct.calcsize('41P 10P 3P 6P')
        class newstyleclass(object):
            pass
        check(newstyleclass, s)
        # builtin type
        check(int, s)
        # NotImplementedType
        import types
        check(types.NotImplementedType, s)
        # unicode
        usize = len(u'\0'.encode('unicode-internal'))
        samples = [u'', u'1'*100]
        # we need to test for both sizes, because we don't know if the string
        # has been cached
        for s in samples:
            check(s, size('PPlP') + usize * (len(s) + 1))
        # weakref
        import weakref
        check(weakref.ref(int), size('2Pl2P'))
        # weakproxy
        # XXX
        # weakcallableproxy
        check(weakref.proxy(int), size('2Pl2P'))
        # xrange
        check(xrange(1), size('3l'))
        check(xrange(66000), size('3l'))

    def test_pythontypes(self):
        # check all types defined in Python/
        size = test.test_support.calcobjsize
        vsize = test.test_support.calcvobjsize
        check = self.check_sizeof
        # _ast.AST
        import _ast
        check(_ast.AST(), size(''))
        # imp.NullImporter
        import imp
        check(imp.NullImporter(self.file.name), size(''))
        try:
            raise TypeError
        except TypeError:
            tb = sys.exc_info()[2]
            # traceback
            if tb != None:
                check(tb, size('2P2i'))
        # symtable entry
        # XXX
        # sys.flags
        check(sys.flags, vsize('') + self.P * len(sys.flags))


def test_main():
    test_classes = (SysModuleTest, SizeofTest)

    test.test_support.run_unittest(*test_classes)

if __name__ == "__main__":
    test_main()
