from test.test_support import verify, verbose, TestFailed, run_unittest
import sys
import dis
import StringIO

# Minimal tests for dis module

import unittest

def _f(a):
    print a
    return 1

dis_f = """\
 %-4d         0 LOAD_FAST                0 (a)
              3 PRINT_ITEM
              4 PRINT_NEWLINE

 %-4d         5 LOAD_CONST               1 (1)
              8 RETURN_VALUE
"""%(_f.func_code.co_firstlineno + 1,
     _f.func_code.co_firstlineno + 2)


def bug708901():
    for res in range(1,
                     10):
        pass

dis_bug708901 = """\
 %-4d         0 SETUP_LOOP              23 (to 26)
              3 LOAD_GLOBAL              0 (range)
              6 LOAD_CONST               1 (1)

 %-4d         9 LOAD_CONST               2 (10)
             12 CALL_FUNCTION            2
             15 GET_ITER
        >>   16 FOR_ITER                 6 (to 25)
             19 STORE_FAST               0 (res)

 %-4d        22 JUMP_ABSOLUTE           16
        >>   25 POP_BLOCK
        >>   26 LOAD_CONST               0 (None)
             29 RETURN_VALUE
"""%(bug708901.func_code.co_firstlineno + 1,
     bug708901.func_code.co_firstlineno + 2,
     bug708901.func_code.co_firstlineno + 3)

class DisTests(unittest.TestCase):
    def do_disassembly_test(self, func, expected):
        s = StringIO.StringIO()
        save_stdout = sys.stdout
        sys.stdout = s
        dis.dis(func)
        sys.stdout = save_stdout
        got = s.getvalue()
        # Trim trailing blanks (if any).
        lines = got.split('\n')
        lines = [line.rstrip() for line in lines]
        expected = expected.split("\n")
        import difflib
        if expected != lines:
            self.fail(
                "events did not match expectation:\n" +
                "\n".join(difflib.ndiff(expected,
                                        lines)))

    def test_opmap(self):
        self.assertEqual(dis.opmap["STOP_CODE"], 0)
        self.assertEqual(dis.opmap["LOAD_CONST"] in dis.hasconst, True)
        self.assertEqual(dis.opmap["STORE_NAME"] in dis.hasname, True)

    def test_opname(self):
        self.assertEqual(dis.opname[dis.opmap["LOAD_FAST"]], "LOAD_FAST")

    def test_boundaries(self):
        self.assertEqual(dis.opmap["EXTENDED_ARG"], dis.EXTENDED_ARG)
        self.assertEqual(dis.opmap["STORE_NAME"], dis.HAVE_ARGUMENT)

    def test_dis(self):
        self.do_disassembly_test(_f, dis_f)

    def test_bug_708901(self):
        self.do_disassembly_test(bug708901, dis_bug708901)

def test_main():
    run_unittest(DisTests)


if __name__ == "__main__":
    test_main()
