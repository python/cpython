# -*- coding: iso-8859-1 -*-
import unittest, test.support
import sys, io, os
import struct
import subprocess
import textwrap

# count the number of test runs, used to create unique
# strings to intern in test_intern()
numruns = 0


class SysModuleTest(unittest.TestCase):

    def setUp(self):
        self.orig_stdout = sys.stdout
        self.orig_stderr = sys.stderr
        self.orig_displayhook = sys.displayhook

    def tearDown(self):
        sys.stdout = self.orig_stdout
        sys.stderr = self.orig_stderr
        sys.displayhook = self.orig_displayhook

    def test_original_displayhook(self):
        import builtins
        out = io.StringIO()
        sys.stdout = out

        dh = sys.__displayhook__

        self.assertRaises(TypeError, dh)
        if hasattr(builtins, "_"):
            del builtins._

        dh(None)
        self.assertEqual(out.getvalue(), "")
        self.assertTrue(not hasattr(builtins, "_"))
        dh(42)
        self.assertEqual(out.getvalue(), "42\n")
        self.assertEqual(builtins._, 42)

        del sys.stdout
        self.assertRaises(RuntimeError, dh, 42)

    def test_lost_displayhook(self):
        del sys.displayhook
        code = compile("42", "<string>", "single")
        self.assertRaises(RuntimeError, eval, code)

    def test_custom_displayhook(self):
        def baddisplayhook(obj):
            raise ValueError
        sys.displayhook = baddisplayhook
        code = compile("42", "<string>", "single")
        self.assertRaises(ValueError, eval, code)

    def test_original_excepthook(self):
        err = io.StringIO()
        sys.stderr = err

        eh = sys.__excepthook__

        self.assertRaises(TypeError, eh)
        try:
            raise ValueError(42)
        except ValueError as exc:
            eh(*sys.exc_info())

        self.assertTrue(err.getvalue().endswith("ValueError: 42\n"))

    def test_excepthook(self):
        with test.support.captured_output("stderr") as stderr:
            sys.excepthook(1, '1', 1)
        self.assertTrue("TypeError: print_exception(): Exception expected for " \
                         "value, str found" in stderr.getvalue())

    # FIXME: testing the code for a lost or replaced excepthook in
    # Python/pythonrun.c::PyErr_PrintEx() is tricky.

    def test_exit(self):

        self.assertRaises(TypeError, sys.exit, 42, 42)

        # call without argument
        try:
            sys.exit(0)
        except SystemExit as exc:
            self.assertEqual(exc.code, 0)
        except:
            self.fail("wrong exception")
        else:
            self.fail("no exception")

        # call with tuple argument with one entry
        # entry will be unpacked
        try:
            sys.exit(42)
        except SystemExit as exc:
            self.assertEqual(exc.code, 42)
        except:
            self.fail("wrong exception")
        else:
            self.fail("no exception")

        # call with integer argument
        try:
            sys.exit((42,))
        except SystemExit as exc:
            self.assertEqual(exc.code, 42)
        except:
            self.fail("wrong exception")
        else:
            self.fail("no exception")

        # call with string argument
        try:
            sys.exit("exit")
        except SystemExit as exc:
            self.assertEqual(exc.code, "exit")
        except:
            self.fail("wrong exception")
        else:
            self.fail("no exception")

        # call with tuple argument with two entries
        try:
            sys.exit((17, 23))
        except SystemExit as exc:
            self.assertEqual(exc.code, (17, 23))
        except:
            self.fail("wrong exception")
        else:
            self.fail("no exception")

        # test that the exit machinery handles SystemExits properly
        rc = subprocess.call([sys.executable, "-c",
                              "raise SystemExit(47)"])
        self.assertEqual(rc, 47)

        def check_exit_message(code, expected):
            process = subprocess.Popen([sys.executable, "-c", code],
                                       stderr=subprocess.PIPE)
            stdout, stderr = process.communicate()
            self.assertEqual(process.returncode, 1)
            self.assertTrue(stderr.startswith(expected),
                "%s doesn't start with %s" % (ascii(stderr), ascii(expected)))

        # test that stderr buffer if flushed before the exit message is written
        # into stderr
        check_exit_message(
            r'import sys; sys.stderr.write("unflushed,"); sys.exit("message")',
            b"unflushed,message")

        # test that the exit message is written with backslashreplace error
        # handler to stderr
        check_exit_message(
            r'import sys; sys.exit("surrogates:\uDCFF")',
            b"surrogates:\\udcff")

    def test_getdefaultencoding(self):
        self.assertRaises(TypeError, sys.getdefaultencoding, 42)
        # can't check more than the type, as the user might have changed it
        self.assertTrue(isinstance(sys.getdefaultencoding(), str))

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

    def test_recursionlimit_recovery(self):
        # NOTE: this test is slightly fragile in that it depends on the current
        # recursion count when executing the test being low enough so as to
        # trigger the recursion recovery detection in the _Py_MakeEndRecCheck
        # macro (see ceval.h).
        oldlimit = sys.getrecursionlimit()
        def f():
            f()
        try:
            for i in (50, 1000):
                # Issue #5392: stack overflow after hitting recursion limit twice
                sys.setrecursionlimit(i)
                self.assertRaises(RuntimeError, f)
                self.assertRaises(RuntimeError, f)
        finally:
            sys.setrecursionlimit(oldlimit)

    def test_recursionlimit_fatalerror(self):
        # A fatal error occurs if a second recursion limit is hit when recovering
        # from a first one.
        if os.name == "nt":
            raise unittest.SkipTest(
                "under Windows, test would generate a spurious crash dialog")
        code = textwrap.dedent("""
            import sys

            def f():
                try:
                    f()
                except RuntimeError:
                    f()

            sys.setrecursionlimit(%d)
            f()""")
        for i in (50, 1000):
            sub = subprocess.Popen([sys.executable, '-c', code % i],
                stderr=subprocess.PIPE)
            err = sub.communicate()[1]
            self.assertTrue(sub.returncode, sub.returncode)
            self.assertTrue(
                b"Fatal Python error: Cannot recover from stack overflow" in err,
                err)

    def test_getwindowsversion(self):
        if hasattr(sys, "getwindowsversion"):
            v = sys.getwindowsversion()
            self.assertTrue(isinstance(v, tuple))
            self.assertEqual(len(v), 5)
            self.assertTrue(isinstance(v[0], int))
            self.assertTrue(isinstance(v[1], int))
            self.assertTrue(isinstance(v[2], int))
            self.assertTrue(isinstance(v[3], int))
            self.assertTrue(isinstance(v[4], str))

    def test_call_tracing(self):
        self.assertRaises(TypeError, sys.call_tracing, type, 2)

    def test_dlopenflags(self):
        if hasattr(sys, "setdlopenflags"):
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
            self.assertTrue(isinstance(sys.gettotalrefcount(), int))

    def test_getframe(self):
        self.assertRaises(TypeError, sys._getframe, 42, 42)
        self.assertRaises(ValueError, sys._getframe, 2000000000)
        self.assertTrue(
            SysModuleTest.test_getframe.__code__ \
            is sys._getframe().f_code
        )

    # sys._current_frames() is a CPython-only gimmick.
    def test_current_frames(self):
        have_threads = True
        try:
            import _thread
        except ImportError:
            have_threads = False

        if have_threads:
            self.current_frames_with_threads()
        else:
            self.current_frames_without_threads()

    # Test sys._current_frames() in a WITH_THREADS build.
    def current_frames_with_threads(self):
        import threading, _thread
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
            thread_info.append(_thread.get_ident())
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

        main_id = _thread.get_ident()
        self.assertTrue(main_id in d)
        self.assertTrue(thread_id in d)

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
        self.assertTrue(sourceline in ["leave_g.wait()", "entered_g.set()"])

        # Reap the spawned thread.
        leave_g.set()
        t.join()

    # Test sys._current_frames() when thread support doesn't exist.
    def current_frames_without_threads(self):
        # Not much happens here:  there is only one thread, with artificial
        # "thread id" 0.
        d = sys._current_frames()
        self.assertEqual(len(d), 1)
        self.assertTrue(0 in d)
        self.assertTrue(d[0] is sys._getframe())

    def test_attributes(self):
        self.assertTrue(isinstance(sys.api_version, int))
        self.assertTrue(isinstance(sys.argv, list))
        self.assertTrue(sys.byteorder in ("little", "big"))
        self.assertTrue(isinstance(sys.builtin_module_names, tuple))
        self.assertTrue(isinstance(sys.copyright, str))
        self.assertTrue(isinstance(sys.exec_prefix, str))
        self.assertTrue(isinstance(sys.executable, str))
        self.assertEqual(len(sys.float_info), 11)
        self.assertEqual(sys.float_info.radix, 2)
        self.assertEqual(len(sys.int_info), 2)
        self.assertTrue(sys.int_info.bits_per_digit % 5 == 0)
        self.assertTrue(sys.int_info.sizeof_digit >= 1)
        self.assertEqual(type(sys.int_info.bits_per_digit), int)
        self.assertEqual(type(sys.int_info.sizeof_digit), int)
        self.assertTrue(isinstance(sys.hexversion, int))
        self.assertTrue(isinstance(sys.maxsize, int))
        self.assertTrue(isinstance(sys.maxunicode, int))
        self.assertTrue(isinstance(sys.platform, str))
        self.assertTrue(isinstance(sys.prefix, str))
        self.assertTrue(isinstance(sys.version, str))
        vi = sys.version_info
        self.assertTrue(isinstance(vi[:], tuple))
        self.assertEqual(len(vi), 5)
        self.assertTrue(isinstance(vi[0], int))
        self.assertTrue(isinstance(vi[1], int))
        self.assertTrue(isinstance(vi[2], int))
        self.assertTrue(vi[3] in ("alpha", "beta", "candidate", "final"))
        self.assertTrue(isinstance(vi[4], int))
        self.assertTrue(isinstance(vi.major, int))
        self.assertTrue(isinstance(vi.minor, int))
        self.assertTrue(isinstance(vi.micro, int))
        self.assertTrue(vi.releaselevel in
                     ("alpha", "beta", "candidate", "final"))
        self.assertTrue(isinstance(vi.serial, int))
        self.assertEqual(vi[0], vi.major)
        self.assertEqual(vi[1], vi.minor)
        self.assertEqual(vi[2], vi.micro)
        self.assertEqual(vi[3], vi.releaselevel)
        self.assertEqual(vi[4], vi.serial)
        self.assertTrue(vi > (1,0,0))

    def test_43581(self):
        # Can't use sys.stdout, as this is a StringIO object when
        # the test runs under regrtest.
        self.assertEqual(sys.__stdout__.encoding, sys.__stderr__.encoding)

    def test_intern(self):
        global numruns
        numruns += 1
        self.assertRaises(TypeError, sys.intern)
        s = "never interned before" + str(numruns)
        self.assertTrue(sys.intern(s) is s)
        s2 = s.swapcase().swapcase()
        self.assertTrue(sys.intern(s2) is s)

        # Subclasses of string can't be interned, because they
        # provide too much opportunity for insane things to happen.
        # We don't want them in the interned dict and if they aren't
        # actually interned, we don't want to create the appearance
        # that they are by allowing intern() to succeed.
        class S(str):
            def __hash__(self):
                return 123

        self.assertRaises(TypeError, sys.intern, S("abc"))

    def test_main_invalid_unicode(self):
        import locale
        non_decodable = b"\xff"
        encoding = locale.getpreferredencoding()
        try:
            non_decodable.decode(encoding)
        except UnicodeDecodeError:
            pass
        else:
            self.skipTest('%r is decodable with encoding %s'
                % (non_decodable, encoding))
        code = b'print("' + non_decodable + b'")'
        p = subprocess.Popen([sys.executable, "-c", code], stderr=subprocess.PIPE)
        stdout, stderr = p.communicate()
        self.assertEqual(p.returncode, 1)
        self.assertTrue(b"UnicodeEncodeError:" in stderr,
            "%r not in %s" % (b"UnicodeEncodeError:", ascii(stderr)))

    def test_sys_flags(self):
        self.assertTrue(sys.flags)
        attrs = ("debug", "division_warning",
                 "inspect", "interactive", "optimize", "dont_write_bytecode",
                 "no_user_site", "no_site", "ignore_environment", "verbose",
                 "bytes_warning")
        for attr in attrs:
            self.assertTrue(hasattr(sys.flags, attr), attr)
            self.assertEqual(type(getattr(sys.flags, attr)), int, attr)
        self.assertTrue(repr(sys.flags))
        self.assertEqual(len(sys.flags), len(attrs))

    def test_clear_type_cache(self):
        sys._clear_type_cache()

    def test_ioencoding(self):
        env = dict(os.environ)

        # Test character: cent sign, encoded as 0x4A (ASCII J) in CP424,
        # not representable in ASCII.

        env["PYTHONIOENCODING"] = "cp424"
        p = subprocess.Popen([sys.executable, "-c", 'print(chr(0xa2))'],
                             stdout = subprocess.PIPE, env=env)
        out = p.stdout.read()
        self.assertEqual(out, "\xa2\n".encode("cp424"))

        env["PYTHONIOENCODING"] = "ascii:replace"
        p = subprocess.Popen([sys.executable, "-c", 'print(chr(0xa2))'],
                             stdout = subprocess.PIPE, env=env)
        out = p.stdout.read().strip()
        self.assertEqual(out, b'?')

    def test_executable(self):
        # Issue #7774: Ensure that sys.executable is an empty string if argv[0]
        # has been set to an non existent program name and Python is unable to
        # retrieve the real program name

        # For a normal installation, it should work without 'cwd'
        # argument. For test runs in the build directory, see #7774.
        python_dir = os.path.dirname(os.path.realpath(sys.executable))
        p = subprocess.Popen(
            ["nonexistent", "-c",
             'import sys; print(sys.executable.encode("ascii", "backslashreplace"))'],
            executable=sys.executable, stdout=subprocess.PIPE, cwd=python_dir)
        stdout = p.communicate()[0]
        executable = stdout.strip().decode("ASCII")
        p.wait()
        self.assertIn(executable, ["b''", repr(sys.executable.encode("ascii", "backslashreplace"))])


class SizeofTest(unittest.TestCase):

    TPFLAGS_HAVE_GC = 1<<14
    TPFLAGS_HEAPTYPE = 1<<9

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
        self.longdigit = sys.int_info.sizeof_digit
        import _testcapi
        self.gc_headsize = _testcapi.SIZEOF_PYGC_HEAD
        self.file = open(test.support.TESTFN, 'wb')

    def tearDown(self):
        self.file.close()
        test.support.unlink(test.support.TESTFN)

    def check_sizeof(self, o, size):
        result = sys.getsizeof(o)
        # add GC header size
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
        vh = self.vheader
        size = self.calcsize
        gc_header_size = self.gc_headsize
        # bool objects are not gc tracked
        self.assertEqual(sys.getsizeof(True), size(vh) + self.longdigit)
        # but lists are
        self.assertEqual(sys.getsizeof([]), size(vh + 'PP') + gc_header_size)

    def test_default(self):
        h = self.header
        vh = self.vheader
        size = self.calcsize
        self.assertEqual(sys.getsizeof(True), size(vh) + self.longdigit)
        self.assertEqual(sys.getsizeof(True, -1), size(vh) + self.longdigit)

    def test_objecttypes(self):
        # check all types defined in Objects/
        h = self.header
        vh = self.vheader
        size = self.calcsize
        check = self.check_sizeof
        # bool
        check(True, size(vh) + self.longdigit)
        # buffer
        # XXX
        # builtin_function_or_method
        check(len, size(h + '3P'))
        # bytearray
        samples = [b'', b'u'*100000]
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
        check(get_cell().__closure__[0], size(h + 'P'))
        # code
        check(get_cell().__code__, size(h + '5i8Pi2P'))
        # complex
        check(complex(0,1), size(h + '2d'))
        # method_descriptor (descriptor object)
        check(str.lower, size(h + '2PP'))
        # classmethod_descriptor (descriptor object)
        # XXX
        # member_descriptor (descriptor object)
        import datetime
        check(datetime.timedelta.days, size(h + '2PP'))
        # getset_descriptor (descriptor object)
        import collections
        check(collections.defaultdict.default_factory, size(h + '2PP'))
        # wrapper_descriptor (descriptor object)
        check(int.__add__, size(h + '2P2P'))
        # method-wrapper (descriptor object)
        check({}.__iter__, size(h + '2P'))
        # dict
        check({}, size(h + '3P2P' + 8*'P2P'))
        longdict = {1:1, 2:2, 3:3, 4:4, 5:5, 6:6, 7:7, 8:8}
        check(longdict, size(h + '3P2P' + 8*'P2P') + 16*size('P2P'))
        # dictionary-keyiterator
        check({}.keys(), size(h + 'P'))
        # dictionary-valueiterator
        check({}.values(), size(h + 'P'))
        # dictionary-itemiterator
        check({}.items(), size(h + 'P'))
        # dictproxy
        class C(object): pass
        check(C.__dict__, size(h + 'P'))
        # BaseException
        check(BaseException(), size(h + '5P'))
        # UnicodeEncodeError
        check(UnicodeEncodeError("", "", 0, 0, ""), size(h + '5P 2P2PP'))
        # UnicodeDecodeError
        # XXX
#        check(UnicodeDecodeError("", "", 0, 0, ""), size(h + '5P2PP'))
        # UnicodeTranslateError
        check(UnicodeTranslateError("", 0, 1, ""), size(h + '5P 2P2PP'))
        # ellipses
        check(Ellipsis, size(h + ''))
        # EncodingMap
        import codecs, encodings.iso8859_3
        x = codecs.charmap_build(encodings.iso8859_3.decoding_table)
        check(x, size(h + '32B2iB'))
        # enumerate
        check(enumerate([]), size(h + 'l3P'))
        # reverse
        check(reversed(''), size(h + 'PP'))
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
        check(func, size(h + '11P'))
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
        check(0, size(vh))
        check(1, size(vh) + self.longdigit)
        check(-1, size(vh) + self.longdigit)
        PyLong_BASE = 2**sys.int_info.bits_per_digit
        check(int(PyLong_BASE), size(vh) + 2*self.longdigit)
        check(int(PyLong_BASE**2-1), size(vh) + 2*self.longdigit)
        check(int(PyLong_BASE**2), size(vh) + 3*self.longdigit)
        # memory
        check(memoryview(b''), size(h + 'P PP2P2i7P'))
        # module
        check(unittest, size(h + '3P'))
        # None
        check(None, size(h + ''))
        # NotImplementedType
        check(NotImplemented, size(h))
        # object
        check(object(), size(h + ''))
        # property (descriptor object)
        class C(object):
            def getx(self): return self.__x
            def setx(self, value): self.__x = value
            def delx(self): del self.__x
            x = property(getx, setx, delx, "")
            check(x, size(h + '4Pi'))
        # PyCapsule
        # XXX
        # rangeiterator
        check(iter(range(1)), size(h + '4l'))
        # reverse
        check(reversed(''), size(h + 'PP'))
        # range
        check(range(1), size(h + '3P'))
        check(range(66000), size(h + '3P'))
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
        check(slice(0), size(h + '3P'))
        # super
        check(super(int), size(h + '3P'))
        # tuple
        check((), size(vh))
        check((1,2,3), size(vh) + 3*self.P)
        # type
        # (PyTypeObject + PyNumberMethods + PyMappingMethods +
        #  PySequenceMethods + PyBufferProcs)
        s = size(vh + 'P2P15Pl4PP9PP11PI') + size('16Pi17P 3P 10P 2P 2P')
        check(int, s)
        # class
        class newstyleclass(object): pass
        check(newstyleclass, s)
        # unicode
        usize = len('\0'.encode('unicode-internal'))
        samples = ['', '1'*100]
        # we need to test for both sizes, because we don't know if the string
        # has been cached
        for s in samples:
            basicsize =  size(h + 'PPliP') + usize * (len(s) + 1)
            check(s, basicsize)
        # weakref
        import weakref
        check(weakref.ref(int), size(h + '2Pl2P'))
        # weakproxy
        # XXX
        # weakcallableproxy
        check(weakref.proxy(int), size(h + '2Pl2P'))

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

    def test_getfilesystemencoding(self):
        import codecs

        def check_fsencoding(fs_encoding):
            if sys.platform == 'darwin':
                self.assertEqual(fs_encoding, 'utf-8')
            elif fs_encoding is None:
                return
            codecs.lookup(fs_encoding)

        fs_encoding = sys.getfilesystemencoding()
        check_fsencoding(fs_encoding)

        # Even in C locale
        try:
            sys.executable.encode('ascii')
        except UnicodeEncodeError:
            # Python doesn't start with ASCII locale if its path is not ASCII,
            # see issue #8611
            pass
        else:
            env = os.environ.copy()
            env['LANG'] = 'C'
            output = subprocess.check_output(
                [sys.executable, "-c",
                 "import sys; print(sys.getfilesystemencoding())"],
                env=env)
            fs_encoding = output.rstrip().decode('ascii')
            check_fsencoding(fs_encoding)

    def test_setfilesystemencoding(self):
        old = sys.getfilesystemencoding()
        try:
            sys.setfilesystemencoding("iso-8859-1")
            self.assertEqual(sys.getfilesystemencoding(), "iso-8859-1")
        finally:
            sys.setfilesystemencoding(old)
        try:
            self.assertRaises(LookupError, sys.setfilesystemencoding, "xxx")
        finally:
            sys.setfilesystemencoding(old)

def test_main():
    test.support.run_unittest(SysModuleTest, SizeofTest)

if __name__ == "__main__":
    test_main()
