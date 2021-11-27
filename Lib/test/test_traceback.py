"""Test cases for traceback module"""

from collections import namedtuple
from io import StringIO
import linecache
import sys
import unittest
import re
from test import support
from test.support import TESTFN, Error, captured_output, unlink, cpython_only, ALWAYS_EQ
from test.support.script_helper import assert_python_ok
import textwrap

import traceback


test_code = namedtuple('code', ['co_filename', 'co_name'])
test_frame = namedtuple('frame', ['f_code', 'f_globals', 'f_locals'])
test_tb = namedtuple('tb', ['tb_frame', 'tb_lineno', 'tb_next'])


class TracebackCases(unittest.TestCase):
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

    def syntax_error_with_caret_non_ascii(self):
        compile('Python = "\u1e54\xfd\u0163\u0125\xf2\xf1" +', "?", "exec")

    def syntax_error_bad_indentation2(self):
        compile(" print(2)", "?", "exec")

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
        self.assertEqual(err[2].count('\n'), 1)   # and no additional newline
        self.assertEqual(err[1].find("+") + 1, err[2].find("^"))  # in the right place

        err = self.get_exception_format(self.syntax_error_with_caret_non_ascii,
                                        SyntaxError)
        self.assertIn("^", err[2]) # third line has caret
        self.assertEqual(err[2].count('\n'), 1)   # and no additional newline
        self.assertEqual(err[1].find("+") + 1, err[2].find("^"))  # in the right place

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
        self.assertEqual(err[1].find(")") + 1, err[2].find("^"))

        # No caret for "unexpected indent"
        err = self.get_exception_format(self.syntax_error_bad_indentation2,
                                        IndentationError)
        self.assertEqual(len(err), 3)
        self.assertEqual(err[1].strip(), "print(2)")

    def test_base_exception(self):
        # Test that exceptions derived from BaseException are formatted right
        e = KeyboardInterrupt()
        lst = traceback.format_exception_only(e.__class__, e)
        self.assertEqual(lst, ['KeyboardInterrupt\n'])

    def test_traceback_context_recursionerror(self):
        # Test that for long traceback chains traceback does not itself
        # raise a recursion error while printing (Issue43048)

        # Calling f() creates a stack-overflowing __context__ chain.
        def f():
            try:
                raise ValueError('hello')
            except ValueError:
                f()

        try:
            f()
        except RecursionError:
            exc_info = sys.exc_info()

        traceback.format_exception(exc_info[0], exc_info[1], exc_info[2])

    def test_traceback_cause_recursionerror(self):
        # Same as test_traceback_context_recursionerror, but with
        # a __cause__ chain.

        def f():
            e = None
            try:
                f()
            except Exception as exc:
                e = exc
            raise Exception from e

        try:
            f()
        except Exception:
            exc_info = sys.exc_info()

        traceback.format_exception(exc_info[0], exc_info[1], exc_info[2])

    def test_format_exception_only_bad__str__(self):
        class X(Exception):
            def __str__(self):
                1/0
        err = traceback.format_exception_only(X, X())
        self.assertEqual(len(err), 1)
        str_value = '<unprintable %s object>' % X.__name__
        if X.__module__ in ('__main__', 'builtins'):
            str_name = X.__qualname__
        else:
            str_name = '.'.join([X.__module__, X.__qualname__])
        self.assertEqual(err[0], "%s: %s\n" % (str_name, str_value))

    def test_encoded_file(self):
        # Test that tracebacks are correctly printed for encoded source files:
        # - correct line number (Issue2384)
        # - respect file encoding (Issue3975)
        import sys, subprocess

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
                with open(TESTFN, "w", encoding=charset) as output:
                    output.write("""{0}if 1:
                        import traceback;
                        raise RuntimeError('{1}')
                        """.format(firstlines, message))

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
            do_test(" \t\f\n# coding: {0}\n".format(charset),
                    text, charset, 5)
        # Issue #18960: coding spec should have no effect
        do_test("x=0\n# coding: GBK\n", "h\xe9 ho", 'utf-8', 5)

    def test_print_traceback_at_exit(self):
        # Issue #22599: Ensure that it is possible to use the traceback module
        # to display an exception at Python exit
        code = textwrap.dedent("""
            import sys
            import traceback

            class PrintExceptionAtExit(object):
                def __init__(self):
                    try:
                        x = 1 / 0
                    except Exception:
                        self.exc_info = sys.exc_info()
                        # self.exc_info[1] (traceback) contains frames:
                        # explicitly clear the reference to self in the current
                        # frame to break a reference cycle
                        self = None

                def __del__(self):
                    traceback.print_exception(*self.exc_info)

            # Keep a reference in the module namespace to call the destructor
            # when the module is unloaded
            obj = PrintExceptionAtExit()
        """)
        rc, stdout, stderr = assert_python_ok('-c', code)
        expected = [b'Traceback (most recent call last):',
                    b'  File "<string>", line 8, in __init__',
                    b'ZeroDivisionError: division by zero']
        self.assertEqual(stderr.splitlines(), expected)

    def test_print_exception(self):
        output = StringIO()
        traceback.print_exception(
            Exception, Exception("projector"), None, file=output
        )
        self.assertEqual(output.getvalue(), "Exception: projector\n")


class TracebackFormatTests(unittest.TestCase):

    def some_exception(self):
        raise KeyError('blah')

    @cpython_only
    def check_traceback_format(self, cleanup_func=None):
        from _testcapi import traceback_print
        try:
            self.some_exception()
        except KeyError:
            type_, value, tb = sys.exc_info()
            if cleanup_func is not None:
                # Clear the inner frames, not this one
                cleanup_func(tb.tb_next)
            traceback_fmt = 'Traceback (most recent call last):\n' + \
                            ''.join(traceback.format_tb(tb))
            file_ = StringIO()
            traceback_print(tb, file_)
            python_fmt  = file_.getvalue()
            # Call all _tb and _exc functions
            with captured_output("stderr") as tbstderr:
                traceback.print_tb(tb)
            tbfile = StringIO()
            traceback.print_tb(tb, file=tbfile)
            with captured_output("stderr") as excstderr:
                traceback.print_exc()
            excfmt = traceback.format_exc()
            excfile = StringIO()
            traceback.print_exc(file=excfile)
        else:
            raise Error("unable to create test traceback string")

        # Make sure that Python and the traceback module format the same thing
        self.assertEqual(traceback_fmt, python_fmt)
        # Now verify the _tb func output
        self.assertEqual(tbstderr.getvalue(), tbfile.getvalue())
        # Now verify the _exc func output
        self.assertEqual(excstderr.getvalue(), excfile.getvalue())
        self.assertEqual(excfmt, excfile.getvalue())

        # Make sure that the traceback is properly indented.
        tb_lines = python_fmt.splitlines()
        self.assertEqual(len(tb_lines), 5)
        banner = tb_lines[0]
        location, source_line = tb_lines[-2:]
        self.assertTrue(banner.startswith('Traceback'))
        self.assertTrue(location.startswith('  File'))
        self.assertTrue(source_line.startswith('    raise'))

    def test_traceback_format(self):
        self.check_traceback_format()

    def test_traceback_format_with_cleared_frames(self):
        # Check that traceback formatting also works with a clear()ed frame
        def cleanup_tb(tb):
            tb.tb_frame.clear()
        self.check_traceback_format(cleanup_tb)

    def test_stack_format(self):
        # Verify _stack functions. Note we have to use _getframe(1) to
        # compare them without this frame appearing in the output
        with captured_output("stderr") as ststderr:
            traceback.print_stack(sys._getframe(1))
        stfile = StringIO()
        traceback.print_stack(sys._getframe(1), file=stfile)
        self.assertEqual(ststderr.getvalue(), stfile.getvalue())

        stfmt = traceback.format_stack(sys._getframe(1))

        self.assertEqual(ststderr.getvalue(), "".join(stfmt))

    def test_print_stack(self):
        def prn():
            traceback.print_stack()
        with captured_output("stderr") as stderr:
            prn()
        lineno = prn.__code__.co_firstlineno
        self.assertEqual(stderr.getvalue().splitlines()[-4:], [
            '  File "%s", line %d, in test_print_stack' % (__file__, lineno+3),
            '    prn()',
            '  File "%s", line %d, in prn' % (__file__, lineno+1),
            '    traceback.print_stack()',
        ])

    # issue 26823 - Shrink recursive tracebacks
    def _check_recursive_traceback_display(self, render_exc):
        # Always show full diffs when this test fails
        # Note that rearranging things may require adjusting
        # the relative line numbers in the expected tracebacks
        self.maxDiff = None

        # Check hitting the recursion limit
        def f():
            f()

        with captured_output("stderr") as stderr_f:
            try:
                f()
            except RecursionError:
                render_exc()
            else:
                self.fail("no recursion occurred")

        lineno_f = f.__code__.co_firstlineno
        result_f = (
            'Traceback (most recent call last):\n'
            f'  File "{__file__}", line {lineno_f+5}, in _check_recursive_traceback_display\n'
            '    f()\n'
            f'  File "{__file__}", line {lineno_f+1}, in f\n'
            '    f()\n'
            f'  File "{__file__}", line {lineno_f+1}, in f\n'
            '    f()\n'
            f'  File "{__file__}", line {lineno_f+1}, in f\n'
            '    f()\n'
            # XXX: The following line changes depending on whether the tests
            # are run through the interactive interpreter or with -m
            # It also varies depending on the platform (stack size)
            # Fortunately, we don't care about exactness here, so we use regex
            r'  \[Previous line repeated (\d+) more times\]' '\n'
            'RecursionError: maximum recursion depth exceeded\n'
        )

        expected = result_f.splitlines()
        actual = stderr_f.getvalue().splitlines()

        # Check the output text matches expectations
        # 2nd last line contains the repetition count
        self.assertEqual(actual[:-2], expected[:-2])
        self.assertRegex(actual[-2], expected[-2])
        # last line can have additional text appended
        self.assertIn(expected[-1], actual[-1])

        # Check the recursion count is roughly as expected
        rec_limit = sys.getrecursionlimit()
        self.assertIn(int(re.search(r"\d+", actual[-2]).group()), range(rec_limit-60, rec_limit))

        # Check a known (limited) number of recursive invocations
        def g(count=10):
            if count:
                return g(count-1)
            raise ValueError

        with captured_output("stderr") as stderr_g:
            try:
                g()
            except ValueError:
                render_exc()
            else:
                self.fail("no value error was raised")

        lineno_g = g.__code__.co_firstlineno
        result_g = (
            f'  File "{__file__}", line {lineno_g+2}, in g\n'
            '    return g(count-1)\n'
            f'  File "{__file__}", line {lineno_g+2}, in g\n'
            '    return g(count-1)\n'
            f'  File "{__file__}", line {lineno_g+2}, in g\n'
            '    return g(count-1)\n'
            '  [Previous line repeated 7 more times]\n'
            f'  File "{__file__}", line {lineno_g+3}, in g\n'
            '    raise ValueError\n'
            'ValueError\n'
        )
        tb_line = (
            'Traceback (most recent call last):\n'
            f'  File "{__file__}", line {lineno_g+7}, in _check_recursive_traceback_display\n'
            '    g()\n'
        )
        expected = (tb_line + result_g).splitlines()
        actual = stderr_g.getvalue().splitlines()
        self.assertEqual(actual, expected)

        # Check 2 different repetitive sections
        def h(count=10):
            if count:
                return h(count-1)
            g()

        with captured_output("stderr") as stderr_h:
            try:
                h()
            except ValueError:
                render_exc()
            else:
                self.fail("no value error was raised")

        lineno_h = h.__code__.co_firstlineno
        result_h = (
            'Traceback (most recent call last):\n'
            f'  File "{__file__}", line {lineno_h+7}, in _check_recursive_traceback_display\n'
            '    h()\n'
            f'  File "{__file__}", line {lineno_h+2}, in h\n'
            '    return h(count-1)\n'
            f'  File "{__file__}", line {lineno_h+2}, in h\n'
            '    return h(count-1)\n'
            f'  File "{__file__}", line {lineno_h+2}, in h\n'
            '    return h(count-1)\n'
            '  [Previous line repeated 7 more times]\n'
            f'  File "{__file__}", line {lineno_h+3}, in h\n'
            '    g()\n'
        )
        expected = (result_h + result_g).splitlines()
        actual = stderr_h.getvalue().splitlines()
        self.assertEqual(actual, expected)

        # Check the boundary conditions. First, test just below the cutoff.
        with captured_output("stderr") as stderr_g:
            try:
                g(traceback._RECURSIVE_CUTOFF)
            except ValueError:
                render_exc()
            else:
                self.fail("no error raised")
        result_g = (
            f'  File "{__file__}", line {lineno_g+2}, in g\n'
            '    return g(count-1)\n'
            f'  File "{__file__}", line {lineno_g+2}, in g\n'
            '    return g(count-1)\n'
            f'  File "{__file__}", line {lineno_g+2}, in g\n'
            '    return g(count-1)\n'
            f'  File "{__file__}", line {lineno_g+3}, in g\n'
            '    raise ValueError\n'
            'ValueError\n'
        )
        tb_line = (
            'Traceback (most recent call last):\n'
            f'  File "{__file__}", line {lineno_g+71}, in _check_recursive_traceback_display\n'
            '    g(traceback._RECURSIVE_CUTOFF)\n'
        )
        expected = (tb_line + result_g).splitlines()
        actual = stderr_g.getvalue().splitlines()
        self.assertEqual(actual, expected)

        # Second, test just above the cutoff.
        with captured_output("stderr") as stderr_g:
            try:
                g(traceback._RECURSIVE_CUTOFF + 1)
            except ValueError:
                render_exc()
            else:
                self.fail("no error raised")
        result_g = (
            f'  File "{__file__}", line {lineno_g+2}, in g\n'
            '    return g(count-1)\n'
            f'  File "{__file__}", line {lineno_g+2}, in g\n'
            '    return g(count-1)\n'
            f'  File "{__file__}", line {lineno_g+2}, in g\n'
            '    return g(count-1)\n'
            '  [Previous line repeated 1 more time]\n'
            f'  File "{__file__}", line {lineno_g+3}, in g\n'
            '    raise ValueError\n'
            'ValueError\n'
        )
        tb_line = (
            'Traceback (most recent call last):\n'
            f'  File "{__file__}", line {lineno_g+99}, in _check_recursive_traceback_display\n'
            '    g(traceback._RECURSIVE_CUTOFF + 1)\n'
        )
        expected = (tb_line + result_g).splitlines()
        actual = stderr_g.getvalue().splitlines()
        self.assertEqual(actual, expected)

    def test_recursive_traceback_python(self):
        self._check_recursive_traceback_display(traceback.print_exc)

    @cpython_only
    def test_recursive_traceback_cpython_internal(self):
        from _testcapi import exception_print
        def render_exc():
            exc_type, exc_value, exc_tb = sys.exc_info()
            exception_print(exc_value)
        self._check_recursive_traceback_display(render_exc)

    def test_format_stack(self):
        def fmt():
            return traceback.format_stack()
        result = fmt()
        lineno = fmt.__code__.co_firstlineno
        self.assertEqual(result[-2:], [
            '  File "%s", line %d, in test_format_stack\n'
            '    result = fmt()\n' % (__file__, lineno+2),
            '  File "%s", line %d, in fmt\n'
            '    return traceback.format_stack()\n' % (__file__, lineno+1),
        ])

    @cpython_only
    def test_unhashable(self):
        from _testcapi import exception_print

        class UnhashableException(Exception):
            def __eq__(self, other):
                return True

        ex1 = UnhashableException('ex1')
        ex2 = UnhashableException('ex2')
        try:
            raise ex2 from ex1
        except UnhashableException:
            try:
                raise ex1
            except UnhashableException:
                exc_type, exc_val, exc_tb = sys.exc_info()

        with captured_output("stderr") as stderr_f:
            exception_print(exc_val)

        tb = stderr_f.getvalue().strip().splitlines()
        self.assertEqual(11, len(tb))
        self.assertEqual(context_message.strip(), tb[5])
        self.assertIn('UnhashableException: ex2', tb[3])
        self.assertIn('UnhashableException: ex1', tb[10])


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
        self.assertEqual(len(lines), 4)
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
        self.assertEqual(len(blocks), 3)
        self.assertEqual(blocks[1], cause_message)
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
        self.assertEqual(len(blocks), 3)
        self.assertEqual(blocks[1], context_message)
        self.check_zero_div(blocks[0])
        self.assertIn('inner_raise() # Marker', blocks[2])

    def test_context_suppression(self):
        try:
            try:
                raise Exception
            except:
                raise ZeroDivisionError from None
        except ZeroDivisionError as _:
            e = _
        lines = self.get_report(e).splitlines()
        self.assertEqual(len(lines), 4)
        self.assertTrue(lines[0].startswith('Traceback'))
        self.assertTrue(lines[1].startswith('  File'))
        self.assertIn('ZeroDivisionError from None', lines[2])
        self.assertTrue(lines[3].startswith('ZeroDivisionError'))

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
        self.assertEqual(len(blocks), 3)
        self.assertEqual(blocks[1], cause_message)
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
        self.assertEqual(len(blocks), 3)
        self.assertEqual(blocks[1], cause_message)
        # The first block is the KeyError raised from the ZeroDivisionError
        self.assertIn('raise KeyError from e', blocks[0])
        self.assertNotIn('1/0', blocks[0])
        # The second block (apart from the boundary) is the ZeroDivisionError
        # re-raised from the KeyError
        self.assertIn('inner_raise() # Marker', blocks[2])
        self.check_zero_div(blocks[2])

    @unittest.skipIf(support.use_old_parser(), "Pegen is arguably better here, so no need to fix this")
    def test_syntax_error_offset_at_eol(self):
        # See #10186.
        def e():
            raise SyntaxError('', ('', 0, 5, 'hello'))
        msg = self.get_report(e).splitlines()
        self.assertEqual(msg[-2], "        ^")
        def e():
            exec("x = 5 | 4 |")
        msg = self.get_report(e).splitlines()
        self.assertEqual(msg[-2], '               ^')

    def test_syntax_error_no_lineno(self):
        # See #34463.

        # Without filename
        e = SyntaxError('bad syntax')
        msg = self.get_report(e).splitlines()
        self.assertEqual(msg,
            ['SyntaxError: bad syntax'])
        e.lineno = 100
        msg = self.get_report(e).splitlines()
        self.assertEqual(msg,
            ['  File "<string>", line 100', 'SyntaxError: bad syntax'])

        # With filename
        e = SyntaxError('bad syntax')
        e.filename = 'myfile.py'

        msg = self.get_report(e).splitlines()
        self.assertEqual(msg,
            ['SyntaxError: bad syntax (myfile.py)'])
        e.lineno = 100
        msg = self.get_report(e).splitlines()
        self.assertEqual(msg,
            ['  File "myfile.py", line 100', 'SyntaxError: bad syntax'])

    def test_message_none(self):
        # A message that looks like "None" should not be treated specially
        err = self.get_report(Exception(None))
        self.assertIn('Exception: None\n', err)
        err = self.get_report(Exception('None'))
        self.assertIn('Exception: None\n', err)
        err = self.get_report(Exception())
        self.assertIn('Exception\n', err)
        err = self.get_report(Exception(''))
        self.assertIn('Exception\n', err)

    def test_exception_modulename_not_unicode(self):
        class X(Exception):
            def __str__(self):
                return "I am X"

        X.__module__ = 42

        err = self.get_report(X())
        exp = f'<unknown>.{X.__qualname__}: I am X\n'
        self.assertEqual(exp, err)

    def test_syntax_error_various_offsets(self):
        for offset in range(-5, 10):
            for add in [0, 2]:
                text = " "*add + "text%d" % offset
                expected = ['  File "file.py", line 1']
                if offset < 1:
                    expected.append("    %s" % text.lstrip())
                elif offset <= 6:
                    expected.append("    %s" % text.lstrip())
                    expected.append("    %s^" % (" "*(offset-1)))
                else:
                    expected.append("    %s" % text.lstrip())
                    expected.append("    %s^" % (" "*5))
                expected.append("SyntaxError: msg")
                expected.append("")
                err = self.get_report(SyntaxError("msg", ("file.py", 1, offset+add, text)))
                exp = "\n".join(expected)
                self.assertEqual(exp, err)

    def test_format_exception_only_qualname(self):
        class A:
            class B:
                class X(Exception):
                    def __str__(self):
                        return "I am X"
                    pass
        err = self.get_report(A.B.X())
        str_value = 'I am X'
        str_name = '.'.join([A.B.X.__module__, A.B.X.__qualname__])
        exp = "%s: %s\n" % (str_name, str_value)
        self.assertEqual(exp, err)


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
        self.assertEqual(sio.getvalue(), s)
        return s


class CExcReportingTests(BaseExceptionReportingTests, unittest.TestCase):
    #
    # This checks built-in reporting by the interpreter.
    #

    @cpython_only
    def get_report(self, e):
        from _testcapi import exception_print
        e = self.get_exception(e)
        with captured_output("stderr") as s:
            exception_print(e)
        return s.getvalue()


class LimitTests(unittest.TestCase):

    ''' Tests for limit argument.
        It's enough to test extact_tb, extract_stack and format_exception '''

    def last_raises1(self):
        raise Exception('Last raised')

    def last_raises2(self):
        self.last_raises1()

    def last_raises3(self):
        self.last_raises2()

    def last_raises4(self):
        self.last_raises3()

    def last_raises5(self):
        self.last_raises4()

    def last_returns_frame1(self):
        return sys._getframe()

    def last_returns_frame2(self):
        return self.last_returns_frame1()

    def last_returns_frame3(self):
        return self.last_returns_frame2()

    def last_returns_frame4(self):
        return self.last_returns_frame3()

    def last_returns_frame5(self):
        return self.last_returns_frame4()

    def test_extract_stack(self):
        frame = self.last_returns_frame5()
        def extract(**kwargs):
            return traceback.extract_stack(frame, **kwargs)
        def assertEqualExcept(actual, expected, ignore):
            self.assertEqual(actual[:ignore], expected[:ignore])
            self.assertEqual(actual[ignore+1:], expected[ignore+1:])
            self.assertEqual(len(actual), len(expected))

        with support.swap_attr(sys, 'tracebacklimit', 1000):
            nolim = extract()
            self.assertGreater(len(nolim), 5)
            self.assertEqual(extract(limit=2), nolim[-2:])
            assertEqualExcept(extract(limit=100), nolim[-100:], -5-1)
            self.assertEqual(extract(limit=-2), nolim[:2])
            assertEqualExcept(extract(limit=-100), nolim[:100], len(nolim)-5-1)
            self.assertEqual(extract(limit=0), [])
            del sys.tracebacklimit
            assertEqualExcept(extract(), nolim, -5-1)
            sys.tracebacklimit = 2
            self.assertEqual(extract(), nolim[-2:])
            self.assertEqual(extract(limit=3), nolim[-3:])
            self.assertEqual(extract(limit=-3), nolim[:3])
            sys.tracebacklimit = 0
            self.assertEqual(extract(), [])
            sys.tracebacklimit = -1
            self.assertEqual(extract(), [])

    def test_extract_tb(self):
        try:
            self.last_raises5()
        except Exception:
            exc_type, exc_value, tb = sys.exc_info()
        def extract(**kwargs):
            return traceback.extract_tb(tb, **kwargs)

        with support.swap_attr(sys, 'tracebacklimit', 1000):
            nolim = extract()
            self.assertEqual(len(nolim), 5+1)
            self.assertEqual(extract(limit=2), nolim[:2])
            self.assertEqual(extract(limit=10), nolim)
            self.assertEqual(extract(limit=-2), nolim[-2:])
            self.assertEqual(extract(limit=-10), nolim)
            self.assertEqual(extract(limit=0), [])
            del sys.tracebacklimit
            self.assertEqual(extract(), nolim)
            sys.tracebacklimit = 2
            self.assertEqual(extract(), nolim[:2])
            self.assertEqual(extract(limit=3), nolim[:3])
            self.assertEqual(extract(limit=-3), nolim[-3:])
            sys.tracebacklimit = 0
            self.assertEqual(extract(), [])
            sys.tracebacklimit = -1
            self.assertEqual(extract(), [])

    def test_format_exception(self):
        try:
            self.last_raises5()
        except Exception:
            exc_type, exc_value, tb = sys.exc_info()
        # [1:-1] to exclude "Traceback (...)" header and
        # exception type and value
        def extract(**kwargs):
            return traceback.format_exception(exc_type, exc_value, tb, **kwargs)[1:-1]

        with support.swap_attr(sys, 'tracebacklimit', 1000):
            nolim = extract()
            self.assertEqual(len(nolim), 5+1)
            self.assertEqual(extract(limit=2), nolim[:2])
            self.assertEqual(extract(limit=10), nolim)
            self.assertEqual(extract(limit=-2), nolim[-2:])
            self.assertEqual(extract(limit=-10), nolim)
            self.assertEqual(extract(limit=0), [])
            del sys.tracebacklimit
            self.assertEqual(extract(), nolim)
            sys.tracebacklimit = 2
            self.assertEqual(extract(), nolim[:2])
            self.assertEqual(extract(limit=3), nolim[:3])
            self.assertEqual(extract(limit=-3), nolim[-3:])
            sys.tracebacklimit = 0
            self.assertEqual(extract(), [])
            sys.tracebacklimit = -1
            self.assertEqual(extract(), [])


class MiscTracebackCases(unittest.TestCase):
    #
    # Check non-printing functions in traceback module
    #

    def test_clear(self):
        def outer():
            middle()
        def middle():
            inner()
        def inner():
            i = 1
            1/0

        try:
            outer()
        except:
            type_, value, tb = sys.exc_info()

        # Initial assertion: there's one local in the inner frame.
        inner_frame = tb.tb_next.tb_next.tb_next.tb_frame
        self.assertEqual(len(inner_frame.f_locals), 1)

        # Clear traceback frames
        traceback.clear_frames(tb)

        # Local variable dict should now be empty.
        self.assertEqual(len(inner_frame.f_locals), 0)

    def test_extract_stack(self):
        def extract():
            return traceback.extract_stack()
        result = extract()
        lineno = extract.__code__.co_firstlineno
        self.assertEqual(result[-2:], [
            (__file__, lineno+2, 'test_extract_stack', 'result = extract()'),
            (__file__, lineno+1, 'extract', 'return traceback.extract_stack()'),
            ])
        self.assertEqual(len(result[0]), 4)


class TestFrame(unittest.TestCase):

    def test_basics(self):
        linecache.clearcache()
        linecache.lazycache("f", globals())
        f = traceback.FrameSummary("f", 1, "dummy")
        self.assertEqual(f,
            ("f", 1, "dummy", '"""Test cases for traceback module"""'))
        self.assertEqual(tuple(f),
            ("f", 1, "dummy", '"""Test cases for traceback module"""'))
        self.assertEqual(f, traceback.FrameSummary("f", 1, "dummy"))
        self.assertEqual(f, tuple(f))
        # Since tuple.__eq__ doesn't support FrameSummary, the equality
        # operator fallbacks to FrameSummary.__eq__.
        self.assertEqual(tuple(f), f)
        self.assertIsNone(f.locals)
        self.assertNotEqual(f, object())
        self.assertEqual(f, ALWAYS_EQ)

    def test_lazy_lines(self):
        linecache.clearcache()
        f = traceback.FrameSummary("f", 1, "dummy", lookup_line=False)
        self.assertEqual(None, f._line)
        linecache.lazycache("f", globals())
        self.assertEqual(
            '"""Test cases for traceback module"""',
            f.line)

    def test_explicit_line(self):
        f = traceback.FrameSummary("f", 1, "dummy", line="line")
        self.assertEqual("line", f.line)

    def test_len(self):
        f = traceback.FrameSummary("f", 1, "dummy", line="line")
        self.assertEqual(len(f), 4)


class TestStack(unittest.TestCase):

    def test_walk_stack(self):
        def deeper():
            return list(traceback.walk_stack(None))
        s1 = list(traceback.walk_stack(None))
        s2 = deeper()
        self.assertEqual(len(s2) - len(s1), 1)
        self.assertEqual(s2[1:], s1)

    def test_walk_tb(self):
        try:
            1/0
        except Exception:
            _, _, tb = sys.exc_info()
        s = list(traceback.walk_tb(tb))
        self.assertEqual(len(s), 1)

    def test_extract_stack(self):
        s = traceback.StackSummary.extract(traceback.walk_stack(None))
        self.assertIsInstance(s, traceback.StackSummary)

    def test_extract_stack_limit(self):
        s = traceback.StackSummary.extract(traceback.walk_stack(None), limit=5)
        self.assertEqual(len(s), 5)

    def test_extract_stack_lookup_lines(self):
        linecache.clearcache()
        linecache.updatecache('/foo.py', globals())
        c = test_code('/foo.py', 'method')
        f = test_frame(c, None, None)
        s = traceback.StackSummary.extract(iter([(f, 6)]), lookup_lines=True)
        linecache.clearcache()
        self.assertEqual(s[0].line, "import sys")

    def test_extract_stackup_deferred_lookup_lines(self):
        linecache.clearcache()
        c = test_code('/foo.py', 'method')
        f = test_frame(c, None, None)
        s = traceback.StackSummary.extract(iter([(f, 6)]), lookup_lines=False)
        self.assertEqual({}, linecache.cache)
        linecache.updatecache('/foo.py', globals())
        self.assertEqual(s[0].line, "import sys")

    def test_from_list(self):
        s = traceback.StackSummary.from_list([('foo.py', 1, 'fred', 'line')])
        self.assertEqual(
            ['  File "foo.py", line 1, in fred\n    line\n'],
            s.format())

    def test_from_list_edited_stack(self):
        s = traceback.StackSummary.from_list([('foo.py', 1, 'fred', 'line')])
        s[0] = ('foo.py', 2, 'fred', 'line')
        s2 = traceback.StackSummary.from_list(s)
        self.assertEqual(
            ['  File "foo.py", line 2, in fred\n    line\n'],
            s2.format())

    def test_format_smoke(self):
        # For detailed tests see the format_list tests, which consume the same
        # code.
        s = traceback.StackSummary.from_list([('foo.py', 1, 'fred', 'line')])
        self.assertEqual(
            ['  File "foo.py", line 1, in fred\n    line\n'],
            s.format())

    def test_locals(self):
        linecache.updatecache('/foo.py', globals())
        c = test_code('/foo.py', 'method')
        f = test_frame(c, globals(), {'something': 1})
        s = traceback.StackSummary.extract(iter([(f, 6)]), capture_locals=True)
        self.assertEqual(s[0].locals, {'something': '1'})

    def test_no_locals(self):
        linecache.updatecache('/foo.py', globals())
        c = test_code('/foo.py', 'method')
        f = test_frame(c, globals(), {'something': 1})
        s = traceback.StackSummary.extract(iter([(f, 6)]))
        self.assertEqual(s[0].locals, None)

    def test_format_locals(self):
        def some_inner(k, v):
            a = 1
            b = 2
            return traceback.StackSummary.extract(
                traceback.walk_stack(None), capture_locals=True, limit=1)
        s = some_inner(3, 4)
        self.assertEqual(
            ['  File "%s", line %d, in some_inner\n'
             '    return traceback.StackSummary.extract(\n'
             '    a = 1\n'
             '    b = 2\n'
             '    k = 3\n'
             '    v = 4\n' % (__file__, some_inner.__code__.co_firstlineno + 3)
            ], s.format())

class TestTracebackException(unittest.TestCase):

    def test_smoke(self):
        try:
            1/0
        except Exception:
            exc_info = sys.exc_info()
            exc = traceback.TracebackException(*exc_info)
            expected_stack = traceback.StackSummary.extract(
                traceback.walk_tb(exc_info[2]))
        self.assertEqual(None, exc.__cause__)
        self.assertEqual(None, exc.__context__)
        self.assertEqual(False, exc.__suppress_context__)
        self.assertEqual(expected_stack, exc.stack)
        self.assertEqual(exc_info[0], exc.exc_type)
        self.assertEqual(str(exc_info[1]), str(exc))

    def test_from_exception(self):
        # Check all the parameters are accepted.
        def foo():
            1/0
        try:
            foo()
        except Exception as e:
            exc_info = sys.exc_info()
            self.expected_stack = traceback.StackSummary.extract(
                traceback.walk_tb(exc_info[2]), limit=1, lookup_lines=False,
                capture_locals=True)
            self.exc = traceback.TracebackException.from_exception(
                e, limit=1, lookup_lines=False, capture_locals=True)
        expected_stack = self.expected_stack
        exc = self.exc
        self.assertEqual(None, exc.__cause__)
        self.assertEqual(None, exc.__context__)
        self.assertEqual(False, exc.__suppress_context__)
        self.assertEqual(expected_stack, exc.stack)
        self.assertEqual(exc_info[0], exc.exc_type)
        self.assertEqual(str(exc_info[1]), str(exc))

    def test_cause(self):
        try:
            try:
                1/0
            finally:
                exc_info_context = sys.exc_info()
                exc_context = traceback.TracebackException(*exc_info_context)
                cause = Exception("cause")
                raise Exception("uh oh") from cause
        except Exception:
            exc_info = sys.exc_info()
            exc = traceback.TracebackException(*exc_info)
            expected_stack = traceback.StackSummary.extract(
                traceback.walk_tb(exc_info[2]))
        exc_cause = traceback.TracebackException(Exception, cause, None)
        self.assertEqual(exc_cause, exc.__cause__)
        self.assertEqual(exc_context, exc.__context__)
        self.assertEqual(True, exc.__suppress_context__)
        self.assertEqual(expected_stack, exc.stack)
        self.assertEqual(exc_info[0], exc.exc_type)
        self.assertEqual(str(exc_info[1]), str(exc))

    def test_context(self):
        try:
            try:
                1/0
            finally:
                exc_info_context = sys.exc_info()
                exc_context = traceback.TracebackException(*exc_info_context)
                raise Exception("uh oh")
        except Exception:
            exc_info = sys.exc_info()
            exc = traceback.TracebackException(*exc_info)
            expected_stack = traceback.StackSummary.extract(
                traceback.walk_tb(exc_info[2]))
        self.assertEqual(None, exc.__cause__)
        self.assertEqual(exc_context, exc.__context__)
        self.assertEqual(False, exc.__suppress_context__)
        self.assertEqual(expected_stack, exc.stack)
        self.assertEqual(exc_info[0], exc.exc_type)
        self.assertEqual(str(exc_info[1]), str(exc))

    def test_no_refs_to_exception_and_traceback_objects(self):
        try:
            1/0
        except Exception:
            exc_info = sys.exc_info()

        refcnt1 = sys.getrefcount(exc_info[1])
        refcnt2 = sys.getrefcount(exc_info[2])
        exc = traceback.TracebackException(*exc_info)
        self.assertEqual(sys.getrefcount(exc_info[1]), refcnt1)
        self.assertEqual(sys.getrefcount(exc_info[2]), refcnt2)

    def test_comparison_basic(self):
        try:
            1/0
        except Exception:
            exc_info = sys.exc_info()
            exc = traceback.TracebackException(*exc_info)
            exc2 = traceback.TracebackException(*exc_info)
        self.assertIsNot(exc, exc2)
        self.assertEqual(exc, exc2)
        self.assertNotEqual(exc, object())
        self.assertEqual(exc, ALWAYS_EQ)

    def test_comparison_params_variations(self):
        def raise_exc():
            try:
                raise ValueError('bad value')
            except:
                raise

        def raise_with_locals():
            x, y = 1, 2
            raise_exc()

        try:
            raise_with_locals()
        except Exception:
            exc_info = sys.exc_info()

        exc = traceback.TracebackException(*exc_info)
        exc1 = traceback.TracebackException(*exc_info, limit=10)
        exc2 = traceback.TracebackException(*exc_info, limit=2)

        self.assertEqual(exc, exc1)      # limit=10 gets all frames
        self.assertNotEqual(exc, exc2)   # limit=2 truncates the output

        # locals change the output
        exc3 = traceback.TracebackException(*exc_info, capture_locals=True)
        self.assertNotEqual(exc, exc3)

        # there are no locals in the innermost frame
        exc4 = traceback.TracebackException(*exc_info, limit=-1)
        exc5 = traceback.TracebackException(*exc_info, limit=-1, capture_locals=True)
        self.assertEqual(exc4, exc5)

        # there are locals in the next-to-innermost frame
        exc6 = traceback.TracebackException(*exc_info, limit=-2)
        exc7 = traceback.TracebackException(*exc_info, limit=-2, capture_locals=True)
        self.assertNotEqual(exc6, exc7)

    def test_comparison_equivalent_exceptions_are_equal(self):
        excs = []
        for _ in range(2):
            try:
                1/0
            except:
                excs.append(traceback.TracebackException(*sys.exc_info()))
        self.assertEqual(excs[0], excs[1])
        self.assertEqual(list(excs[0].format()), list(excs[1].format()))

    def test_unhashable(self):
        class UnhashableException(Exception):
            def __eq__(self, other):
                return True

        ex1 = UnhashableException('ex1')
        ex2 = UnhashableException('ex2')
        try:
            raise ex2 from ex1
        except UnhashableException:
            try:
                raise ex1
            except UnhashableException:
                exc_info = sys.exc_info()
        exc = traceback.TracebackException(*exc_info)
        formatted = list(exc.format())
        self.assertIn('UnhashableException: ex2\n', formatted[2])
        self.assertIn('UnhashableException: ex1\n', formatted[6])

    def test_limit(self):
        def recurse(n):
            if n:
                recurse(n-1)
            else:
                1/0
        try:
            recurse(10)
        except Exception:
            exc_info = sys.exc_info()
            exc = traceback.TracebackException(*exc_info, limit=5)
            expected_stack = traceback.StackSummary.extract(
                traceback.walk_tb(exc_info[2]), limit=5)
        self.assertEqual(expected_stack, exc.stack)

    def test_lookup_lines(self):
        linecache.clearcache()
        e = Exception("uh oh")
        c = test_code('/foo.py', 'method')
        f = test_frame(c, None, None)
        tb = test_tb(f, 6, None)
        exc = traceback.TracebackException(Exception, e, tb, lookup_lines=False)
        self.assertEqual(linecache.cache, {})
        linecache.updatecache('/foo.py', globals())
        self.assertEqual(exc.stack[0].line, "import sys")

    def test_locals(self):
        linecache.updatecache('/foo.py', globals())
        e = Exception("uh oh")
        c = test_code('/foo.py', 'method')
        f = test_frame(c, globals(), {'something': 1, 'other': 'string'})
        tb = test_tb(f, 6, None)
        exc = traceback.TracebackException(
            Exception, e, tb, capture_locals=True)
        self.assertEqual(
            exc.stack[0].locals, {'something': '1', 'other': "'string'"})

    def test_no_locals(self):
        linecache.updatecache('/foo.py', globals())
        e = Exception("uh oh")
        c = test_code('/foo.py', 'method')
        f = test_frame(c, globals(), {'something': 1})
        tb = test_tb(f, 6, None)
        exc = traceback.TracebackException(Exception, e, tb)
        self.assertEqual(exc.stack[0].locals, None)

    def test_traceback_header(self):
        # do not print a traceback header if exc_traceback is None
        # see issue #24695
        exc = traceback.TracebackException(Exception, Exception("haven"), None)
        self.assertEqual(list(exc.format()), ["Exception: haven\n"])


class MiscTest(unittest.TestCase):

    def test_all(self):
        expected = set()
        blacklist = {'print_list'}
        for name in dir(traceback):
            if name.startswith('_') or name in blacklist:
                continue
            module_object = getattr(traceback, name)
            if getattr(module_object, '__module__', None) == 'traceback':
                expected.add(name)
        self.assertCountEqual(traceback.__all__, expected)


if __name__ == "__main__":
    unittest.main()
