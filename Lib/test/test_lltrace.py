import opcode
import re
import sys
import textwrap
import unittest

from test.support import os_helper, verbose
from test.support.script_helper import assert_python_ok


Py_DEBUG = hasattr(sys, 'gettotalrefcount')

@unittest.skipUnless(Py_DEBUG, "lltrace requires Py_DEBUG")
class TestLLTrace(unittest.TestCase):

    def test_lltrace_does_not_crash_on_subscript_operator(self):
        # If this test fails, it will reproduce a crash reported as
        # bpo-34113. The crash happened at the command line console of
        # debug Python builds with __ltrace__ enabled (only possible in console),
        # when the internal Python stack was negatively adjusted
        with open(os_helper.TESTFN, 'w', encoding='utf-8') as fd:
            self.addCleanup(os_helper.unlink, os_helper.TESTFN)
            fd.write(textwrap.dedent("""\
            import code

            console = code.InteractiveConsole()
            console.push('__ltrace__ = 1')
            console.push('a = [1, 2, 3]')
            console.push('a[0] = 1')
            print('unreachable if bug exists')
            """))

            assert_python_ok(os_helper.TESTFN)

    def run_code(self, code):
        code = textwrap.dedent(code).strip()
        with open(os_helper.TESTFN, 'w', encoding='utf-8') as fd:
            self.addCleanup(os_helper.unlink, os_helper.TESTFN)
            fd.write(code)
        status, stdout, stderr = assert_python_ok(os_helper.TESTFN)
        self.assertEqual(stderr, b"")
        self.assertEqual(status, 0)
        result = stdout.decode('utf-8')
        if verbose:
            print("\n\n--- code ---")
            print(code)
            print("\n--- stdout ---")
            print(result)
            print()
        return result

    def check_op(self, op, stdout, present):
        op = opcode.opmap[op]
        regex = re.compile(f': {op}($|, )', re.MULTILINE)
        if present:
            self.assertTrue(regex.search(stdout),
                            f'": {op}" not found in: {stdout}')
        else:
            self.assertFalse(regex.search(stdout),
                             f'": {op}" found in: {stdout}')

    def check_op_in(self, op, stdout):
        self.check_op(op, stdout, True)

    def check_op_not_in(self, op, stdout):
        self.check_op(op, stdout, False)

    def test_lltrace(self):
        stdout = self.run_code("""
            def dont_trace_1():
                a = "a"
                a = 10 * a
            def trace_me():
                for i in range(3):
                    +i
            def dont_trace_2():
                x = 42
                y = -x
            dont_trace_1()
            __ltrace__ = 1
            trace_me()
            del __ltrace__
            dont_trace_2()
        """)
        self.check_op_in("GET_ITER", stdout)
        self.check_op_in("FOR_ITER", stdout)
        self.check_op_in("UNARY_POSITIVE", stdout)
        self.check_op_in("POP_TOP", stdout)

        # before: dont_trace_1() is not traced
        self.check_op_not_in("BINARY_MULTIPLY", stdout)

        # after: dont_trace_2() is not traced
        self.check_op_not_in("UNARY_NEGATIVE", stdout)


if __name__ == "__main__":
    unittest.main()
