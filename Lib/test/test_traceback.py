"""Test cases for traceback module"""

from _testcapi import traceback_print, exception_print
from io import StringIO
import sys
import unittest
import re
from test.support import run_unittest, Error, captured_output
from test.support import TESTFN, unlink

import traceback


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

    def syntax_error_with_caret_2(self):
        compile("1 +\n", "?", "exec")

    def syntax_error_bad_indentation(self):
        compile("def spam():\n  print(1)\n print(2)", "?", "exec")

    def test_caret(self):
        err = self.get_exception_format(self.syntax_error_with_caret,
                                        SyntaxError)
        self.assertEqual(len(err), 4)
        self.assertTrue(err[1].strip() == "return x!")
        self.assertIn("^", err[2]) # third line has caret
        self.assertEqual(err[1].find("!"), err[2].find("^")) # in the right place

        err = self.get_exception_format(self.syntax_error_with_caret_2,
                                        SyntaxError)
        self.assertIn("^", err[2]) # third line has caret
        self.assertTrue(err[2].count('\n') == 1) # and no additional newline
        self.assertTrue(err[1].find("+") == err[2].find("^")) # in the right place

    def test_nocaret(self):
        exc = SyntaxError("error", ("x.py", 23, None, "bad syntax"))
        err = traceback.format_exception_only(SyntaxError, exc)
        self.assertEqual(len(err), 3)
        self.assertEqual(err[1].strip(), "bad syntax")

    def test_bad_indentation(self):
        err = self.get_exception_format(self.syntax_error_bad_indentation,
                                        IndentationError)
        self.assertEqual(len(err), 4)
        self.assertEqual(err[1].strip(), "print(2)")
        self.assertIn("^", err[2])
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

    def test_encoded_file(self):
        # Test that tracebacks are correctly printed for encoded source files:
        # - correct line number (Issue2384)
        # - respect file encoding (Issue3975)
        import tempfile, sys, subprocess, os

        # The spawned subprocess has its stdout redirected to a PIPE, and its
        # encoding may be different from the current interpreter, on Windows
        # at least.
        process = subprocess.Popen([sys.executable, "-c",
                                    "import sys; print(sys.stdout.encoding)"],
                                   stdout=subprocess.PIPE,
                                   stderr=subprocess.STDOUT)
        stdout, stderr = process.communicate()
        output_encoding = str(stdout, 'ascii').splitlines()[0]

        def do_test(firstlines, message, charset, lineno):
            # Raise the message in a subprocess, and catch the output
            try:
                output = open(TESTFN, "w", encoding=charset)
                output.write("""{0}if 1:
                    import traceback;
                    raise RuntimeError('{1}')
                    """.format(firstlines, message))
                output.close()
                process = subprocess.Popen([sys.executable, TESTFN],
                    stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
                stdout, stderr = process.communicate()
                stdout = stdout.decode(output_encoding).splitlines()
            finally:
                unlink(TESTFN)

            # The source lines are encoded with the 'backslashreplace' handler
            encoded_message = message.encode(output_encoding,
                                             'backslashreplace')
            # and we just decoded them with the output_encoding.
            message_ascii = encoded_message.decode(output_encoding)

            err_line = "raise RuntimeError('{0}')".format(message_ascii)
            err_msg = "RuntimeError: {0}".format(message_ascii)

            self.assertIn(("line %s" % lineno), stdout[1],
                "Invalid line number: {0!r} instead of {1}".format(
                    stdout[1], lineno))
            self.assertTrue(stdout[2].endswith(err_line),
                "Invalid traceback line: {0!r} instead of {1!r}".format(
                    stdout[2], err_line))
            self.assertTrue(stdout[3] == err_msg,
                "Invalid error message: {0!r} instead of {1!r}".format(
                    stdout[3], err_msg))

        do_test("", "foo", "ascii", 3)
        for charset in ("ascii", "iso-8859-1", "utf-8", "GBK"):
            if charset == "ascii":
                text = "foo"
            elif charset == "GBK":
                text = "\u4E02\u5100"
            else:
                text = "h\xe9 ho"
            do_test("# coding: {0}\n".format(charset),
                    text, charset, 4)
            do_test("#!shebang\n# coding: {0}\n".format(charset),
                    text, charset, 5)


class TracebackFormatTests(unittest.TestCase):

    def test_traceback_format(self):
        try:
            raise KeyError('blah')
        except KeyError:
            type_, value, tb = sys.exc_info()
            traceback_fmt = 'Traceback (most recent call last):\n' + \
                            ''.join(traceback.format_tb(tb))
            file_ = StringIO()
            traceback_print(tb, file_)
            python_fmt  = file_.getvalue()
        else:
            raise Error("unable to create test traceback string")

        # Make sure that Python and the traceback module format the same thing
        self.assertEquals(traceback_fmt, python_fmt)

        # Make sure that the traceback is properly indented.
        tb_lines = python_fmt.splitlines()
        self.assertEquals(len(tb_lines), 3)
        banner, location, source_line = tb_lines
        self.assertTrue(banner.startswith('Traceback'))
        self.assertTrue(location.startswith('  File'))
        self.assertTrue(source_line.startswith('    raise'))


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
        self.assertTrue(lines[-3].startswith('  File'))
        self.assertIn('1/0 # In zero_div', lines[-2])
        self.assertTrue(lines[-1].startswith('ZeroDivisionError'), lines[-1])

    def test_simple(self):
        try:
            1/0 # Marker
        except ZeroDivisionError as _:
            e = _
        lines = self.get_report(e).splitlines()
        self.assertEquals(len(lines), 4)
        self.assertTrue(lines[0].startswith('Traceback'))
        self.assertTrue(lines[1].startswith('  File'))
        self.assertIn('1/0 # Marker', lines[2])
        self.assertTrue(lines[3].startswith('ZeroDivisionError'))

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
        self.assertIn('inner_raise() # Marker', blocks[2])

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
        self.assertIn('inner_raise() # Marker', blocks[2])

    def test_cause_and_context(self):
        # When both a cause and a context are set, only the cause should be
        # displayed and the context should be muted.
        def inner_raise():
            try:
                self.zero_div()
            except ZeroDivisionError as _e:
                e = _e
            try:
                xyzzy
            except NameError:
                raise KeyError from e
        def outer_raise():
            inner_raise() # Marker
        blocks = boundaries.split(self.get_report(outer_raise))
        self.assertEquals(len(blocks), 3)
        self.assertEquals(blocks[1], cause_message)
        self.check_zero_div(blocks[0])
        self.assertIn('inner_raise() # Marker', blocks[2])

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
        self.assertIn('raise KeyError from e', blocks[0])
        self.assertNotIn('1/0', blocks[0])
        # The second block (apart from the boundary) is the ZeroDivisionError
        # re-raised from the KeyError
        self.assertIn('inner_raise() # Marker', blocks[2])
        self.check_zero_div(blocks[2])

    def test_syntax_error_offset_at_eol(self):
        # See #10186.
        def e():
            raise SyntaxError('', ('', 0, 5, 'hello'))
        msg = self.get_report(e).splitlines()
        self.assertEqual(msg[-2], "        ^")
        def e():
            exec("x = 5 | 4 |")
        msg = self.get_report(e).splitlines()
        self.assertEqual(msg[-2], '              ^')


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
