# Python test set -- part 5, built-in exceptions

import os
import sys
import unittest
import pickle, cPickle

from test.test_support import (TESTFN, unlink, run_unittest, captured_output,
                               check_warnings)
from test.test_pep352 import ignore_deprecation_warnings

# XXX This is not really enough, each *operation* should be tested!

class ExceptionTests(unittest.TestCase):

    def testReload(self):
        # Reloading the built-in exceptions module failed prior to Py2.2, while it
        # should act the same as reloading built-in sys.
        try:
            from imp import reload
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
        try: x = 1 // 0
        except ZeroDivisionError: pass

        self.raise_catch(Exception, "Exception")
        try: x = 1 // 0
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

    @ignore_deprecation_warnings
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


    def testDeprecatedMessageAttribute(self):
        # Accessing BaseException.message and relying on its value set by
        # BaseException.__init__ triggers a deprecation warning.
        exc = BaseException("foo")
        with check_warnings(("BaseException.message has been deprecated "
                             "as of Python 2.6", DeprecationWarning)) as w:
            self.assertEqual(exc.message, "foo")
        self.assertEqual(len(w.warnings), 1)

    def testRegularMessageAttribute(self):
        # Accessing BaseException.message after explicitly setting a value
        # for it does not trigger a deprecation warning.
        exc = BaseException("foo")
        exc.message = "bar"
        with check_warnings(quiet=True) as w:
            self.assertEqual(exc.message, "bar")
        self.assertEqual(len(w.warnings), 0)
        # Deleting the message is supported, too.
        del exc.message
        self.assertRaises(AttributeError, getattr, exc, "message")

    @ignore_deprecation_warnings
    def testPickleMessageAttribute(self):
        # Pickling with message attribute must work, as well.
        e = Exception("foo")
        f = Exception("foo")
        f.message = "bar"
        for p in pickle, cPickle:
            ep = p.loads(p.dumps(e))
            self.assertEqual(ep.message, "foo")
            fp = p.loads(p.dumps(f))
            self.assertEqual(fp.message, "bar")

    @ignore_deprecation_warnings
    def testSlicing(self):
        # Test that you can slice an exception directly instead of requiring
        # going through the 'args' attribute.
        args = (1, 2, 3)
        exc = BaseException(*args)
        self.failUnlessEqual(exc[:], args)
        self.assertEqual(exc.args[:], args)

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

        # The test prints an unraisable recursion error when
        # doing "except ValueError", this is because subclass
        # checking has recursion checking too.
        with captured_output("stderr"):
            try:
                g()
            except RuntimeError:
                pass
            except:
                self.fail("Should have raised KeyError")
            else:
                self.fail("Should have raised KeyError")

    def testUnicodeStrUsage(self):
        # Make sure both instances and classes have a str and unicode
        # representation.
        self.failUnless(str(Exception))
        self.failUnless(unicode(Exception))
        self.failUnless(str(Exception('a')))
        self.failUnless(unicode(Exception(u'a')))
        self.failUnless(unicode(Exception(u'\xe1')))

    def testUnicodeChangeAttributes(self):
        # See issue 7309. This was a crasher.

        u = UnicodeEncodeError('baz', u'xxxxx', 1, 5, 'foo')
        self.assertEqual(str(u), "'baz' codec can't encode characters in position 1-4: foo")
        u.end = 2
        self.assertEqual(str(u), "'baz' codec can't encode character u'\\x78' in position 1: foo")
        u.end = 5
        u.reason = 0x345345345345345345
        self.assertEqual(str(u), "'baz' codec can't encode characters in position 1-4: 965230951443685724997")
        u.encoding = 4000
        self.assertEqual(str(u), "'4000' codec can't encode characters in position 1-4: 965230951443685724997")
        u.start = 1000
        self.assertEqual(str(u), "'4000' codec can't encode characters in position 1000-4: 965230951443685724997")

        u = UnicodeDecodeError('baz', 'xxxxx', 1, 5, 'foo')
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

        u = UnicodeTranslateError(u'xxxx', 1, 5, 'foo')
        self.assertEqual(str(u), "can't translate characters in position 1-4: foo")
        u.end = 2
        self.assertEqual(str(u), "can't translate character u'\\x78' in position 1: foo")
        u.end = 5
        u.reason = 0x345345345345345345
        self.assertEqual(str(u), "can't translate characters in position 1-4: 965230951443685724997")
        u.start = 1000
        self.assertEqual(str(u), "can't translate characters in position 1000-4: 965230951443685724997")

    def test_badisinstance(self):
        # Bug #2542: if issubclass(e, MyException) raises an exception,
        # it should be ignored
        class Meta(type):
            def __subclasscheck__(cls, subclass):
                raise ValueError()

        class MyException(Exception):
            __metaclass__ = Meta
            pass

        with captured_output("stderr") as stderr:
            try:
                raise KeyError()
            except MyException, e:
                self.fail("exception should not be a MyException")
            except KeyError:
                pass
            except:
                self.fail("Should have raised KeyError")
            else:
                self.fail("Should have raised KeyError")

        with captured_output("stderr") as stderr:
            def g():
                try:
                    return g()
                except RuntimeError:
                    return sys.exc_info()
            e, v, tb = g()
            self.assert_(e is RuntimeError, e)
            self.assert_("maximum recursion depth exceeded" in str(v), v)



# Helper class used by TestSameStrAndUnicodeMsg
class ExcWithOverriddenStr(Exception):
    """Subclass of Exception that accepts a keyword 'msg' arg that is
    returned by __str__. 'msg' won't be included in self.args"""
    def __init__(self, *args, **kwargs):
        self.msg = kwargs.pop('msg') # msg should always be present
        super(ExcWithOverriddenStr, self).__init__(*args, **kwargs)
    def __str__(self):
        return self.msg


class TestSameStrAndUnicodeMsg(unittest.TestCase):
    """unicode(err) should return the same message of str(err). See #6108"""

    def check_same_msg(self, exc, msg):
        """Helper function that checks if str(exc) == unicode(exc) == msg"""
        self.assertEqual(str(exc), msg)
        self.assertEqual(str(exc), unicode(exc))

    def test_builtin_exceptions(self):
        """Check same msg for built-in exceptions"""
        # These exceptions implement a __str__ method that uses the args
        # to create a better error message. unicode(e) should return the same
        # message.
        exceptions = [
            SyntaxError('invalid syntax', ('<string>', 1, 3, '2+*3')),
            IOError(2, 'No such file or directory'),
            KeyError('both should have the same quotes'),
            UnicodeDecodeError('ascii', '\xc3\xa0', 0, 1,
                               'ordinal not in range(128)'),
            UnicodeEncodeError('ascii', u'\u1234', 0, 1,
                               'ordinal not in range(128)')
        ]
        for exception in exceptions:
            self.assertEqual(str(exception), unicode(exception))

    def test_0_args(self):
        """Check same msg for Exception with 0 args"""
        # str() and unicode() on an Exception with no args should return an
        # empty string
        self.check_same_msg(Exception(), '')

    def test_0_args_with_overridden___str__(self):
        """Check same msg for exceptions with 0 args and overridden __str__"""
        # str() and unicode() on an exception with overridden __str__ that
        # returns an ascii-only string should return the same string
        for msg in ('foo', u'foo'):
            self.check_same_msg(ExcWithOverriddenStr(msg=msg), msg)

        # if __str__ returns a non-ascii unicode string str() should fail
        # but unicode() should return the unicode string
        e = ExcWithOverriddenStr(msg=u'f\xf6\xf6') # no args
        self.assertRaises(UnicodeEncodeError, str, e)
        self.assertEqual(unicode(e), u'f\xf6\xf6')

    def test_1_arg(self):
        """Check same msg for Exceptions with 1 arg"""
        for arg in ('foo', u'foo'):
            self.check_same_msg(Exception(arg), arg)

        # if __str__ is not overridden and self.args[0] is a non-ascii unicode
        # string, str() should try to return str(self.args[0]) and fail.
        # unicode() should return unicode(self.args[0]) and succeed.
        e = Exception(u'f\xf6\xf6')
        self.assertRaises(UnicodeEncodeError, str, e)
        self.assertEqual(unicode(e), u'f\xf6\xf6')

    def test_1_arg_with_overridden___str__(self):
        """Check same msg for exceptions with overridden __str__ and 1 arg"""
        # when __str__ is overridden and __unicode__ is not implemented
        # unicode(e) returns the same as unicode(e.__str__()).
        for msg in ('foo', u'foo'):
            self.check_same_msg(ExcWithOverriddenStr('arg', msg=msg), msg)

        # if __str__ returns a non-ascii unicode string, str() should fail
        # but unicode() should succeed.
        e = ExcWithOverriddenStr('arg', msg=u'f\xf6\xf6') # 1 arg
        self.assertRaises(UnicodeEncodeError, str, e)
        self.assertEqual(unicode(e), u'f\xf6\xf6')

    def test_many_args(self):
        """Check same msg for Exceptions with many args"""
        argslist = [
            (3, 'foo'),
            (1, u'foo', 'bar'),
            (4, u'f\xf6\xf6', u'bar', 'baz')
        ]
        # both str() and unicode() should return a repr() of the args
        for args in argslist:
            self.check_same_msg(Exception(*args), repr(args))

    def test_many_args_with_overridden___str__(self):
        """Check same msg for exceptions with overridden __str__ and many args"""
        # if __str__ returns an ascii string / ascii unicode string
        # both str() and unicode() should succeed
        for msg in ('foo', u'foo'):
            e = ExcWithOverriddenStr('arg1', u'arg2', u'f\xf6\xf6', msg=msg)
            self.check_same_msg(e, msg)

        # if __str__ returns a non-ascii unicode string, str() should fail
        # but unicode() should succeed
        e = ExcWithOverriddenStr('arg1', u'f\xf6\xf6', u'arg3', # 3 args
                                 msg=u'f\xf6\xf6')
        self.assertRaises(UnicodeEncodeError, str, e)
        self.assertEqual(unicode(e), u'f\xf6\xf6')


def test_main():
    run_unittest(ExceptionTests, TestSameStrAndUnicodeMsg)

if __name__ == '__main__':
    test_main()
