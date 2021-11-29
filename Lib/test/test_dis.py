# Minimal tests for dis module

from test.support import captured_stdout, requires_debug_ranges
from test.support.bytecode_helper import BytecodeTestCase
import unittest
import sys
import dis
import io
import re
import types
import contextlib

def get_tb():
    def _error():
        try:
            1 / 0
        except Exception as e:
            tb = e.__traceback__
        return tb

    tb = _error()
    while tb.tb_next:
        tb = tb.tb_next
    return tb

TRACEBACK_CODE = get_tb().tb_frame.f_code

class _C:
    def __init__(self, x):
        self.x = x == 1

    @staticmethod
    def sm(x):
        x = x == 1

    @classmethod
    def cm(cls, x):
        cls.x = x == 1

dis_c_instance_method = """\
%3d           0 LOAD_FAST                1 (x)
              2 LOAD_CONST               1 (1)
              4 COMPARE_OP               2 (==)
              6 LOAD_FAST                0 (self)
              8 STORE_ATTR               0 (x)
             10 LOAD_CONST               0 (None)
             12 RETURN_VALUE
""" % (_C.__init__.__code__.co_firstlineno + 1,)

dis_c_instance_method_bytes = """\
          0 LOAD_FAST                1
          2 LOAD_CONST               1
          4 COMPARE_OP               2 (==)
          6 LOAD_FAST                0
          8 STORE_ATTR               0
         10 LOAD_CONST               0
         12 RETURN_VALUE
"""

dis_c_class_method = """\
%3d           0 LOAD_FAST                1 (x)
              2 LOAD_CONST               1 (1)
              4 COMPARE_OP               2 (==)
              6 LOAD_FAST                0 (cls)
              8 STORE_ATTR               0 (x)
             10 LOAD_CONST               0 (None)
             12 RETURN_VALUE
""" % (_C.cm.__code__.co_firstlineno + 2,)

dis_c_static_method = """\
%3d           0 LOAD_FAST                0 (x)
              2 LOAD_CONST               1 (1)
              4 COMPARE_OP               2 (==)
              6 STORE_FAST               0 (x)
              8 LOAD_CONST               0 (None)
             10 RETURN_VALUE
""" % (_C.sm.__code__.co_firstlineno + 2,)

# Class disassembling info has an extra newline at end.
dis_c = """\
Disassembly of %s:
%s
Disassembly of %s:
%s
Disassembly of %s:
%s
""" % (_C.__init__.__name__, dis_c_instance_method,
       _C.cm.__name__, dis_c_class_method,
       _C.sm.__name__, dis_c_static_method)

def _f(a):
    print(a)
    return 1

dis_f = """\
%3d           0 LOAD_GLOBAL              0 (print)
              2 LOAD_FAST                0 (a)
              4 CALL_FUNCTION            1
              6 POP_TOP

%3d           8 LOAD_CONST               1 (1)
             10 RETURN_VALUE
""" % (_f.__code__.co_firstlineno + 1,
       _f.__code__.co_firstlineno + 2)


dis_f_co_code = """\
          0 LOAD_GLOBAL              0
          2 LOAD_FAST                0
          4 CALL_FUNCTION            1
          6 POP_TOP
          8 LOAD_CONST               1
         10 RETURN_VALUE
"""


def bug708901():
    for res in range(1,
                     10):
        pass

dis_bug708901 = """\
%3d           0 LOAD_GLOBAL              0 (range)
              2 LOAD_CONST               1 (1)

%3d           4 LOAD_CONST               2 (10)

%3d           6 CALL_FUNCTION            2
              8 GET_ITER
        >>   10 FOR_ITER                 2 (to 16)
             12 STORE_FAST               0 (res)

%3d          14 JUMP_ABSOLUTE            5 (to 10)

%3d     >>   16 LOAD_CONST               0 (None)
             18 RETURN_VALUE
""" % (bug708901.__code__.co_firstlineno + 1,
       bug708901.__code__.co_firstlineno + 2,
       bug708901.__code__.co_firstlineno + 1,
       bug708901.__code__.co_firstlineno + 3,
       bug708901.__code__.co_firstlineno + 1)


def bug1333982(x=[]):
    assert 0, ([s for s in x] +
              1)
    pass

dis_bug1333982 = """\
%3d           0 LOAD_ASSERTION_ERROR
              2 LOAD_CONST               2 (<code object <listcomp> at 0x..., file "%s", line %d>)
              4 MAKE_FUNCTION            0
              6 LOAD_FAST                0 (x)
              8 GET_ITER
             10 CALL_FUNCTION            1

%3d          12 LOAD_CONST               3 (1)

%3d          14 BINARY_OP                0 (+)
             16 CALL_FUNCTION            1
             18 RAISE_VARARGS            1
""" % (bug1333982.__code__.co_firstlineno + 1,
       __file__,
       bug1333982.__code__.co_firstlineno + 1,
       bug1333982.__code__.co_firstlineno + 2,
       bug1333982.__code__.co_firstlineno + 1)


def bug42562():
    pass


# Set line number for 'pass' to None
bug42562.__code__ = bug42562.__code__.replace(co_linetable=b'\x04\x80')


dis_bug42562 = """\
          0 LOAD_CONST               0 (None)
          2 RETURN_VALUE
"""

# Extended arg followed by NOP
code_bug_45757 = bytes([
        0x90, 0x01,  # EXTENDED_ARG 0x01
        0x09, 0xFF,  # NOP 0xFF
        0x90, 0x01,  # EXTENDED_ARG 0x01
        0x64, 0x29,  # LOAD_CONST 0x29
        0x53, 0x00,  # RETURN_VALUE 0x00
    ])

dis_bug_45757 = """\
          0 EXTENDED_ARG             1
          2 NOP
          4 EXTENDED_ARG             1
          6 LOAD_CONST             297
          8 RETURN_VALUE
"""

_BIG_LINENO_FORMAT = """\
%3d           0 LOAD_GLOBAL              0 (spam)
              2 POP_TOP
              4 LOAD_CONST               0 (None)
              6 RETURN_VALUE
"""

_BIG_LINENO_FORMAT2 = """\
%4d           0 LOAD_GLOBAL              0 (spam)
               2 POP_TOP
               4 LOAD_CONST               0 (None)
               6 RETURN_VALUE
"""

dis_module_expected_results = """\
Disassembly of f:
  4           0 LOAD_CONST               0 (None)
              2 RETURN_VALUE

Disassembly of g:
  5           0 LOAD_CONST               0 (None)
              2 RETURN_VALUE

"""

expr_str = "x + 1"

dis_expr_str = """\
  1           0 LOAD_NAME                0 (x)
              2 LOAD_CONST               0 (1)
              4 BINARY_OP                0 (+)
              6 RETURN_VALUE
"""

simple_stmt_str = "x = x + 1"

dis_simple_stmt_str = """\
  1           0 LOAD_NAME                0 (x)
              2 LOAD_CONST               0 (1)
              4 BINARY_OP                0 (+)
              6 STORE_NAME               0 (x)
              8 LOAD_CONST               1 (None)
             10 RETURN_VALUE
"""

annot_stmt_str = """\

x: int = 1
y: fun(1)
lst[fun(0)]: int = 1
"""
# leading newline is for a reason (tests lineno)

dis_annot_stmt_str = """\
  2           0 SETUP_ANNOTATIONS
              2 LOAD_CONST               0 (1)
              4 STORE_NAME               0 (x)
              6 LOAD_NAME                1 (int)
              8 LOAD_NAME                2 (__annotations__)
             10 LOAD_CONST               1 ('x')
             12 STORE_SUBSCR

  3          14 LOAD_NAME                3 (fun)
             16 LOAD_CONST               0 (1)
             18 CALL_FUNCTION            1
             20 LOAD_NAME                2 (__annotations__)
             22 LOAD_CONST               2 ('y')
             24 STORE_SUBSCR

  4          26 LOAD_CONST               0 (1)
             28 LOAD_NAME                4 (lst)
             30 LOAD_NAME                3 (fun)
             32 LOAD_CONST               3 (0)
             34 CALL_FUNCTION            1
             36 STORE_SUBSCR
             38 LOAD_NAME                1 (int)
             40 POP_TOP
             42 LOAD_CONST               4 (None)
             44 RETURN_VALUE
"""

compound_stmt_str = """\
x = 0
while 1:
    x += 1"""
# Trailing newline has been deliberately omitted

dis_compound_stmt_str = """\
  1           0 LOAD_CONST               0 (0)
              2 STORE_NAME               0 (x)

  2           4 NOP

  3     >>    6 LOAD_NAME                0 (x)
              8 LOAD_CONST               1 (1)
             10 BINARY_OP               13 (+=)
             12 STORE_NAME               0 (x)

  2          14 JUMP_ABSOLUTE            3 (to 6)
"""

dis_traceback = """\
%3d           0 NOP

%3d           2 LOAD_CONST               1 (1)
              4 LOAD_CONST               2 (0)
    -->       6 BINARY_OP               11 (/)
              8 POP_TOP

%3d          10 LOAD_FAST                1 (tb)
             12 RETURN_VALUE
        >>   14 PUSH_EXC_INFO

%3d          16 LOAD_GLOBAL              0 (Exception)
             18 JUMP_IF_NOT_EXC_MATCH    26 (to 52)
             20 POP_TOP
             22 STORE_FAST               0 (e)
             24 POP_TOP

%3d          26 LOAD_FAST                0 (e)
             28 LOAD_ATTR                1 (__traceback__)
             30 STORE_FAST               1 (tb)
             32 POP_EXCEPT
             34 LOAD_CONST               0 (None)
             36 STORE_FAST               0 (e)
             38 DELETE_FAST              0 (e)

%3d          40 LOAD_FAST                1 (tb)
             42 RETURN_VALUE
        >>   44 LOAD_CONST               0 (None)
             46 STORE_FAST               0 (e)
             48 DELETE_FAST              0 (e)
             50 RERAISE                  1

%3d     >>   52 RERAISE                  0
        >>   54 POP_EXCEPT_AND_RERAISE
ExceptionTable:
  2 to 8 -> 14 [0]
  14 to 24 -> 54 [3] lasti
  26 to 30 -> 44 [3] lasti
  44 to 52 -> 54 [3] lasti
""" % (TRACEBACK_CODE.co_firstlineno + 1,
       TRACEBACK_CODE.co_firstlineno + 2,
       TRACEBACK_CODE.co_firstlineno + 5,
       TRACEBACK_CODE.co_firstlineno + 3,
       TRACEBACK_CODE.co_firstlineno + 4,
       TRACEBACK_CODE.co_firstlineno + 5,
       TRACEBACK_CODE.co_firstlineno + 3)

def _fstring(a, b, c, d):
    return f'{a} {b:4} {c!r} {d!r:4}'

dis_fstring = """\
%3d           0 LOAD_FAST                0 (a)
              2 FORMAT_VALUE             0
              4 LOAD_CONST               1 (' ')
              6 LOAD_FAST                1 (b)
              8 LOAD_CONST               2 ('4')
             10 FORMAT_VALUE             4 (with format)
             12 LOAD_CONST               1 (' ')
             14 LOAD_FAST                2 (c)
             16 FORMAT_VALUE             2 (repr)
             18 LOAD_CONST               1 (' ')
             20 LOAD_FAST                3 (d)
             22 LOAD_CONST               2 ('4')
             24 FORMAT_VALUE             6 (repr, with format)
             26 BUILD_STRING             7
             28 RETURN_VALUE
""" % (_fstring.__code__.co_firstlineno + 1,)

def _tryfinally(a, b):
    try:
        return a
    finally:
        b()

def _tryfinallyconst(b):
    try:
        return 1
    finally:
        b()

dis_tryfinally = """\
%3d           0 NOP

%3d           2 LOAD_FAST                0 (a)

%3d           4 LOAD_FAST                1 (b)
              6 CALL_FUNCTION            0
              8 POP_TOP
             10 RETURN_VALUE
        >>   12 PUSH_EXC_INFO
             14 LOAD_FAST                1 (b)
             16 CALL_FUNCTION            0
             18 POP_TOP
             20 RERAISE                  0
        >>   22 POP_EXCEPT_AND_RERAISE
ExceptionTable:
  2 to 2 -> 12 [0]
  12 to 20 -> 22 [3] lasti
""" % (_tryfinally.__code__.co_firstlineno + 1,
       _tryfinally.__code__.co_firstlineno + 2,
       _tryfinally.__code__.co_firstlineno + 4,
       )

dis_tryfinallyconst = """\
%3d           0 NOP

%3d           2 NOP

%3d           4 LOAD_FAST                0 (b)
              6 CALL_FUNCTION            0
              8 POP_TOP
             10 LOAD_CONST               1 (1)
             12 RETURN_VALUE
             14 PUSH_EXC_INFO
             16 LOAD_FAST                0 (b)
             18 CALL_FUNCTION            0
             20 POP_TOP
             22 RERAISE                  0
        >>   24 POP_EXCEPT_AND_RERAISE
ExceptionTable:
  14 to 22 -> 24 [3] lasti
""" % (_tryfinallyconst.__code__.co_firstlineno + 1,
       _tryfinallyconst.__code__.co_firstlineno + 2,
       _tryfinallyconst.__code__.co_firstlineno + 4,
       )

def _g(x):
    yield x

async def _ag(x):
    yield x

async def _co(x):
    async for item in _ag(x):
        pass

def _h(y):
    def foo(x):
        '''funcdoc'''
        return [x + z for z in y]
    return foo

dis_nested_0 = """\
              0 MAKE_CELL                0 (y)

%3d           2 LOAD_CLOSURE             0 (y)
              4 BUILD_TUPLE              1
              6 LOAD_CONST               1 (<code object foo at 0x..., file "%s", line %d>)
              8 MAKE_FUNCTION            8 (closure)
             10 STORE_FAST               1 (foo)

%3d          12 LOAD_FAST                1 (foo)
             14 RETURN_VALUE
""" % (_h.__code__.co_firstlineno + 1,
       __file__,
       _h.__code__.co_firstlineno + 1,
       _h.__code__.co_firstlineno + 4,
)

dis_nested_1 = """%s
Disassembly of <code object foo at 0x..., file "%s", line %d>:
              0 COPY_FREE_VARS           1
              2 MAKE_CELL                0 (x)

%3d           4 LOAD_CLOSURE             0 (x)
              6 BUILD_TUPLE              1
              8 LOAD_CONST               1 (<code object <listcomp> at 0x..., file "%s", line %d>)
             10 MAKE_FUNCTION            8 (closure)
             12 LOAD_DEREF               1 (y)
             14 GET_ITER
             16 CALL_FUNCTION            1
             18 RETURN_VALUE
""" % (dis_nested_0,
       __file__,
       _h.__code__.co_firstlineno + 1,
       _h.__code__.co_firstlineno + 3,
       __file__,
       _h.__code__.co_firstlineno + 3,
)

dis_nested_2 = """%s
Disassembly of <code object <listcomp> at 0x..., file "%s", line %d>:
              0 COPY_FREE_VARS           1

%3d           2 BUILD_LIST               0
              4 LOAD_FAST                0 (.0)
        >>    6 FOR_ITER                 6 (to 20)
              8 STORE_FAST               1 (z)
             10 LOAD_DEREF               2 (x)
             12 LOAD_FAST                1 (z)
             14 BINARY_OP                0 (+)
             16 LIST_APPEND              2
             18 JUMP_ABSOLUTE            3 (to 6)
        >>   20 RETURN_VALUE
""" % (dis_nested_1,
       __file__,
       _h.__code__.co_firstlineno + 3,
       _h.__code__.co_firstlineno + 3,
)


class DisTests(unittest.TestCase):

    maxDiff = None

    def get_disassembly(self, func, lasti=-1, wrapper=True, **kwargs):
        # We want to test the default printing behaviour, not the file arg
        output = io.StringIO()
        with contextlib.redirect_stdout(output):
            if wrapper:
                dis.dis(func, **kwargs)
            else:
                dis.disassemble(func, lasti, **kwargs)
        return output.getvalue()

    def get_disassemble_as_string(self, func, lasti=-1):
        return self.get_disassembly(func, lasti, False)

    def strip_addresses(self, text):
        return re.sub(r'\b0x[0-9A-Fa-f]+\b', '0x...', text)

    def do_disassembly_test(self, func, expected):
        got = self.get_disassembly(func, depth=0)
        if got != expected:
            got = self.strip_addresses(got)
        self.assertEqual(got, expected)

    def test_opmap(self):
        self.assertEqual(dis.opmap["NOP"], 9)
        self.assertIn(dis.opmap["LOAD_CONST"], dis.hasconst)
        self.assertIn(dis.opmap["STORE_NAME"], dis.hasname)

    def test_opname(self):
        self.assertEqual(dis.opname[dis.opmap["LOAD_FAST"]], "LOAD_FAST")

    def test_boundaries(self):
        self.assertEqual(dis.opmap["EXTENDED_ARG"], dis.EXTENDED_ARG)
        self.assertEqual(dis.opmap["STORE_NAME"], dis.HAVE_ARGUMENT)

    def test_widths(self):
        for opcode, opname in enumerate(dis.opname):
            if opname in ('BUILD_MAP_UNPACK_WITH_CALL',
                          'BUILD_TUPLE_UNPACK_WITH_CALL',
                          'JUMP_IF_NOT_EXC_MATCH'):
                continue
            with self.subTest(opname=opname):
                width = dis._OPNAME_WIDTH
                if opcode < dis.HAVE_ARGUMENT:
                    width += 1 + dis._OPARG_WIDTH
                self.assertLessEqual(len(opname), width)

    def test_dis(self):
        self.do_disassembly_test(_f, dis_f)

    def test_bug_708901(self):
        self.do_disassembly_test(bug708901, dis_bug708901)

    def test_bug_1333982(self):
        # This one is checking bytecodes generated for an `assert` statement,
        # so fails if the tests are run with -O.  Skip this test then.
        if not __debug__:
            self.skipTest('need asserts, run without -O')

        self.do_disassembly_test(bug1333982, dis_bug1333982)

    def test_bug_42562(self):
        self.do_disassembly_test(bug42562, dis_bug42562)

    def test_bug_45757(self):
        # Extended arg followed by NOP
        self.do_disassembly_test(code_bug_45757, dis_bug_45757)

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
        for i in range(300, 1000, 10):
            expected = _BIG_LINENO_FORMAT % (i + 2)
            self.do_disassembly_test(func(i), expected)

        for i in range(1000, 5000, 10):
            expected = _BIG_LINENO_FORMAT2 % (i + 2)
            self.do_disassembly_test(func(i), expected)

        from test import dis_module
        self.do_disassembly_test(dis_module, dis_module_expected_results)

    def test_big_offsets(self):
        def func(count):
            namespace = {}
            func = "def foo(x):\n " + ";".join(["x = x + 1"] * count) + "\n return x"
            exec(func, namespace)
            return namespace['foo']

        def expected(count, w):
            s = ['''\
           %*d LOAD_FAST                0 (x)
           %*d LOAD_CONST               1 (1)
           %*d BINARY_OP                0 (+)
           %*d STORE_FAST               0 (x)
''' % (w, 8*i, w, 8*i + 2, w, 8*i + 4, w, 8*i + 6)
                 for i in range(count)]
            s += ['''\

  3        %*d LOAD_FAST                0 (x)
           %*d RETURN_VALUE
''' % (w, 8*count, w, 8*count + 2)]
            s[0] = '  2' + s[0][3:]
            return ''.join(s)

        for i in range(1, 5):
            self.do_disassembly_test(func(i), expected(i, 4))
        self.do_disassembly_test(func(1249), expected(1249, 4))
        self.do_disassembly_test(func(1250), expected(1250, 5))

    def test_disassemble_str(self):
        self.do_disassembly_test(expr_str, dis_expr_str)
        self.do_disassembly_test(simple_stmt_str, dis_simple_stmt_str)
        self.do_disassembly_test(annot_stmt_str, dis_annot_stmt_str)
        self.do_disassembly_test(compound_stmt_str, dis_compound_stmt_str)

    def test_disassemble_bytes(self):
        self.do_disassembly_test(_f.__code__.co_code, dis_f_co_code)

    def test_disassemble_class(self):
        self.do_disassembly_test(_C, dis_c)

    def test_disassemble_instance_method(self):
        self.do_disassembly_test(_C(1).__init__, dis_c_instance_method)

    def test_disassemble_instance_method_bytes(self):
        method_bytecode = _C(1).__init__.__code__.co_code
        self.do_disassembly_test(method_bytecode, dis_c_instance_method_bytes)

    def test_disassemble_static_method(self):
        self.do_disassembly_test(_C.sm, dis_c_static_method)

    def test_disassemble_class_method(self):
        self.do_disassembly_test(_C.cm, dis_c_class_method)

    def test_disassemble_generator(self):
        gen_func_disas = self.get_disassembly(_g)  # Generator function
        gen_disas = self.get_disassembly(_g(1))  # Generator iterator
        self.assertEqual(gen_disas, gen_func_disas)

    def test_disassemble_async_generator(self):
        agen_func_disas = self.get_disassembly(_ag)  # Async generator function
        agen_disas = self.get_disassembly(_ag(1))  # Async generator iterator
        self.assertEqual(agen_disas, agen_func_disas)

    def test_disassemble_coroutine(self):
        coro_func_disas = self.get_disassembly(_co)  # Coroutine function
        coro = _co(1)  # Coroutine object
        coro.close()  # Avoid a RuntimeWarning (never awaited)
        coro_disas = self.get_disassembly(coro)
        self.assertEqual(coro_disas, coro_func_disas)

    def test_disassemble_fstring(self):
        self.do_disassembly_test(_fstring, dis_fstring)

    def test_disassemble_try_finally(self):
        self.do_disassembly_test(_tryfinally, dis_tryfinally)
        self.do_disassembly_test(_tryfinallyconst, dis_tryfinallyconst)

    def test_dis_none(self):
        try:
            del sys.last_traceback
        except AttributeError:
            pass
        self.assertRaises(RuntimeError, dis.dis, None)

    def test_dis_traceback(self):
        try:
            del sys.last_traceback
        except AttributeError:
            pass

        try:
            1/0
        except Exception as e:
            tb = e.__traceback__
            sys.last_traceback = tb

        tb_dis = self.get_disassemble_as_string(tb.tb_frame.f_code, tb.tb_lasti)
        self.do_disassembly_test(None, tb_dis)

    def test_dis_object(self):
        self.assertRaises(TypeError, dis.dis, object())

    def test_disassemble_recursive(self):
        def check(expected, **kwargs):
            dis = self.get_disassembly(_h, **kwargs)
            dis = self.strip_addresses(dis)
            self.assertEqual(dis, expected)

        check(dis_nested_0, depth=0)
        check(dis_nested_1, depth=1)
        check(dis_nested_2, depth=2)
        check(dis_nested_2, depth=3)
        check(dis_nested_2, depth=None)
        check(dis_nested_2)


class DisWithFileTests(DisTests):

    # Run the tests again, using the file arg instead of print
    def get_disassembly(self, func, lasti=-1, wrapper=True, **kwargs):
        output = io.StringIO()
        if wrapper:
            dis.dis(func, file=output, **kwargs)
        else:
            dis.disassemble(func, lasti, file=output, **kwargs)
        return output.getvalue()


if sys.flags.optimize:
    code_info_consts = "0: None"
else:
    code_info_consts = "0: 'Formatted details of methods, functions, or code.'"

code_info_code_info = f"""\
Name:              code_info
Filename:          (.*)
Argument count:    1
Positional-only arguments: 0
Kw-only arguments: 0
Number of locals:  1
Stack size:        3
Flags:             OPTIMIZED, NEWLOCALS
Constants:
   {code_info_consts}
Names:
   0: _format_code_info
   1: _get_code_object
Variable names:
   0: x"""


@staticmethod
def tricky(a, b, /, x, y, z=True, *args, c, d, e=[], **kwds):
    def f(c=c):
        print(a, b, x, y, z, c, d, e, f)
    yield a, b, x, y, z, c, d, e, f

code_info_tricky = """\
Name:              tricky
Filename:          (.*)
Argument count:    5
Positional-only arguments: 2
Kw-only arguments: 3
Number of locals:  10
Stack size:        9
Flags:             OPTIMIZED, NEWLOCALS, VARARGS, VARKEYWORDS, GENERATOR
Constants:
   0: None
   1: <code object f at (.*), file "(.*)", line (.*)>
Variable names:
   0: a
   1: b
   2: x
   3: y
   4: z
   5: c
   6: d
   7: e
   8: args
   9: kwds
Cell variables:
   0: [abedfxyz]
   1: [abedfxyz]
   2: [abedfxyz]
   3: [abedfxyz]
   4: [abedfxyz]
   5: [abedfxyz]"""
# NOTE: the order of the cell variables above depends on dictionary order!

co_tricky_nested_f = tricky.__func__.__code__.co_consts[1]

code_info_tricky_nested_f = """\
Filename:          (.*)
Argument count:    1
Positional-only arguments: 0
Kw-only arguments: 0
Number of locals:  1
Stack size:        10
Flags:             OPTIMIZED, NEWLOCALS, NESTED
Constants:
   0: None
Names:
   0: print
Variable names:
   0: c
Free variables:
   0: [abedfxyz]
   1: [abedfxyz]
   2: [abedfxyz]
   3: [abedfxyz]
   4: [abedfxyz]
   5: [abedfxyz]"""

code_info_expr_str = """\
Name:              <module>
Filename:          <disassembly>
Argument count:    0
Positional-only arguments: 0
Kw-only arguments: 0
Number of locals:  0
Stack size:        2
Flags:             0x0
Constants:
   0: 1
Names:
   0: x"""

code_info_simple_stmt_str = """\
Name:              <module>
Filename:          <disassembly>
Argument count:    0
Positional-only arguments: 0
Kw-only arguments: 0
Number of locals:  0
Stack size:        2
Flags:             0x0
Constants:
   0: 1
   1: None
Names:
   0: x"""

code_info_compound_stmt_str = """\
Name:              <module>
Filename:          <disassembly>
Argument count:    0
Positional-only arguments: 0
Kw-only arguments: 0
Number of locals:  0
Stack size:        2
Flags:             0x0
Constants:
   0: 0
   1: 1
Names:
   0: x"""


async def async_def():
    await 1
    async for a in b: pass
    async with c as d: pass

code_info_async_def = """\
Name:              async_def
Filename:          (.*)
Argument count:    0
Positional-only arguments: 0
Kw-only arguments: 0
Number of locals:  2
Stack size:        10
Flags:             OPTIMIZED, NEWLOCALS, COROUTINE
Constants:
   0: None
   1: 1
Names:
   0: b
   1: c
Variable names:
   0: a
   1: d"""

class CodeInfoTests(unittest.TestCase):
    test_pairs = [
      (dis.code_info, code_info_code_info),
      (tricky, code_info_tricky),
      (co_tricky_nested_f, code_info_tricky_nested_f),
      (expr_str, code_info_expr_str),
      (simple_stmt_str, code_info_simple_stmt_str),
      (compound_stmt_str, code_info_compound_stmt_str),
      (async_def, code_info_async_def)
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
            output = io.StringIO()
            dis.show_code(x, file=output)
            self.assertRegex(output.getvalue(), expected)

    def test_code_info_object(self):
        self.assertRaises(TypeError, dis.code_info, object())

    def test_pretty_flags_no_flags(self):
        self.assertEqual(dis.pretty_flags(0), '0x0')


# Fodder for instruction introspection tests
#   Editing any of these may require recalculating the expected output
def outer(a=1, b=2):
    def f(c=3, d=4):
        def inner(e=5, f=6):
            print(a, b, c, d, e, f)
        print(a, b, c, d)
        return inner
    print(a, b, '', 1, [], {}, "Hello world!")
    return f

def jumpy():
    # This won't actually run (but that's OK, we only disassemble it)
    for i in range(10):
        print(i)
        if i < 4:
            continue
        if i > 6:
            break
    else:
        print("I can haz else clause?")
    while i:
        print(i)
        i -= 1
        if i > 6:
            continue
        if i < 4:
            break
    else:
        print("Who let lolcatz into this test suite?")
    try:
        1 / 0
    except ZeroDivisionError:
        print("Here we go, here we go, here we go...")
    else:
        with i as dodgy:
            print("Never reach this")
    finally:
        print("OK, now we're done")

# End fodder for opinfo generation tests
expected_outer_line = 1
_line_offset = outer.__code__.co_firstlineno - 1
code_object_f = outer.__code__.co_consts[3]
expected_f_line = code_object_f.co_firstlineno - _line_offset
code_object_inner = code_object_f.co_consts[3]
expected_inner_line = code_object_inner.co_firstlineno - _line_offset
expected_jumpy_line = 1

# The following lines are useful to regenerate the expected results after
# either the fodder is modified or the bytecode generation changes
# After regeneration, update the references to code_object_f and
# code_object_inner before rerunning the tests

def _stringify_instruction(instr):
    # Since line numbers and other offsets change a lot for these
    # test cases, ignore them.
    return str(instr._replace(positions=None))

def _prepare_test_cases():
    _instructions = dis.get_instructions(outer, first_line=expected_outer_line)
    print('expected_opinfo_outer = [\n  ',
          ',\n  '.join(map(_stringify_instruction, _instructions)), ',\n]', sep='')
    _instructions = dis.get_instructions(outer(), first_line=expected_f_line)
    print('expected_opinfo_f = [\n  ',
          ',\n  '.join(map(_stringify_instruction, _instructions)), ',\n]', sep='')
    _instructions = dis.get_instructions(outer()(), first_line=expected_inner_line)
    print('expected_opinfo_inner = [\n  ',
          ',\n  '.join(map(_stringify_instruction, _instructions)), ',\n]', sep='')
    _instructions = dis.get_instructions(jumpy, first_line=expected_jumpy_line)
    print('expected_opinfo_jumpy = [\n  ',
          ',\n  '.join(map(_stringify_instruction, _instructions)), ',\n]', sep='')
    dis.dis(outer)

#_prepare_test_cases()

Instruction = dis.Instruction
expected_opinfo_outer = [
  Instruction(opname='MAKE_CELL', opcode=135, arg=0, argval='a', argrepr='a', offset=0, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='MAKE_CELL', opcode=135, arg=1, argval='b', argrepr='b', offset=2, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='LOAD_CONST', opcode=100, arg=7, argval=(3, 4), argrepr='(3, 4)', offset=4, starts_line=2, is_jump_target=False, positions=None),
  Instruction(opname='LOAD_CLOSURE', opcode=136, arg=0, argval='a', argrepr='a', offset=6, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='LOAD_CLOSURE', opcode=136, arg=1, argval='b', argrepr='b', offset=8, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='BUILD_TUPLE', opcode=102, arg=2, argval=2, argrepr='', offset=10, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='LOAD_CONST', opcode=100, arg=3, argval=code_object_f, argrepr=repr(code_object_f), offset=12, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='MAKE_FUNCTION', opcode=132, arg=9, argval=9, argrepr='defaults, closure', offset=14, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='STORE_FAST', opcode=125, arg=2, argval='f', argrepr='f', offset=16, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='LOAD_GLOBAL', opcode=116, arg=0, argval='print', argrepr='print', offset=18, starts_line=7, is_jump_target=False, positions=None),
  Instruction(opname='LOAD_DEREF', opcode=137, arg=0, argval='a', argrepr='a', offset=20, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='LOAD_DEREF', opcode=137, arg=1, argval='b', argrepr='b', offset=22, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='LOAD_CONST', opcode=100, arg=4, argval='', argrepr="''", offset=24, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='LOAD_CONST', opcode=100, arg=5, argval=1, argrepr='1', offset=26, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='BUILD_LIST', opcode=103, arg=0, argval=0, argrepr='', offset=28, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='BUILD_MAP', opcode=105, arg=0, argval=0, argrepr='', offset=30, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='LOAD_CONST', opcode=100, arg=6, argval='Hello world!', argrepr="'Hello world!'", offset=32, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='CALL_FUNCTION', opcode=131, arg=7, argval=7, argrepr='', offset=34, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='POP_TOP', opcode=1, arg=None, argval=None, argrepr='', offset=36, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='LOAD_FAST', opcode=124, arg=2, argval='f', argrepr='f', offset=38, starts_line=8, is_jump_target=False, positions=None),
  Instruction(opname='RETURN_VALUE', opcode=83, arg=None, argval=None, argrepr='', offset=40, starts_line=None, is_jump_target=False, positions=None),
]

expected_opinfo_f = [
  Instruction(opname='COPY_FREE_VARS', opcode=149, arg=2, argval=2, argrepr='', offset=0, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='MAKE_CELL', opcode=135, arg=0, argval='c', argrepr='c', offset=2, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='MAKE_CELL', opcode=135, arg=1, argval='d', argrepr='d', offset=4, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='LOAD_CONST', opcode=100, arg=4, argval=(5, 6), argrepr='(5, 6)', offset=6, starts_line=3, is_jump_target=False, positions=None),
  Instruction(opname='LOAD_CLOSURE', opcode=136, arg=3, argval='a', argrepr='a', offset=8, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='LOAD_CLOSURE', opcode=136, arg=4, argval='b', argrepr='b', offset=10, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='LOAD_CLOSURE', opcode=136, arg=0, argval='c', argrepr='c', offset=12, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='LOAD_CLOSURE', opcode=136, arg=1, argval='d', argrepr='d', offset=14, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='BUILD_TUPLE', opcode=102, arg=4, argval=4, argrepr='', offset=16, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='LOAD_CONST', opcode=100, arg=3, argval=code_object_inner, argrepr=repr(code_object_inner), offset=18, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='MAKE_FUNCTION', opcode=132, arg=9, argval=9, argrepr='defaults, closure', offset=20, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='STORE_FAST', opcode=125, arg=2, argval='inner', argrepr='inner', offset=22, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='LOAD_GLOBAL', opcode=116, arg=0, argval='print', argrepr='print', offset=24, starts_line=5, is_jump_target=False, positions=None),
  Instruction(opname='LOAD_DEREF', opcode=137, arg=3, argval='a', argrepr='a', offset=26, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='LOAD_DEREF', opcode=137, arg=4, argval='b', argrepr='b', offset=28, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='LOAD_DEREF', opcode=137, arg=0, argval='c', argrepr='c', offset=30, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='LOAD_DEREF', opcode=137, arg=1, argval='d', argrepr='d', offset=32, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='CALL_FUNCTION', opcode=131, arg=4, argval=4, argrepr='', offset=34, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='POP_TOP', opcode=1, arg=None, argval=None, argrepr='', offset=36, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='LOAD_FAST', opcode=124, arg=2, argval='inner', argrepr='inner', offset=38, starts_line=6, is_jump_target=False, positions=None),
  Instruction(opname='RETURN_VALUE', opcode=83, arg=None, argval=None, argrepr='', offset=40, starts_line=None, is_jump_target=False, positions=None),
]

expected_opinfo_inner = [
  Instruction(opname='COPY_FREE_VARS', opcode=149, arg=4, argval=4, argrepr='', offset=0, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='LOAD_GLOBAL', opcode=116, arg=0, argval='print', argrepr='print', offset=2, starts_line=4, is_jump_target=False, positions=None),
  Instruction(opname='LOAD_DEREF', opcode=137, arg=2, argval='a', argrepr='a', offset=4, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='LOAD_DEREF', opcode=137, arg=3, argval='b', argrepr='b', offset=6, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='LOAD_DEREF', opcode=137, arg=4, argval='c', argrepr='c', offset=8, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='LOAD_DEREF', opcode=137, arg=5, argval='d', argrepr='d', offset=10, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='LOAD_FAST', opcode=124, arg=0, argval='e', argrepr='e', offset=12, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='LOAD_FAST', opcode=124, arg=1, argval='f', argrepr='f', offset=14, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='CALL_FUNCTION', opcode=131, arg=6, argval=6, argrepr='', offset=16, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='POP_TOP', opcode=1, arg=None, argval=None, argrepr='', offset=18, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='LOAD_CONST', opcode=100, arg=0, argval=None, argrepr='None', offset=20, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='RETURN_VALUE', opcode=83, arg=None, argval=None, argrepr='', offset=22, starts_line=None, is_jump_target=False, positions=None),
]

expected_opinfo_jumpy = [
  Instruction(opname='LOAD_GLOBAL', opcode=116, arg=0, argval='range', argrepr='range', offset=0, starts_line=3, is_jump_target=False, positions=None),
  Instruction(opname='LOAD_CONST', opcode=100, arg=1, argval=10, argrepr='10', offset=2, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='CALL_FUNCTION', opcode=131, arg=1, argval=1, argrepr='', offset=4, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='GET_ITER', opcode=68, arg=None, argval=None, argrepr='', offset=6, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='FOR_ITER', opcode=93, arg=17, argval=44, argrepr='to 44', offset=8, starts_line=None, is_jump_target=True, positions=None),
  Instruction(opname='STORE_FAST', opcode=125, arg=0, argval='i', argrepr='i', offset=10, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='LOAD_GLOBAL', opcode=116, arg=1, argval='print', argrepr='print', offset=12, starts_line=4, is_jump_target=False, positions=None),
  Instruction(opname='LOAD_FAST', opcode=124, arg=0, argval='i', argrepr='i', offset=14, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='CALL_FUNCTION', opcode=131, arg=1, argval=1, argrepr='', offset=16, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='POP_TOP', opcode=1, arg=None, argval=None, argrepr='', offset=18, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='LOAD_FAST', opcode=124, arg=0, argval='i', argrepr='i', offset=20, starts_line=5, is_jump_target=False, positions=None),
  Instruction(opname='LOAD_CONST', opcode=100, arg=2, argval=4, argrepr='4', offset=22, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='COMPARE_OP', opcode=107, arg=0, argval='<', argrepr='<', offset=24, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='POP_JUMP_IF_FALSE', opcode=114, arg=15, argval=30, argrepr='to 30', offset=26, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='JUMP_ABSOLUTE', opcode=113, arg=4, argval=8, argrepr='to 8', offset=28, starts_line=6, is_jump_target=False, positions=None),
  Instruction(opname='LOAD_FAST', opcode=124, arg=0, argval='i', argrepr='i', offset=30, starts_line=7, is_jump_target=True, positions=None),
  Instruction(opname='LOAD_CONST', opcode=100, arg=3, argval=6, argrepr='6', offset=32, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='COMPARE_OP', opcode=107, arg=4, argval='>', argrepr='>', offset=34, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='POP_JUMP_IF_FALSE', opcode=114, arg=21, argval=42, argrepr='to 42', offset=36, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='POP_TOP', opcode=1, arg=None, argval=None, argrepr='', offset=38, starts_line=8, is_jump_target=False, positions=None),
  Instruction(opname='JUMP_FORWARD', opcode=110, arg=5, argval=52, argrepr='to 52', offset=40, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='JUMP_ABSOLUTE', opcode=113, arg=4, argval=8, argrepr='to 8', offset=42, starts_line=7, is_jump_target=True, positions=None),
  Instruction(opname='LOAD_GLOBAL', opcode=116, arg=1, argval='print', argrepr='print', offset=44, starts_line=10, is_jump_target=True, positions=None),
  Instruction(opname='LOAD_CONST', opcode=100, arg=4, argval='I can haz else clause?', argrepr="'I can haz else clause?'", offset=46, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='CALL_FUNCTION', opcode=131, arg=1, argval=1, argrepr='', offset=48, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='POP_TOP', opcode=1, arg=None, argval=None, argrepr='', offset=50, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='LOAD_FAST', opcode=124, arg=0, argval='i', argrepr='i', offset=52, starts_line=11, is_jump_target=True, positions=None),
  Instruction(opname='POP_JUMP_IF_FALSE', opcode=114, arg=48, argval=96, argrepr='to 96', offset=54, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='LOAD_GLOBAL', opcode=116, arg=1, argval='print', argrepr='print', offset=56, starts_line=12, is_jump_target=True, positions=None),
  Instruction(opname='LOAD_FAST', opcode=124, arg=0, argval='i', argrepr='i', offset=58, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='CALL_FUNCTION', opcode=131, arg=1, argval=1, argrepr='', offset=60, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='POP_TOP', opcode=1, arg=None, argval=None, argrepr='', offset=62, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='LOAD_FAST', opcode=124, arg=0, argval='i', argrepr='i', offset=64, starts_line=13, is_jump_target=False, positions=None),
  Instruction(opname='LOAD_CONST', opcode=100, arg=5, argval=1, argrepr='1', offset=66, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='BINARY_OP', opcode=122, arg=23, argval=23, argrepr='-=', offset=68, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='STORE_FAST', opcode=125, arg=0, argval='i', argrepr='i', offset=70, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='LOAD_FAST', opcode=124, arg=0, argval='i', argrepr='i', offset=72, starts_line=14, is_jump_target=False, positions=None),
  Instruction(opname='LOAD_CONST', opcode=100, arg=3, argval=6, argrepr='6', offset=74, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='COMPARE_OP', opcode=107, arg=4, argval='>', argrepr='>', offset=76, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='POP_JUMP_IF_FALSE', opcode=114, arg=41, argval=82, argrepr='to 82', offset=78, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='JUMP_ABSOLUTE', opcode=113, arg=26, argval=52, argrepr='to 52', offset=80, starts_line=15, is_jump_target=False, positions=None),
  Instruction(opname='LOAD_FAST', opcode=124, arg=0, argval='i', argrepr='i', offset=82, starts_line=16, is_jump_target=True, positions=None),
  Instruction(opname='LOAD_CONST', opcode=100, arg=2, argval=4, argrepr='4', offset=84, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='COMPARE_OP', opcode=107, arg=0, argval='<', argrepr='<', offset=86, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='POP_JUMP_IF_FALSE', opcode=114, arg=46, argval=92, argrepr='to 92', offset=88, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='JUMP_FORWARD', opcode=110, arg=6, argval=104, argrepr='to 104', offset=90, starts_line=17, is_jump_target=False, positions=None),
  Instruction(opname='LOAD_FAST', opcode=124, arg=0, argval='i', argrepr='i', offset=92, starts_line=11, is_jump_target=True, positions=None),
  Instruction(opname='POP_JUMP_IF_TRUE', opcode=115, arg=28, argval=56, argrepr='to 56', offset=94, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='LOAD_GLOBAL', opcode=116, arg=1, argval='print', argrepr='print', offset=96, starts_line=19, is_jump_target=True, positions=None),
  Instruction(opname='LOAD_CONST', opcode=100, arg=6, argval='Who let lolcatz into this test suite?', argrepr="'Who let lolcatz into this test suite?'", offset=98, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='CALL_FUNCTION', opcode=131, arg=1, argval=1, argrepr='', offset=100, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='POP_TOP', opcode=1, arg=None, argval=None, argrepr='', offset=102, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='NOP', opcode=9, arg=None, argval=None, argrepr='', offset=104, starts_line=20, is_jump_target=True, positions=None),
  Instruction(opname='LOAD_CONST', opcode=100, arg=5, argval=1, argrepr='1', offset=106, starts_line=21, is_jump_target=False, positions=None),
  Instruction(opname='LOAD_CONST', opcode=100, arg=7, argval=0, argrepr='0', offset=108, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='BINARY_OP', opcode=122, arg=11, argval=11, argrepr='/', offset=110, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='POP_TOP', opcode=1, arg=None, argval=None, argrepr='', offset=112, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='JUMP_FORWARD', opcode=110, arg=14, argval=144, argrepr='to 144', offset=114, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='PUSH_EXC_INFO', opcode=35, arg=None, argval=None, argrepr='', offset=116, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='LOAD_GLOBAL', opcode=116, arg=2, argval='ZeroDivisionError', argrepr='ZeroDivisionError', offset=118, starts_line=22, is_jump_target=False, positions=None),
  Instruction(opname='JUMP_IF_NOT_EXC_MATCH', opcode=121, arg=70, argval=140, argrepr='to 140', offset=120, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='POP_TOP', opcode=1, arg=None, argval=None, argrepr='', offset=122, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='POP_TOP', opcode=1, arg=None, argval=None, argrepr='', offset=124, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='POP_TOP', opcode=1, arg=None, argval=None, argrepr='', offset=126, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='LOAD_GLOBAL', opcode=116, arg=1, argval='print', argrepr='print', offset=128, starts_line=23, is_jump_target=False, positions=None),
  Instruction(opname='LOAD_CONST', opcode=100, arg=8, argval='Here we go, here we go, here we go...', argrepr="'Here we go, here we go, here we go...'", offset=130, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='CALL_FUNCTION', opcode=131, arg=1, argval=1, argrepr='', offset=132, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='POP_TOP', opcode=1, arg=None, argval=None, argrepr='', offset=134, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='POP_EXCEPT', opcode=89, arg=None, argval=None, argrepr='', offset=136, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='JUMP_FORWARD', opcode=110, arg=33, argval=206, argrepr='to 206', offset=138, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='RERAISE', opcode=119, arg=0, argval=0, argrepr='', offset=140, starts_line=22, is_jump_target=True, positions=None),
  Instruction(opname='POP_EXCEPT_AND_RERAISE', opcode=37, arg=None, argval=None, argrepr='', offset=142, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='LOAD_FAST', opcode=124, arg=0, argval='i', argrepr='i', offset=144, starts_line=25, is_jump_target=True, positions=None),
  Instruction(opname='BEFORE_WITH', opcode=53, arg=None, argval=None, argrepr='', offset=146, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='STORE_FAST', opcode=125, arg=1, argval='dodgy', argrepr='dodgy', offset=148, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='LOAD_GLOBAL', opcode=116, arg=1, argval='print', argrepr='print', offset=150, starts_line=26, is_jump_target=False, positions=None),
  Instruction(opname='LOAD_CONST', opcode=100, arg=9, argval='Never reach this', argrepr="'Never reach this'", offset=152, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='CALL_FUNCTION', opcode=131, arg=1, argval=1, argrepr='', offset=154, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='POP_TOP', opcode=1, arg=None, argval=None, argrepr='', offset=156, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='LOAD_CONST', opcode=100, arg=0, argval=None, argrepr='None', offset=158, starts_line=25, is_jump_target=False, positions=None),
  Instruction(opname='DUP_TOP', opcode=4, arg=None, argval=None, argrepr='', offset=160, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='DUP_TOP', opcode=4, arg=None, argval=None, argrepr='', offset=162, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='CALL_FUNCTION', opcode=131, arg=3, argval=3, argrepr='', offset=164, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='POP_TOP', opcode=1, arg=None, argval=None, argrepr='', offset=166, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='JUMP_FORWARD', opcode=110, arg=25, argval=220, argrepr='to 220', offset=168, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='PUSH_EXC_INFO', opcode=35, arg=None, argval=None, argrepr='', offset=170, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='WITH_EXCEPT_START', opcode=49, arg=None, argval=None, argrepr='', offset=172, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='POP_JUMP_IF_TRUE', opcode=115, arg=90, argval=180, argrepr='to 180', offset=174, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='RERAISE', opcode=119, arg=4, argval=4, argrepr='', offset=176, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='POP_EXCEPT_AND_RERAISE', opcode=37, arg=None, argval=None, argrepr='', offset=178, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='POP_TOP', opcode=1, arg=None, argval=None, argrepr='', offset=180, starts_line=None, is_jump_target=True, positions=None),
  Instruction(opname='POP_TOP', opcode=1, arg=None, argval=None, argrepr='', offset=182, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='POP_TOP', opcode=1, arg=None, argval=None, argrepr='', offset=184, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='POP_EXCEPT', opcode=89, arg=None, argval=None, argrepr='', offset=186, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='POP_TOP', opcode=1, arg=None, argval=None, argrepr='', offset=188, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='POP_TOP', opcode=1, arg=None, argval=None, argrepr='', offset=190, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='NOP', opcode=9, arg=None, argval=None, argrepr='', offset=192, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='LOAD_GLOBAL', opcode=116, arg=1, argval='print', argrepr='print', offset=194, starts_line=28, is_jump_target=False, positions=None),
  Instruction(opname='LOAD_CONST', opcode=100, arg=10, argval="OK, now we're done", argrepr='"OK, now we\'re done"', offset=196, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='CALL_FUNCTION', opcode=131, arg=1, argval=1, argrepr='', offset=198, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='POP_TOP', opcode=1, arg=None, argval=None, argrepr='', offset=200, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='LOAD_CONST', opcode=100, arg=0, argval=None, argrepr='None', offset=202, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='RETURN_VALUE', opcode=83, arg=None, argval=None, argrepr='', offset=204, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='NOP', opcode=9, arg=None, argval=None, argrepr='', offset=206, starts_line=23, is_jump_target=True, positions=None),
  Instruction(opname='LOAD_GLOBAL', opcode=116, arg=1, argval='print', argrepr='print', offset=208, starts_line=28, is_jump_target=False, positions=None),
  Instruction(opname='LOAD_CONST', opcode=100, arg=10, argval="OK, now we're done", argrepr='"OK, now we\'re done"', offset=210, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='CALL_FUNCTION', opcode=131, arg=1, argval=1, argrepr='', offset=212, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='POP_TOP', opcode=1, arg=None, argval=None, argrepr='', offset=214, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='LOAD_CONST', opcode=100, arg=0, argval=None, argrepr='None', offset=216, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='RETURN_VALUE', opcode=83, arg=None, argval=None, argrepr='', offset=218, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='NOP', opcode=9, arg=None, argval=None, argrepr='', offset=220, starts_line=25, is_jump_target=True, positions=None),
  Instruction(opname='LOAD_GLOBAL', opcode=116, arg=1, argval='print', argrepr='print', offset=222, starts_line=28, is_jump_target=False, positions=None),
  Instruction(opname='LOAD_CONST', opcode=100, arg=10, argval="OK, now we're done", argrepr='"OK, now we\'re done"', offset=224, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='CALL_FUNCTION', opcode=131, arg=1, argval=1, argrepr='', offset=226, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='POP_TOP', opcode=1, arg=None, argval=None, argrepr='', offset=228, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='LOAD_CONST', opcode=100, arg=0, argval=None, argrepr='None', offset=230, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='RETURN_VALUE', opcode=83, arg=None, argval=None, argrepr='', offset=232, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='PUSH_EXC_INFO', opcode=35, arg=None, argval=None, argrepr='', offset=234, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='LOAD_GLOBAL', opcode=116, arg=1, argval='print', argrepr='print', offset=236, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='LOAD_CONST', opcode=100, arg=10, argval="OK, now we're done", argrepr='"OK, now we\'re done"', offset=238, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='CALL_FUNCTION', opcode=131, arg=1, argval=1, argrepr='', offset=240, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='POP_TOP', opcode=1, arg=None, argval=None, argrepr='', offset=242, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='RERAISE', opcode=119, arg=0, argval=0, argrepr='', offset=244, starts_line=None, is_jump_target=False, positions=None),
  Instruction(opname='POP_EXCEPT_AND_RERAISE', opcode=37, arg=None, argval=None, argrepr='', offset=246, starts_line=None, is_jump_target=False, positions=None),
]

# One last piece of inspect fodder to check the default line number handling
def simple(): pass
expected_opinfo_simple = [
  Instruction(opname='LOAD_CONST', opcode=100, arg=0, argval=None, argrepr='None', offset=0, starts_line=simple.__code__.co_firstlineno, is_jump_target=False),
  Instruction(opname='RETURN_VALUE', opcode=83, arg=None, argval=None, argrepr='', offset=2, starts_line=None, is_jump_target=False)
]


class InstructionTestCase(BytecodeTestCase):

    def assertInstructionsEqual(self, instrs_1, instrs_2, /):
        instrs_1 = [instr_1._replace(positions=None) for instr_1 in instrs_1]
        instrs_2 = [instr_2._replace(positions=None) for instr_2 in instrs_2]
        self.assertEqual(instrs_1, instrs_2)

class InstructionTests(InstructionTestCase):

    def __init__(self, *args):
        super().__init__(*args)
        self.maxDiff = None

    def test_default_first_line(self):
        actual = dis.get_instructions(simple)
        self.assertInstructionsEqual(list(actual), expected_opinfo_simple)

    def test_first_line_set_to_None(self):
        actual = dis.get_instructions(simple, first_line=None)
        self.assertInstructionsEqual(list(actual), expected_opinfo_simple)

    def test_outer(self):
        actual = dis.get_instructions(outer, first_line=expected_outer_line)
        self.assertInstructionsEqual(list(actual), expected_opinfo_outer)

    def test_nested(self):
        with captured_stdout():
            f = outer()
        actual = dis.get_instructions(f, first_line=expected_f_line)
        self.assertInstructionsEqual(list(actual), expected_opinfo_f)

    def test_doubly_nested(self):
        with captured_stdout():
            inner = outer()()
        actual = dis.get_instructions(inner, first_line=expected_inner_line)
        self.assertInstructionsEqual(list(actual), expected_opinfo_inner)

    def test_jumpy(self):
        actual = dis.get_instructions(jumpy, first_line=expected_jumpy_line)
        self.assertInstructionsEqual(list(actual), expected_opinfo_jumpy)

    @requires_debug_ranges()
    def test_co_positions(self):
        code = compile('f(\n  x, y, z\n)', '<test>', 'exec')
        positions = [
            instr.positions
            for instr in dis.get_instructions(code)
        ]
        expected = [
            (1, 1, 0, 1),
            (2, 2, 2, 3),
            (2, 2, 5, 6),
            (2, 2, 8, 9),
            (1, 3, 0, 1),
            (1, 3, 0, 1),
            (1, 3, 0, 1),
            (1, 3, 0, 1)
        ]
        self.assertEqual(positions, expected)

    @requires_debug_ranges()
    def test_co_positions_missing_info(self):
        code = compile('x, y, z', '<test>', 'exec')
        code_without_column_table = code.replace(co_columntable=b'')
        actual = dis.get_instructions(code_without_column_table)
        for instruction in actual:
            with self.subTest(instruction=instruction):
                start_line, end_line, start_offset, end_offset = instruction.positions
                assert start_line == 1
                assert end_line == 1
                assert start_offset is None
                assert end_offset is None

        code_without_endline_table = code.replace(co_endlinetable=b'')
        actual = dis.get_instructions(code_without_endline_table)
        for instruction in actual:
            with self.subTest(instruction=instruction):
                start_line, end_line, start_offset, end_offset = instruction.positions
                assert start_line == 1
                assert end_line is None
                assert start_offset is not None
                assert end_offset is not None

# get_instructions has its own tests above, so can rely on it to validate
# the object oriented API
class BytecodeTests(InstructionTestCase):

    def test_instantiation(self):
        # Test with function, method, code string and code object
        for obj in [_f, _C(1).__init__, "a=1", _f.__code__]:
            with self.subTest(obj=obj):
                b = dis.Bytecode(obj)
                self.assertIsInstance(b.codeobj, types.CodeType)

        self.assertRaises(TypeError, dis.Bytecode, object())

    def test_iteration(self):
        for obj in [_f, _C(1).__init__, "a=1", _f.__code__]:
            with self.subTest(obj=obj):
                via_object = list(dis.Bytecode(obj))
                via_generator = list(dis.get_instructions(obj))
                self.assertInstructionsEqual(via_object, via_generator)

    def test_explicit_first_line(self):
        actual = dis.Bytecode(outer, first_line=expected_outer_line)
        self.assertInstructionsEqual(list(actual), expected_opinfo_outer)

    def test_source_line_in_disassembly(self):
        # Use the line in the source code
        actual = dis.Bytecode(simple).dis()
        actual = actual.strip().partition(" ")[0]  # extract the line no
        expected = str(simple.__code__.co_firstlineno)
        self.assertEqual(actual, expected)
        # Use an explicit first line number
        actual = dis.Bytecode(simple, first_line=350).dis()
        actual = actual.strip().partition(" ")[0]  # extract the line no
        self.assertEqual(actual, "350")

    def test_info(self):
        self.maxDiff = 1000
        for x, expected in CodeInfoTests.test_pairs:
            b = dis.Bytecode(x)
            self.assertRegex(b.info(), expected)

    def test_disassembled(self):
        actual = dis.Bytecode(_f).dis()
        self.assertEqual(actual, dis_f)

    def test_from_traceback(self):
        tb = get_tb()
        b = dis.Bytecode.from_traceback(tb)
        while tb.tb_next: tb = tb.tb_next

        self.assertEqual(b.current_offset, tb.tb_lasti)

    def test_from_traceback_dis(self):
        self.maxDiff = None
        tb = get_tb()
        b = dis.Bytecode.from_traceback(tb)
        self.assertEqual(b.dis(), dis_traceback)

    @requires_debug_ranges()
    def test_bytecode_co_positions(self):
        bytecode = dis.Bytecode("a=1")
        for instr, positions in zip(bytecode, bytecode.codeobj.co_positions()):
            assert instr.positions == positions

class TestBytecodeTestCase(BytecodeTestCase):
    def test_assert_not_in_with_op_not_in_bytecode(self):
        code = compile("a = 1", "<string>", "exec")
        self.assertInBytecode(code, "LOAD_CONST", 1)
        self.assertNotInBytecode(code, "LOAD_NAME")
        self.assertNotInBytecode(code, "LOAD_NAME", "a")

    def test_assert_not_in_with_arg_not_in_bytecode(self):
        code = compile("a = 1", "<string>", "exec")
        self.assertInBytecode(code, "LOAD_CONST")
        self.assertInBytecode(code, "LOAD_CONST", 1)
        self.assertNotInBytecode(code, "LOAD_CONST", 2)

    def test_assert_not_in_with_arg_in_bytecode(self):
        code = compile("a = 1", "<string>", "exec")
        with self.assertRaises(AssertionError):
            self.assertNotInBytecode(code, "LOAD_CONST", 1)

class TestFinderMethods(unittest.TestCase):
    def test__find_imports(self):
        cases = [
            ("import a.b.c", ('a.b.c', 0, None)),
            ("from a.b import c", ('a.b', 0, ('c',))),
            ("from a.b import c as d", ('a.b', 0, ('c',))),
            ("from a.b import *", ('a.b', 0, ('*',))),
            ("from ...a.b import c as d", ('a.b', 3, ('c',))),
            ("from ..a.b import c as d, e as f", ('a.b', 2, ('c', 'e'))),
            ("from ..a.b import *", ('a.b', 2, ('*',))),
        ]
        for src, expected in cases:
            with self.subTest(src=src):
                code = compile(src, "<string>", "exec")
                res = tuple(dis._find_imports(code))
                self.assertEqual(len(res), 1)
                self.assertEqual(res[0], expected)

    def test__find_store_names(self):
        cases = [
            ("x+y", ()),
            ("x=y=1", ('x', 'y')),
            ("x+=y", ('x',)),
            ("global x\nx=y=1", ('x', 'y')),
            ("global x\nz=x", ('z',)),
        ]
        for src, expected in cases:
            with self.subTest(src=src):
                code = compile(src, "<string>", "exec")
                res = tuple(dis._find_store_names(code))
                self.assertEqual(res, expected)


class TestDisTraceback(unittest.TestCase):
    def setUp(self) -> None:
        try:  # We need to clean up existing tracebacks
            del sys.last_traceback
        except AttributeError:
            pass
        return super().setUp()

    def get_disassembly(self, tb):
        output = io.StringIO()
        with contextlib.redirect_stdout(output):
            dis.distb(tb)
        return output.getvalue()

    def test_distb_empty(self):
        with self.assertRaises(RuntimeError):
            dis.distb()

    def test_distb_last_traceback(self):
        # We need to have an existing last traceback in `sys`:
        tb = get_tb()
        sys.last_traceback = tb

        self.assertEqual(self.get_disassembly(None), dis_traceback)

    def test_distb_explicit_arg(self):
        tb = get_tb()

        self.assertEqual(self.get_disassembly(tb), dis_traceback)


class TestDisTracebackWithFile(TestDisTraceback):
    # Run the `distb` tests again, using the file arg instead of print
    def get_disassembly(self, tb):
        output = io.StringIO()
        with contextlib.redirect_stdout(output):
            dis.distb(tb, file=output)
        return output.getvalue()


if __name__ == "__main__":
    unittest.main()
