from test.test_support import verify, verbose, TestFailed, run_unittest
import sys
import dis
import StringIO

# Minimal tests for dis module

import unittest

# placement is crucial!!!  move the start of _f and you have to adjust the
# line numbers in dis_f
def _f(a):
    print a
    return 1

dis_f = """\
 13           0 LOAD_FAST                0 (a)
              3 PRINT_ITEM
              4 PRINT_NEWLINE

 14           5 LOAD_CONST               1 (1)
              8 RETURN_VALUE
              9 LOAD_CONST               0 (None)
             12 RETURN_VALUE
"""

class DisTests(unittest.TestCase):
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
        s = StringIO.StringIO()
        save_stdout = sys.stdout
        sys.stdout = s
        dis.dis(_f)
        sys.stdout = save_stdout
        got = s.getvalue()
        # Trim trailing blanks (if any).
        lines = got.split('\n')
        lines = [line.rstrip() for line in lines]
        got = '\n'.join(lines)
        self.assertEqual(dis_f, got)

def test_main():
    run_unittest(DisTests)


if __name__ == "__main__":
    test_main()
