"""Test cases for traceback module"""

from _testcapi import traceback_print, exception_print
from io import StringIO
import sys
import unittest
import re
from test.support import run_unittest, is_jython, Error, captured_output

import traceback

try:
    raise KeyError
except KeyError:
    type_, value, tb = sys.exc_info()
    file_ = StringIO()
    traceback_print(tb, file_)
    example_traceback = file_.getvalue()
else:
    raise Error("unable to create test traceback string")


class SyntaxTracebackCases(unittest.TestCase):
    # For now, a very minimal set of tests.  I want to be sure that
    # formatting of SyntaxErrors works based on changes for 2.1.

    def get_exception_format(self, func, exc):
        try:
            func()
        except exc as value:
            return traceback.format_exception_only(exc, value)
        else:
            raise ValueError("call did not raise exception")

    def syntax_error_with_caret(self):
        compile("def fact(x):\n\treturn x!\n", "?", "exec")

    def syntax_error_without_caret(self):
        # XXX why doesn't compile raise the same traceback?
        import test.badsyntax_nocaret

    def syntax_error_bad_indentation(self):
        compile("def spam():\n  print(1)\n print(2)", "?", "exec")

    def test_caret(self):
        err = self.get_exception_format(self.syntax_error_with_caret,
                                        SyntaxError)
        self.assertEqual(len(err), 4)
        self.assert_(err[1].strip() == "return x!")
        self.assert_("^" in err[2]) # third line has caret
        self.assertEqual(err[1].find("!"), err[2].find("^")) # in the right place

    def test_nocaret(self):
        if is_jython:
            # jython adds a caret in this case (why shouldn't it?)
            return
        err = self.get_exception_format(self.syntax_error_without_caret,
                                        SyntaxError)
        self.assertEqual(len(err), 3)
        self.assert_(err[1].strip() == "[x for x in x] = x")

    def test_bad_indentation(self):
        err = self.get_exception_format(self.syntax_error_bad_indentation,
                                        IndentationError)
        self.assertEqual(len(err), 4)
        self.assertEqual(err[1].strip(), "print(2)")
        self.assert_("^" in err[2])
        self.assertEqual(err[1].find(")"), err[2].find("^"))

    def test_base_exception(self):
        # Test that exceptions derived from BaseException are formatted right
        e = KeyboardInterrupt()
        lst = traceback.format_exception_only(e.__class__, e)
        self.assertEqual(lst, ['KeyboardInterrupt\n'])

    def test_format_exception_only_bad__str__(self):
        class X(Exception):
            def __str__(self):
                1/0
        err = traceback.format_exception_only(X, X())
        self.assertEqual(len(err), 1)
        str_value = '<unprintable %s object>' % X.__name__
        if X.__module__ in ('__main__', 'builtins'):
            str_name = X.__name__
        else:
            str_name = '.'.join([X.__module__, X.__name__])
        self.assertEqual(err[0], "%s: %s\n" % (str_name, str_value))

    def test_without_exception(self):
        err = traceback.format_exception_only(None, None)
        self.assertEqual(err, ['None\n'])


class TracebackFormatTests(unittest.TestCase):

    def test_traceback_indentation(self):
        # Make sure that the traceback is properly indented.
        tb_lines = example_traceback.splitlines()
        self.assertEquals(len(tb_lines), 3)
        banner, location, source_line = tb_lines
        self.assert_(banner.startswith('Traceback'))
        self.assert_(location.startswith('  File'))
        self.assert_(source_line.startswith('    raise'))


cause_message = (
    "\nThe above exception was the direct cause "
    "of the following exception:\n\n")

context_message = (
    "\nDuring handling of the above exception, "
    "another exception occurred:\n\n")

boundaries = re.compile(
    '(%s|%s)' % (re.escape(cause_message), re.escape(context_message)))


class BaseExceptionReportingTests:

    def get_exception(self, exception_or_callable):
        if isinstance(exception_or_callable, Exception):
            return exception_or_callable
        try:
            exception_or_callable()
        except Exception as e:
            return e

    def zero_div(self):
        1/0 # In zero_div

    def check_zero_div(self, msg):
        lines = msg.splitlines()
        self.assert_(lines[-3].startswith('  File'))
        self.assert_('1/0 # In zero_div' in lines[-2], lines[-2])
        self.assert_(lines[-1].startswith('ZeroDivisionError'), lines[-1])

    def test_simple(self):
        try:
            1/0 # Marker
        except ZeroDivisionError as _:
            e = _
        lines = self.get_report(e).splitlines()
        self.assertEquals(len(lines), 4)
        self.assert_(lines[0].startswith('Traceback'))
        self.assert_(lines[1].startswith('  File'))
        self.assert_('1/0 # Marker' in lines[2])
        self.assert_(lines[3].startswith('ZeroDivisionError'))

    def test_cause(self):
        def inner_raise():
            try:
                self.zero_div()
            except ZeroDivisionError as e:
                raise KeyError from e
        def outer_raise():
            inner_raise() # Marker
        blocks = boundaries.split(self.get_report(outer_raise))
        self.assertEquals(len(blocks), 3)
        self.assertEquals(blocks[1], cause_message)
        self.check_zero_div(blocks[0])
        self.assert_('inner_raise() # Marker' in blocks[2])

    def test_context(self):
        def inner_raise():
            try:
                self.zero_div()
            except ZeroDivisionError:
                raise KeyError
        def outer_raise():
            inner_raise() # Marker
        blocks = boundaries.split(self.get_report(outer_raise))
        self.assertEquals(len(blocks), 3)
        self.assertEquals(blocks[1], context_message)
        self.check_zero_div(blocks[0])
        self.assert_('inner_raise() # Marker' in blocks[2])

    def test_cause_recursive(self):
        def inner_raise():
            try:
                try:
                    self.zero_div()
                except ZeroDivisionError as e:
                    z = e
                    raise KeyError from e
            except KeyError as e:
                raise z from e
        def outer_raise():
            inner_raise() # Marker
        blocks = boundaries.split(self.get_report(outer_raise))
        self.assertEquals(len(blocks), 3)
        self.assertEquals(blocks[1], cause_message)
        # The first block is the KeyError raised from the ZeroDivisionError
        self.assert_('raise KeyError from e' in blocks[0])
        self.assert_('1/0' not in blocks[0])
        # The second block (apart from the boundary) is the ZeroDivisionError
        # re-raised from the KeyError
        self.assert_('inner_raise() # Marker' in blocks[2])
        self.check_zero_div(blocks[2])



class PyExcReportingTests(BaseExceptionReportingTests, unittest.TestCase):
    #
    # This checks reporting through the 'traceback' module, with both
    # format_exception() and print_exception().
    #

    def get_report(self, e):
        e = self.get_exception(e)
        s = ''.join(
            traceback.format_exception(type(e), e, e.__traceback__))
        with captured_output("stderr") as sio:
            traceback.print_exception(type(e), e, e.__traceback__)
        self.assertEquals(sio.getvalue(), s)
        return s


class CExcReportingTests(BaseExceptionReportingTests, unittest.TestCase):
    #
    # This checks built-in reporting by the interpreter.
    #

    def get_report(self, e):
        e = self.get_exception(e)
        with captured_output("stderr") as s:
            exception_print(e)
        return s.getvalue()


def test_main():
    run_unittest(__name__)

if __name__ == "__main__":
    test_main()
