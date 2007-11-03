# Python test set -- part 5, built-in exceptions

import os
import sys
import unittest
import pickle, cPickle

from test.test_support import (TESTFN, unlink, run_unittest,
                                catch_warning)
from test.test_pep352 import ignore_message_warning

# XXX This is not really enough, each *operation* should be tested!

class ExceptionTests(unittest.TestCase):

    def testReload(self):
        # Reloading the built-in exceptions module failed prior to Py2.2, while it
        # should act the same as reloading built-in sys.
        try:
            import exceptions
            reload(exceptions)
        except ImportError, e:
            self.fail("reloading exceptions: %s" % e)

    def raise_catch(self, exc, excname):
        try:
            raise exc, "spam"
        except exc, err:
            buf1 = str(err)
        try:
            raise exc("spam")
        except exc, err:
            buf2 = str(err)
        self.assertEquals(buf1, buf2)
        self.assertEquals(exc.__name__, excname)

    def testRaising(self):
        self.raise_catch(AttributeError, "AttributeError")
        self.assertRaises(AttributeError, getattr, sys, "undefined_attribute")

        self.raise_catch(EOFError, "EOFError")
        fp = open(TESTFN, 'w')
        fp.close()
        fp = open(TESTFN, 'r')
        savestdin = sys.stdin
        try:
            try:
                sys.stdin = fp
                x = raw_input()
            except EOFError:
                pass
        finally:
            sys.stdin = savestdin
            fp.close()
            unlink(TESTFN)

        self.raise_catch(IOError, "IOError")
        self.assertRaises(IOError, open, 'this file does not exist', 'r')

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

        self.raise_catch(SyntaxError, "SyntaxError")
        try: exec '/\n'
        except SyntaxError: pass

        self.raise_catch(IndentationError, "IndentationError")

        self.raise_catch(TabError, "TabError")
        # can only be tested under -tt, and is the only test for -tt
        #try: compile("try:\n\t1/0\n    \t1/0\nfinally:\n pass\n", '<string>', 'exec')
        #except TabError: pass
        #else: self.fail("TabError not raised")

        self.raise_catch(SystemError, "SystemError")

        self.raise_catch(SystemExit, "SystemExit")
        self.assertRaises(SystemExit, sys.exit, 0)

        self.raise_catch(TypeError, "TypeError")
        try: [] + ()
        except TypeError: pass

        self.raise_catch(ValueError, "ValueError")
        self.assertRaises(ValueError, chr, 10000)

        self.raise_catch(ZeroDivisionError, "ZeroDivisionError")
        try: x = 1/0
        except ZeroDivisionError: pass

        self.raise_catch(Exception, "Exception")
        try: x = 1/0
        except Exception, e: pass

    def testSyntaxErrorMessage(self):
        # make sure the right exception message is raised for each of
        # these code fragments

        def ckmsg(src, msg):
            try:
                compile(src, '<fragment>', 'exec')
            except SyntaxError, e:
                if e.msg != msg:
                    self.fail("expected %s, got %s" % (msg, e.msg))
            else:
                self.fail("failed to get expected SyntaxError")

        s = '''while 1:
            try:
                pass
            finally:
                continue'''

        if not sys.platform.startswith('java'):
            ckmsg(s, "'continue' not supported inside 'finally' clause")

        s = '''if 1:
        try:
            continue
        except:
            pass'''

        ckmsg(s, "'continue' not properly in loop")
        ckmsg("continue\n", "'continue' not properly in loop")

    def testSettingException(self):
        # test that setting an exception at the C level works even if the
        # exception object can't be constructed.

        class BadException:
            def __init__(self_):
                raise RuntimeError, "can't instantiate BadException"

        def test_capi1():
            import _testcapi
            try:
                _testcapi.raise_exception(BadException, 1)
            except TypeError, err:
                exc, err, tb = sys.exc_info()
                co = tb.tb_frame.f_code
                self.assertEquals(co.co_name, "test_capi1")
                self.assert_(co.co_filename.endswith('test_exceptions'+os.extsep+'py'))
            else:
                self.fail("Expected exception")

        def test_capi2():
            import _testcapi
            try:
                _testcapi.raise_exception(BadException, 0)
            except RuntimeError, err:
                exc, err, tb = sys.exc_info()
                co = tb.tb_frame.f_code
                self.assertEquals(co.co_name, "__init__")
                self.assert_(co.co_filename.endswith('test_exceptions'+os.extsep+'py'))
                co2 = tb.tb_frame.f_back.f_code
                self.assertEquals(co2.co_name, "test_capi2")
            else:
                self.fail("Expected exception")

        if not sys.platform.startswith('java'):
            test_capi1()
            test_capi2()

    def test_WindowsError(self):
        try:
            WindowsError
        except NameError:
            pass
        else:
            self.failUnlessEqual(str(WindowsError(1001)),
                                 "1001")
            self.failUnlessEqual(str(WindowsError(1001, "message")),
                                 "[Error 1001] message")
            self.failUnlessEqual(WindowsError(1001, "message").errno, 22)
            self.failUnlessEqual(WindowsError(1001, "message").winerror, 1001)

    def testAttributes(self):
        # test that exception attributes are happy

        exceptionList = [
            (BaseException, (), {'message' : '', 'args' : ()}),
            (BaseException, (1, ), {'message' : 1, 'args' : (1,)}),
            (BaseException, ('foo',),
                {'message' : 'foo', 'args' : ('foo',)}),
            (BaseException, ('foo', 1),
                {'message' : '', 'args' : ('foo', 1)}),
            (SystemExit, ('foo',),
                {'message' : 'foo', 'args' : ('foo',), 'code' : 'foo'}),
            (IOError, ('foo',),
                {'message' : 'foo', 'args' : ('foo',), 'filename' : None,
                 'errno' : None, 'strerror' : None}),
            (IOError, ('foo', 'bar'),
                {'message' : '', 'args' : ('foo', 'bar'), 'filename' : None,
                 'errno' : 'foo', 'strerror' : 'bar'}),
            (IOError, ('foo', 'bar', 'baz'),
                {'message' : '', 'args' : ('foo', 'bar'), 'filename' : 'baz',
                 'errno' : 'foo', 'strerror' : 'bar'}),
            (IOError, ('foo', 'bar', 'baz', 'quux'),
                {'message' : '', 'args' : ('foo', 'bar', 'baz', 'quux')}),
            (EnvironmentError, ('errnoStr', 'strErrorStr', 'filenameStr'),
                {'message' : '', 'args' : ('errnoStr', 'strErrorStr'),
                 'strerror' : 'strErrorStr', 'errno' : 'errnoStr',
                 'filename' : 'filenameStr'}),
            (EnvironmentError, (1, 'strErrorStr', 'filenameStr'),
                {'message' : '', 'args' : (1, 'strErrorStr'), 'errno' : 1,
                 'strerror' : 'strErrorStr', 'filename' : 'filenameStr'}),
            (SyntaxError, (), {'message' : '', 'msg' : None, 'text' : None,
                'filename' : None, 'lineno' : None, 'offset' : None,
                'print_file_and_line' : None}),
            (SyntaxError, ('msgStr',),
                {'message' : 'msgStr', 'args' : ('msgStr',), 'text' : None,
                 'print_file_and_line' : None, 'msg' : 'msgStr',
                 'filename' : None, 'lineno' : None, 'offset' : None}),
            (SyntaxError, ('msgStr', ('filenameStr', 'linenoStr', 'offsetStr',
                           'textStr')),
                {'message' : '', 'offset' : 'offsetStr', 'text' : 'textStr',
                 'args' : ('msgStr', ('filenameStr', 'linenoStr',
                                      'offsetStr', 'textStr')),
                 'print_file_and_line' : None, 'msg' : 'msgStr',
                 'filename' : 'filenameStr', 'lineno' : 'linenoStr'}),
            (SyntaxError, ('msgStr', 'filenameStr', 'linenoStr', 'offsetStr',
                           'textStr', 'print_file_and_lineStr'),
                {'message' : '', 'text' : None,
                 'args' : ('msgStr', 'filenameStr', 'linenoStr', 'offsetStr',
                           'textStr', 'print_file_and_lineStr'),
                 'print_file_and_line' : None, 'msg' : 'msgStr',
                 'filename' : None, 'lineno' : None, 'offset' : None}),
            (UnicodeError, (), {'message' : '', 'args' : (),}),
            (UnicodeEncodeError, ('ascii', u'a', 0, 1, 'ordinal not in range'),
                {'message' : '', 'args' : ('ascii', u'a', 0, 1,
                                           'ordinal not in range'),
                 'encoding' : 'ascii', 'object' : u'a',
                 'start' : 0, 'reason' : 'ordinal not in range'}),
            (UnicodeDecodeError, ('ascii', '\xff', 0, 1, 'ordinal not in range'),
                {'message' : '', 'args' : ('ascii', '\xff', 0, 1,
                                           'ordinal not in range'),
                 'encoding' : 'ascii', 'object' : '\xff',
                 'start' : 0, 'reason' : 'ordinal not in range'}),
            (UnicodeTranslateError, (u"\u3042", 0, 1, "ouch"),
                {'message' : '', 'args' : (u'\u3042', 0, 1, 'ouch'),
                 'object' : u'\u3042', 'reason' : 'ouch',
                 'start' : 0, 'end' : 1}),
        ]
        try:
            exceptionList.append(
                (WindowsError, (1, 'strErrorStr', 'filenameStr'),
                    {'message' : '', 'args' : (1, 'strErrorStr'),
                     'strerror' : 'strErrorStr', 'winerror' : 1,
                     'errno' : 22, 'filename' : 'filenameStr'})
            )
        except NameError:
            pass

        with catch_warning():
            ignore_message_warning()
            for exc, args, expected in exceptionList:
                try:
                    raise exc(*args)
                except BaseException, e:
                    if type(e) is not exc:
                        raise
                    # Verify module name
                    self.assertEquals(type(e).__module__, 'exceptions')
                    # Verify no ref leaks in Exc_str()
                    s = str(e)
                    for checkArgName in expected:
                        self.assertEquals(repr(getattr(e, checkArgName)),
                                          repr(expected[checkArgName]),
                                          'exception "%s", attribute "%s"' %
                                           (repr(e), checkArgName))

                    # test for pickling support
                    for p in pickle, cPickle:
                        for protocol in range(p.HIGHEST_PROTOCOL + 1):
                            new = p.loads(p.dumps(e, protocol))
                            for checkArgName in expected:
                                got = repr(getattr(new, checkArgName))
                                want = repr(expected[checkArgName])
                                self.assertEquals(got, want,
                                                  'pickled "%r", attribute "%s"' %
                                                  (e, checkArgName))

    def testSlicing(self):
        # Test that you can slice an exception directly instead of requiring
        # going through the 'args' attribute.
        args = (1, 2, 3)
        exc = BaseException(*args)
        self.failUnlessEqual(exc[:], args)

    def testKeywordArgs(self):
        # test that builtin exception don't take keyword args,
        # but user-defined subclasses can if they want
        self.assertRaises(TypeError, BaseException, a=1)

        class DerivedException(BaseException):
            def __init__(self, fancy_arg):
                BaseException.__init__(self)
                self.fancy_arg = fancy_arg

        x = DerivedException(fancy_arg=42)
        self.assertEquals(x.fancy_arg, 42)

    def testInfiniteRecursion(self):
        def f():
            return f()
        self.assertRaises(RuntimeError, f)

        def g():
            try:
                return g()
            except ValueError:
                return -1
        self.assertRaises(RuntimeError, g)

    def testUnicodeStrUsage(self):
        # Make sure both instances and classes have a str and unicode
        # representation.
        self.failUnless(str(Exception))
        self.failUnless(unicode(Exception))
        self.failUnless(str(Exception('a')))
        self.failUnless(unicode(Exception(u'a')))


def test_main():
    run_unittest(ExceptionTests)

if __name__ == '__main__':
    test_main()
