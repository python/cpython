# Minimal tests for dis module

from test.support import run_unittest, captured_stdout
import difflib
import unittest
import sys
import dis
import io

class _C:
    def __init__(self, x):
        self.x = x == 1

dis_c_instance_method = """\
 %-4d         0 LOAD_FAST                1 (x)
              3 LOAD_CONST               1 (1)
              6 COMPARE_OP               2 (==)
              9 LOAD_FAST                0 (self)
             12 STORE_ATTR               0 (x)
             15 LOAD_CONST               0 (None)
             18 RETURN_VALUE
""" % (_C.__init__.__code__.co_firstlineno + 1,)

dis_c_instance_method_bytes = """\
          0 LOAD_FAST           1 (1)
          3 LOAD_CONST          1 (1)
          6 COMPARE_OP          2 (==)
          9 LOAD_FAST           0 (0)
         12 STORE_ATTR          0 (0)
         15 LOAD_CONST          0 (0)
         18 RETURN_VALUE
"""

def _f(a):
    print(a)
    return 1

dis_f = """\
 %-4d         0 LOAD_GLOBAL              0 (print)
              3 LOAD_FAST                0 (a)
              6 CALL_FUNCTION            1
              9 POP_TOP

 %-4d        10 LOAD_CONST               1 (1)
             13 RETURN_VALUE
""" % (_f.__code__.co_firstlineno + 1,
       _f.__code__.co_firstlineno + 2)


dis_f_co_code = """\
          0 LOAD_GLOBAL         0 (0)
          3 LOAD_FAST           0 (0)
          6 CALL_FUNCTION       1
          9 POP_TOP
         10 LOAD_CONST          1 (1)
         13 RETURN_VALUE
"""


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
""" % (bug708901.__code__.co_firstlineno + 1,
       bug708901.__code__.co_firstlineno + 2,
       bug708901.__code__.co_firstlineno + 3)


def bug1333982(x=[]):
    assert 0, ([s for s in x] +
              1)
    pass

dis_bug1333982 = """\
 %-4d         0 LOAD_CONST               1 (0)
              3 JUMP_IF_TRUE            33 (to 39)
              6 POP_TOP
              7 LOAD_GLOBAL              0 (AssertionError)
             10 BUILD_LIST               0
             13 LOAD_FAST                0 (x)
             16 GET_ITER
        >>   17 FOR_ITER                12 (to 32)
             20 STORE_FAST               1 (s)
             23 LOAD_FAST                1 (s)
             26 LIST_APPEND              2
             29 JUMP_ABSOLUTE           17

 %-4d   >>   32 LOAD_CONST               2 (1)
             35 BINARY_ADD
             36 RAISE_VARARGS            2
        >>   39 POP_TOP

 %-4d        40 LOAD_CONST               0 (None)
             43 RETURN_VALUE
""" % (bug1333982.__code__.co_firstlineno + 1,
       bug1333982.__code__.co_firstlineno + 2,
       bug1333982.__code__.co_firstlineno + 3)

_BIG_LINENO_FORMAT = """\
%3d           0 LOAD_GLOBAL              0 (spam)
              3 POP_TOP
              4 LOAD_CONST               0 (None)
              7 RETURN_VALUE
"""

dis_module_expected_results = """\
Disassembly of f:
  4           0 LOAD_CONST               0 (None)
              3 RETURN_VALUE

Disassembly of g:
  5           0 LOAD_CONST               0 (None)
              3 RETURN_VALUE

"""

expr_str = "x + 1"

dis_expr_str = """\
  1           0 LOAD_NAME                0 (x)
              3 LOAD_CONST               0 (1)
              6 BINARY_ADD
              7 RETURN_VALUE
"""

simple_stmt_str = "x = x + 1"

dis_simple_stmt_str = """\
  1           0 LOAD_NAME                0 (x)
              3 LOAD_CONST               0 (1)
              6 BINARY_ADD
              7 STORE_NAME               0 (x)
             10 LOAD_CONST               1 (None)
             13 RETURN_VALUE
"""

compound_stmt_str = """\
x = 0
while 1:
    x += 1"""
# Trailing newline has been deliberately omitted

dis_compound_stmt_str = """\
  1           0 LOAD_CONST               0 (0)
              3 STORE_NAME               0 (x)

  2           6 SETUP_LOOP              13 (to 22)

  3     >>    9 LOAD_NAME                0 (x)
             12 LOAD_CONST               1 (1)
             15 INPLACE_ADD
             16 STORE_NAME               0 (x)
             19 JUMP_ABSOLUTE            9
        >>   22 LOAD_CONST               2 (None)
             25 RETURN_VALUE
"""

class DisTests(unittest.TestCase):

    def get_disassembly(self, func, lasti=-1, wrapper=True):
        s = io.StringIO()
        save_stdout = sys.stdout
        sys.stdout = s
        try:
            if wrapper:
                dis.dis(func)
            else:
                dis.disassemble(func, lasti)
        finally:
            sys.stdout = save_stdout
        # Trim trailing blanks (if any).
        return [line.rstrip() for line in s.getvalue().splitlines()]

    def get_disassemble_as_string(self, func, lasti=-1):
        return '\n'.join(self.get_disassembly(func, lasti, False))

    def do_disassembly_test(self, func, expected):
        lines = self.get_disassembly(func)
        expected = expected.splitlines()
        if expected != lines:
            self.fail(
                "events did not match expectation:\n" +
                "\n".join(difflib.ndiff(expected,
                                        lines)))

    def test_opmap(self):
        self.assertEqual(dis.opmap["STOP_CODE"], 0)
        self.assertIn(dis.opmap["LOAD_CONST"], dis.hasconst)
        self.assertIn(dis.opmap["STORE_NAME"], dis.hasname)

    def test_opname(self):
        self.assertEqual(dis.opname[dis.opmap["LOAD_FAST"]], "LOAD_FAST")

    def test_boundaries(self):
        self.assertEqual(dis.opmap["EXTENDED_ARG"], dis.EXTENDED_ARG)
        self.assertEqual(dis.opmap["STORE_NAME"], dis.HAVE_ARGUMENT)

    def test_dis(self):
        self.do_disassembly_test(_f, dis_f)

    def test_bug_708901(self):
        self.do_disassembly_test(bug708901, dis_bug708901)

    def test_bug_1333982(self):
        # XXX: re-enable this test!
        # This one is checking bytecodes generated for an `assert` statement,
        # so fails if the tests are run with -O.  Skip this test then.
        pass # Test has been disabled due to change in the way
             # list comps are handled. The byte code now includes
             # a memory address and a file location, so they change from
             # run to run.
        # if __debug__:
        #    self.do_disassembly_test(bug1333982, dis_bug1333982)

    def test_big_linenos(self):
        def func(count):
            namespace = {}
            func = "def foo():\n " + "".join(["\n "] * count + ["spam\n"])
            exec(func, namespace)
            return namespace['foo']

        # Test all small ranges
        for i in range(1, 300):
            expected = _BIG_LINENO_FORMAT % (i + 2)
            self.do_disassembly_test(func(i), expected)

        # Test some larger ranges too
        for i in range(300, 5000, 10):
            expected = _BIG_LINENO_FORMAT % (i + 2)
            self.do_disassembly_test(func(i), expected)

    def test_big_linenos(self):
        from test import dis_module
        self.do_disassembly_test(dis_module, dis_module_expected_results)

    def test_disassemble_str(self):
        self.do_disassembly_test(expr_str, dis_expr_str)
        self.do_disassembly_test(simple_stmt_str, dis_simple_stmt_str)
        self.do_disassembly_test(compound_stmt_str, dis_compound_stmt_str)

    def test_disassemble_bytes(self):
        self.do_disassembly_test(_f.__code__.co_code, dis_f_co_code)

    def test_disassemble_method(self):
        self.do_disassembly_test(_C(1).__init__, dis_c_instance_method)

    def test_disassemble_method_bytes(self):
        method_bytecode = _C(1).__init__.__code__.co_code
        self.do_disassembly_test(method_bytecode, dis_c_instance_method_bytes)

    def test_dis_none(self):
        self.assertRaises(RuntimeError, dis.dis, None)

    def test_dis_object(self):
        self.assertRaises(TypeError, dis.dis, object())

    def test_dis_traceback(self):
        not_defined = object()
        tb = None
        old = getattr(sys, 'last_traceback', not_defined)

        def cleanup():
            if old is not not_defined:
                sys.last_traceback = old
            else:
                del sys.last_traceback

        try:
            1/0
        except Exception as e:
            tb = e.__traceback__
            sys.last_traceback = tb
            self.addCleanup(cleanup)

        tb_dis = self.get_disassemble_as_string(tb.tb_frame.f_code, tb.tb_lasti)
        self.do_disassembly_test(None, tb_dis)

    def test_dis_object(self):
        self.assertRaises(TypeError, dis.dis, object())

code_info_code_info = """\
Name:              code_info
Filename:          (.*)
Argument count:    1
Kw-only arguments: 0
Number of locals:  1
Stack size:        4
Flags:             OPTIMIZED, NEWLOCALS, NOFREE
Constants:
   0: %r
   1: '__func__'
   2: '__code__'
   3: '<code_info>'
   4: 'co_code'
   5: "don't know how to disassemble %%s objects"
%sNames:
   0: hasattr
   1: __func__
   2: __code__
   3: isinstance
   4: str
   5: _try_compile
   6: _format_code_info
   7: TypeError
   8: type
   9: __name__
Variable names:
   0: x""" % (('Formatted details of methods, functions, or code.', '   6: None\n')
              if sys.flags.optimize < 2 else (None, ''))

@staticmethod
def tricky(x, y, z=True, *args, c, d, e=[], **kwds):
    def f(c=c):
        print(x, y, z, c, d, e, f)
    yield x, y, z, c, d, e, f

code_info_tricky = """\
Name:              tricky
Filename:          (.*)
Argument count:    3
Kw-only arguments: 3
Number of locals:  8
Stack size:        7
Flags:             OPTIMIZED, NEWLOCALS, VARARGS, VARKEYWORDS, GENERATOR
Constants:
   0: None
   1: <code object f at (.*), file "(.*)", line (.*)>
Variable names:
   0: x
   1: y
   2: z
   3: c
   4: d
   5: e
   6: args
   7: kwds
Cell variables:
   0: e
   1: d
   2: f
   3: y
   4: x
   5: z"""

co_tricky_nested_f = tricky.__func__.__code__.co_consts[1]

code_info_tricky_nested_f = """\
Name:              f
Filename:          (.*)
Argument count:    1
Kw-only arguments: 0
Number of locals:  1
Stack size:        8
Flags:             OPTIMIZED, NEWLOCALS, NESTED
Constants:
   0: None
Names:
   0: print
Variable names:
   0: c
Free variables:
   0: e
   1: d
   2: f
   3: y
   4: x
   5: z"""

code_info_expr_str = """\
Name:              <module>
Filename:          <code_info>
Argument count:    0
Kw-only arguments: 0
Number of locals:  0
Stack size:        2
Flags:             NOFREE
Constants:
   0: 1
Names:
   0: x"""

code_info_simple_stmt_str = """\
Name:              <module>
Filename:          <code_info>
Argument count:    0
Kw-only arguments: 0
Number of locals:  0
Stack size:        2
Flags:             NOFREE
Constants:
   0: 1
   1: None
Names:
   0: x"""

code_info_compound_stmt_str = """\
Name:              <module>
Filename:          <code_info>
Argument count:    0
Kw-only arguments: 0
Number of locals:  0
Stack size:        2
Flags:             NOFREE
Constants:
   0: 0
   1: 1
   2: None
Names:
   0: x"""

class CodeInfoTests(unittest.TestCase):
    test_pairs = [
      (dis.code_info, code_info_code_info),
      (tricky, code_info_tricky),
      (co_tricky_nested_f, code_info_tricky_nested_f),
      (expr_str, code_info_expr_str),
      (simple_stmt_str, code_info_simple_stmt_str),
      (compound_stmt_str, code_info_compound_stmt_str),
    ]

    def test_code_info(self):
        self.maxDiff = 1000
        for x, expected in self.test_pairs:
            self.assertRegex(dis.code_info(x), expected)

    def test_show_code(self):
        self.maxDiff = 1000
        for x, expected in self.test_pairs:
            with captured_stdout() as output:
                dis.show_code(x)
            self.assertRegex(output.getvalue(), expected+"\n")

    def test_code_info_object(self):
        self.assertRaises(TypeError, dis.code_info, object())

    def test_pretty_flags_no_flags(self):
        self.assertEqual(dis.pretty_flags(0), '0x0')


def test_main():
    run_unittest(DisTests, CodeInfoTests)

if __name__ == "__main__":
    test_main()
