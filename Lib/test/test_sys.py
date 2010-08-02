# -*- coding: iso-8859-1 -*-
import unittest, test.test_support
import sys, cStringIO, os
import struct

class SysModuleTest(unittest.TestCase):

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
        self.assert_(not hasattr(__builtin__, "_"))
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
        self.assert_(err.getvalue().endswith("ValueError: 42\n"))

    # FIXME: testing the code for a lost or replaced excepthook in
    # Python/pythonrun.c::PyErr_PrintEx() is tricky.

    def test_exc_clear(self):
        self.assertRaises(TypeError, sys.exc_clear, 42)

        # Verify that exc_info is present and matches exc, then clear it, and
        # check that it worked.
        def clear_check(exc):
            typ, value, traceback = sys.exc_info()
            self.assert_(typ is not None)
            self.assert_(value is exc)
            self.assert_(traceback is not None)

            with test.test_support._check_py3k_warnings():
                sys.exc_clear()

            typ, value, traceback = sys.exc_info()
            self.assert_(typ is None)
            self.assert_(value is None)
            self.assert_(traceback is None)

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

            self.assert_(typ1 is typ2)
            self.assert_(value1 is exc)
            self.assert_(value1 is value2)
            self.assert_(traceback1 is traceback2)

        # Check that an exception can be cleared outside of an except block
        clear_check(exc)

    def test_exit(self):
        self.assertRaises(TypeError, sys.exit, 42, 42)

        # call without argument
        try:
            sys.exit(0)
        except SystemExit, exc:
            self.assertEquals(exc.code, 0)
        except:
            self.fail("wrong exception")
        else:
            self.fail("no exception")

        # call with tuple argument with one entry
        # entry will be unpacked
        try:
            sys.exit(42)
        except SystemExit, exc:
            self.assertEquals(exc.code, 42)
        except:
            self.fail("wrong exception")
        else:
            self.fail("no exception")

        # call with integer argument
        try:
            sys.exit((42,))
        except SystemExit, exc:
            self.assertEquals(exc.code, 42)
        except:
            self.fail("wrong exception")
        else:
            self.fail("no exception")

        # call with string argument
        try:
            sys.exit("exit")
        except SystemExit, exc:
            self.assertEquals(exc.code, "exit")
        except:
            self.fail("wrong exception")
        else:
            self.fail("no exception")

        # call with tuple argument with two entries
        try:
            sys.exit((17, 23))
        except SystemExit, exc:
            self.assertEquals(exc.code, (17, 23))
        except:
            self.fail("wrong exception")
        else:
            self.fail("no exception")

        # test that the exit machinery handles SystemExits properly
        import subprocess
        # both unnormalized...
        rc = subprocess.call([sys.executable, "-c",
                              "raise SystemExit, 46"])
        self.assertEqual(rc, 46)
        # ... and normalized
        rc = subprocess.call([sys.executable, "-c",
                              "raise SystemExit(47)"])
        self.assertEqual(rc, 47)

        def check_exit_message(code, expected, env=None):
            process = subprocess.Popen([sys.executable, "-c", code],
                                       stderr=subprocess.PIPE, env=env)
            stdout, stderr = process.communicate()
            self.assertEqual(process.returncode, 1)
            self.assertTrue(stderr.startswith(expected),
                "%s doesn't start with %s" % (repr(stderr), repr(expected)))

        # test that stderr buffer if flushed before the exit message is written
        # into stderr
        check_exit_message(
            r'import sys; sys.stderr.write("unflushed,"); sys.exit("message")',
            b"unflushed,message")

        # test that the unicode message is encoded to the stderr encoding
        env = os.environ.copy()
        env['PYTHONIOENCODING'] = 'latin-1'
        check_exit_message(
            r'import sys; sys.exit(u"h\xe9")',
            b"h\xe9", env=env)

    def test_getdefaultencoding(self):
        if test.test_support.have_unicode:
            self.assertRaises(TypeError, sys.getdefaultencoding, 42)
            # can't check more than the type, as the user might have changed it
            self.assert_(isinstance(sys.getdefaultencoding(), str))

    # testing sys.settrace() is done in test_trace.py
    # testing sys.setprofile() is done in test_profile.py

    def test_setcheckinterval(self):
        self.assertRaises(TypeError, sys.setcheckinterval)
        orig = sys.getcheckinterval()
        for n in 0, 100, 120, orig: # orig last to restore starting state
            sys.setcheckinterval(n)
            self.assertEquals(sys.getcheckinterval(), n)

    def test_recursionlimit(self):
        self.assertRaises(TypeError, sys.getrecursionlimit, 42)
        oldlimit = sys.getrecursionlimit()
        self.assertRaises(TypeError, sys.setrecursionlimit)
        self.assertRaises(ValueError, sys.setrecursionlimit, -42)
        sys.setrecursionlimit(10000)
        self.assertEqual(sys.getrecursionlimit(), 10000)
        sys.setrecursionlimit(oldlimit)

    def test_getwindowsversion(self):
        if hasattr(sys, "getwindowsversion"):
            v = sys.getwindowsversion()
            self.assert_(isinstance(v, tuple))
            self.assertEqual(len(v), 5)
            self.assert_(isinstance(v[0], int))
            self.assert_(isinstance(v[1], int))
            self.assert_(isinstance(v[2], int))
            self.assert_(isinstance(v[3], int))
            self.assert_(isinstance(v[4], str))

    def test_dlopenflags(self):
        if hasattr(sys, "setdlopenflags"):
            self.assert_(hasattr(sys, "getdlopenflags"))
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
            self.assert_(isinstance(sys.gettotalrefcount(), int))

    def test_getframe(self):
        self.assertRaises(TypeError, sys._getframe, 42, 42)
        self.assertRaises(ValueError, sys._getframe, 2000000000)
        self.assert_(
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
        self.assert_(main_id in d)
        self.assert_(thread_id in d)

        # Verify that the captured main-thread frame is _this_ frame.
        frame = d.pop(main_id)
        self.assert_(frame is sys._getframe())

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
        self.assert_(sourceline in ["leave_g.wait()", "entered_g.set()"])

        # Reap the spawned thread.
        leave_g.set()
        t.join()

    # Test sys._current_frames() when thread support doesn't exist.
    def current_frames_without_threads(self):
        # Not much happens here:  there is only one thread, with artificial
        # "thread id" 0.
        d = sys._current_frames()
        self.assertEqual(len(d), 1)
        self.assert_(0 in d)
        self.assert_(d[0] is sys._getframe())

    def test_attributes(self):
        self.assert_(isinstance(sys.api_version, int))
        self.assert_(isinstance(sys.argv, list))
        self.assert_(sys.byteorder in ("little", "big"))
        self.assert_(isinstance(sys.builtin_module_names, tuple))
        self.assert_(isinstance(sys.copyright, basestring))
        self.assert_(isinstance(sys.exec_prefix, basestring))
        self.assert_(isinstance(sys.executable, basestring))
        self.assertEqual(len(sys.float_info), 11)
        self.assertEqual(sys.float_info.radix, 2)
        self.assert_(isinstance(sys.hexversion, int))
        self.assert_(isinstance(sys.maxint, int))
        if test.test_support.have_unicode:
            self.assert_(isinstance(sys.maxunicode, int))
        self.assert_(isinstance(sys.platform, basestring))
        self.assert_(isinstance(sys.prefix, basestring))
        self.assert_(isinstance(sys.version, basestring))
        vi = sys.version_info
        self.assert_(isinstance(vi, tuple))
        self.assertEqual(len(vi), 5)
        self.assert_(isinstance(vi[0], int))
        self.assert_(isinstance(vi[1], int))
        self.assert_(isinstance(vi[2], int))
        self.assert_(vi[3] in ("alpha", "beta", "candidate", "final"))
        self.assert_(isinstance(vi[4], int))

    def test_43581(self):
        # Can't use sys.stdout, as this is a cStringIO object when
        # the test runs under regrtest.
        self.assert_(sys.__stdout__.encoding == sys.__stderr__.encoding)

    def test_sys_flags(self):
        self.failUnless(sys.flags)
        attrs = ("debug", "py3k_warning", "division_warning", "division_new",
                 "inspect", "interactive", "optimize", "dont_write_bytecode",
                 "no_site", "ignore_environment", "tabcheck", "verbose",
                 "unicode", "bytes_warning")
        for attr in attrs:
            self.assert_(hasattr(sys.flags, attr), attr)
            self.assertEqual(type(getattr(sys.flags, attr)), int, attr)
        self.assert_(repr(sys.flags))

    def test_clear_type_cache(self):
        sys._clear_type_cache()

    def test_ioencoding(self):
        import subprocess,os
        env = dict(os.environ)

        # Test character: cent sign, encoded as 0x4A (ASCII J) in CP424,
        # not representable in ASCII.

        env["PYTHONIOENCODING"] = "cp424"
        p = subprocess.Popen([sys.executable, "-c", 'print unichr(0xa2)'],
                             stdout = subprocess.PIPE, env=env)
        out = p.stdout.read().strip()
        self.assertEqual(out, unichr(0xa2).encode("cp424"))

        env["PYTHONIOENCODING"] = "ascii:replace"
        p = subprocess.Popen([sys.executable, "-c", 'print unichr(0xa2)'],
                             stdout = subprocess.PIPE, env=env)
        out = p.stdout.read().strip()
        self.assertEqual(out, '?')

    def test_call_tracing(self):
        self.assertEqual(sys.call_tracing(str, (2,)), "2")
        self.assertRaises(TypeError, sys.call_tracing, str, 2)

    def test_executable(self):
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
        self.assert_(executable in ["''", repr(sys.executable)], executable)

class SizeofTest(unittest.TestCase):

    TPFLAGS_HAVE_GC = 1<<14
    TPFLAGS_HEAPTYPE = 1L<<9

    def setUp(self):
        self.c = len(struct.pack('c', ' '))
        self.H = len(struct.pack('H', 0))
        self.i = len(struct.pack('i', 0))
        self.l = len(struct.pack('l', 0))
        self.P = len(struct.pack('P', 0))
        # due to missing size_t information from struct, it is assumed that
        # sizeof(Py_ssize_t) = sizeof(void*)
        self.header = 'PP'
        self.vheader = self.header + 'P'
        if hasattr(sys, "gettotalrefcount"):
            self.header += '2P'
            self.vheader += '2P'
        import _testcapi
        self.gc_headsize = _testcapi.SIZEOF_PYGC_HEAD
        self.file = open(test.test_support.TESTFN, 'wb')

    def tearDown(self):
        self.file.close()
        test.test_support.unlink(test.test_support.TESTFN)

    def check_sizeof(self, o, size):
        result = sys.getsizeof(o)
        if ((type(o) == type) and (o.__flags__ & self.TPFLAGS_HEAPTYPE) or\
           ((type(o) != type) and (type(o).__flags__ & self.TPFLAGS_HAVE_GC))):
            size += self.gc_headsize
        msg = 'wrong size for %s: got %d, expected %d' \
                % (type(o), result, size)
        self.assertEqual(result, size, msg)

    def calcsize(self, fmt):
        """Wrapper around struct.calcsize which enforces the alignment of the
        end of a structure to the alignment requirement of pointer.

        Note: This wrapper should only be used if a pointer member is included
        and no member with a size larger than a pointer exists.
        """
        return struct.calcsize(fmt + '0P')

    def test_gc_head_size(self):
        # Check that the gc header size is added to objects tracked by the gc.
        h = self.header
        size = self.calcsize
        gc_header_size = self.gc_headsize
        # bool objects are not gc tracked
        self.assertEqual(sys.getsizeof(True), size(h + 'l'))
        # but lists are
        self.assertEqual(sys.getsizeof([]), size(h + 'P PP') + gc_header_size)

    def test_default(self):
        h = self.header
        size = self.calcsize
        self.assertEqual(sys.getsizeof(True, -1), size(h + 'l'))

    def test_objecttypes(self):
        # check all types defined in Objects/
        h = self.header
        vh = self.vheader
        size = self.calcsize
        check = self.check_sizeof
        # bool
        check(True, size(h + 'l'))
        # buffer
        with test.test_support._check_py3k_warnings():
            check(buffer(''), size(h + '2P2Pil'))
        # builtin_function_or_method
        check(len, size(h + '3P'))
        # bytearray
        samples = ['', 'u'*100000]
        for sample in samples:
            x = bytearray(sample)
            check(x, size(vh + 'iPP') + x.__alloc__() * self.c)
        # bytearray_iterator
        check(iter(bytearray()), size(h + 'PP'))
        # cell
        def get_cell():
            x = 42
            def inner():
                return x
            return inner
        check(get_cell().func_closure[0], size(h + 'P'))
        # classobj (old-style class)
        class class_oldstyle():
            def method():
                pass
        check(class_oldstyle, size(h + '6P'))
        # instance (old-style class)
        check(class_oldstyle(), size(h + '3P'))
        # instancemethod (old-style class)
        check(class_oldstyle().method, size(h + '4P'))
        # complex
        check(complex(0,1), size(h + '2d'))
        # code
        check(get_cell().func_code, size(h + '4i8Pi2P'))
        # BaseException
        check(BaseException(), size(h + '3P'))
        # UnicodeEncodeError
        check(UnicodeEncodeError("", u"", 0, 0, ""), size(h + '5P2PP'))
        # UnicodeDecodeError
        check(UnicodeDecodeError("", "", 0, 0, ""), size(h + '5P2PP'))
        # UnicodeTranslateError
        check(UnicodeTranslateError(u"", 0, 1, ""), size(h + '5P2PP'))
        # method_descriptor (descriptor object)
        check(str.lower, size(h + '2PP'))
        # classmethod_descriptor (descriptor object)
        # XXX
        # member_descriptor (descriptor object)
        import datetime
        check(datetime.timedelta.days, size(h + '2PP'))
        # getset_descriptor (descriptor object)
        import __builtin__
        check(__builtin__.file.closed, size(h + '2PP'))
        # wrapper_descriptor (descriptor object)
        check(int.__add__, size(h + '2P2P'))
        # dictproxy
        class C(object): pass
        check(C.__dict__, size(h + 'P'))
        # method-wrapper (descriptor object)
        check({}.__iter__, size(h + '2P'))
        # dict
        check({}, size(h + '3P2P' + 8*'P2P'))
        x = {1:1, 2:2, 3:3, 4:4, 5:5, 6:6, 7:7, 8:8}
        check(x, size(h + '3P2P' + 8*'P2P') + 16*size('P2P'))
        # dictionary-keyiterator
        check({}.iterkeys(), size(h + 'P2PPP'))
        # dictionary-valueiterator
        check({}.itervalues(), size(h + 'P2PPP'))
        # dictionary-itemiterator
        check({}.iteritems(), size(h + 'P2PPP'))
        # ellipses
        check(Ellipsis, size(h + ''))
        # EncodingMap
        import codecs, encodings.iso8859_3
        x = codecs.charmap_build(encodings.iso8859_3.decoding_table)
        check(x, size(h + '32B2iB'))
        # enumerate
        check(enumerate([]), size(h + 'l3P'))
        # file
        check(self.file, size(h + '4P2i4P3i3P3i'))
        # float
        check(float(0), size(h + 'd'))
        # sys.floatinfo
        check(sys.float_info, size(vh) + self.P * len(sys.float_info))
        # frame
        import inspect
        CO_MAXBLOCKS = 20
        x = inspect.currentframe()
        ncells = len(x.f_code.co_cellvars)
        nfrees = len(x.f_code.co_freevars)
        extras = x.f_code.co_stacksize + x.f_code.co_nlocals +\
                 ncells + nfrees - 1
        check(x, size(vh + '12P3i' + CO_MAXBLOCKS*'3i' + 'P' + extras*'P'))
        # function
        def func(): pass
        check(func, size(h + '9P'))
        class c():
            @staticmethod
            def foo():
                pass
            @classmethod
            def bar(cls):
                pass
            # staticmethod
            check(foo, size(h + 'P'))
            # classmethod
            check(bar, size(h + 'P'))
        # generator
        def get_gen(): yield 1
        check(get_gen(), size(h + 'Pi2P'))
        # integer
        check(1, size(h + 'l'))
        check(100, size(h + 'l'))
        # iterator
        check(iter('abc'), size(h + 'lP'))
        # callable-iterator
        import re
        check(re.finditer('',''), size(h + '2P'))
        # list
        samples = [[], [1,2,3], ['1', '2', '3']]
        for sample in samples:
            check(sample, size(vh + 'PP') + len(sample)*self.P)
        # sortwrapper (list)
        # XXX
        # cmpwrapper (list)
        # XXX
        # listiterator (list)
        check(iter([]), size(h + 'lP'))
        # listreverseiterator (list)
        check(reversed([]), size(h + 'lP'))
        # long
        check(0L, size(vh + 'H') - self.H)
        check(1L, size(vh + 'H'))
        check(-1L, size(vh + 'H'))
        check(32768L, size(vh + 'H') + self.H)
        check(32768L*32768L-1, size(vh + 'H') + self.H)
        check(32768L*32768L, size(vh + 'H') + 2*self.H)
        # module
        check(unittest, size(h + 'P'))
        # None
        check(None, size(h + ''))
        # object
        check(object(), size(h + ''))
        # property (descriptor object)
        class C(object):
            def getx(self): return self.__x
            def setx(self, value): self.__x = value
            def delx(self): del self.__x
            x = property(getx, setx, delx, "")
            check(x, size(h + '4Pi'))
        # PyCObject
        # XXX
        # rangeiterator
        check(iter(xrange(1)), size(h + '4l'))
        # reverse
        check(reversed(''), size(h + 'PP'))
        # set
        # frozenset
        PySet_MINSIZE = 8
        samples = [[], range(10), range(50)]
        s = size(h + '3P2P' + PySet_MINSIZE*'lP' + 'lP')
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
        check(iter(set()), size(h + 'P3P'))
        # slice
        check(slice(1), size(h + '3P'))
        # str
        check('', size(vh + 'lic'))
        check('abc', size(vh + 'lic') + 3*self.c)
        # super
        check(super(int), size(h + '3P'))
        # tuple
        check((), size(vh))
        check((1,2,3), size(vh) + 3*self.P)
        # tupleiterator
        check(iter(()), size(h + 'lP'))
        # type
        # (PyTypeObject + PyNumberMethods +  PyMappingMethods +
        #  PySequenceMethods + PyBufferProcs)
        s = size(vh + 'P2P15Pl4PP9PP11PI') + size('41P 10P 3P 6P')
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
            check(s, size(h + 'PPlP') + usize * (len(s) + 1))
        # weakref
        import weakref
        check(weakref.ref(int), size(h + '2Pl2P'))
        # weakproxy
        # XXX
        # weakcallableproxy
        check(weakref.proxy(int), size(h + '2Pl2P'))
        # xrange
        check(xrange(1), size(h + '3l'))
        check(xrange(66000), size(h + '3l'))

    def test_pythontypes(self):
        # check all types defined in Python/
        h = self.header
        vh = self.vheader
        size = self.calcsize
        check = self.check_sizeof
        # _ast.AST
        import _ast
        check(_ast.AST(), size(h + ''))
        # imp.NullImporter
        import imp
        check(imp.NullImporter(self.file.name), size(h + ''))
        try:
            raise TypeError
        except TypeError:
            tb = sys.exc_info()[2]
            # traceback
            if tb != None:
                check(tb, size(h + '2P2i'))
        # symtable entry
        # XXX
        # sys.flags
        check(sys.flags, size(vh) + self.P * len(sys.flags))


def test_main():
    test_classes = (SysModuleTest, SizeofTest)

    test.test_support.run_unittest(*test_classes)

if __name__ == "__main__":
    test_main()
