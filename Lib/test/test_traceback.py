"""Test cases for traceback module"""

from collections import namedtuple
from io import StringIO
import linecache
import sys
import types
import inspect
import unittest
import re
from test import support
from test.support import (Error, captured_output, cpython_only, ALWAYS_EQ,
                          requires_debug_ranges, has_no_debug_ranges,
                          requires_subprocess)
from test.support.os_helper import TESTFN, unlink
from test.support.script_helper import assert_python_ok, assert_python_failure

import os
import textwrap
import traceback
from functools import partial

MODULE_PREFIX = f'{__name__}.' if __name__ == '__main__' else ''

test_code = namedtuple('code', ['co_filename', 'co_name'])
test_code.co_positions = lambda _: iter([(6, 6, 0, 0)])
test_frame = namedtuple('frame', ['f_code', 'f_globals', 'f_locals'])
test_tb = namedtuple('tb', ['tb_frame', 'tb_lineno', 'tb_next', 'tb_lasti'])


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

    def syntax_error_with_caret_range(self):
        compile("f(x, y for y in range(30), z)", "?", "exec")

    def syntax_error_bad_indentation(self):
        compile("def spam():\n  print(1)\n print(2)", "?", "exec")

    def syntax_error_with_caret_non_ascii(self):
        compile('Python = "\u1e54\xfd\u0163\u0125\xf2\xf1" +', "?", "exec")

    def syntax_error_bad_indentation2(self):
        compile(" print(2)", "?", "exec")

    def tokenizer_error_with_caret_range(self):
        compile("blech  (  ", "?", "exec")

    def test_caret(self):
        err = self.get_exception_format(self.syntax_error_with_caret,
                                        SyntaxError)
        self.assertEqual(len(err), 4)
        self.assertTrue(err[1].strip() == "return x!")
        self.assertIn("^", err[2]) # third line has caret
        self.assertEqual(err[1].find("!"), err[2].find("^")) # in the right place
        self.assertEqual(err[2].count("^"), 1)

        err = self.get_exception_format(self.syntax_error_with_caret_2,
                                        SyntaxError)
        self.assertIn("^", err[2]) # third line has caret
        self.assertEqual(err[2].count('\n'), 1)   # and no additional newline
        self.assertEqual(err[1].find("+") + 1, err[2].find("^"))  # in the right place
        self.assertEqual(err[2].count("^"), 1)

        err = self.get_exception_format(self.syntax_error_with_caret_non_ascii,
                                        SyntaxError)
        self.assertIn("^", err[2]) # third line has caret
        self.assertEqual(err[2].count('\n'), 1)   # and no additional newline
        self.assertEqual(err[1].find("+") + 1, err[2].find("^"))  # in the right place
        self.assertEqual(err[2].count("^"), 1)

        err = self.get_exception_format(self.syntax_error_with_caret_range,
                                        SyntaxError)
        self.assertIn("^", err[2]) # third line has caret
        self.assertEqual(err[2].count('\n'), 1)   # and no additional newline
        self.assertEqual(err[1].find("y"), err[2].find("^"))  # in the right place
        self.assertEqual(err[2].count("^"), len("y for y in range(30)"))

        err = self.get_exception_format(self.tokenizer_error_with_caret_range,
                                        SyntaxError)
        self.assertIn("^", err[2]) # third line has caret
        self.assertEqual(err[2].count('\n'), 1)   # and no additional newline
        self.assertEqual(err[1].find("("), err[2].find("^"))  # in the right place
        self.assertEqual(err[2].count("^"), 1)

    def test_nocaret(self):
        exc = SyntaxError("error", ("x.py", 23, None, "bad syntax"))
        err = traceback.format_exception_only(SyntaxError, exc)
        self.assertEqual(len(err), 3)
        self.assertEqual(err[1].strip(), "bad syntax")

    def test_no_caret_with_no_debug_ranges_flag(self):
        # Make sure that if `-X no_debug_ranges` is used, there are no carets
        # in the traceback.
        try:
            with open(TESTFN, 'w') as f:
                f.write("x = 1 / 0\n")

            _, _, stderr = assert_python_failure(
                '-X', 'no_debug_ranges', TESTFN)

            lines = stderr.splitlines()
            self.assertEqual(len(lines), 4)
            self.assertEqual(lines[0], b'Traceback (most recent call last):')
            self.assertIn(b'line 1, in <module>', lines[1])
            self.assertEqual(lines[2], b'    x = 1 / 0')
            self.assertEqual(lines[3], b'ZeroDivisionError: division by zero')
        finally:
            unlink(TESTFN)

    def test_no_caret_with_no_debug_ranges_flag_python_traceback(self):
        code = textwrap.dedent("""
            import traceback
            try:
                x = 1 / 0
            except:
                traceback.print_exc()
            """)
        try:
            with open(TESTFN, 'w') as f:
                f.write(code)

            _, _, stderr = assert_python_ok(
                '-X', 'no_debug_ranges', TESTFN)

            lines = stderr.splitlines()
            self.assertEqual(len(lines), 4)
            self.assertEqual(lines[0], b'Traceback (most recent call last):')
            self.assertIn(b'line 4, in <module>', lines[1])
            self.assertEqual(lines[2], b'    x = 1 / 0')
            self.assertEqual(lines[3], b'ZeroDivisionError: division by zero')
        finally:
            unlink(TESTFN)

    def test_recursion_error_during_traceback(self):
        code = textwrap.dedent("""
                import sys
                from weakref import ref

                sys.setrecursionlimit(15)

                def f():
                    ref(lambda: 0, [])
                    f()

                try:
                    f()
                except RecursionError:
                    pass
        """)
        try:
            with open(TESTFN, 'w') as f:
                f.write(code)

            rc, _, _ = assert_python_ok(TESTFN)
            self.assertEqual(rc, 0)
        finally:
            unlink(TESTFN)

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

    def test_format_exception_only_bad__str__(self):
        class X(Exception):
            def __str__(self):
                1/0
        err = traceback.format_exception_only(X, X())
        self.assertEqual(len(err), 1)
        str_value = '<exception str() failed>'
        if X.__module__ in ('__main__', 'builtins'):
            str_name = X.__qualname__
        else:
            str_name = '.'.join([X.__module__, X.__qualname__])
        self.assertEqual(err[0], "%s: %s\n" % (str_name, str_value))

    @requires_subprocess()
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
            actual_err_msg = stdout[3]
            self.assertTrue(actual_err_msg == err_msg,
                "Invalid error message: {0!r} instead of {1!r}".format(
                    actual_err_msg, err_msg))

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

    def test_print_exception_exc(self):
        output = StringIO()
        traceback.print_exception(Exception("projector"), file=output)
        self.assertEqual(output.getvalue(), "Exception: projector\n")

    def test_format_exception_exc(self):
        e = Exception("projector")
        output = traceback.format_exception(e)
        self.assertEqual(output, ["Exception: projector\n"])
        with self.assertRaisesRegex(ValueError, 'Both or neither'):
            traceback.format_exception(e.__class__, e)
        with self.assertRaisesRegex(ValueError, 'Both or neither'):
            traceback.format_exception(e.__class__, tb=e.__traceback__)
        with self.assertRaisesRegex(TypeError, 'positional-only'):
            traceback.format_exception(exc=e)

    def test_format_exception_only_exc(self):
        output = traceback.format_exception_only(Exception("projector"))
        self.assertEqual(output, ["Exception: projector\n"])

    def test_exception_is_None(self):
        NONE_EXC_STRING = 'NoneType: None\n'
        excfile = StringIO()
        traceback.print_exception(None, file=excfile)
        self.assertEqual(excfile.getvalue(), NONE_EXC_STRING)

        excfile = StringIO()
        traceback.print_exception(None, None, None, file=excfile)
        self.assertEqual(excfile.getvalue(), NONE_EXC_STRING)

        excfile = StringIO()
        traceback.print_exc(None, file=excfile)
        self.assertEqual(excfile.getvalue(), NONE_EXC_STRING)

        self.assertEqual(traceback.format_exc(None), NONE_EXC_STRING)
        self.assertEqual(traceback.format_exception(None), [NONE_EXC_STRING])
        self.assertEqual(
            traceback.format_exception(None, None, None), [NONE_EXC_STRING])
        self.assertEqual(
            traceback.format_exception_only(None), [NONE_EXC_STRING])
        self.assertEqual(
            traceback.format_exception_only(None, None), [NONE_EXC_STRING])

    def test_signatures(self):
        self.assertEqual(
            str(inspect.signature(traceback.print_exception)),
            ('(exc, /, value=<implicit>, tb=<implicit>, '
             'limit=None, file=None, chain=True)'))

        self.assertEqual(
            str(inspect.signature(traceback.format_exception)),
            ('(exc, /, value=<implicit>, tb=<implicit>, limit=None, '
             'chain=True)'))

        self.assertEqual(
            str(inspect.signature(traceback.format_exception_only)),
            '(exc, /, value=<implicit>)')


@requires_debug_ranges()
class TracebackErrorLocationCaretTests(unittest.TestCase):
    """
    Tests for printing code error expressions as part of PEP 657
    """
    def get_exception(self, callable):
        try:
            callable()
            self.fail("No exception thrown.")
        except:
            return traceback.format_exc().splitlines()[:-1]

    callable_line = get_exception.__code__.co_firstlineno + 2

    def test_basic_caret(self):
        # NOTE: In caret tests, "if True:" is used as a way to force indicator
        #   display, since the raising expression spans only part of the line.
        def f():
            if True: raise ValueError("basic caret tests")

        lineno_f = f.__code__.co_firstlineno
        expected_f = (
            'Traceback (most recent call last):\n'
            f'  File "{__file__}", line {self.callable_line}, in get_exception\n'
            '    callable()\n'
            f'  File "{__file__}", line {lineno_f+1}, in f\n'
            '    if True: raise ValueError("basic caret tests")\n'
            '             ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n'
        )
        result_lines = self.get_exception(f)
        self.assertEqual(result_lines, expected_f.splitlines())

    def test_line_with_unicode(self):
        # Make sure that even if a line contains multi-byte unicode characters
        # the correct carets are printed.
        def f_with_unicode():
            if True: raise ValueError("Ĥellö Wörld")

        lineno_f = f_with_unicode.__code__.co_firstlineno
        expected_f = (
            'Traceback (most recent call last):\n'
            f'  File "{__file__}", line {self.callable_line}, in get_exception\n'
            '    callable()\n'
            f'  File "{__file__}", line {lineno_f+1}, in f_with_unicode\n'
            '    if True: raise ValueError("Ĥellö Wörld")\n'
            '             ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n'
        )
        result_lines = self.get_exception(f_with_unicode)
        self.assertEqual(result_lines, expected_f.splitlines())

    def test_caret_in_type_annotation(self):
        def f_with_type():
            def foo(a: THIS_DOES_NOT_EXIST ) -> int:
                return 0

        lineno_f = f_with_type.__code__.co_firstlineno
        expected_f = (
            'Traceback (most recent call last):\n'
            f'  File "{__file__}", line {self.callable_line}, in get_exception\n'
            '    callable()\n'
            f'  File "{__file__}", line {lineno_f+1}, in f_with_type\n'
            '    def foo(a: THIS_DOES_NOT_EXIST ) -> int:\n'
            '               ^^^^^^^^^^^^^^^^^^^\n'
        )
        result_lines = self.get_exception(f_with_type)
        self.assertEqual(result_lines, expected_f.splitlines())

    def test_caret_multiline_expression(self):
        # Make sure no carets are printed for expressions spanning multiple
        # lines.
        def f_with_multiline():
            if True: raise ValueError(
                "error over multiple lines"
            )

        lineno_f = f_with_multiline.__code__.co_firstlineno
        expected_f = (
            'Traceback (most recent call last):\n'
            f'  File "{__file__}", line {self.callable_line}, in get_exception\n'
            '    callable()\n'
            f'  File "{__file__}", line {lineno_f+1}, in f_with_multiline\n'
            '    if True: raise ValueError(\n'
            '             ^^^^^^^^^^^^^^^^^'
        )
        result_lines = self.get_exception(f_with_multiline)
        self.assertEqual(result_lines, expected_f.splitlines())

    def test_caret_multiline_expression_syntax_error(self):
        # Make sure an expression spanning multiple lines that has
        # a syntax error is correctly marked with carets.
        code = textwrap.dedent("""
        def foo(*args, **kwargs):
            pass

        a, b, c = 1, 2, 3

        foo(a, z
                for z in
                    range(10), b, c)
        """)

        def f_with_multiline():
            # Need to defer the compilation until in self.get_exception(..)
            return compile(code, "?", "exec")

        lineno_f = f_with_multiline.__code__.co_firstlineno

        expected_f = (
            'Traceback (most recent call last):\n'
            f'  File "{__file__}", line {self.callable_line}, in get_exception\n'
            '    callable()\n'
            f'  File "{__file__}", line {lineno_f+2}, in f_with_multiline\n'
            '    return compile(code, "?", "exec")\n'
            '           ^^^^^^^^^^^^^^^^^^^^^^^^^^\n'
            '  File "?", line 7\n'
            '    foo(a, z\n'
            '           ^'
            )

        result_lines = self.get_exception(f_with_multiline)
        self.assertEqual(result_lines, expected_f.splitlines())

    def test_caret_multiline_expression_bin_op(self):
        # Make sure no carets are printed for expressions spanning multiple
        # lines.
        def f_with_multiline():
            return (
                2 + 1 /
                0
            )

        lineno_f = f_with_multiline.__code__.co_firstlineno
        expected_f = (
            'Traceback (most recent call last):\n'
            f'  File "{__file__}", line {self.callable_line}, in get_exception\n'
            '    callable()\n'
            f'  File "{__file__}", line {lineno_f+2}, in f_with_multiline\n'
            '    2 + 1 /\n'
            '        ^^^'
        )
        result_lines = self.get_exception(f_with_multiline)
        self.assertEqual(result_lines, expected_f.splitlines())

    def test_caret_for_binary_operators(self):
        def f_with_binary_operator():
            divisor = 20
            return 10 + divisor / 0 + 30

        lineno_f = f_with_binary_operator.__code__.co_firstlineno
        expected_error = (
            'Traceback (most recent call last):\n'
            f'  File "{__file__}", line {self.callable_line}, in get_exception\n'
            '    callable()\n'
            f'  File "{__file__}", line {lineno_f+2}, in f_with_binary_operator\n'
            '    return 10 + divisor / 0 + 30\n'
            '                ~~~~~~~~^~~\n'
        )
        result_lines = self.get_exception(f_with_binary_operator)
        self.assertEqual(result_lines, expected_error.splitlines())

    def test_caret_for_binary_operators_with_unicode(self):
        def f_with_binary_operator():
            áóí = 20
            return 10 + áóí / 0 + 30

        lineno_f = f_with_binary_operator.__code__.co_firstlineno
        expected_error = (
            'Traceback (most recent call last):\n'
            f'  File "{__file__}", line {self.callable_line}, in get_exception\n'
            '    callable()\n'
            f'  File "{__file__}", line {lineno_f+2}, in f_with_binary_operator\n'
            '    return 10 + áóí / 0 + 30\n'
            '                ~~~~^~~\n'
        )
        result_lines = self.get_exception(f_with_binary_operator)
        self.assertEqual(result_lines, expected_error.splitlines())

    def test_caret_for_binary_operators_two_char(self):
        def f_with_binary_operator():
            divisor = 20
            return 10 + divisor // 0 + 30

        lineno_f = f_with_binary_operator.__code__.co_firstlineno
        expected_error = (
            'Traceback (most recent call last):\n'
            f'  File "{__file__}", line {self.callable_line}, in get_exception\n'
            '    callable()\n'
            f'  File "{__file__}", line {lineno_f+2}, in f_with_binary_operator\n'
            '    return 10 + divisor // 0 + 30\n'
            '                ~~~~~~~~^^~~\n'
        )
        result_lines = self.get_exception(f_with_binary_operator)
        self.assertEqual(result_lines, expected_error.splitlines())

    def test_caret_for_binary_operators_with_spaces_and_parenthesis(self):
        def f_with_binary_operator():
            a = 1
            b = ""
            return ( a   )   + b

        lineno_f = f_with_binary_operator.__code__.co_firstlineno
        expected_error = (
            'Traceback (most recent call last):\n'
            f'  File "{__file__}", line {self.callable_line}, in get_exception\n'
            '    callable()\n'
            f'  File "{__file__}", line {lineno_f+3}, in f_with_binary_operator\n'
            '    return ( a   )   + b\n'
            '           ~~~~~~~~~~^~~\n'
        )
        result_lines = self.get_exception(f_with_binary_operator)
        self.assertEqual(result_lines, expected_error.splitlines())

    def test_caret_for_subscript(self):
        def f_with_subscript():
            some_dict = {'x': {'y': None}}
            return some_dict['x']['y']['z']

        lineno_f = f_with_subscript.__code__.co_firstlineno
        expected_error = (
            'Traceback (most recent call last):\n'
            f'  File "{__file__}", line {self.callable_line}, in get_exception\n'
            '    callable()\n'
            f'  File "{__file__}", line {lineno_f+2}, in f_with_subscript\n'
            "    return some_dict['x']['y']['z']\n"
            '           ~~~~~~~~~~~~~~~~~~~^^^^^\n'
        )
        result_lines = self.get_exception(f_with_subscript)
        self.assertEqual(result_lines, expected_error.splitlines())

    def test_caret_for_subscript_unicode(self):
        def f_with_subscript():
            some_dict = {'ó': {'á': {'í': {'theta': 1}}}}
            return some_dict['ó']['á']['í']['beta']

        lineno_f = f_with_subscript.__code__.co_firstlineno
        expected_error = (
            'Traceback (most recent call last):\n'
            f'  File "{__file__}", line {self.callable_line}, in get_exception\n'
            '    callable()\n'
            f'  File "{__file__}", line {lineno_f+2}, in f_with_subscript\n'
            "    return some_dict['ó']['á']['í']['beta']\n"
            '           ~~~~~~~~~~~~~~~~~~~~~~~~^^^^^^^^\n'
        )
        result_lines = self.get_exception(f_with_subscript)
        self.assertEqual(result_lines, expected_error.splitlines())

    def test_caret_for_subscript_with_spaces_and_parenthesis(self):
        def f_with_binary_operator():
            a = []
            b = c = 1
            return b     [    a  ] + c

        lineno_f = f_with_binary_operator.__code__.co_firstlineno
        expected_error = (
            'Traceback (most recent call last):\n'
            f'  File "{__file__}", line {self.callable_line}, in get_exception\n'
            '    callable()\n'
            f'  File "{__file__}", line {lineno_f+3}, in f_with_binary_operator\n'
            '    return b     [    a  ] + c\n'
            '           ~~~~~~^^^^^^^^^\n'
        )
        result_lines = self.get_exception(f_with_binary_operator)
        self.assertEqual(result_lines, expected_error.splitlines())

    def test_caret_for_failed_assertion(self):
        def f_assert():
            test = 3
            assert test == 1 and test == 2, "Bug found?"

        lineno_f = f_assert.__code__.co_firstlineno
        expected_error = (
            'Traceback (most recent call last):\n'
            f'  File "{__file__}", line {self.callable_line}, in get_exception\n'
            '    callable()\n'
            f'  File "{__file__}", line {lineno_f+2}, in f_assert\n'
            '    assert test == 1 and test == 2, "Bug found?"\n'
            '           ^^^^^^^^^^^^^^^^^^^^^^^\n'
        )
        result_lines = self.get_exception(f_assert)
        self.assertEqual(result_lines, expected_error.splitlines())

    def test_traceback_specialization_with_syntax_error(self):
        bytecode = compile("1 / 0 / 1 / 2\n", TESTFN, "exec")

        with open(TESTFN, "w") as file:
            # make the file's contents invalid
            file.write("1 $ 0 / 1 / 2\n")
        self.addCleanup(unlink, TESTFN)

        func = partial(exec, bytecode)
        result_lines = self.get_exception(func)

        lineno_f = bytecode.co_firstlineno
        expected_error = (
            'Traceback (most recent call last):\n'
            f'  File "{__file__}", line {self.callable_line}, in get_exception\n'
            '    callable()\n'
            f'  File "{TESTFN}", line {lineno_f}, in <module>\n'
            "    1 $ 0 / 1 / 2\n"
            '    ^^^^^\n'
        )
        self.assertEqual(result_lines, expected_error.splitlines())

    def test_traceback_very_long_line(self):
        source = "if True: " + "a" * 256
        bytecode = compile(source, TESTFN, "exec")

        with open(TESTFN, "w") as file:
            file.write(source)
        self.addCleanup(unlink, TESTFN)

        func = partial(exec, bytecode)
        result_lines = self.get_exception(func)

        lineno_f = bytecode.co_firstlineno
        expected_error = (
            'Traceback (most recent call last):\n'
            f'  File "{__file__}", line {self.callable_line}, in get_exception\n'
            '    callable()\n'
            f'  File "{TESTFN}", line {lineno_f}, in <module>\n'
            f'    {source}\n'
            f'    {" "*len("if True: ") + "^"*256}\n'
        )
        self.assertEqual(result_lines, expected_error.splitlines())

    def test_secondary_caret_not_elided(self):
        # Always show a line's indicators if they include the secondary character.
        def f_with_subscript():
            some_dict = {'x': {'y': None}}
            some_dict['x']['y']['z']

        lineno_f = f_with_subscript.__code__.co_firstlineno
        expected_error = (
            'Traceback (most recent call last):\n'
            f'  File "{__file__}", line {self.callable_line}, in get_exception\n'
            '    callable()\n'
            f'  File "{__file__}", line {lineno_f+2}, in f_with_subscript\n'
            "    some_dict['x']['y']['z']\n"
            '    ~~~~~~~~~~~~~~~~~~~^^^^^\n'
        )
        result_lines = self.get_exception(f_with_subscript)
        self.assertEqual(result_lines, expected_error.splitlines())

    def test_caret_exception_group(self):
        # Notably, this covers whether indicators handle margin strings correctly.
        # (Exception groups use margin strings to display vertical indicators.)
        # The implementation must account for both "indent" and "margin" offsets.

        def exc():
            if True: raise ExceptionGroup("eg", [ValueError(1), TypeError(2)])

        expected_error = (
             f'  + Exception Group Traceback (most recent call last):\n'
             f'  |   File "{__file__}", line {self.callable_line}, in get_exception\n'
             f'  |     callable()\n'
             f'  |   File "{__file__}", line {exc.__code__.co_firstlineno + 1}, in exc\n'
             f'  |     if True: raise ExceptionGroup("eg", [ValueError(1), TypeError(2)])\n'
             f'  |              ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n'
             f'  | ExceptionGroup: eg (2 sub-exceptions)\n'
             f'  +-+---------------- 1 ----------------\n'
             f'    | ValueError: 1\n'
             f'    +---------------- 2 ----------------\n'
             f'    | TypeError: 2\n')

        result_lines = self.get_exception(exc)
        self.assertEqual(result_lines, expected_error.splitlines())

    def assertSpecialized(self, func, expected_specialization):
        result_lines = self.get_exception(func)
        specialization_line = result_lines[-1]
        self.assertEqual(specialization_line.lstrip(), expected_specialization)

    def test_specialization_variations(self):
        self.assertSpecialized(lambda: 1/0,
                                      "~^~")
        self.assertSpecialized(lambda: 1/0/3,
                                      "~^~")
        self.assertSpecialized(lambda: 1 / 0,
                                      "~~^~~")
        self.assertSpecialized(lambda: 1 / 0 / 3,
                                      "~~^~~")
        self.assertSpecialized(lambda: 1/ 0,
                                      "~^~~")
        self.assertSpecialized(lambda: 1/ 0/3,
                                      "~^~~")
        self.assertSpecialized(lambda: 1    /  0,
                                      "~~~~~^~~~")
        self.assertSpecialized(lambda: 1    /  0   / 5,
                                      "~~~~~^~~~")
        self.assertSpecialized(lambda: 1 /0,
                                      "~~^~")
        self.assertSpecialized(lambda: 1//0,
                                      "~^^~")
        self.assertSpecialized(lambda: 1//0//4,
                                      "~^^~")
        self.assertSpecialized(lambda: 1 // 0,
                                      "~~^^~~")
        self.assertSpecialized(lambda: 1 // 0 // 4,
                                      "~~^^~~")
        self.assertSpecialized(lambda: 1 //0,
                                      "~~^^~")
        self.assertSpecialized(lambda: 1// 0,
                                      "~^^~~")

    def test_decorator_application_lineno_correct(self):
        def dec_error(func):
            raise TypeError
        def dec_fine(func):
            return func
        def applydecs():
            @dec_error
            @dec_fine
            def g(): pass
        result_lines = self.get_exception(applydecs)
        lineno_applydescs = applydecs.__code__.co_firstlineno
        lineno_dec_error = dec_error.__code__.co_firstlineno
        expected_error = (
            'Traceback (most recent call last):\n'
            f'  File "{__file__}", line {self.callable_line}, in get_exception\n'
            '    callable()\n'
            f'  File "{__file__}", line {lineno_applydescs + 1}, in applydecs\n'
            '    @dec_error\n'
            '     ^^^^^^^^^\n'
            f'  File "{__file__}", line {lineno_dec_error + 1}, in dec_error\n'
            '    raise TypeError\n'
        )
        self.assertEqual(result_lines, expected_error.splitlines())

        def applydecs_class():
            @dec_error
            @dec_fine
            class A: pass
        result_lines = self.get_exception(applydecs_class)
        lineno_applydescs_class = applydecs_class.__code__.co_firstlineno
        expected_error = (
            'Traceback (most recent call last):\n'
            f'  File "{__file__}", line {self.callable_line}, in get_exception\n'
            '    callable()\n'
            f'  File "{__file__}", line {lineno_applydescs_class + 1}, in applydecs_class\n'
            '    @dec_error\n'
            '     ^^^^^^^^^\n'
            f'  File "{__file__}", line {lineno_dec_error + 1}, in dec_error\n'
            '    raise TypeError\n'
        )
        self.assertEqual(result_lines, expected_error.splitlines())

    def test_multiline_method_call_a(self):
        def f():
            (None
                .method
            )()
        actual = self.get_exception(f)
        expected = [
            f"Traceback (most recent call last):",
            f"  File \"{__file__}\", line {self.callable_line}, in get_exception",
            f"    callable()",
            f"  File \"{__file__}\", line {f.__code__.co_firstlineno + 2}, in f",
            f"    .method",
            f"     ^^^^^^",
        ]
        self.assertEqual(actual, expected)

    def test_multiline_method_call_b(self):
        def f():
            (None.
                method
            )()
        actual = self.get_exception(f)
        expected = [
            f"Traceback (most recent call last):",
            f"  File \"{__file__}\", line {self.callable_line}, in get_exception",
            f"    callable()",
            f"  File \"{__file__}\", line {f.__code__.co_firstlineno + 2}, in f",
            f"    method",
        ]
        self.assertEqual(actual, expected)

    def test_multiline_method_call_c(self):
        def f():
            (None
                . method
            )()
        actual = self.get_exception(f)
        expected = [
            f"Traceback (most recent call last):",
            f"  File \"{__file__}\", line {self.callable_line}, in get_exception",
            f"    callable()",
            f"  File \"{__file__}\", line {f.__code__.co_firstlineno + 2}, in f",
            f"    . method",
            f"      ^^^^^^",
        ]
        self.assertEqual(actual, expected)

    def test_wide_characters_unicode_with_problematic_byte_offset(self):
        def f():
            ｗｉｄｔｈ

        actual = self.get_exception(f)
        expected = [
            f"Traceback (most recent call last):",
            f"  File \"{__file__}\", line {self.callable_line}, in get_exception",
            f"    callable()",
            f"  File \"{__file__}\", line {f.__code__.co_firstlineno + 1}, in f",
            f"    ｗｉｄｔｈ",
        ]
        self.assertEqual(actual, expected)


    def test_byte_offset_with_wide_characters_middle(self):
        def f():
            ｗｉｄｔｈ = 1
            raise ValueError(ｗｉｄｔｈ)

        actual = self.get_exception(f)
        expected = [
            f"Traceback (most recent call last):",
            f"  File \"{__file__}\", line {self.callable_line}, in get_exception",
            f"    callable()",
            f"  File \"{__file__}\", line {f.__code__.co_firstlineno + 2}, in f",
            f"    raise ValueError(ｗｉｄｔｈ)",
        ]
        self.assertEqual(actual, expected)

    def test_byte_offset_multiline(self):
        def f():
            ｗｗｗ = 1
            ｔｈ = 0

            print(1, ｗｗｗ(
                    ｔｈ))

        actual = self.get_exception(f)
        expected = [
            f"Traceback (most recent call last):",
            f"  File \"{__file__}\", line {self.callable_line}, in get_exception",
            f"    callable()",
            f"  File \"{__file__}\", line {f.__code__.co_firstlineno + 4}, in f",
            f"    print(1, ｗｗｗ(",
            f"             ^^^^^^^",
        ]
        self.assertEqual(actual, expected)

    def test_byte_offset_with_wide_characters_term_highlight(self):
        def f():
            说明说明 = 1
            şçöğıĤellö = 0 # not wide but still non-ascii
            return 说明说明 / şçöğıĤellö

        actual = self.get_exception(f)
        expected = [
            f"Traceback (most recent call last):",
            f"  File \"{__file__}\", line {self.callable_line}, in get_exception",
            f"    callable()",
            f"  File \"{__file__}\", line {f.__code__.co_firstlineno + 3}, in f",
            f"    return 说明说明 / şçöğıĤellö",
            f"           ~~~~~~~~~^~~~~~~~~~~~",
        ]
        self.assertEqual(actual, expected)

    def test_byte_offset_with_emojis_term_highlight(self):
        def f():
            return "✨🐍" + func_说明说明("📗🚛",
                "📗🚛") + "🐍"

        actual = self.get_exception(f)
        expected = [
            f"Traceback (most recent call last):",
            f"  File \"{__file__}\", line {self.callable_line}, in get_exception",
            f"    callable()",
            f"  File \"{__file__}\", line {f.__code__.co_firstlineno + 1}, in f",
            f'    return "✨🐍" + func_说明说明("📗🚛",',
            f"                    ^^^^^^^^^^^^^",
        ]
        self.assertEqual(actual, expected)

    def test_byte_offset_wide_chars_subscript(self):
        def f():
            my_dct = {
                "✨🚛✨": {
                    "说明": {
                        "🐍🐍🐍": None
                    }
                }
            }
            return my_dct["✨🚛✨"]["说明"]["🐍"]["说明"]["🐍🐍"]

        actual = self.get_exception(f)
        expected = [
            f"Traceback (most recent call last):",
            f"  File \"{__file__}\", line {self.callable_line}, in get_exception",
            f"    callable()",
            f"  File \"{__file__}\", line {f.__code__.co_firstlineno + 8}, in f",
            f'    return my_dct["✨🚛✨"]["说明"]["🐍"]["说明"]["🐍🐍"]',
            f"           ~~~~~~~~~~~~~~~~~~~~~~~~^^^^^^",
        ]
        self.assertEqual(actual, expected)


@cpython_only
@requires_debug_ranges()
class CPythonTracebackErrorCaretTests(TracebackErrorLocationCaretTests):
    """
    Same set of tests as above but with Python's internal traceback printing.
    """
    def get_exception(self, callable):
        from _testcapi import exception_print
        try:
            callable()
            self.fail("No exception thrown.")
        except Exception as e:
            with captured_output("stderr") as tbstderr:
                exception_print(e)
            return tbstderr.getvalue().splitlines()[:-1]

    callable_line = get_exception.__code__.co_firstlineno + 3


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
        banner = tb_lines[0]
        self.assertEqual(len(tb_lines), 5)
        location, source_line = tb_lines[-2], tb_lines[-1]
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
            '           ^^^^^^^^^^\n'
            f'  File "{__file__}", line {lineno_g+2}, in g\n'
            '    return g(count-1)\n'
            '           ^^^^^^^^^^\n'
            f'  File "{__file__}", line {lineno_g+2}, in g\n'
            '    return g(count-1)\n'
            '           ^^^^^^^^^^\n'
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
            '           ^^^^^^^^^^\n'
            f'  File "{__file__}", line {lineno_h+2}, in h\n'
            '    return h(count-1)\n'
            '           ^^^^^^^^^^\n'
            f'  File "{__file__}", line {lineno_h+2}, in h\n'
            '    return h(count-1)\n'
            '           ^^^^^^^^^^\n'
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
            '           ^^^^^^^^^^\n'
            f'  File "{__file__}", line {lineno_g+2}, in g\n'
            '    return g(count-1)\n'
            '           ^^^^^^^^^^\n'
            f'  File "{__file__}", line {lineno_g+2}, in g\n'
            '    return g(count-1)\n'
            '           ^^^^^^^^^^\n'
            f'  File "{__file__}", line {lineno_g+3}, in g\n'
            '    raise ValueError\n'
            'ValueError\n'
        )
        tb_line = (
            'Traceback (most recent call last):\n'
            f'  File "{__file__}", line {lineno_g+77}, in _check_recursive_traceback_display\n'
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
            '           ^^^^^^^^^^\n'
            f'  File "{__file__}", line {lineno_g+2}, in g\n'
            '    return g(count-1)\n'
            '           ^^^^^^^^^^\n'
            f'  File "{__file__}", line {lineno_g+2}, in g\n'
            '    return g(count-1)\n'
            '           ^^^^^^^^^^\n'
            '  [Previous line repeated 1 more time]\n'
            f'  File "{__file__}", line {lineno_g+3}, in g\n'
            '    raise ValueError\n'
            'ValueError\n'
        )
        tb_line = (
            'Traceback (most recent call last):\n'
            f'  File "{__file__}", line {lineno_g+108}, in _check_recursive_traceback_display\n'
            '    g(traceback._RECURSIVE_CUTOFF + 1)\n'
        )
        expected = (tb_line + result_g).splitlines()
        actual = stderr_g.getvalue().splitlines()
        self.assertEqual(actual, expected)

    @requires_debug_ranges()
    def test_recursive_traceback_python(self):
        self._check_recursive_traceback_display(traceback.print_exc)

    @cpython_only
    @requires_debug_ranges()
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

    def deep_eg(self):
        e = TypeError(1)
        for i in range(2000):
            e = ExceptionGroup('eg', [e])
        return e

    @cpython_only
    def test_exception_group_deep_recursion_capi(self):
        from _testcapi import exception_print
        LIMIT = 75
        eg = self.deep_eg()
        with captured_output("stderr") as stderr_f:
            with support.infinite_recursion(max_depth=LIMIT):
                exception_print(eg)
        output = stderr_f.getvalue()
        self.assertIn('ExceptionGroup', output)
        self.assertLessEqual(output.count('ExceptionGroup'), LIMIT)

    def test_exception_group_deep_recursion_traceback(self):
        LIMIT = 75
        eg = self.deep_eg()
        with captured_output("stderr") as stderr_f:
            with support.infinite_recursion(max_depth=LIMIT):
                traceback.print_exception(type(eg), eg, eg.__traceback__)
        output = stderr_f.getvalue()
        self.assertIn('ExceptionGroup', output)
        self.assertLessEqual(output.count('ExceptionGroup'), LIMIT)

    @cpython_only
    def test_print_exception_bad_type_capi(self):
        from _testcapi import exception_print
        with captured_output("stderr") as stderr:
            exception_print(42)
        self.assertEqual(
            stderr.getvalue(),
            ('TypeError: print_exception(): '
             'Exception expected for value, int found\n')
        )

    def test_print_exception_bad_type_python(self):
        msg = "Exception expected for value, int found"
        with self.assertRaisesRegex(TypeError, msg):
            traceback.print_exception(42)


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
        if isinstance(exception_or_callable, BaseException):
            return exception_or_callable
        try:
            exception_or_callable()
        except Exception as e:
            return e

    callable_line = get_exception.__code__.co_firstlineno + 4

    def zero_div(self):
        1/0 # In zero_div

    def check_zero_div(self, msg):
        lines = msg.splitlines()
        if has_no_debug_ranges():
            self.assertTrue(lines[-3].startswith('  File'))
            self.assertIn('1/0 # In zero_div', lines[-2])
        else:
            self.assertTrue(lines[-4].startswith('  File'))
            self.assertIn('1/0 # In zero_div', lines[-3])
        self.assertTrue(lines[-1].startswith('ZeroDivisionError'), lines[-1])

    def test_simple(self):
        try:
            1/0 # Marker
        except ZeroDivisionError as _:
            e = _
        lines = self.get_report(e).splitlines()
        if has_no_debug_ranges():
            self.assertEqual(len(lines), 4)
            self.assertTrue(lines[3].startswith('ZeroDivisionError'))
        else:
            self.assertEqual(len(lines), 5)
            self.assertTrue(lines[4].startswith('ZeroDivisionError'))
        self.assertTrue(lines[0].startswith('Traceback'))
        self.assertTrue(lines[1].startswith('  File'))
        self.assertIn('1/0 # Marker', lines[2])

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
        self.assertTrue(lines[3].startswith('ZeroDivisionError'))
        self.assertTrue(lines[0].startswith('Traceback'))
        self.assertTrue(lines[1].startswith('  File'))
        self.assertIn('ZeroDivisionError from None', lines[2])

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

    def test_exception_with_note(self):
        e = ValueError(123)
        vanilla = self.get_report(e)

        e.add_note('My Note')
        self.assertEqual(self.get_report(e), vanilla + 'My Note\n')

        del e.__notes__
        e.add_note('')
        self.assertEqual(self.get_report(e), vanilla + '\n')

        del e.__notes__
        e.add_note('Your Note')
        self.assertEqual(self.get_report(e), vanilla + 'Your Note\n')

        del e.__notes__
        self.assertEqual(self.get_report(e), vanilla)

    def test_exception_with_invalid_notes(self):
        e = ValueError(123)
        vanilla = self.get_report(e)

        # non-sequence __notes__
        class BadThing:
            def __str__(self):
                return 'bad str'

            def __repr__(self):
                return 'bad repr'

        # unprintable, non-sequence __notes__
        class Unprintable:
            def __repr__(self):
                raise ValueError('bad value')

        e.__notes__ = BadThing()
        notes_repr = 'bad repr'
        self.assertEqual(self.get_report(e), vanilla + notes_repr)

        e.__notes__ = Unprintable()
        err_msg = '<__notes__ repr() failed>'
        self.assertEqual(self.get_report(e), vanilla + err_msg)

        # non-string item in the __notes__ sequence
        e.__notes__  = [BadThing(), 'Final Note']
        bad_note = 'bad str'
        self.assertEqual(self.get_report(e), vanilla + bad_note + '\nFinal Note\n')

        # unprintable, non-string item in the __notes__ sequence
        e.__notes__  = [Unprintable(), 'Final Note']
        err_msg = '<note str() failed>'
        self.assertEqual(self.get_report(e), vanilla + err_msg + '\nFinal Note\n')

        # an exception with a broken __getattr__ raising a non expected error
        class BrokenException(Exception):
            broken = False
            def __getattr__(self, name):
                if self.broken:
                    raise ValueError(f'no {name}')
                raise AttributeError(name)

        e = BrokenException(123)
        vanilla = self.get_report(e)
        e.broken = True
        self.assertEqual(
            self.get_report(e),
            vanilla + "Ignored error getting __notes__: ValueError('no __notes__')\n")

    def test_exception_with_multiple_notes(self):
        for e in [ValueError(42), SyntaxError('bad syntax')]:
            with self.subTest(e=e):
                vanilla = self.get_report(e)

                e.add_note('Note 1')
                e.add_note('Note 2')
                e.add_note('Note 3')

                self.assertEqual(
                    self.get_report(e),
                    vanilla + 'Note 1\n' + 'Note 2\n' + 'Note 3\n')

                del e.__notes__
                e.add_note('Note 4')
                del e.__notes__
                e.add_note('Note 5')
                e.add_note('Note 6')

                self.assertEqual(
                    self.get_report(e),
                    vanilla + 'Note 5\n' + 'Note 6\n')

    def test_exception_qualname(self):
        class A:
            class B:
                class X(Exception):
                    def __str__(self):
                        return "I am X"

        err = self.get_report(A.B.X())
        str_value = 'I am X'
        str_name = '.'.join([A.B.X.__module__, A.B.X.__qualname__])
        exp = "%s: %s\n" % (str_name, str_value)
        self.assertEqual(exp, MODULE_PREFIX + err)

    def test_exception_modulename(self):
        class X(Exception):
            def __str__(self):
                return "I am X"

        for modulename in '__main__', 'builtins', 'some_module':
            X.__module__ = modulename
            with self.subTest(modulename=modulename):
                err = self.get_report(X())
                str_value = 'I am X'
                if modulename in ['builtins', '__main__']:
                    str_name = X.__qualname__
                else:
                    str_name = '.'.join([X.__module__, X.__qualname__])
                exp = "%s: %s\n" % (str_name, str_value)
                self.assertEqual(exp, err)

    def test_exception_modulename_not_unicode(self):
        class X(Exception):
            def __str__(self):
                return "I am X"

        X.__module__ = 42

        err = self.get_report(X())
        exp = f'<unknown>.{X.__qualname__}: I am X\n'
        self.assertEqual(exp, err)

    def test_exception_bad__str__(self):
        class X(Exception):
            def __str__(self):
                1/0
        err = self.get_report(X())
        str_value = '<exception str() failed>'
        str_name = '.'.join([X.__module__, X.__qualname__])
        self.assertEqual(MODULE_PREFIX + err, f"{str_name}: {str_value}\n")


    # #### Exception Groups ####

    def test_exception_group_basic(self):
        def exc():
            raise ExceptionGroup("eg", [ValueError(1), TypeError(2)])

        expected = (
             f'  + Exception Group Traceback (most recent call last):\n'
             f'  |   File "{__file__}", line {self.callable_line}, in get_exception\n'
             f'  |     exception_or_callable()\n'
             f'  |   File "{__file__}", line {exc.__code__.co_firstlineno + 1}, in exc\n'
             f'  |     raise ExceptionGroup("eg", [ValueError(1), TypeError(2)])\n'
             f'  | ExceptionGroup: eg (2 sub-exceptions)\n'
             f'  +-+---------------- 1 ----------------\n'
             f'    | ValueError: 1\n'
             f'    +---------------- 2 ----------------\n'
             f'    | TypeError: 2\n'
             f'    +------------------------------------\n')

        report = self.get_report(exc)
        self.assertEqual(report, expected)

    def test_exception_group_cause(self):
        def exc():
            EG = ExceptionGroup
            try:
                raise EG("eg1", [ValueError(1), TypeError(2)])
            except Exception as e:
                raise EG("eg2", [ValueError(3), TypeError(4)]) from e

        expected = (f'  + Exception Group Traceback (most recent call last):\n'
                    f'  |   File "{__file__}", line {exc.__code__.co_firstlineno + 3}, in exc\n'
                    f'  |     raise EG("eg1", [ValueError(1), TypeError(2)])\n'
                    f'  | ExceptionGroup: eg1 (2 sub-exceptions)\n'
                    f'  +-+---------------- 1 ----------------\n'
                    f'    | ValueError: 1\n'
                    f'    +---------------- 2 ----------------\n'
                    f'    | TypeError: 2\n'
                    f'    +------------------------------------\n'
                    f'\n'
                    f'The above exception was the direct cause of the following exception:\n'
                    f'\n'
                    f'  + Exception Group Traceback (most recent call last):\n'
                    f'  |   File "{__file__}", line {self.callable_line}, in get_exception\n'
                    f'  |     exception_or_callable()\n'
                    f'  |   File "{__file__}", line {exc.__code__.co_firstlineno + 5}, in exc\n'
                    f'  |     raise EG("eg2", [ValueError(3), TypeError(4)]) from e\n'
                    f'  | ExceptionGroup: eg2 (2 sub-exceptions)\n'
                    f'  +-+---------------- 1 ----------------\n'
                    f'    | ValueError: 3\n'
                    f'    +---------------- 2 ----------------\n'
                    f'    | TypeError: 4\n'
                    f'    +------------------------------------\n')

        report = self.get_report(exc)
        self.assertEqual(report, expected)

    def test_exception_group_context_with_context(self):
        def exc():
            EG = ExceptionGroup
            try:
                try:
                    raise EG("eg1", [ValueError(1), TypeError(2)])
                except:
                    raise EG("eg2", [ValueError(3), TypeError(4)])
            except:
                raise ImportError(5)

        expected = (
             f'  + Exception Group Traceback (most recent call last):\n'
             f'  |   File "{__file__}", line {exc.__code__.co_firstlineno + 4}, in exc\n'
             f'  |     raise EG("eg1", [ValueError(1), TypeError(2)])\n'
             f'  | ExceptionGroup: eg1 (2 sub-exceptions)\n'
             f'  +-+---------------- 1 ----------------\n'
             f'    | ValueError: 1\n'
             f'    +---------------- 2 ----------------\n'
             f'    | TypeError: 2\n'
             f'    +------------------------------------\n'
             f'\n'
             f'During handling of the above exception, another exception occurred:\n'
             f'\n'
             f'  + Exception Group Traceback (most recent call last):\n'
             f'  |   File "{__file__}", line {exc.__code__.co_firstlineno + 6}, in exc\n'
             f'  |     raise EG("eg2", [ValueError(3), TypeError(4)])\n'
             f'  | ExceptionGroup: eg2 (2 sub-exceptions)\n'
             f'  +-+---------------- 1 ----------------\n'
             f'    | ValueError: 3\n'
             f'    +---------------- 2 ----------------\n'
             f'    | TypeError: 4\n'
             f'    +------------------------------------\n'
             f'\n'
             f'During handling of the above exception, another exception occurred:\n'
             f'\n'
             f'Traceback (most recent call last):\n'
             f'  File "{__file__}", line {self.callable_line}, in get_exception\n'
             f'    exception_or_callable()\n'
             f'  File "{__file__}", line {exc.__code__.co_firstlineno + 8}, in exc\n'
             f'    raise ImportError(5)\n'
             f'ImportError: 5\n')

        report = self.get_report(exc)
        self.assertEqual(report, expected)

    def test_exception_group_nested(self):
        def exc():
            EG = ExceptionGroup
            VE = ValueError
            TE = TypeError
            try:
                try:
                    raise EG("nested", [TE(2), TE(3)])
                except Exception as e:
                    exc = e
                raise EG("eg", [VE(1), exc, VE(4)])
            except:
                raise EG("top", [VE(5)])

        expected = (f'  + Exception Group Traceback (most recent call last):\n'
                    f'  |   File "{__file__}", line {exc.__code__.co_firstlineno + 9}, in exc\n'
                    f'  |     raise EG("eg", [VE(1), exc, VE(4)])\n'
                    f'  | ExceptionGroup: eg (3 sub-exceptions)\n'
                    f'  +-+---------------- 1 ----------------\n'
                    f'    | ValueError: 1\n'
                    f'    +---------------- 2 ----------------\n'
                    f'    | Exception Group Traceback (most recent call last):\n'
                    f'    |   File "{__file__}", line {exc.__code__.co_firstlineno + 6}, in exc\n'
                    f'    |     raise EG("nested", [TE(2), TE(3)])\n'
                    f'    | ExceptionGroup: nested (2 sub-exceptions)\n'
                    f'    +-+---------------- 1 ----------------\n'
                    f'      | TypeError: 2\n'
                    f'      +---------------- 2 ----------------\n'
                    f'      | TypeError: 3\n'
                    f'      +------------------------------------\n'
                    f'    +---------------- 3 ----------------\n'
                    f'    | ValueError: 4\n'
                    f'    +------------------------------------\n'
                    f'\n'
                    f'During handling of the above exception, another exception occurred:\n'
                    f'\n'
                    f'  + Exception Group Traceback (most recent call last):\n'
                    f'  |   File "{__file__}", line {self.callable_line}, in get_exception\n'
                    f'  |     exception_or_callable()\n'
                    f'  |   File "{__file__}", line {exc.__code__.co_firstlineno + 11}, in exc\n'
                    f'  |     raise EG("top", [VE(5)])\n'
                    f'  | ExceptionGroup: top (1 sub-exception)\n'
                    f'  +-+---------------- 1 ----------------\n'
                    f'    | ValueError: 5\n'
                    f'    +------------------------------------\n')

        report = self.get_report(exc)
        self.assertEqual(report, expected)

    def test_exception_group_width_limit(self):
        excs = []
        for i in range(1000):
            excs.append(ValueError(i))
        eg = ExceptionGroup('eg', excs)

        expected = ('  | ExceptionGroup: eg (1000 sub-exceptions)\n'
                    '  +-+---------------- 1 ----------------\n'
                    '    | ValueError: 0\n'
                    '    +---------------- 2 ----------------\n'
                    '    | ValueError: 1\n'
                    '    +---------------- 3 ----------------\n'
                    '    | ValueError: 2\n'
                    '    +---------------- 4 ----------------\n'
                    '    | ValueError: 3\n'
                    '    +---------------- 5 ----------------\n'
                    '    | ValueError: 4\n'
                    '    +---------------- 6 ----------------\n'
                    '    | ValueError: 5\n'
                    '    +---------------- 7 ----------------\n'
                    '    | ValueError: 6\n'
                    '    +---------------- 8 ----------------\n'
                    '    | ValueError: 7\n'
                    '    +---------------- 9 ----------------\n'
                    '    | ValueError: 8\n'
                    '    +---------------- 10 ----------------\n'
                    '    | ValueError: 9\n'
                    '    +---------------- 11 ----------------\n'
                    '    | ValueError: 10\n'
                    '    +---------------- 12 ----------------\n'
                    '    | ValueError: 11\n'
                    '    +---------------- 13 ----------------\n'
                    '    | ValueError: 12\n'
                    '    +---------------- 14 ----------------\n'
                    '    | ValueError: 13\n'
                    '    +---------------- 15 ----------------\n'
                    '    | ValueError: 14\n'
                    '    +---------------- ... ----------------\n'
                    '    | and 985 more exceptions\n'
                    '    +------------------------------------\n')

        report = self.get_report(eg)
        self.assertEqual(report, expected)

    def test_exception_group_depth_limit(self):
        exc = TypeError('bad type')
        for i in range(1000):
            exc = ExceptionGroup(
                f'eg{i}',
                [ValueError(i), exc, ValueError(-i)])

        expected = ('  | ExceptionGroup: eg999 (3 sub-exceptions)\n'
                    '  +-+---------------- 1 ----------------\n'
                    '    | ValueError: 999\n'
                    '    +---------------- 2 ----------------\n'
                    '    | ExceptionGroup: eg998 (3 sub-exceptions)\n'
                    '    +-+---------------- 1 ----------------\n'
                    '      | ValueError: 998\n'
                    '      +---------------- 2 ----------------\n'
                    '      | ExceptionGroup: eg997 (3 sub-exceptions)\n'
                    '      +-+---------------- 1 ----------------\n'
                    '        | ValueError: 997\n'
                    '        +---------------- 2 ----------------\n'
                    '        | ExceptionGroup: eg996 (3 sub-exceptions)\n'
                    '        +-+---------------- 1 ----------------\n'
                    '          | ValueError: 996\n'
                    '          +---------------- 2 ----------------\n'
                    '          | ExceptionGroup: eg995 (3 sub-exceptions)\n'
                    '          +-+---------------- 1 ----------------\n'
                    '            | ValueError: 995\n'
                    '            +---------------- 2 ----------------\n'
                    '            | ExceptionGroup: eg994 (3 sub-exceptions)\n'
                    '            +-+---------------- 1 ----------------\n'
                    '              | ValueError: 994\n'
                    '              +---------------- 2 ----------------\n'
                    '              | ExceptionGroup: eg993 (3 sub-exceptions)\n'
                    '              +-+---------------- 1 ----------------\n'
                    '                | ValueError: 993\n'
                    '                +---------------- 2 ----------------\n'
                    '                | ExceptionGroup: eg992 (3 sub-exceptions)\n'
                    '                +-+---------------- 1 ----------------\n'
                    '                  | ValueError: 992\n'
                    '                  +---------------- 2 ----------------\n'
                    '                  | ExceptionGroup: eg991 (3 sub-exceptions)\n'
                    '                  +-+---------------- 1 ----------------\n'
                    '                    | ValueError: 991\n'
                    '                    +---------------- 2 ----------------\n'
                    '                    | ExceptionGroup: eg990 (3 sub-exceptions)\n'
                    '                    +-+---------------- 1 ----------------\n'
                    '                      | ValueError: 990\n'
                    '                      +---------------- 2 ----------------\n'
                    '                      | ... (max_group_depth is 10)\n'
                    '                      +---------------- 3 ----------------\n'
                    '                      | ValueError: -990\n'
                    '                      +------------------------------------\n'
                    '                    +---------------- 3 ----------------\n'
                    '                    | ValueError: -991\n'
                    '                    +------------------------------------\n'
                    '                  +---------------- 3 ----------------\n'
                    '                  | ValueError: -992\n'
                    '                  +------------------------------------\n'
                    '                +---------------- 3 ----------------\n'
                    '                | ValueError: -993\n'
                    '                +------------------------------------\n'
                    '              +---------------- 3 ----------------\n'
                    '              | ValueError: -994\n'
                    '              +------------------------------------\n'
                    '            +---------------- 3 ----------------\n'
                    '            | ValueError: -995\n'
                    '            +------------------------------------\n'
                    '          +---------------- 3 ----------------\n'
                    '          | ValueError: -996\n'
                    '          +------------------------------------\n'
                    '        +---------------- 3 ----------------\n'
                    '        | ValueError: -997\n'
                    '        +------------------------------------\n'
                    '      +---------------- 3 ----------------\n'
                    '      | ValueError: -998\n'
                    '      +------------------------------------\n'
                    '    +---------------- 3 ----------------\n'
                    '    | ValueError: -999\n'
                    '    +------------------------------------\n')

        report = self.get_report(exc)
        self.assertEqual(report, expected)

    def test_exception_group_with_notes(self):
        def exc():
            try:
                excs = []
                for msg in ['bad value', 'terrible value']:
                    try:
                        raise ValueError(msg)
                    except ValueError as e:
                        e.add_note(f'the {msg}')
                        excs.append(e)
                raise ExceptionGroup("nested", excs)
            except ExceptionGroup as e:
                e.add_note(('>> Multi line note\n'
                            '>> Because I am such\n'
                            '>> an important exception.\n'
                            '>> empty lines work too\n'
                            '\n'
                            '(that was an empty line)'))
                raise

        expected = (f'  + Exception Group Traceback (most recent call last):\n'
                    f'  |   File "{__file__}", line {self.callable_line}, in get_exception\n'
                    f'  |     exception_or_callable()\n'
                    f'  |   File "{__file__}", line {exc.__code__.co_firstlineno + 9}, in exc\n'
                    f'  |     raise ExceptionGroup("nested", excs)\n'
                    f'  | ExceptionGroup: nested (2 sub-exceptions)\n'
                    f'  | >> Multi line note\n'
                    f'  | >> Because I am such\n'
                    f'  | >> an important exception.\n'
                    f'  | >> empty lines work too\n'
                    f'  | \n'
                    f'  | (that was an empty line)\n'
                    f'  +-+---------------- 1 ----------------\n'
                    f'    | Traceback (most recent call last):\n'
                    f'    |   File "{__file__}", line {exc.__code__.co_firstlineno + 5}, in exc\n'
                    f'    |     raise ValueError(msg)\n'
                    f'    | ValueError: bad value\n'
                    f'    | the bad value\n'
                    f'    +---------------- 2 ----------------\n'
                    f'    | Traceback (most recent call last):\n'
                    f'    |   File "{__file__}", line {exc.__code__.co_firstlineno + 5}, in exc\n'
                    f'    |     raise ValueError(msg)\n'
                    f'    | ValueError: terrible value\n'
                    f'    | the terrible value\n'
                    f'    +------------------------------------\n')

        report = self.get_report(exc)
        self.assertEqual(report, expected)

    def test_exception_group_with_multiple_notes(self):
        def exc():
            try:
                excs = []
                for msg in ['bad value', 'terrible value']:
                    try:
                        raise ValueError(msg)
                    except ValueError as e:
                        e.add_note(f'the {msg}')
                        e.add_note(f'Goodbye {msg}')
                        excs.append(e)
                raise ExceptionGroup("nested", excs)
            except ExceptionGroup as e:
                e.add_note(('>> Multi line note\n'
                            '>> Because I am such\n'
                            '>> an important exception.\n'
                            '>> empty lines work too\n'
                            '\n'
                            '(that was an empty line)'))
                e.add_note('Goodbye!')
                raise

        expected = (f'  + Exception Group Traceback (most recent call last):\n'
                    f'  |   File "{__file__}", line {self.callable_line}, in get_exception\n'
                    f'  |     exception_or_callable()\n'
                    f'  |   File "{__file__}", line {exc.__code__.co_firstlineno + 10}, in exc\n'
                    f'  |     raise ExceptionGroup("nested", excs)\n'
                    f'  | ExceptionGroup: nested (2 sub-exceptions)\n'
                    f'  | >> Multi line note\n'
                    f'  | >> Because I am such\n'
                    f'  | >> an important exception.\n'
                    f'  | >> empty lines work too\n'
                    f'  | \n'
                    f'  | (that was an empty line)\n'
                    f'  | Goodbye!\n'
                    f'  +-+---------------- 1 ----------------\n'
                    f'    | Traceback (most recent call last):\n'
                    f'    |   File "{__file__}", line {exc.__code__.co_firstlineno + 5}, in exc\n'
                    f'    |     raise ValueError(msg)\n'
                    f'    | ValueError: bad value\n'
                    f'    | the bad value\n'
                    f'    | Goodbye bad value\n'
                    f'    +---------------- 2 ----------------\n'
                    f'    | Traceback (most recent call last):\n'
                    f'    |   File "{__file__}", line {exc.__code__.co_firstlineno + 5}, in exc\n'
                    f'    |     raise ValueError(msg)\n'
                    f'    | ValueError: terrible value\n'
                    f'    | the terrible value\n'
                    f'    | Goodbye terrible value\n'
                    f'    +------------------------------------\n')

        report = self.get_report(exc)
        self.assertEqual(report, expected)

    def test_KeyboardInterrupt_at_first_line_of_frame(self):
        # see GH-93249
        def f():
            return sys._getframe()

        tb_next = None
        frame = f()
        lasti = 0
        lineno = f.__code__.co_firstlineno
        tb = types.TracebackType(tb_next, frame, lasti, lineno)

        exc = KeyboardInterrupt()
        exc.__traceback__ = tb

        expected = (f'Traceback (most recent call last):\n'
                    f'  File "{__file__}", line {lineno}, in f\n'
                    f'    def f():\n'
                    f'\n'
                    f'KeyboardInterrupt\n')

        report = self.get_report(exc)
        # remove trailing writespace:
        report = '\n'.join([l.rstrip() for l in report.split('\n')])
        self.assertEqual(report, expected)


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

    def test_no_line(self):
        f = traceback.FrameSummary("f", None, "dummy")
        self.assertEqual(f.line, None)

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

    def test_custom_format_frame(self):
        class CustomStackSummary(traceback.StackSummary):
            def format_frame_summary(self, frame_summary):
                return f'{frame_summary.filename}:{frame_summary.lineno}'

        def some_inner():
            return CustomStackSummary.extract(
                traceback.walk_stack(None), limit=1)

        s = some_inner()
        self.assertEqual(
            s.format(),
            [f'{__file__}:{some_inner.__code__.co_firstlineno + 1}'])

    def test_dropping_frames(self):
        def f():
            1/0

        def g():
            try:
                f()
            except:
                return sys.exc_info()

        exc_info = g()

        class Skip_G(traceback.StackSummary):
            def format_frame_summary(self, frame_summary):
                if frame_summary.name == 'g':
                    return None
                return super().format_frame_summary(frame_summary)

        stack = Skip_G.extract(
            traceback.walk_tb(exc_info[2])).format()

        self.assertEqual(len(stack), 1)
        lno = f.__code__.co_firstlineno + 1
        self.assertEqual(
            stack[0],
            f'  File "{__file__}", line {lno}, in f\n    1/0\n'
        )


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

    def test_long_context_chain(self):
        def f():
            try:
                1/0
            except:
                f()

        try:
            f()
        except RecursionError:
            exc_info = sys.exc_info()
        else:
            self.fail("Exception not raised")

        te = traceback.TracebackException(*exc_info)
        res = list(te.format())

        # many ZeroDiv errors followed by the RecursionError
        self.assertGreater(len(res), sys.getrecursionlimit())
        self.assertGreater(
            len([l for l in res if 'ZeroDivisionError:' in l]),
            sys.getrecursionlimit() * 0.5)
        self.assertIn(
            "RecursionError: maximum recursion depth exceeded", res[-1])

    def test_compact_with_cause(self):
        try:
            try:
                1/0
            finally:
                cause = Exception("cause")
                raise Exception("uh oh") from cause
        except Exception:
            exc_info = sys.exc_info()
            exc = traceback.TracebackException(*exc_info, compact=True)
            expected_stack = traceback.StackSummary.extract(
                traceback.walk_tb(exc_info[2]))
        exc_cause = traceback.TracebackException(Exception, cause, None)
        self.assertEqual(exc_cause, exc.__cause__)
        self.assertEqual(None, exc.__context__)
        self.assertEqual(True, exc.__suppress_context__)
        self.assertEqual(expected_stack, exc.stack)
        self.assertEqual(exc_info[0], exc.exc_type)
        self.assertEqual(str(exc_info[1]), str(exc))

    def test_compact_no_cause(self):
        try:
            try:
                1/0
            finally:
                exc_info_context = sys.exc_info()
                exc_context = traceback.TracebackException(*exc_info_context)
                raise Exception("uh oh")
        except Exception:
            exc_info = sys.exc_info()
            exc = traceback.TracebackException(*exc_info, compact=True)
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
        tb = test_tb(f, 6, None, 0)
        exc = traceback.TracebackException(Exception, e, tb, lookup_lines=False)
        self.assertEqual(linecache.cache, {})
        linecache.updatecache('/foo.py', globals())
        self.assertEqual(exc.stack[0].line, "import sys")

    def test_locals(self):
        linecache.updatecache('/foo.py', globals())
        e = Exception("uh oh")
        c = test_code('/foo.py', 'method')
        f = test_frame(c, globals(), {'something': 1, 'other': 'string'})
        tb = test_tb(f, 6, None, 0)
        exc = traceback.TracebackException(
            Exception, e, tb, capture_locals=True)
        self.assertEqual(
            exc.stack[0].locals, {'something': '1', 'other': "'string'"})

    def test_no_locals(self):
        linecache.updatecache('/foo.py', globals())
        e = Exception("uh oh")
        c = test_code('/foo.py', 'method')
        f = test_frame(c, globals(), {'something': 1})
        tb = test_tb(f, 6, None, 0)
        exc = traceback.TracebackException(Exception, e, tb)
        self.assertEqual(exc.stack[0].locals, None)

    def test_traceback_header(self):
        # do not print a traceback header if exc_traceback is None
        # see issue #24695
        exc = traceback.TracebackException(Exception, Exception("haven"), None)
        self.assertEqual(list(exc.format()), ["Exception: haven\n"])

    @requires_debug_ranges()
    def test_print(self):
        def f():
            x = 12
            try:
                x/0
            except Exception:
                return sys.exc_info()
        exc = traceback.TracebackException(*f(), capture_locals=True)
        output = StringIO()
        exc.print(file=output)
        self.assertEqual(
            output.getvalue().split('\n')[-5:],
            ['    x/0',
             '    ~^~',
             '    x = 12',
             'ZeroDivisionError: division by zero',
             ''])


class TestTracebackException_ExceptionGroups(unittest.TestCase):
    def setUp(self):
        super().setUp()
        self.eg_info = self._get_exception_group()

    def _get_exception_group(self):
        def f():
            1/0

        def g(v):
            raise ValueError(v)

        self.lno_f = f.__code__.co_firstlineno
        self.lno_g = g.__code__.co_firstlineno

        try:
            try:
                try:
                    f()
                except Exception as e:
                    exc1 = e
                try:
                    g(42)
                except Exception as e:
                    exc2 = e
                raise ExceptionGroup("eg1", [exc1, exc2])
            except ExceptionGroup as e:
                exc3 = e
            try:
                g(24)
            except Exception as e:
                exc4 = e
            raise ExceptionGroup("eg2", [exc3, exc4])
        except ExceptionGroup:
            return sys.exc_info()
        self.fail('Exception Not Raised')

    def test_exception_group_construction(self):
        eg_info = self.eg_info
        teg1 = traceback.TracebackException(*eg_info)
        teg2 = traceback.TracebackException.from_exception(eg_info[1])
        self.assertIsNot(teg1, teg2)
        self.assertEqual(teg1, teg2)

    def test_exception_group_format_exception_only(self):
        teg = traceback.TracebackException(*self.eg_info)
        formatted = ''.join(teg.format_exception_only()).split('\n')
        expected = "ExceptionGroup: eg2 (2 sub-exceptions)\n".split('\n')

        self.assertEqual(formatted, expected)

    def test_exception_group_format(self):
        teg = traceback.TracebackException(*self.eg_info)

        formatted = ''.join(teg.format()).split('\n')
        lno_f = self.lno_f
        lno_g = self.lno_g

        expected = [
                    f'  + Exception Group Traceback (most recent call last):',
                    f'  |   File "{__file__}", line {lno_g+23}, in _get_exception_group',
                    f'  |     raise ExceptionGroup("eg2", [exc3, exc4])',
                    f'  | ExceptionGroup: eg2 (2 sub-exceptions)',
                    f'  +-+---------------- 1 ----------------',
                    f'    | Exception Group Traceback (most recent call last):',
                    f'    |   File "{__file__}", line {lno_g+16}, in _get_exception_group',
                    f'    |     raise ExceptionGroup("eg1", [exc1, exc2])',
                    f'    | ExceptionGroup: eg1 (2 sub-exceptions)',
                    f'    +-+---------------- 1 ----------------',
                    f'      | Traceback (most recent call last):',
                    f'      |   File "{__file__}", line {lno_g+9}, in _get_exception_group',
                    f'      |     f()',
                    f'      |   File "{__file__}", line {lno_f+1}, in f',
                    f'      |     1/0',
                    f'      |     ~^~',
                    f'      | ZeroDivisionError: division by zero',
                    f'      +---------------- 2 ----------------',
                    f'      | Traceback (most recent call last):',
                    f'      |   File "{__file__}", line {lno_g+13}, in _get_exception_group',
                    f'      |     g(42)',
                    f'      |   File "{__file__}", line {lno_g+1}, in g',
                    f'      |     raise ValueError(v)',
                    f'      | ValueError: 42',
                    f'      +------------------------------------',
                    f'    +---------------- 2 ----------------',
                    f'    | Traceback (most recent call last):',
                    f'    |   File "{__file__}", line {lno_g+20}, in _get_exception_group',
                    f'    |     g(24)',
                    f'    |   File "{__file__}", line {lno_g+1}, in g',
                    f'    |     raise ValueError(v)',
                    f'    | ValueError: 24',
                    f'    +------------------------------------',
                    f'']

        self.assertEqual(formatted, expected)

    def test_max_group_width(self):
        excs1 = []
        excs2 = []
        for i in range(3):
            excs1.append(ValueError(i))
        for i in range(10):
            excs2.append(TypeError(i))

        EG = ExceptionGroup
        eg = EG('eg', [EG('eg1', excs1), EG('eg2', excs2)])

        teg = traceback.TracebackException.from_exception(eg, max_group_width=2)
        formatted = ''.join(teg.format()).split('\n')

        expected = [
                    f'  | ExceptionGroup: eg (2 sub-exceptions)',
                    f'  +-+---------------- 1 ----------------',
                    f'    | ExceptionGroup: eg1 (3 sub-exceptions)',
                    f'    +-+---------------- 1 ----------------',
                    f'      | ValueError: 0',
                    f'      +---------------- 2 ----------------',
                    f'      | ValueError: 1',
                    f'      +---------------- ... ----------------',
                    f'      | and 1 more exception',
                    f'      +------------------------------------',
                    f'    +---------------- 2 ----------------',
                    f'    | ExceptionGroup: eg2 (10 sub-exceptions)',
                    f'    +-+---------------- 1 ----------------',
                    f'      | TypeError: 0',
                    f'      +---------------- 2 ----------------',
                    f'      | TypeError: 1',
                    f'      +---------------- ... ----------------',
                    f'      | and 8 more exceptions',
                    f'      +------------------------------------',
                    f'']

        self.assertEqual(formatted, expected)

    def test_max_group_depth(self):
        exc = TypeError('bad type')
        for i in range(3):
            exc = ExceptionGroup('exc', [ValueError(-i), exc, ValueError(i)])

        teg = traceback.TracebackException.from_exception(exc, max_group_depth=2)
        formatted = ''.join(teg.format()).split('\n')

        expected = [
                    f'  | ExceptionGroup: exc (3 sub-exceptions)',
                    f'  +-+---------------- 1 ----------------',
                    f'    | ValueError: -2',
                    f'    +---------------- 2 ----------------',
                    f'    | ExceptionGroup: exc (3 sub-exceptions)',
                    f'    +-+---------------- 1 ----------------',
                    f'      | ValueError: -1',
                    f'      +---------------- 2 ----------------',
                    f'      | ... (max_group_depth is 2)',
                    f'      +---------------- 3 ----------------',
                    f'      | ValueError: 1',
                    f'      +------------------------------------',
                    f'    +---------------- 3 ----------------',
                    f'    | ValueError: 2',
                    f'    +------------------------------------',
                    f'']

        self.assertEqual(formatted, expected)

    def test_comparison(self):
        try:
            raise self.eg_info[1]
        except ExceptionGroup:
            exc_info = sys.exc_info()
        for _ in range(5):
            try:
                raise exc_info[1]
            except:
                exc_info = sys.exc_info()
        exc = traceback.TracebackException(*exc_info)
        exc2 = traceback.TracebackException(*exc_info)
        exc3 = traceback.TracebackException(*exc_info, limit=300)
        ne = traceback.TracebackException(*exc_info, limit=3)
        self.assertIsNot(exc, exc2)
        self.assertEqual(exc, exc2)
        self.assertEqual(exc, exc3)
        self.assertNotEqual(exc, ne)
        self.assertNotEqual(exc, object())
        self.assertEqual(exc, ALWAYS_EQ)


class MiscTest(unittest.TestCase):

    def test_all(self):
        expected = set()
        denylist = {'print_list'}
        for name in dir(traceback):
            if name.startswith('_') or name in denylist:
                continue
            module_object = getattr(traceback, name)
            if getattr(module_object, '__module__', None) == 'traceback':
                expected.add(name)
        self.assertCountEqual(traceback.__all__, expected)


if __name__ == "__main__":
    unittest.main()
