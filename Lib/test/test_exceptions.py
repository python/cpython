# Python test set -- part 5, built-in exceptions

import copy
import gc
import os
import sys
import unittest
import pickle
import weakref
import errno
from textwrap import dedent

from test.support import (captured_stderr, check_impl_detail,
                          cpython_only, gc_collect,
                          no_tracing, script_helper,
                          SuppressCrashReport)
from test.support.import_helper import import_module
from test.support.os_helper import TESTFN, unlink
from test.support.warnings_helper import check_warnings
from test import support


class NaiveException(Exception):
    def __init__(self, x):
        self.x = x

class SlottedNaiveException(Exception):
    __slots__ = ('x',)
    def __init__(self, x):
        self.x = x

class BrokenStrException(Exception):
    def __str__(self):
        raise Exception("str() is broken")

# XXX This is not really enough, each *operation* should be tested!

class ExceptionTests(unittest.TestCase):

    def raise_catch(self, exc, excname):
        with self.subTest(exc=exc, excname=excname):
            try:
                raise exc("spam")
            except exc as err:
                buf1 = str(err)
            try:
                raise exc("spam")
            except exc as err:
                buf2 = str(err)
            self.assertEqual(buf1, buf2)
            self.assertEqual(exc.__name__, excname)

    def testRaising(self):
        self.raise_catch(AttributeError, "AttributeError")
        self.assertRaises(AttributeError, getattr, sys, "undefined_attribute")

        self.raise_catch(EOFError, "EOFError")
        fp = open(TESTFN, 'w', encoding="utf-8")
        fp.close()
        fp = open(TESTFN, 'r', encoding="utf-8")
        savestdin = sys.stdin
        try:
            try:
                import marshal
                marshal.loads(b'')
            except EOFError:
                pass
        finally:
            sys.stdin = savestdin
            fp.close()
            unlink(TESTFN)

        self.raise_catch(OSError, "OSError")
        self.assertRaises(OSError, open, 'this file does not exist', 'r')

        self.raise_catch(ImportError, "ImportError")
        self.assertRaises(ImportError, __import__, "undefined_module")

        self.raise_catch(IndexError, "IndexError")
        x = []
        self.assertRaises(IndexError, x.__getitem__, 10)

        self.raise_catch(KeyError, "KeyError")
        x = {}
        self.assertRaises(KeyError, x.__getitem__, 'key')

        self.raise_catch(KeyboardInterrupt, "KeyboardInterrupt")

        self.raise_catch(MemoryError, "MemoryError")

        self.raise_catch(NameError, "NameError")
        try: x = undefined_variable
        except NameError: pass

        self.raise_catch(OverflowError, "OverflowError")
        x = 1
        for dummy in range(128):
            x += x  # this simply shouldn't blow up

        self.raise_catch(RuntimeError, "RuntimeError")
        self.raise_catch(RecursionError, "RecursionError")

        self.raise_catch(SyntaxError, "SyntaxError")
        try: exec('/\n')
        except SyntaxError: pass

        self.raise_catch(IndentationError, "IndentationError")

        self.raise_catch(TabError, "TabError")
        try: compile("try:\n\t1/0\n    \t1/0\nfinally:\n pass\n",
                     '<string>', 'exec')
        except TabError: pass
        else: self.fail("TabError not raised")

        self.raise_catch(SystemError, "SystemError")

        self.raise_catch(SystemExit, "SystemExit")
        self.assertRaises(SystemExit, sys.exit, 0)

        self.raise_catch(TypeError, "TypeError")
        try: [] + ()
        except TypeError: pass

        self.raise_catch(ValueError, "ValueError")
        self.assertRaises(ValueError, chr, 17<<16)

        self.raise_catch(ZeroDivisionError, "ZeroDivisionError")
        try: x = 1/0
        except ZeroDivisionError: pass

        self.raise_catch(Exception, "Exception")
        try: x = 1/0
        except Exception as e: pass

        self.raise_catch(StopAsyncIteration, "StopAsyncIteration")

    def testSyntaxErrorMessage(self):
        # make sure the right exception message is raised for each of
        # these code fragments

        def ckmsg(src, msg):
            with self.subTest(src=src, msg=msg):
                try:
                    compile(src, '<fragment>', 'exec')
                except SyntaxError as e:
                    if e.msg != msg:
                        self.fail("expected %s, got %s" % (msg, e.msg))
                else:
                    self.fail("failed to get expected SyntaxError")

        s = '''if 1:
        try:
            continue
        except:
            pass'''

        ckmsg(s, "'continue' not properly in loop")
        ckmsg("continue\n", "'continue' not properly in loop")

    def testSyntaxErrorMissingParens(self):
        def ckmsg(src, msg, exception=SyntaxError):
            try:
                compile(src, '<fragment>', 'exec')
            except exception as e:
                if e.msg != msg:
                    self.fail("expected %s, got %s" % (msg, e.msg))
            else:
                self.fail("failed to get expected SyntaxError")

        s = '''print "old style"'''
        ckmsg(s, "Missing parentheses in call to 'print'. Did you mean print(...)?")

        s = '''print "old style",'''
        ckmsg(s, "Missing parentheses in call to 'print'. Did you mean print(...)?")

        s = 'print f(a+b,c)'
        ckmsg(s, "Missing parentheses in call to 'print'. Did you mean print(...)?")

        s = '''exec "old style"'''
        ckmsg(s, "Missing parentheses in call to 'exec'. Did you mean exec(...)?")

        s = 'exec f(a+b,c)'
        ckmsg(s, "Missing parentheses in call to 'exec'. Did you mean exec(...)?")

        # should not apply to subclasses, see issue #31161
        s = '''if True:\nprint "No indent"'''
        ckmsg(s, "expected an indented block after 'if' statement on line 1", IndentationError)

        s = '''if True:\n        print()\n\texec "mixed tabs and spaces"'''
        ckmsg(s, "inconsistent use of tabs and spaces in indentation", TabError)

    def check(self, src, lineno, offset, encoding='utf-8'):
        with self.subTest(source=src, lineno=lineno, offset=offset):
            with self.assertRaises(SyntaxError) as cm:
                compile(src, '<fragment>', 'exec')
            self.assertEqual(cm.exception.lineno, lineno)
            self.assertEqual(cm.exception.offset, offset)
            if cm.exception.text is not None:
                if not isinstance(src, str):
                    src = src.decode(encoding, 'replace')
                line = src.split('\n')[lineno-1]
                self.assertIn(line, cm.exception.text)

    def testSyntaxErrorOffset(self):
        check = self.check
        check('def fact(x):\n\treturn x!\n', 2, 10)
        check('1 +\n', 1, 4)
        check('def spam():\n  print(1)\n print(2)', 3, 10)
        check('Python = "Python" +', 1, 20)
        check('Python = "\u1e54\xfd\u0163\u0125\xf2\xf1" +', 1, 20)
        check(b'# -*- coding: cp1251 -*-\nPython = "\xcf\xb3\xf2\xee\xed" +',
              2, 19, encoding='cp1251')
        check(b'Python = "\xcf\xb3\xf2\xee\xed" +', 1, 18)
        check('x = "a', 1, 5)
        check('lambda x: x = 2', 1, 1)
        check('f{a + b + c}', 1, 2)
        check('[file for str(file) in []\n])', 2, 2)
        check('a = « hello » « world »', 1, 5)
        check('[\nfile\nfor str(file)\nin\n[]\n]', 3, 5)
        check('[file for\n str(file) in []]', 2, 2)
        check("ages = {'Alice'=22, 'Bob'=23}", 1, 16)
        check('match ...:\n    case {**rest, "key": value}:\n        ...', 2, 19)

        # Errors thrown by compile.c
        check('class foo:return 1', 1, 11)
        check('def f():\n  continue', 2, 3)
        check('def f():\n  break', 2, 3)
        check('try:\n  pass\nexcept:\n  pass\nexcept ValueError:\n  pass', 3, 1)

        # Errors thrown by tokenizer.c
        check('(0x+1)', 1, 3)
        check('x = 0xI', 1, 6)
        check('0010 + 2', 1, 1)
        check('x = 32e-+4', 1, 8)
        check('x = 0o9', 1, 7)
        check('\u03b1 = 0xI', 1, 6)
        check(b'\xce\xb1 = 0xI', 1, 6)
        check(b'# -*- coding: iso8859-7 -*-\n\xe1 = 0xI', 2, 6,
              encoding='iso8859-7')
        check(b"""if 1:
            def foo():
                '''

            def bar():
                pass

            def baz():
                '''quux'''
            """, 9, 24)
        check("pass\npass\npass\n(1+)\npass\npass\npass", 4, 4)
        check("(1+)", 1, 4)
        check("[interesting\nfoo()\n", 1, 1)
        check(b"\xef\xbb\xbf#coding: utf8\nprint('\xe6\x88\x91')\n", 0, -1)

        # Errors thrown by symtable.c
        check('x = [(yield i) for i in range(3)]', 1, 5)
        check('def f():\n  from _ import *', 1, 1)
        check('def f(x, x):\n  pass', 1, 1)
        check('def f(x):\n  nonlocal x', 2, 3)
        check('def f(x):\n  x = 1\n  global x', 3, 3)
        check('nonlocal x', 1, 1)
        check('def f():\n  global x\n  nonlocal x', 2, 3)

        # Errors thrown by future.c
        check('from __future__ import doesnt_exist', 1, 1)
        check('from __future__ import braces', 1, 1)
        check('x=1\nfrom __future__ import division', 2, 1)
        check('foo(1=2)', 1, 5)
        check('def f():\n  x, y: int', 2, 3)
        check('[*x for x in xs]', 1, 2)
        check('foo(x for x in range(10), 100)', 1, 5)
        check('for 1 in []: pass', 1, 5)
        check('(yield i) = 2', 1, 2)
        check('def f(*):\n  pass', 1, 7)

    @cpython_only
    def testSettingException(self):
        # test that setting an exception at the C level works even if the
        # exception object can't be constructed.

        class BadException(Exception):
            def __init__(self_):
                raise RuntimeError("can't instantiate BadException")

        class InvalidException:
            pass

        def test_capi1():
            import _testcapi
            try:
                _testcapi.raise_exception(BadException, 1)
            except TypeError as err:
                exc, err, tb = sys.exc_info()
                co = tb.tb_frame.f_code
                self.assertEqual(co.co_name, "test_capi1")
                self.assertTrue(co.co_filename.endswith('test_exceptions.py'))
            else:
                self.fail("Expected exception")

        def test_capi2():
            import _testcapi
            try:
                _testcapi.raise_exception(BadException, 0)
            except RuntimeError as err:
                exc, err, tb = sys.exc_info()
                co = tb.tb_frame.f_code
                self.assertEqual(co.co_name, "__init__")
                self.assertTrue(co.co_filename.endswith('test_exceptions.py'))
                co2 = tb.tb_frame.f_back.f_code
                self.assertEqual(co2.co_name, "test_capi2")
            else:
                self.fail("Expected exception")

        def test_capi3():
            import _testcapi
            self.assertRaises(SystemError, _testcapi.raise_exception,
                              InvalidException, 1)

        if not sys.platform.startswith('java'):
            test_capi1()
            test_capi2()
            test_capi3()

    def test_WindowsError(self):
        try:
            WindowsError
        except NameError:
            pass
        else:
            self.assertIs(WindowsError, OSError)
            self.assertEqual(str(OSError(1001)), "1001")
            self.assertEqual(str(OSError(1001, "message")),
                             "[Errno 1001] message")
            # POSIX errno (9 aka EBADF) is untranslated
            w = OSError(9, 'foo', 'bar')
            self.assertEqual(w.errno, 9)
            self.assertEqual(w.winerror, None)
            self.assertEqual(str(w), "[Errno 9] foo: 'bar'")
            # ERROR_PATH_NOT_FOUND (win error 3) becomes ENOENT (2)
            w = OSError(0, 'foo', 'bar', 3)
            self.assertEqual(w.errno, 2)
            self.assertEqual(w.winerror, 3)
            self.assertEqual(w.strerror, 'foo')
            self.assertEqual(w.filename, 'bar')
            self.assertEqual(w.filename2, None)
            self.assertEqual(str(w), "[WinError 3] foo: 'bar'")
            # Unknown win error becomes EINVAL (22)
            w = OSError(0, 'foo', None, 1001)
            self.assertEqual(w.errno, 22)
            self.assertEqual(w.winerror, 1001)
            self.assertEqual(w.strerror, 'foo')
            self.assertEqual(w.filename, None)
            self.assertEqual(w.filename2, None)
            self.assertEqual(str(w), "[WinError 1001] foo")
            # Non-numeric "errno"
            w = OSError('bar', 'foo')
            self.assertEqual(w.errno, 'bar')
            self.assertEqual(w.winerror, None)
            self.assertEqual(w.strerror, 'foo')
            self.assertEqual(w.filename, None)
            self.assertEqual(w.filename2, None)

    @unittest.skipUnless(sys.platform == 'win32',
                         'test specific to Windows')
    def test_windows_message(self):
        """Should fill in unknown error code in Windows error message"""
        ctypes = import_module('ctypes')
        # this error code has no message, Python formats it as hexadecimal
        code = 3765269347
        with self.assertRaisesRegex(OSError, 'Windows Error 0x%x' % code):
            ctypes.pythonapi.PyErr_SetFromWindowsErr(code)

    def testAttributes(self):
        # test that exception attributes are happy

        exceptionList = [
            (BaseException, (), {'args' : ()}),
            (BaseException, (1, ), {'args' : (1,)}),
            (BaseException, ('foo',),
                {'args' : ('foo',)}),
            (BaseException, ('foo', 1),
                {'args' : ('foo', 1)}),
            (SystemExit, ('foo',),
                {'args' : ('foo',), 'code' : 'foo'}),
            (OSError, ('foo',),
                {'args' : ('foo',), 'filename' : None, 'filename2' : None,
                 'errno' : None, 'strerror' : None}),
            (OSError, ('foo', 'bar'),
                {'args' : ('foo', 'bar'),
                 'filename' : None, 'filename2' : None,
                 'errno' : 'foo', 'strerror' : 'bar'}),
            (OSError, ('foo', 'bar', 'baz'),
                {'args' : ('foo', 'bar'),
                 'filename' : 'baz', 'filename2' : None,
                 'errno' : 'foo', 'strerror' : 'bar'}),
            (OSError, ('foo', 'bar', 'baz', None, 'quux'),
                {'args' : ('foo', 'bar'), 'filename' : 'baz', 'filename2': 'quux'}),
            (OSError, ('errnoStr', 'strErrorStr', 'filenameStr'),
                {'args' : ('errnoStr', 'strErrorStr'),
                 'strerror' : 'strErrorStr', 'errno' : 'errnoStr',
                 'filename' : 'filenameStr'}),
            (OSError, (1, 'strErrorStr', 'filenameStr'),
                {'args' : (1, 'strErrorStr'), 'errno' : 1,
                 'strerror' : 'strErrorStr',
                 'filename' : 'filenameStr', 'filename2' : None}),
            (SyntaxError, (), {'msg' : None, 'text' : None,
                'filename' : None, 'lineno' : None, 'offset' : None,
                'end_offset': None, 'print_file_and_line' : None}),
            (SyntaxError, ('msgStr',),
                {'args' : ('msgStr',), 'text' : None,
                 'print_file_and_line' : None, 'msg' : 'msgStr',
                 'filename' : None, 'lineno' : None, 'offset' : None,
                 'end_offset': None}),
            (SyntaxError, ('msgStr', ('filenameStr', 'linenoStr', 'offsetStr',
                           'textStr', 'endLinenoStr', 'endOffsetStr')),
                {'offset' : 'offsetStr', 'text' : 'textStr',
                 'args' : ('msgStr', ('filenameStr', 'linenoStr',
                                      'offsetStr', 'textStr',
                                      'endLinenoStr', 'endOffsetStr')),
                 'print_file_and_line' : None, 'msg' : 'msgStr',
                 'filename' : 'filenameStr', 'lineno' : 'linenoStr',
                 'end_lineno': 'endLinenoStr', 'end_offset': 'endOffsetStr'}),
            (SyntaxError, ('msgStr', 'filenameStr', 'linenoStr', 'offsetStr',
                           'textStr', 'endLinenoStr', 'endOffsetStr',
                           'print_file_and_lineStr'),
                {'text' : None,
                 'args' : ('msgStr', 'filenameStr', 'linenoStr', 'offsetStr',
                           'textStr', 'endLinenoStr', 'endOffsetStr',
                           'print_file_and_lineStr'),
                 'print_file_and_line' : None, 'msg' : 'msgStr',
                 'filename' : None, 'lineno' : None, 'offset' : None,
                 'end_lineno': None, 'end_offset': None}),
            (UnicodeError, (), {'args' : (),}),
            (UnicodeEncodeError, ('ascii', 'a', 0, 1,
                                  'ordinal not in range'),
                {'args' : ('ascii', 'a', 0, 1,
                                           'ordinal not in range'),
                 'encoding' : 'ascii', 'object' : 'a',
                 'start' : 0, 'reason' : 'ordinal not in range'}),
            (UnicodeDecodeError, ('ascii', bytearray(b'\xff'), 0, 1,
                                  'ordinal not in range'),
                {'args' : ('ascii', bytearray(b'\xff'), 0, 1,
                                           'ordinal not in range'),
                 'encoding' : 'ascii', 'object' : b'\xff',
                 'start' : 0, 'reason' : 'ordinal not in range'}),
            (UnicodeDecodeError, ('ascii', b'\xff', 0, 1,
                                  'ordinal not in range'),
                {'args' : ('ascii', b'\xff', 0, 1,
                                           'ordinal not in range'),
                 'encoding' : 'ascii', 'object' : b'\xff',
                 'start' : 0, 'reason' : 'ordinal not in range'}),
            (UnicodeTranslateError, ("\u3042", 0, 1, "ouch"),
                {'args' : ('\u3042', 0, 1, 'ouch'),
                 'object' : '\u3042', 'reason' : 'ouch',
                 'start' : 0, 'end' : 1}),
            (NaiveException, ('foo',),
                {'args': ('foo',), 'x': 'foo'}),
            (SlottedNaiveException, ('foo',),
                {'args': ('foo',), 'x': 'foo'}),
        ]
        try:
            # More tests are in test_WindowsError
            exceptionList.append(
                (WindowsError, (1, 'strErrorStr', 'filenameStr'),
                    {'args' : (1, 'strErrorStr'),
                     'strerror' : 'strErrorStr', 'winerror' : None,
                     'errno' : 1,
                     'filename' : 'filenameStr', 'filename2' : None})
            )
        except NameError:
            pass

        for exc, args, expected in exceptionList:
            try:
                e = exc(*args)
            except:
                print("\nexc=%r, args=%r" % (exc, args), file=sys.stderr)
                # raise
            else:
                # Verify module name
                if not type(e).__name__.endswith('NaiveException'):
                    self.assertEqual(type(e).__module__, 'builtins')
                # Verify no ref leaks in Exc_str()
                s = str(e)
                for checkArgName in expected:
                    value = getattr(e, checkArgName)
                    self.assertEqual(repr(value),
                                     repr(expected[checkArgName]),
                                     '%r.%s == %r, expected %r' % (
                                     e, checkArgName,
                                     value, expected[checkArgName]))

                # test for pickling support
                for p in [pickle]:
                    for protocol in range(p.HIGHEST_PROTOCOL + 1):
                        s = p.dumps(e, protocol)
                        new = p.loads(s)
                        for checkArgName in expected:
                            got = repr(getattr(new, checkArgName))
                            want = repr(expected[checkArgName])
                            self.assertEqual(got, want,
                                             'pickled "%r", attribute "%s' %
                                             (e, checkArgName))

    def testWithTraceback(self):
        try:
            raise IndexError(4)
        except:
            tb = sys.exc_info()[2]

        e = BaseException().with_traceback(tb)
        self.assertIsInstance(e, BaseException)
        self.assertEqual(e.__traceback__, tb)

        e = IndexError(5).with_traceback(tb)
        self.assertIsInstance(e, IndexError)
        self.assertEqual(e.__traceback__, tb)

        class MyException(Exception):
            pass

        e = MyException().with_traceback(tb)
        self.assertIsInstance(e, MyException)
        self.assertEqual(e.__traceback__, tb)

    def testInvalidTraceback(self):
        try:
            Exception().__traceback__ = 5
        except TypeError as e:
            self.assertIn("__traceback__ must be a traceback", str(e))
        else:
            self.fail("No exception raised")

    def testInvalidAttrs(self):
        self.assertRaises(TypeError, setattr, Exception(), '__cause__', 1)
        self.assertRaises(TypeError, delattr, Exception(), '__cause__')
        self.assertRaises(TypeError, setattr, Exception(), '__context__', 1)
        self.assertRaises(TypeError, delattr, Exception(), '__context__')

    def testNoneClearsTracebackAttr(self):
        try:
            raise IndexError(4)
        except:
            tb = sys.exc_info()[2]

        e = Exception()
        e.__traceback__ = tb
        e.__traceback__ = None
        self.assertEqual(e.__traceback__, None)

    def testChainingAttrs(self):
        e = Exception()
        self.assertIsNone(e.__context__)
        self.assertIsNone(e.__cause__)

        e = TypeError()
        self.assertIsNone(e.__context__)
        self.assertIsNone(e.__cause__)

        class MyException(OSError):
            pass

        e = MyException()
        self.assertIsNone(e.__context__)
        self.assertIsNone(e.__cause__)

    def testChainingDescriptors(self):
        try:
            raise Exception()
        except Exception as exc:
            e = exc

        self.assertIsNone(e.__context__)
        self.assertIsNone(e.__cause__)
        self.assertFalse(e.__suppress_context__)

        e.__context__ = NameError()
        e.__cause__ = None
        self.assertIsInstance(e.__context__, NameError)
        self.assertIsNone(e.__cause__)
        self.assertTrue(e.__suppress_context__)
        e.__suppress_context__ = False
        self.assertFalse(e.__suppress_context__)

    def testKeywordArgs(self):
        # test that builtin exception don't take keyword args,
        # but user-defined subclasses can if they want
        self.assertRaises(TypeError, BaseException, a=1)

        class DerivedException(BaseException):
            def __init__(self, fancy_arg):
                BaseException.__init__(self)
                self.fancy_arg = fancy_arg

        x = DerivedException(fancy_arg=42)
        self.assertEqual(x.fancy_arg, 42)

    @no_tracing
    def testInfiniteRecursion(self):
        def f():
            return f()
        self.assertRaises(RecursionError, f)

        def g():
            try:
                return g()
            except ValueError:
                return -1
        self.assertRaises(RecursionError, g)

    def test_str(self):
        # Make sure both instances and classes have a str representation.
        self.assertTrue(str(Exception))
        self.assertTrue(str(Exception('a')))
        self.assertTrue(str(Exception('a', 'b')))

    def testExceptionCleanupNames(self):
        # Make sure the local variable bound to the exception instance by
        # an "except" statement is only visible inside the except block.
        try:
            raise Exception()
        except Exception as e:
            self.assertTrue(e)
            del e
        self.assertNotIn('e', locals())

    def testExceptionCleanupState(self):
        # Make sure exception state is cleaned up as soon as the except
        # block is left. See #2507

        class MyException(Exception):
            def __init__(self, obj):
                self.obj = obj
        class MyObj:
            pass

        def inner_raising_func():
            # Create some references in exception value and traceback
            local_ref = obj
            raise MyException(obj)

        # Qualified "except" with "as"
        obj = MyObj()
        wr = weakref.ref(obj)
        try:
            inner_raising_func()
        except MyException as e:
            pass
        obj = None
        obj = wr()
        self.assertIsNone(obj)

        # Qualified "except" without "as"
        obj = MyObj()
        wr = weakref.ref(obj)
        try:
            inner_raising_func()
        except MyException:
            pass
        obj = None
        obj = wr()
        self.assertIsNone(obj)

        # Bare "except"
        obj = MyObj()
        wr = weakref.ref(obj)
        try:
            inner_raising_func()
        except:
            pass
        obj = None
        obj = wr()
        self.assertIsNone(obj)

        # "except" with premature block leave
        obj = MyObj()
        wr = weakref.ref(obj)
        for i in [0]:
            try:
                inner_raising_func()
            except:
                break
        obj = None
        obj = wr()
        self.assertIsNone(obj)

        # "except" block raising another exception
        obj = MyObj()
        wr = weakref.ref(obj)
        try:
            try:
                inner_raising_func()
            except:
                raise KeyError
        except KeyError as e:
            # We want to test that the except block above got rid of
            # the exception raised in inner_raising_func(), but it
            # also ends up in the __context__ of the KeyError, so we
            # must clear the latter manually for our test to succeed.
            e.__context__ = None
            obj = None
            obj = wr()
            # guarantee no ref cycles on CPython (don't gc_collect)
            if check_impl_detail(cpython=False):
                gc_collect()
            self.assertIsNone(obj)

        # Some complicated construct
        obj = MyObj()
        wr = weakref.ref(obj)
        try:
            inner_raising_func()
        except MyException:
            try:
                try:
                    raise
                finally:
                    raise
            except MyException:
                pass
        obj = None
        if check_impl_detail(cpython=False):
            gc_collect()
        obj = wr()
        self.assertIsNone(obj)

        # Inside an exception-silencing "with" block
        class Context:
            def __enter__(self):
                return self
            def __exit__ (self, exc_type, exc_value, exc_tb):
                return True
        obj = MyObj()
        wr = weakref.ref(obj)
        with Context():
            inner_raising_func()
        obj = None
        if check_impl_detail(cpython=False):
            gc_collect()
        obj = wr()
        self.assertIsNone(obj)

    def test_exception_target_in_nested_scope(self):
        # issue 4617: This used to raise a SyntaxError
        # "can not delete variable 'e' referenced in nested scope"
        def print_error():
            e
        try:
            something
        except Exception as e:
            print_error()
            # implicit "del e" here

    def test_generator_leaking(self):
        # Test that generator exception state doesn't leak into the calling
        # frame
        def yield_raise():
            try:
                raise KeyError("caught")
            except KeyError:
                yield sys.exc_info()[0]
                yield sys.exc_info()[0]
            yield sys.exc_info()[0]
        g = yield_raise()
        self.assertEqual(next(g), KeyError)
        self.assertEqual(sys.exc_info()[0], None)
        self.assertEqual(next(g), KeyError)
        self.assertEqual(sys.exc_info()[0], None)
        self.assertEqual(next(g), None)

        # Same test, but inside an exception handler
        try:
            raise TypeError("foo")
        except TypeError:
            g = yield_raise()
            self.assertEqual(next(g), KeyError)
            self.assertEqual(sys.exc_info()[0], TypeError)
            self.assertEqual(next(g), KeyError)
            self.assertEqual(sys.exc_info()[0], TypeError)
            self.assertEqual(next(g), TypeError)
            del g
            self.assertEqual(sys.exc_info()[0], TypeError)

    def test_generator_leaking2(self):
        # See issue 12475.
        def g():
            yield
        try:
            raise RuntimeError
        except RuntimeError:
            it = g()
            next(it)
        try:
            next(it)
        except StopIteration:
            pass
        self.assertEqual(sys.exc_info(), (None, None, None))

    def test_generator_leaking3(self):
        # See issue #23353.  When gen.throw() is called, the caller's
        # exception state should be save and restored.
        def g():
            try:
                yield
            except ZeroDivisionError:
                yield sys.exc_info()[1]
        it = g()
        next(it)
        try:
            1/0
        except ZeroDivisionError as e:
            self.assertIs(sys.exc_info()[1], e)
            gen_exc = it.throw(e)
            self.assertIs(sys.exc_info()[1], e)
            self.assertIs(gen_exc, e)
        self.assertEqual(sys.exc_info(), (None, None, None))

    def test_generator_leaking4(self):
        # See issue #23353.  When an exception is raised by a generator,
        # the caller's exception state should still be restored.
        def g():
            try:
                1/0
            except ZeroDivisionError:
                yield sys.exc_info()[0]
                raise
        it = g()
        try:
            raise TypeError
        except TypeError:
            # The caller's exception state (TypeError) is temporarily
            # saved in the generator.
            tp = next(it)
        self.assertIs(tp, ZeroDivisionError)
        try:
            next(it)
            # We can't check it immediately, but while next() returns
            # with an exception, it shouldn't have restored the old
            # exception state (TypeError).
        except ZeroDivisionError as e:
            self.assertIs(sys.exc_info()[1], e)
        # We used to find TypeError here.
        self.assertEqual(sys.exc_info(), (None, None, None))

    def test_generator_doesnt_retain_old_exc(self):
        def g():
            self.assertIsInstance(sys.exc_info()[1], RuntimeError)
            yield
            self.assertEqual(sys.exc_info(), (None, None, None))
        it = g()
        try:
            raise RuntimeError
        except RuntimeError:
            next(it)
        self.assertRaises(StopIteration, next, it)

    def test_generator_finalizing_and_exc_info(self):
        # See #7173
        def simple_gen():
            yield 1
        def run_gen():
            gen = simple_gen()
            try:
                raise RuntimeError
            except RuntimeError:
                return next(gen)
        run_gen()
        gc_collect()
        self.assertEqual(sys.exc_info(), (None, None, None))

    def _check_generator_cleanup_exc_state(self, testfunc):
        # Issue #12791: exception state is cleaned up as soon as a generator
        # is closed (reference cycles are broken).
        class MyException(Exception):
            def __init__(self, obj):
                self.obj = obj
        class MyObj:
            pass

        def raising_gen():
            try:
                raise MyException(obj)
            except MyException:
                yield

        obj = MyObj()
        wr = weakref.ref(obj)
        g = raising_gen()
        next(g)
        testfunc(g)
        g = obj = None
        obj = wr()
        self.assertIsNone(obj)

    def test_generator_throw_cleanup_exc_state(self):
        def do_throw(g):
            try:
                g.throw(RuntimeError())
            except RuntimeError:
                pass
        self._check_generator_cleanup_exc_state(do_throw)

    def test_generator_close_cleanup_exc_state(self):
        def do_close(g):
            g.close()
        self._check_generator_cleanup_exc_state(do_close)

    def test_generator_del_cleanup_exc_state(self):
        def do_del(g):
            g = None
        self._check_generator_cleanup_exc_state(do_del)

    def test_generator_next_cleanup_exc_state(self):
        def do_next(g):
            try:
                next(g)
            except StopIteration:
                pass
            else:
                self.fail("should have raised StopIteration")
        self._check_generator_cleanup_exc_state(do_next)

    def test_generator_send_cleanup_exc_state(self):
        def do_send(g):
            try:
                g.send(None)
            except StopIteration:
                pass
            else:
                self.fail("should have raised StopIteration")
        self._check_generator_cleanup_exc_state(do_send)

    def test_3114(self):
        # Bug #3114: in its destructor, MyObject retrieves a pointer to
        # obsolete and/or deallocated objects.
        class MyObject:
            def __del__(self):
                nonlocal e
                e = sys.exc_info()
        e = ()
        try:
            raise Exception(MyObject())
        except:
            pass
        self.assertEqual(e, (None, None, None))

    def test_unicode_change_attributes(self):
        # See issue 7309. This was a crasher.

        u = UnicodeEncodeError('baz', 'xxxxx', 1, 5, 'foo')
        self.assertEqual(str(u), "'baz' codec can't encode characters in position 1-4: foo")
        u.end = 2
        self.assertEqual(str(u), "'baz' codec can't encode character '\\x78' in position 1: foo")
        u.end = 5
        u.reason = 0x345345345345345345
        self.assertEqual(str(u), "'baz' codec can't encode characters in position 1-4: 965230951443685724997")
        u.encoding = 4000
        self.assertEqual(str(u), "'4000' codec can't encode characters in position 1-4: 965230951443685724997")
        u.start = 1000
        self.assertEqual(str(u), "'4000' codec can't encode characters in position 1000-4: 965230951443685724997")

        u = UnicodeDecodeError('baz', b'xxxxx', 1, 5, 'foo')
        self.assertEqual(str(u), "'baz' codec can't decode bytes in position 1-4: foo")
        u.end = 2
        self.assertEqual(str(u), "'baz' codec can't decode byte 0x78 in position 1: foo")
        u.end = 5
        u.reason = 0x345345345345345345
        self.assertEqual(str(u), "'baz' codec can't decode bytes in position 1-4: 965230951443685724997")
        u.encoding = 4000
        self.assertEqual(str(u), "'4000' codec can't decode bytes in position 1-4: 965230951443685724997")
        u.start = 1000
        self.assertEqual(str(u), "'4000' codec can't decode bytes in position 1000-4: 965230951443685724997")

        u = UnicodeTranslateError('xxxx', 1, 5, 'foo')
        self.assertEqual(str(u), "can't translate characters in position 1-4: foo")
        u.end = 2
        self.assertEqual(str(u), "can't translate character '\\x78' in position 1: foo")
        u.end = 5
        u.reason = 0x345345345345345345
        self.assertEqual(str(u), "can't translate characters in position 1-4: 965230951443685724997")
        u.start = 1000
        self.assertEqual(str(u), "can't translate characters in position 1000-4: 965230951443685724997")

    def test_unicode_errors_no_object(self):
        # See issue #21134.
        klasses = UnicodeEncodeError, UnicodeDecodeError, UnicodeTranslateError
        for klass in klasses:
            self.assertEqual(str(klass.__new__(klass)), "")

    @no_tracing
    def test_badisinstance(self):
        # Bug #2542: if issubclass(e, MyException) raises an exception,
        # it should be ignored
        class Meta(type):
            def __subclasscheck__(cls, subclass):
                raise ValueError()
        class MyException(Exception, metaclass=Meta):
            pass

        with captured_stderr() as stderr:
            try:
                raise KeyError()
            except MyException as e:
                self.fail("exception should not be a MyException")
            except KeyError:
                pass
            except:
                self.fail("Should have raised KeyError")
            else:
                self.fail("Should have raised KeyError")

        def g():
            try:
                return g()
            except RecursionError:
                return sys.exc_info()
        e, v, tb = g()
        self.assertIsInstance(v, RecursionError, type(v))
        self.assertIn("maximum recursion depth exceeded", str(v))

    @cpython_only
    def test_recursion_normalizing_exception(self):
        # Issue #22898.
        # Test that a RecursionError is raised when tstate->recursion_depth is
        # equal to recursion_limit in PyErr_NormalizeException() and check
        # that a ResourceWarning is printed.
        # Prior to #22898, the recursivity of PyErr_NormalizeException() was
        # controlled by tstate->recursion_depth and a PyExc_RecursionErrorInst
        # singleton was being used in that case, that held traceback data and
        # locals indefinitely and would cause a segfault in _PyExc_Fini() upon
        # finalization of these locals.
        code = """if 1:
            import sys
            from _testinternalcapi import get_recursion_depth

            class MyException(Exception): pass

            def setrecursionlimit(depth):
                while 1:
                    try:
                        sys.setrecursionlimit(depth)
                        return depth
                    except RecursionError:
                        # sys.setrecursionlimit() raises a RecursionError if
                        # the new recursion limit is too low (issue #25274).
                        depth += 1

            def recurse(cnt):
                cnt -= 1
                if cnt:
                    recurse(cnt)
                else:
                    generator.throw(MyException)

            def gen():
                f = open(%a, mode='rb', buffering=0)
                yield

            generator = gen()
            next(generator)
            recursionlimit = sys.getrecursionlimit()
            depth = get_recursion_depth()
            try:
                # Upon the last recursive invocation of recurse(),
                # tstate->recursion_depth is equal to (recursion_limit - 1)
                # and is equal to recursion_limit when _gen_throw() calls
                # PyErr_NormalizeException().
                recurse(setrecursionlimit(depth + 2) - depth)
            finally:
                sys.setrecursionlimit(recursionlimit)
                print('Done.')
        """ % __file__
        rc, out, err = script_helper.assert_python_failure("-Wd", "-c", code)
        # Check that the program does not fail with SIGABRT.
        self.assertEqual(rc, 1)
        self.assertIn(b'RecursionError', err)
        self.assertIn(b'ResourceWarning', err)
        self.assertIn(b'Done.', out)

    @cpython_only
    def test_recursion_normalizing_infinite_exception(self):
        # Issue #30697. Test that a RecursionError is raised when
        # PyErr_NormalizeException() maximum recursion depth has been
        # exceeded.
        code = """if 1:
            import _testcapi
            try:
                raise _testcapi.RecursingInfinitelyError
            finally:
                print('Done.')
        """
        rc, out, err = script_helper.assert_python_failure("-c", code)
        self.assertEqual(rc, 1)
        self.assertIn(b'RecursionError: maximum recursion depth exceeded '
                      b'while normalizing an exception', err)
        self.assertIn(b'Done.', out)


    def test_recursion_in_except_handler(self):

        def set_relative_recursion_limit(n):
            depth = 1
            while True:
                try:
                    sys.setrecursionlimit(depth)
                except RecursionError:
                    depth += 1
                else:
                    break
            sys.setrecursionlimit(depth+n)

        def recurse_in_except():
            try:
                1/0
            except:
                recurse_in_except()

        def recurse_after_except():
            try:
                1/0
            except:
                pass
            recurse_after_except()

        def recurse_in_body_and_except():
            try:
                recurse_in_body_and_except()
            except:
                recurse_in_body_and_except()

        recursionlimit = sys.getrecursionlimit()
        try:
            set_relative_recursion_limit(10)
            for func in (recurse_in_except, recurse_after_except, recurse_in_body_and_except):
                with self.subTest(func=func):
                    try:
                        func()
                    except RecursionError:
                        pass
                    else:
                        self.fail("Should have raised a RecursionError")
        finally:
            sys.setrecursionlimit(recursionlimit)


    @cpython_only
    def test_recursion_normalizing_with_no_memory(self):
        # Issue #30697. Test that in the abort that occurs when there is no
        # memory left and the size of the Python frames stack is greater than
        # the size of the list of preallocated MemoryError instances, the
        # Fatal Python error message mentions MemoryError.
        code = """if 1:
            import _testcapi
            class C(): pass
            def recurse(cnt):
                cnt -= 1
                if cnt:
                    recurse(cnt)
                else:
                    _testcapi.set_nomemory(0)
                    C()
            recurse(16)
        """
        with SuppressCrashReport():
            rc, out, err = script_helper.assert_python_failure("-c", code)
            self.assertIn(b'Fatal Python error: _PyErr_NormalizeException: '
                          b'Cannot recover from MemoryErrors while '
                          b'normalizing exceptions.', err)

    @cpython_only
    def test_MemoryError(self):
        # PyErr_NoMemory always raises the same exception instance.
        # Check that the traceback is not doubled.
        import traceback
        from _testcapi import raise_memoryerror
        def raiseMemError():
            try:
                raise_memoryerror()
            except MemoryError as e:
                tb = e.__traceback__
            else:
                self.fail("Should have raised a MemoryError")
            return traceback.format_tb(tb)

        tb1 = raiseMemError()
        tb2 = raiseMemError()
        self.assertEqual(tb1, tb2)

    @cpython_only
    def test_exception_with_doc(self):
        import _testcapi
        doc2 = "This is a test docstring."
        doc4 = "This is another test docstring."

        self.assertRaises(SystemError, _testcapi.make_exception_with_doc,
                          "error1")

        # test basic usage of PyErr_NewException
        error1 = _testcapi.make_exception_with_doc("_testcapi.error1")
        self.assertIs(type(error1), type)
        self.assertTrue(issubclass(error1, Exception))
        self.assertIsNone(error1.__doc__)

        # test with given docstring
        error2 = _testcapi.make_exception_with_doc("_testcapi.error2", doc2)
        self.assertEqual(error2.__doc__, doc2)

        # test with explicit base (without docstring)
        error3 = _testcapi.make_exception_with_doc("_testcapi.error3",
                                                   base=error2)
        self.assertTrue(issubclass(error3, error2))

        # test with explicit base tuple
        class C(object):
            pass
        error4 = _testcapi.make_exception_with_doc("_testcapi.error4", doc4,
                                                   (error3, C))
        self.assertTrue(issubclass(error4, error3))
        self.assertTrue(issubclass(error4, C))
        self.assertEqual(error4.__doc__, doc4)

        # test with explicit dictionary
        error5 = _testcapi.make_exception_with_doc("_testcapi.error5", "",
                                                   error4, {'a': 1})
        self.assertTrue(issubclass(error5, error4))
        self.assertEqual(error5.a, 1)
        self.assertEqual(error5.__doc__, "")

    @cpython_only
    def test_memory_error_cleanup(self):
        # Issue #5437: preallocated MemoryError instances should not keep
        # traceback objects alive.
        from _testcapi import raise_memoryerror
        class C:
            pass
        wr = None
        def inner():
            nonlocal wr
            c = C()
            wr = weakref.ref(c)
            raise_memoryerror()
        # We cannot use assertRaises since it manually deletes the traceback
        try:
            inner()
        except MemoryError as e:
            self.assertNotEqual(wr(), None)
        else:
            self.fail("MemoryError not raised")
        self.assertEqual(wr(), None)

    @no_tracing
    def test_recursion_error_cleanup(self):
        # Same test as above, but with "recursion exceeded" errors
        class C:
            pass
        wr = None
        def inner():
            nonlocal wr
            c = C()
            wr = weakref.ref(c)
            inner()
        # We cannot use assertRaises since it manually deletes the traceback
        try:
            inner()
        except RecursionError as e:
            self.assertNotEqual(wr(), None)
        else:
            self.fail("RecursionError not raised")
        self.assertEqual(wr(), None)

    def test_errno_ENOTDIR(self):
        # Issue #12802: "not a directory" errors are ENOTDIR even on Windows
        with self.assertRaises(OSError) as cm:
            os.listdir(__file__)
        self.assertEqual(cm.exception.errno, errno.ENOTDIR, cm.exception)

    def test_unraisable(self):
        # Issue #22836: PyErr_WriteUnraisable() should give sensible reports
        class BrokenDel:
            def __del__(self):
                exc = ValueError("del is broken")
                # The following line is included in the traceback report:
                raise exc

        obj = BrokenDel()
        with support.catch_unraisable_exception() as cm:
            del obj

            self.assertEqual(cm.unraisable.object, BrokenDel.__del__)
            self.assertIsNotNone(cm.unraisable.exc_traceback)

    def test_unhandled(self):
        # Check for sensible reporting of unhandled exceptions
        for exc_type in (ValueError, BrokenStrException):
            with self.subTest(exc_type):
                try:
                    exc = exc_type("test message")
                    # The following line is included in the traceback report:
                    raise exc
                except exc_type:
                    with captured_stderr() as stderr:
                        sys.__excepthook__(*sys.exc_info())
                report = stderr.getvalue()
                self.assertIn("test_exceptions.py", report)
                self.assertIn("raise exc", report)
                self.assertIn(exc_type.__name__, report)
                if exc_type is BrokenStrException:
                    self.assertIn("<exception str() failed>", report)
                else:
                    self.assertIn("test message", report)
                self.assertTrue(report.endswith("\n"))

    @cpython_only
    def test_memory_error_in_PyErr_PrintEx(self):
        code = """if 1:
            import _testcapi
            class C(): pass
            _testcapi.set_nomemory(0, %d)
            C()
        """

        # Issue #30817: Abort in PyErr_PrintEx() when no memory.
        # Span a large range of tests as the CPython code always evolves with
        # changes that add or remove memory allocations.
        for i in range(1, 20):
            rc, out, err = script_helper.assert_python_failure("-c", code % i)
            self.assertIn(rc, (1, 120))
            self.assertIn(b'MemoryError', err)

    def test_yield_in_nested_try_excepts(self):
        #Issue #25612
        class MainError(Exception):
            pass

        class SubError(Exception):
            pass

        def main():
            try:
                raise MainError()
            except MainError:
                try:
                    yield
                except SubError:
                    pass
                raise

        coro = main()
        coro.send(None)
        with self.assertRaises(MainError):
            coro.throw(SubError())

    def test_generator_doesnt_retain_old_exc2(self):
        #Issue 28884#msg282532
        def g():
            try:
                raise ValueError
            except ValueError:
                yield 1
            self.assertEqual(sys.exc_info(), (None, None, None))
            yield 2

        gen = g()

        try:
            raise IndexError
        except IndexError:
            self.assertEqual(next(gen), 1)
        self.assertEqual(next(gen), 2)

    def test_raise_in_generator(self):
        #Issue 25612#msg304117
        def g():
            yield 1
            raise
            yield 2

        with self.assertRaises(ZeroDivisionError):
            i = g()
            try:
                1/0
            except:
                next(i)
                next(i)

    @unittest.skipUnless(__debug__, "Won't work if __debug__ is False")
    def test_assert_shadowing(self):
        # Shadowing AssertionError would cause the assert statement to
        # misbehave.
        global AssertionError
        AssertionError = TypeError
        try:
            assert False, 'hello'
        except BaseException as e:
            del AssertionError
            self.assertIsInstance(e, AssertionError)
            self.assertEqual(str(e), 'hello')
        else:
            del AssertionError
            self.fail('Expected exception')

    def test_memory_error_subclasses(self):
        # bpo-41654: MemoryError instances use a freelist of objects that are
        # linked using the 'dict' attribute when they are inactive/dead.
        # Subclasses of MemoryError should not participate in the freelist
        # schema. This test creates a MemoryError object and keeps it alive
        # (therefore advancing the freelist) and then it creates and destroys a
        # subclass object. Finally, it checks that creating a new MemoryError
        # succeeds, proving that the freelist is not corrupted.

        class TestException(MemoryError):
            pass

        try:
            raise MemoryError
        except MemoryError as exc:
            inst = exc

        try:
            raise TestException
        except Exception:
            pass

        for _ in range(10):
            try:
                raise MemoryError
            except MemoryError as exc:
                pass

            gc_collect()

global_for_suggestions = None

class NameErrorTests(unittest.TestCase):
    def test_name_error_has_name(self):
        try:
            bluch
        except NameError as exc:
            self.assertEqual("bluch", exc.name)

    def test_name_error_suggestions(self):
        def Substitution():
            noise = more_noise = a = bc = None
            blech = None
            print(bluch)

        def Elimination():
            noise = more_noise = a = bc = None
            blch = None
            print(bluch)

        def Addition():
            noise = more_noise = a = bc = None
            bluchin = None
            print(bluch)

        def SubstitutionOverElimination():
            blach = None
            bluc = None
            print(bluch)

        def SubstitutionOverAddition():
            blach = None
            bluchi = None
            print(bluch)

        def EliminationOverAddition():
            blucha = None
            bluc = None
            print(bluch)

        for func, suggestion in [(Substitution, "'blech'?"),
                                (Elimination, "'blch'?"),
                                (Addition, "'bluchin'?"),
                                (EliminationOverAddition, "'blucha'?"),
                                (SubstitutionOverElimination, "'blach'?"),
                                (SubstitutionOverAddition, "'blach'?")]:
            err = None
            try:
                func()
            except NameError as exc:
                with support.captured_stderr() as err:
                    sys.__excepthook__(*sys.exc_info())
            self.assertIn(suggestion, err.getvalue())

    def test_name_error_suggestions_from_globals(self):
        def func():
            print(global_for_suggestio)
        try:
            func()
        except NameError as exc:
            with support.captured_stderr() as err:
                sys.__excepthook__(*sys.exc_info())
        self.assertIn("'global_for_suggestions'?", err.getvalue())

    def test_name_error_suggestions_from_builtins(self):
        def func():
            print(ZeroDivisionErrrrr)
        try:
            func()
        except NameError as exc:
            with support.captured_stderr() as err:
                sys.__excepthook__(*sys.exc_info())
        self.assertIn("'ZeroDivisionError'?", err.getvalue())

    def test_name_error_suggestions_do_not_trigger_for_long_names(self):
        def f():
            somethingverywronghehehehehehe = None
            print(somethingverywronghe)

        try:
            f()
        except NameError as exc:
            with support.captured_stderr() as err:
                sys.__excepthook__(*sys.exc_info())

        self.assertNotIn("somethingverywronghehe", err.getvalue())

    def test_name_error_bad_suggestions_do_not_trigger_for_small_names(self):
        vvv = mom = w = id = pytho = None

        with self.subTest(name="b"):
            try:
                b
            except NameError as exc:
                with support.captured_stderr() as err:
                    sys.__excepthook__(*sys.exc_info())
            self.assertNotIn("you mean", err.getvalue())
            self.assertNotIn("vvv", err.getvalue())
            self.assertNotIn("mom", err.getvalue())
            self.assertNotIn("'id'", err.getvalue())
            self.assertNotIn("'w'", err.getvalue())
            self.assertNotIn("'pytho'", err.getvalue())

        with self.subTest(name="v"):
            try:
                v
            except NameError as exc:
                with support.captured_stderr() as err:
                    sys.__excepthook__(*sys.exc_info())
            self.assertNotIn("you mean", err.getvalue())
            self.assertNotIn("vvv", err.getvalue())
            self.assertNotIn("mom", err.getvalue())
            self.assertNotIn("'id'", err.getvalue())
            self.assertNotIn("'w'", err.getvalue())
            self.assertNotIn("'pytho'", err.getvalue())

        with self.subTest(name="m"):
            try:
                m
            except NameError as exc:
                with support.captured_stderr() as err:
                    sys.__excepthook__(*sys.exc_info())
            self.assertNotIn("you mean", err.getvalue())
            self.assertNotIn("vvv", err.getvalue())
            self.assertNotIn("mom", err.getvalue())
            self.assertNotIn("'id'", err.getvalue())
            self.assertNotIn("'w'", err.getvalue())
            self.assertNotIn("'pytho'", err.getvalue())

        with self.subTest(name="py"):
            try:
                py
            except NameError as exc:
                with support.captured_stderr() as err:
                    sys.__excepthook__(*sys.exc_info())
            self.assertNotIn("you mean", err.getvalue())
            self.assertNotIn("vvv", err.getvalue())
            self.assertNotIn("mom", err.getvalue())
            self.assertNotIn("'id'", err.getvalue())
            self.assertNotIn("'w'", err.getvalue())
            self.assertNotIn("'pytho'", err.getvalue())

    def test_name_error_suggestions_do_not_trigger_for_too_many_locals(self):
        def f():
            # Mutating locals() is unreliable, so we need to do it by hand
            a1 = a2 = a3 = a4 = a5 = a6 = a7 = a8 = a9 = a10 = \
            a11 = a12 = a13 = a14 = a15 = a16 = a17 = a18 = a19 = a20 = \
            a21 = a22 = a23 = a24 = a25 = a26 = a27 = a28 = a29 = a30 = \
            a31 = a32 = a33 = a34 = a35 = a36 = a37 = a38 = a39 = a40 = \
            a41 = a42 = a43 = a44 = a45 = a46 = a47 = a48 = a49 = a50 = \
            a51 = a52 = a53 = a54 = a55 = a56 = a57 = a58 = a59 = a60 = \
            a61 = a62 = a63 = a64 = a65 = a66 = a67 = a68 = a69 = a70 = \
            a71 = a72 = a73 = a74 = a75 = a76 = a77 = a78 = a79 = a80 = \
            a81 = a82 = a83 = a84 = a85 = a86 = a87 = a88 = a89 = a90 = \
            a91 = a92 = a93 = a94 = a95 = a96 = a97 = a98 = a99 = a100 = \
            a101 = a102 = a103 = a104 = a105 = a106 = a107 = a108 = a109 = a110 = \
            a111 = a112 = a113 = a114 = a115 = a116 = a117 = a118 = a119 = a120 = \
            a121 = a122 = a123 = a124 = a125 = a126 = a127 = a128 = a129 = a130 = \
            a131 = a132 = a133 = a134 = a135 = a136 = a137 = a138 = a139 = a140 = \
            a141 = a142 = a143 = a144 = a145 = a146 = a147 = a148 = a149 = a150 = \
            a151 = a152 = a153 = a154 = a155 = a156 = a157 = a158 = a159 = a160 = \
            a161 = a162 = a163 = a164 = a165 = a166 = a167 = a168 = a169 = a170 = \
            a171 = a172 = a173 = a174 = a175 = a176 = a177 = a178 = a179 = a180 = \
            a181 = a182 = a183 = a184 = a185 = a186 = a187 = a188 = a189 = a190 = \
            a191 = a192 = a193 = a194 = a195 = a196 = a197 = a198 = a199 = a200 = \
            a201 = a202 = a203 = a204 = a205 = a206 = a207 = a208 = a209 = a210 = \
            a211 = a212 = a213 = a214 = a215 = a216 = a217 = a218 = a219 = a220 = \
            a221 = a222 = a223 = a224 = a225 = a226 = a227 = a228 = a229 = a230 = \
            a231 = a232 = a233 = a234 = a235 = a236 = a237 = a238 = a239 = a240 = \
            a241 = a242 = a243 = a244 = a245 = a246 = a247 = a248 = a249 = a250 = \
            a251 = a252 = a253 = a254 = a255 = a256 = a257 = a258 = a259 = a260 = \
            a261 = a262 = a263 = a264 = a265 = a266 = a267 = a268 = a269 = a270 = \
            a271 = a272 = a273 = a274 = a275 = a276 = a277 = a278 = a279 = a280 = \
            a281 = a282 = a283 = a284 = a285 = a286 = a287 = a288 = a289 = a290 = \
            a291 = a292 = a293 = a294 = a295 = a296 = a297 = a298 = a299 = a300 = \
            a301 = a302 = a303 = a304 = a305 = a306 = a307 = a308 = a309 = a310 = \
            a311 = a312 = a313 = a314 = a315 = a316 = a317 = a318 = a319 = a320 = \
            a321 = a322 = a323 = a324 = a325 = a326 = a327 = a328 = a329 = a330 = \
            a331 = a332 = a333 = a334 = a335 = a336 = a337 = a338 = a339 = a340 = \
            a341 = a342 = a343 = a344 = a345 = a346 = a347 = a348 = a349 = a350 = \
            a351 = a352 = a353 = a354 = a355 = a356 = a357 = a358 = a359 = a360 = \
            a361 = a362 = a363 = a364 = a365 = a366 = a367 = a368 = a369 = a370 = \
            a371 = a372 = a373 = a374 = a375 = a376 = a377 = a378 = a379 = a380 = \
            a381 = a382 = a383 = a384 = a385 = a386 = a387 = a388 = a389 = a390 = \
            a391 = a392 = a393 = a394 = a395 = a396 = a397 = a398 = a399 = a400 = \
            a401 = a402 = a403 = a404 = a405 = a406 = a407 = a408 = a409 = a410 = \
            a411 = a412 = a413 = a414 = a415 = a416 = a417 = a418 = a419 = a420 = \
            a421 = a422 = a423 = a424 = a425 = a426 = a427 = a428 = a429 = a430 = \
            a431 = a432 = a433 = a434 = a435 = a436 = a437 = a438 = a439 = a440 = \
            a441 = a442 = a443 = a444 = a445 = a446 = a447 = a448 = a449 = a450 = \
            a451 = a452 = a453 = a454 = a455 = a456 = a457 = a458 = a459 = a460 = \
            a461 = a462 = a463 = a464 = a465 = a466 = a467 = a468 = a469 = a470 = \
            a471 = a472 = a473 = a474 = a475 = a476 = a477 = a478 = a479 = a480 = \
            a481 = a482 = a483 = a484 = a485 = a486 = a487 = a488 = a489 = a490 = \
            a491 = a492 = a493 = a494 = a495 = a496 = a497 = a498 = a499 = a500 = \
            a501 = a502 = a503 = a504 = a505 = a506 = a507 = a508 = a509 = a510 = \
            a511 = a512 = a513 = a514 = a515 = a516 = a517 = a518 = a519 = a520 = \
            a521 = a522 = a523 = a524 = a525 = a526 = a527 = a528 = a529 = a530 = \
            a531 = a532 = a533 = a534 = a535 = a536 = a537 = a538 = a539 = a540 = \
            a541 = a542 = a543 = a544 = a545 = a546 = a547 = a548 = a549 = a550 = \
            a551 = a552 = a553 = a554 = a555 = a556 = a557 = a558 = a559 = a560 = \
            a561 = a562 = a563 = a564 = a565 = a566 = a567 = a568 = a569 = a570 = \
            a571 = a572 = a573 = a574 = a575 = a576 = a577 = a578 = a579 = a580 = \
            a581 = a582 = a583 = a584 = a585 = a586 = a587 = a588 = a589 = a590 = \
            a591 = a592 = a593 = a594 = a595 = a596 = a597 = a598 = a599 = a600 = \
            a601 = a602 = a603 = a604 = a605 = a606 = a607 = a608 = a609 = a610 = \
            a611 = a612 = a613 = a614 = a615 = a616 = a617 = a618 = a619 = a620 = \
            a621 = a622 = a623 = a624 = a625 = a626 = a627 = a628 = a629 = a630 = \
            a631 = a632 = a633 = a634 = a635 = a636 = a637 = a638 = a639 = a640 = \
            a641 = a642 = a643 = a644 = a645 = a646 = a647 = a648 = a649 = a650 = \
            a651 = a652 = a653 = a654 = a655 = a656 = a657 = a658 = a659 = a660 = \
            a661 = a662 = a663 = a664 = a665 = a666 = a667 = a668 = a669 = a670 = \
            a671 = a672 = a673 = a674 = a675 = a676 = a677 = a678 = a679 = a680 = \
            a681 = a682 = a683 = a684 = a685 = a686 = a687 = a688 = a689 = a690 = \
            a691 = a692 = a693 = a694 = a695 = a696 = a697 = a698 = a699 = a700 = \
            a701 = a702 = a703 = a704 = a705 = a706 = a707 = a708 = a709 = a710 = \
            a711 = a712 = a713 = a714 = a715 = a716 = a717 = a718 = a719 = a720 = \
            a721 = a722 = a723 = a724 = a725 = a726 = a727 = a728 = a729 = a730 = \
            a731 = a732 = a733 = a734 = a735 = a736 = a737 = a738 = a739 = a740 = \
            a741 = a742 = a743 = a744 = a745 = a746 = a747 = a748 = a749 = a750 = \
            a751 = a752 = a753 = a754 = a755 = a756 = a757 = a758 = a759 = a760 = \
            a761 = a762 = a763 = a764 = a765 = a766 = a767 = a768 = a769 = a770 = \
            a771 = a772 = a773 = a774 = a775 = a776 = a777 = a778 = a779 = a780 = \
            a781 = a782 = a783 = a784 = a785 = a786 = a787 = a788 = a789 = a790 = \
            a791 = a792 = a793 = a794 = a795 = a796 = a797 = a798 = a799 = a800 \
                = None
            print(a0)

        try:
            f()
        except NameError as exc:
            with support.captured_stderr() as err:
                sys.__excepthook__(*sys.exc_info())

        self.assertNotIn("a1", err.getvalue())

    def test_name_error_with_custom_exceptions(self):
        def f():
            blech = None
            raise NameError()

        try:
            f()
        except NameError as exc:
            with support.captured_stderr() as err:
                sys.__excepthook__(*sys.exc_info())

        self.assertNotIn("blech", err.getvalue())

        def f():
            blech = None
            raise NameError

        try:
            f()
        except NameError as exc:
            with support.captured_stderr() as err:
                sys.__excepthook__(*sys.exc_info())

        self.assertNotIn("blech", err.getvalue())

    def test_unbound_local_error_doesn_not_match(self):
        def foo():
            something = 3
            print(somethong)
            somethong = 3

        try:
            foo()
        except UnboundLocalError as exc:
            with support.captured_stderr() as err:
                sys.__excepthook__(*sys.exc_info())

        self.assertNotIn("something", err.getvalue())


class AttributeErrorTests(unittest.TestCase):
    def test_attributes(self):
        # Setting 'attr' should not be a problem.
        exc = AttributeError('Ouch!')
        self.assertIsNone(exc.name)
        self.assertIsNone(exc.obj)

        sentinel = object()
        exc = AttributeError('Ouch', name='carry', obj=sentinel)
        self.assertEqual(exc.name, 'carry')
        self.assertIs(exc.obj, sentinel)

    def test_getattr_has_name_and_obj(self):
        class A:
            blech = None

        obj = A()
        try:
            obj.bluch
        except AttributeError as exc:
            self.assertEqual("bluch", exc.name)
            self.assertEqual(obj, exc.obj)

    def test_getattr_has_name_and_obj_for_method(self):
        class A:
            def blech(self):
                return

        obj = A()
        try:
            obj.bluch()
        except AttributeError as exc:
            self.assertEqual("bluch", exc.name)
            self.assertEqual(obj, exc.obj)

    def test_getattr_suggestions(self):
        class Substitution:
            noise = more_noise = a = bc = None
            blech = None

        class Elimination:
            noise = more_noise = a = bc = None
            blch = None

        class Addition:
            noise = more_noise = a = bc = None
            bluchin = None

        class SubstitutionOverElimination:
            blach = None
            bluc = None

        class SubstitutionOverAddition:
            blach = None
            bluchi = None

        class EliminationOverAddition:
            blucha = None
            bluc = None

        for cls, suggestion in [(Substitution, "'blech'?"),
                                (Elimination, "'blch'?"),
                                (Addition, "'bluchin'?"),
                                (EliminationOverAddition, "'bluc'?"),
                                (SubstitutionOverElimination, "'blach'?"),
                                (SubstitutionOverAddition, "'blach'?")]:
            try:
                cls().bluch
            except AttributeError as exc:
                with support.captured_stderr() as err:
                    sys.__excepthook__(*sys.exc_info())

            self.assertIn(suggestion, err.getvalue())

    def test_getattr_suggestions_do_not_trigger_for_long_attributes(self):
        class A:
            blech = None

        try:
            A().somethingverywrong
        except AttributeError as exc:
            with support.captured_stderr() as err:
                sys.__excepthook__(*sys.exc_info())

        self.assertNotIn("blech", err.getvalue())

    def test_getattr_error_bad_suggestions_do_not_trigger_for_small_names(self):
        class MyClass:
            vvv = mom = w = id = pytho = None

        with self.subTest(name="b"):
            try:
                MyClass.b
            except AttributeError as exc:
                with support.captured_stderr() as err:
                    sys.__excepthook__(*sys.exc_info())
            self.assertNotIn("you mean", err.getvalue())
            self.assertNotIn("vvv", err.getvalue())
            self.assertNotIn("mom", err.getvalue())
            self.assertNotIn("'id'", err.getvalue())
            self.assertNotIn("'w'", err.getvalue())
            self.assertNotIn("'pytho'", err.getvalue())

        with self.subTest(name="v"):
            try:
                MyClass.v
            except AttributeError as exc:
                with support.captured_stderr() as err:
                    sys.__excepthook__(*sys.exc_info())
            self.assertNotIn("you mean", err.getvalue())
            self.assertNotIn("vvv", err.getvalue())
            self.assertNotIn("mom", err.getvalue())
            self.assertNotIn("'id'", err.getvalue())
            self.assertNotIn("'w'", err.getvalue())
            self.assertNotIn("'pytho'", err.getvalue())

        with self.subTest(name="m"):
            try:
                MyClass.m
            except AttributeError as exc:
                with support.captured_stderr() as err:
                    sys.__excepthook__(*sys.exc_info())
            self.assertNotIn("you mean", err.getvalue())
            self.assertNotIn("vvv", err.getvalue())
            self.assertNotIn("mom", err.getvalue())
            self.assertNotIn("'id'", err.getvalue())
            self.assertNotIn("'w'", err.getvalue())
            self.assertNotIn("'pytho'", err.getvalue())

        with self.subTest(name="py"):
            try:
                MyClass.py
            except AttributeError as exc:
                with support.captured_stderr() as err:
                    sys.__excepthook__(*sys.exc_info())
            self.assertNotIn("you mean", err.getvalue())
            self.assertNotIn("vvv", err.getvalue())
            self.assertNotIn("mom", err.getvalue())
            self.assertNotIn("'id'", err.getvalue())
            self.assertNotIn("'w'", err.getvalue())
            self.assertNotIn("'pytho'", err.getvalue())


    def test_getattr_suggestions_do_not_trigger_for_big_dicts(self):
        class A:
            blech = None
        # A class with a very big __dict__ will not be consider
        # for suggestions.
        for index in range(2000):
            setattr(A, f"index_{index}", None)

        try:
            A().bluch
        except AttributeError as exc:
            with support.captured_stderr() as err:
                sys.__excepthook__(*sys.exc_info())

        self.assertNotIn("blech", err.getvalue())

    def test_getattr_suggestions_no_args(self):
        class A:
            blech = None
            def __getattr__(self, attr):
                raise AttributeError()

        try:
            A().bluch
        except AttributeError as exc:
            with support.captured_stderr() as err:
                sys.__excepthook__(*sys.exc_info())

        self.assertIn("blech", err.getvalue())

        class A:
            blech = None
            def __getattr__(self, attr):
                raise AttributeError

        try:
            A().bluch
        except AttributeError as exc:
            with support.captured_stderr() as err:
                sys.__excepthook__(*sys.exc_info())

        self.assertIn("blech", err.getvalue())

    def test_getattr_suggestions_invalid_args(self):
        class NonStringifyClass:
            __str__ = None
            __repr__ = None

        class A:
            blech = None
            def __getattr__(self, attr):
                raise AttributeError(NonStringifyClass())

        class B:
            blech = None
            def __getattr__(self, attr):
                raise AttributeError("Error", 23)

        class C:
            blech = None
            def __getattr__(self, attr):
                raise AttributeError(23)

        for cls in [A, B, C]:
            try:
                cls().bluch
            except AttributeError as exc:
                with support.captured_stderr() as err:
                    sys.__excepthook__(*sys.exc_info())

            self.assertIn("blech", err.getvalue())

    def test_getattr_suggestions_for_same_name(self):
        class A:
            def __dir__(self):
                return ['blech']
        try:
            A().blech
        except AttributeError as exc:
            with support.captured_stderr() as err:
                sys.__excepthook__(*sys.exc_info())

        self.assertNotIn("Did you mean", err.getvalue())

    def test_attribute_error_with_failing_dict(self):
        class T:
            bluch = 1
            def __dir__(self):
                raise AttributeError("oh no!")

        try:
            T().blich
        except AttributeError as exc:
            with support.captured_stderr() as err:
                sys.__excepthook__(*sys.exc_info())

        self.assertNotIn("blech", err.getvalue())
        self.assertNotIn("oh no!", err.getvalue())

    def test_attribute_error_with_bad_name(self):
        try:
            raise AttributeError(name=12, obj=23)
        except AttributeError as exc:
            with support.captured_stderr() as err:
                sys.__excepthook__(*sys.exc_info())

        self.assertNotIn("?", err.getvalue())


class ImportErrorTests(unittest.TestCase):

    def test_attributes(self):
        # Setting 'name' and 'path' should not be a problem.
        exc = ImportError('test')
        self.assertIsNone(exc.name)
        self.assertIsNone(exc.path)

        exc = ImportError('test', name='somemodule')
        self.assertEqual(exc.name, 'somemodule')
        self.assertIsNone(exc.path)

        exc = ImportError('test', path='somepath')
        self.assertEqual(exc.path, 'somepath')
        self.assertIsNone(exc.name)

        exc = ImportError('test', path='somepath', name='somename')
        self.assertEqual(exc.name, 'somename')
        self.assertEqual(exc.path, 'somepath')

        msg = "'invalid' is an invalid keyword argument for ImportError"
        with self.assertRaisesRegex(TypeError, msg):
            ImportError('test', invalid='keyword')

        with self.assertRaisesRegex(TypeError, msg):
            ImportError('test', name='name', invalid='keyword')

        with self.assertRaisesRegex(TypeError, msg):
            ImportError('test', path='path', invalid='keyword')

        with self.assertRaisesRegex(TypeError, msg):
            ImportError(invalid='keyword')

        with self.assertRaisesRegex(TypeError, msg):
            ImportError('test', invalid='keyword', another=True)

    def test_reset_attributes(self):
        exc = ImportError('test', name='name', path='path')
        self.assertEqual(exc.args, ('test',))
        self.assertEqual(exc.msg, 'test')
        self.assertEqual(exc.name, 'name')
        self.assertEqual(exc.path, 'path')

        # Reset not specified attributes
        exc.__init__()
        self.assertEqual(exc.args, ())
        self.assertEqual(exc.msg, None)
        self.assertEqual(exc.name, None)
        self.assertEqual(exc.path, None)

    def test_non_str_argument(self):
        # Issue #15778
        with check_warnings(('', BytesWarning), quiet=True):
            arg = b'abc'
            exc = ImportError(arg)
            self.assertEqual(str(arg), str(exc))

    def test_copy_pickle(self):
        for kwargs in (dict(),
                       dict(name='somename'),
                       dict(path='somepath'),
                       dict(name='somename', path='somepath')):
            orig = ImportError('test', **kwargs)
            for proto in range(pickle.HIGHEST_PROTOCOL + 1):
                exc = pickle.loads(pickle.dumps(orig, proto))
                self.assertEqual(exc.args, ('test',))
                self.assertEqual(exc.msg, 'test')
                self.assertEqual(exc.name, orig.name)
                self.assertEqual(exc.path, orig.path)
            for c in copy.copy, copy.deepcopy:
                exc = c(orig)
                self.assertEqual(exc.args, ('test',))
                self.assertEqual(exc.msg, 'test')
                self.assertEqual(exc.name, orig.name)
                self.assertEqual(exc.path, orig.path)

class SyntaxErrorTests(unittest.TestCase):
    def test_range_of_offsets(self):
        cases = [
            # Basic range from 2->7
            (("bad.py", 1, 2, "abcdefg", 1, 7),
             dedent(
             """
               File "bad.py", line 1
                 abcdefg
                  ^^^^^
             SyntaxError: bad bad
             """)),
            # end_offset = start_offset + 1
            (("bad.py", 1, 2, "abcdefg", 1, 3),
             dedent(
             """
               File "bad.py", line 1
                 abcdefg
                  ^
             SyntaxError: bad bad
             """)),
            # Negative end offset
            (("bad.py", 1, 2, "abcdefg", 1, -2),
             dedent(
             """
               File "bad.py", line 1
                 abcdefg
                  ^
             SyntaxError: bad bad
             """)),
            # end offset before starting offset
            (("bad.py", 1, 4, "abcdefg", 1, 2),
             dedent(
             """
               File "bad.py", line 1
                 abcdefg
                    ^
             SyntaxError: bad bad
             """)),
            # Both offsets negative
            (("bad.py", 1, -4, "abcdefg", 1, -2),
             dedent(
             """
               File "bad.py", line 1
                 abcdefg
             SyntaxError: bad bad
             """)),
            # Both offsets negative and the end more negative
            (("bad.py", 1, -4, "abcdefg", 1, -5),
             dedent(
             """
               File "bad.py", line 1
                 abcdefg
             SyntaxError: bad bad
             """)),
            # Both offsets 0
            (("bad.py", 1, 0, "abcdefg", 1, 0),
             dedent(
             """
               File "bad.py", line 1
                 abcdefg
             SyntaxError: bad bad
             """)),
            # Start offset 0 and end offset not 0
            (("bad.py", 1, 0, "abcdefg", 1, 5),
             dedent(
             """
               File "bad.py", line 1
                 abcdefg
             SyntaxError: bad bad
             """)),
            # End offset pass the source lenght
            (("bad.py", 1, 2, "abcdefg", 1, 100),
             dedent(
             """
               File "bad.py", line 1
                 abcdefg
                  ^^^^^^
             SyntaxError: bad bad
             """)),
        ]
        for args, expected in cases:
            with self.subTest(args=args):
                try:
                    raise SyntaxError("bad bad", args)
                except SyntaxError as exc:
                    with support.captured_stderr() as err:
                        sys.__excepthook__(*sys.exc_info())
                    the_exception = exc

    def test_encodings(self):
        source = (
            '# -*- coding: cp437 -*-\n'
            '"┬ó┬ó┬ó┬ó┬ó┬ó" + f(4, x for x in range(1))\n'
        )
        try:
            with open(TESTFN, 'w', encoding='cp437') as testfile:
                testfile.write(source)
            rc, out, err = script_helper.assert_python_failure('-Wd', '-X', 'utf8', TESTFN)
            err = err.decode('utf-8').splitlines()

            self.assertEqual(err[-3], '    "┬ó┬ó┬ó┬ó┬ó┬ó" + f(4, x for x in range(1))')
            self.assertEqual(err[-2], '                          ^^^^^^^^^^^^^^^^^^^')
        finally:
            unlink(TESTFN)

    def test_attributes_new_constructor(self):
        args = ("bad.py", 1, 2, "abcdefg", 1, 100)
        the_exception = SyntaxError("bad bad", args)
        filename, lineno, offset, error, end_lineno, end_offset = args
        self.assertEqual(filename, the_exception.filename)
        self.assertEqual(lineno, the_exception.lineno)
        self.assertEqual(end_lineno, the_exception.end_lineno)
        self.assertEqual(offset, the_exception.offset)
        self.assertEqual(end_offset, the_exception.end_offset)
        self.assertEqual(error, the_exception.text)
        self.assertEqual("bad bad", the_exception.msg)

    def test_attributes_old_constructor(self):
        args = ("bad.py", 1, 2, "abcdefg")
        the_exception = SyntaxError("bad bad", args)
        filename, lineno, offset, error = args
        self.assertEqual(filename, the_exception.filename)
        self.assertEqual(lineno, the_exception.lineno)
        self.assertEqual(None, the_exception.end_lineno)
        self.assertEqual(offset, the_exception.offset)
        self.assertEqual(None, the_exception.end_offset)
        self.assertEqual(error, the_exception.text)
        self.assertEqual("bad bad", the_exception.msg)

    def test_incorrect_constructor(self):
        args = ("bad.py", 1, 2)
        self.assertRaises(TypeError, SyntaxError, "bad bad", args)

        args = ("bad.py", 1, 2, 4, 5, 6, 7)
        self.assertRaises(TypeError, SyntaxError, "bad bad", args)

        args = ("bad.py", 1, 2, "abcdefg", 1)
        self.assertRaises(TypeError, SyntaxError, "bad bad", args)


class PEP626Tests(unittest.TestCase):

    def lineno_after_raise(self, f, *expected):
        try:
            f()
        except Exception as ex:
            t = ex.__traceback__
        else:
            self.fail("No exception raised")
        lines = []
        t = t.tb_next # Skip this function
        while t:
            frame = t.tb_frame
            lines.append(
                None if frame.f_lineno is None else
                frame.f_lineno-frame.f_code.co_firstlineno
            )
            t = t.tb_next
        self.assertEqual(tuple(lines), expected)

    def test_lineno_after_raise_simple(self):
        def simple():
            1/0
            pass
        self.lineno_after_raise(simple, 1)

    def test_lineno_after_raise_in_except(self):
        def in_except():
            try:
                1/0
            except:
                1/0
                pass
        self.lineno_after_raise(in_except, 4)

    def test_lineno_after_other_except(self):
        def other_except():
            try:
                1/0
            except TypeError as ex:
                pass
        self.lineno_after_raise(other_except, 3)

    def test_lineno_in_named_except(self):
        def in_named_except():
            try:
                1/0
            except Exception as ex:
                1/0
                pass
        self.lineno_after_raise(in_named_except, 4)

    def test_lineno_in_try(self):
        def in_try():
            try:
                1/0
            finally:
                pass
        self.lineno_after_raise(in_try, 4)

    def test_lineno_in_finally_normal(self):
        def in_finally_normal():
            try:
                pass
            finally:
                1/0
                pass
        self.lineno_after_raise(in_finally_normal, 4)

    def test_lineno_in_finally_except(self):
        def in_finally_except():
            try:
                1/0
            finally:
                1/0
                pass
        self.lineno_after_raise(in_finally_except, 4)

    def test_lineno_after_with(self):
        class Noop:
            def __enter__(self):
                return self
            def __exit__(self, *args):
                pass
        def after_with():
            with Noop():
                1/0
                pass
        self.lineno_after_raise(after_with, 2)

    def test_missing_lineno_shows_as_none(self):
        def f():
            1/0
        self.lineno_after_raise(f, 1)
        f.__code__ = f.__code__.replace(co_linetable=b'\x04\x80\xff\x80')
        self.lineno_after_raise(f, None)

    def test_lineno_after_raise_in_with_exit(self):
        class ExitFails:
            def __enter__(self):
                return self
            def __exit__(self, *args):
                raise ValueError

        def after_with():
            with ExitFails():
                1/0
        self.lineno_after_raise(after_with, 1, 1)

if __name__ == '__main__':
    unittest.main()
