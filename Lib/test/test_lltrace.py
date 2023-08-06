import dis
import textwrap
import unittest

from test import support
from test.support import os_helper
from test.support.script_helper import assert_python_ok

def example():
    x = []
    for i in range(0):
        x.append(i)
    x = "this is"
    y = "an example"
    print(x, y)


@unittest.skipUnless(support.Py_DEBUG, "lltrace requires Py_DEBUG")
class TestLLTrace(unittest.TestCase):

    def run_code(self, code):
        code = textwrap.dedent(code).strip()
        with open(os_helper.TESTFN, 'w', encoding='utf-8') as fd:
            self.addCleanup(os_helper.unlink, os_helper.TESTFN)
            fd.write(code)
        status, stdout, stderr = assert_python_ok(os_helper.TESTFN)
        self.assertEqual(stderr, b"")
        self.assertEqual(status, 0)
        result = stdout.decode('utf-8')
        if support.verbose:
            print("\n\n--- code ---")
            print(code)
            print("\n--- stdout ---")
            print(result)
            print()
        return result

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
            __lltrace__ = 1
            trace_me()
            del __lltrace__
            dont_trace_2()
        """)
        self.assertIn("GET_ITER", stdout)
        self.assertIn("FOR_ITER", stdout)
        self.assertIn("CALL_INTRINSIC_1", stdout)
        self.assertIn("POP_TOP", stdout)
        self.assertNotIn("BINARY_OP", stdout)
        self.assertNotIn("UNARY_NEGATIVE", stdout)

        self.assertIn("'trace_me' in module '__main__'", stdout)
        self.assertNotIn("dont_trace_1", stdout)
        self.assertNotIn("'dont_trace_2' in module", stdout)

    def test_lltrace_different_module(self):
        stdout = self.run_code("""
            from test import test_lltrace
            test_lltrace.__lltrace__ = 1
            test_lltrace.example()
        """)
        self.assertIn("'example' in module 'test.test_lltrace'", stdout)
        self.assertIn('LOAD_CONST', stdout)
        self.assertIn('FOR_ITER', stdout)
        self.assertIn('this is an example', stdout)

        # check that offsets match the output of dis.dis()
        instr_map = {i.offset: i for i in dis.get_instructions(example, adaptive=True)}
        for line in stdout.splitlines():
            offset, colon, opname_oparg = line.partition(":")
            if not colon:
                continue
            offset = int(offset)
            opname_oparg = opname_oparg.split()
            if len(opname_oparg) == 2:
                opname, oparg = opname_oparg
                oparg = int(oparg)
            else:
                (opname,) = opname_oparg
                oparg = None
            self.assertEqual(instr_map[offset].opname, opname)
            self.assertEqual(instr_map[offset].arg, oparg)

    def test_lltrace_does_not_crash_on_subscript_operator(self):
        # If this test fails, it will reproduce a crash reported as
        # bpo-34113. The crash happened at the command line console of
        # debug Python builds with __lltrace__ enabled (only possible in console),
        # when the internal Python stack was negatively adjusted
        stdout = self.run_code("""
            import code

            console = code.InteractiveConsole()
            console.push('__lltrace__ = 1')
            console.push('a = [1, 2, 3]')
            console.push('a[0] = 1')
            print('unreachable if bug exists')
        """)
        self.assertIn("unreachable if bug exists", stdout)

if __name__ == "__main__":
    unittest.main()
