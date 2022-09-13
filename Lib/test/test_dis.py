# Minimal tests for dis module

import contextlib
import dis
import io
import re
import sys
import types
import unittest
from test.support import captured_stdout, requires_debug_ranges, cpython_only
from test.support.bytecode_helper import BytecodeTestCase

import opcode


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
%3d        RESUME                   0

%3d        LOAD_FAST                1 (x)
           LOAD_CONST               1 (1)
           COMPARE_OP               2 (==)
           LOAD_FAST                0 (self)
           STORE_ATTR               0 (x)
           LOAD_CONST               0 (None)
           RETURN_VALUE
""" % (_C.__init__.__code__.co_firstlineno, _C.__init__.__code__.co_firstlineno + 1,)

dis_c_instance_method_bytes = """\
       RESUME                   0
       LOAD_FAST                1
       LOAD_CONST               1
       COMPARE_OP               2 (==)
       LOAD_FAST                0
       STORE_ATTR               0
       LOAD_CONST               0
       RETURN_VALUE
"""

dis_c_class_method = """\
%3d        RESUME                   0

%3d        LOAD_FAST                1 (x)
           LOAD_CONST               1 (1)
           COMPARE_OP               2 (==)
           LOAD_FAST                0 (cls)
           STORE_ATTR               0 (x)
           LOAD_CONST               0 (None)
           RETURN_VALUE
""" % (_C.cm.__code__.co_firstlineno, _C.cm.__code__.co_firstlineno + 2,)

dis_c_static_method = """\
%3d        RESUME                   0

%3d        LOAD_FAST                0 (x)
           LOAD_CONST               1 (1)
           COMPARE_OP               2 (==)
           STORE_FAST               0 (x)
           LOAD_CONST               0 (None)
           RETURN_VALUE
""" % (_C.sm.__code__.co_firstlineno, _C.sm.__code__.co_firstlineno + 2,)

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
%3d        RESUME                   0

%3d        LOAD_GLOBAL              1 (NULL + print)
           LOAD_FAST                0 (a)
           CALL                     1
           POP_TOP

%3d        LOAD_CONST               1 (1)
           RETURN_VALUE
""" % (_f.__code__.co_firstlineno,
       _f.__code__.co_firstlineno + 1,
       _f.__code__.co_firstlineno + 2)


dis_f_co_code = """\
       RESUME                   0
       LOAD_GLOBAL              1
       LOAD_FAST                0
       CALL                     1
       POP_TOP
       LOAD_CONST               1
       RETURN_VALUE
"""


def bug708901():
    for res in range(1,
                     10):
        pass

dis_bug708901 = """\
%3d        RESUME                   0

%3d        LOAD_GLOBAL              1 (NULL + range)
           LOAD_CONST               1 (1)

%3d        LOAD_CONST               2 (10)

%3d        CALL                     2
           GET_ITER
        >> FOR_ITER                 2 (to 38)
           STORE_FAST               0 (res)

%3d        JUMP_BACKWARD            4 (to 30)

%3d     >> LOAD_CONST               0 (None)
           RETURN_VALUE
""" % (bug708901.__code__.co_firstlineno,
       bug708901.__code__.co_firstlineno + 1,
       bug708901.__code__.co_firstlineno + 2,
       bug708901.__code__.co_firstlineno + 1,
       bug708901.__code__.co_firstlineno + 3,
       bug708901.__code__.co_firstlineno + 1)


def bug1333982(x=[]):
    assert 0, ([s for s in x] +
              1)
    pass

dis_bug1333982 = """\
%3d        RESUME                   0

%3d        LOAD_ASSERTION_ERROR
           LOAD_CONST               2 (<code object <listcomp> at 0x..., file "%s", line %d>)
           MAKE_FUNCTION            0
           LOAD_FAST                0 (x)
           GET_ITER
           CALL                     0

%3d        LOAD_CONST               3 (1)

%3d        BINARY_OP                0 (+)
           CALL                     0
           RAISE_VARARGS            1
""" % (bug1333982.__code__.co_firstlineno,
       bug1333982.__code__.co_firstlineno + 1,
       __file__,
       bug1333982.__code__.co_firstlineno + 1,
       bug1333982.__code__.co_firstlineno + 2,
       bug1333982.__code__.co_firstlineno + 1)


def bug42562():
    pass


# Set line number for 'pass' to None
bug42562.__code__ = bug42562.__code__.replace(co_linetable=b'\xf8')


dis_bug42562 = """\
       RESUME                   0
       LOAD_CONST               0 (None)
       RETURN_VALUE
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
       EXTENDED_ARG             1
       NOP
       EXTENDED_ARG             1
       LOAD_CONST             297
       RETURN_VALUE
"""

# [255, 255, 255, 252] is -4 in a 4 byte signed integer
bug46724 = bytes([
    opcode.EXTENDED_ARG, 255,
    opcode.EXTENDED_ARG, 255,
    opcode.EXTENDED_ARG, 255,
    opcode.opmap['JUMP_FORWARD'], 252,
])


dis_bug46724 = """\
    >> EXTENDED_ARG           255
       EXTENDED_ARG         65535
       EXTENDED_ARG         16777215
       JUMP_FORWARD            -4 (to 0)
"""

_BIG_LINENO_FORMAT = """\
  1        RESUME                   0

%3d        LOAD_GLOBAL              0 (spam)
           POP_TOP
           LOAD_CONST               0 (None)
           RETURN_VALUE
"""

_BIG_LINENO_FORMAT2 = """\
   1        RESUME                   0

%4d        LOAD_GLOBAL              0 (spam)
            POP_TOP
            LOAD_CONST               0 (None)
            RETURN_VALUE
"""

dis_module_expected_results = """\
Disassembly of f:
  4        RESUME                   0
           LOAD_CONST               0 (None)
           RETURN_VALUE

Disassembly of g:
  5        RESUME                   0
           LOAD_CONST               0 (None)
           RETURN_VALUE

"""

expr_str = "x + 1"

dis_expr_str = """\
  0        RESUME                   0

  1        LOAD_NAME                0 (x)
           LOAD_CONST               0 (1)
           BINARY_OP                0 (+)
           RETURN_VALUE
"""

simple_stmt_str = "x = x + 1"

dis_simple_stmt_str = """\
  0        RESUME                   0

  1        LOAD_NAME                0 (x)
           LOAD_CONST               0 (1)
           BINARY_OP                0 (+)
           STORE_NAME               0 (x)
           LOAD_CONST               1 (None)
           RETURN_VALUE
"""

annot_stmt_str = """\

x: int = 1
y: fun(1)
lst[fun(0)]: int = 1
"""
# leading newline is for a reason (tests lineno)

dis_annot_stmt_str = """\
  0        RESUME                   0

  2        SETUP_ANNOTATIONS
           LOAD_CONST               0 (1)
           STORE_NAME               0 (x)
           LOAD_NAME                1 (int)
           LOAD_NAME                2 (__annotations__)
           LOAD_CONST               1 ('x')
           STORE_SUBSCR

  3        PUSH_NULL
           LOAD_NAME                3 (fun)
           LOAD_CONST               0 (1)
           CALL                     1
           LOAD_NAME                2 (__annotations__)
           LOAD_CONST               2 ('y')
           STORE_SUBSCR

  4        LOAD_CONST               0 (1)
           LOAD_NAME                4 (lst)
           PUSH_NULL
           LOAD_NAME                3 (fun)
           LOAD_CONST               3 (0)
           CALL                     1
           STORE_SUBSCR
           LOAD_NAME                1 (int)
           POP_TOP
           LOAD_CONST               4 (None)
           RETURN_VALUE
"""

compound_stmt_str = """\
x = 0
while 1:
    x += 1"""
# Trailing newline has been deliberately omitted

dis_compound_stmt_str = """\
  0        RESUME                   0

  1        LOAD_CONST               0 (0)
           STORE_NAME               0 (x)

  2        NOP

  3     >> LOAD_NAME                0 (x)
           LOAD_CONST               1 (1)
           BINARY_OP               13 (+=)
           STORE_NAME               0 (x)

  2        JUMP_BACKWARD            6 (to 8)
"""

dis_traceback = """\
%3d        RESUME                   0

%3d        NOP

%3d        LOAD_CONST               1 (1)
           LOAD_CONST               2 (0)
    -->    BINARY_OP               11 (/)
           POP_TOP

%3d        LOAD_FAST_CHECK          1 (tb)
           RETURN_VALUE
        >> PUSH_EXC_INFO

%3d        LOAD_GLOBAL              0 (Exception)
           CHECK_EXC_MATCH
           POP_JUMP_IF_FALSE       23 (to 82)
           STORE_FAST               0 (e)

%3d        LOAD_FAST                0 (e)
           LOAD_ATTR                2 (__traceback__)
           STORE_FAST               1 (tb)
           POP_EXCEPT
           LOAD_CONST               0 (None)
           STORE_FAST               0 (e)
           DELETE_FAST              0 (e)

%3d        LOAD_FAST                1 (tb)
           RETURN_VALUE
        >> LOAD_CONST               0 (None)
           STORE_FAST               0 (e)
           DELETE_FAST              0 (e)
           RERAISE                  1

%3d     >> RERAISE                  0
        >> COPY                     3
           POP_EXCEPT
           RERAISE                  1
ExceptionTable:
4 rows
""" % (TRACEBACK_CODE.co_firstlineno,
       TRACEBACK_CODE.co_firstlineno + 1,
       TRACEBACK_CODE.co_firstlineno + 2,
       TRACEBACK_CODE.co_firstlineno + 5,
       TRACEBACK_CODE.co_firstlineno + 3,
       TRACEBACK_CODE.co_firstlineno + 4,
       TRACEBACK_CODE.co_firstlineno + 5,
       TRACEBACK_CODE.co_firstlineno + 3)

def _fstring(a, b, c, d):
    return f'{a} {b:4} {c!r} {d!r:4}'

dis_fstring = """\
%3d        RESUME                   0

%3d        LOAD_FAST                0 (a)
           FORMAT_VALUE             0
           LOAD_CONST               1 (' ')
           LOAD_FAST                1 (b)
           LOAD_CONST               2 ('4')
           FORMAT_VALUE             4 (with format)
           LOAD_CONST               1 (' ')
           LOAD_FAST                2 (c)
           FORMAT_VALUE             2 (repr)
           LOAD_CONST               1 (' ')
           LOAD_FAST                3 (d)
           LOAD_CONST               2 ('4')
           FORMAT_VALUE             6 (repr, with format)
           BUILD_STRING             7
           RETURN_VALUE
""" % (_fstring.__code__.co_firstlineno, _fstring.__code__.co_firstlineno + 1)

def _with(c):
    with c:
        x = 1
    y = 2

dis_with = """\
%3d        RESUME                   0

%3d        LOAD_FAST                0 (c)
           BEFORE_WITH
           POP_TOP

%3d        LOAD_CONST               1 (1)
           STORE_FAST               1 (x)

%3d        LOAD_CONST               0 (None)
           LOAD_CONST               0 (None)
           LOAD_CONST               0 (None)
           CALL                     2
           POP_TOP

%3d        LOAD_CONST               2 (2)
           STORE_FAST               2 (y)
           LOAD_CONST               0 (None)
           RETURN_VALUE

%3d     >> PUSH_EXC_INFO
           WITH_EXCEPT_START
           POP_JUMP_IF_TRUE         1 (to 46)
           RERAISE                  2
        >> POP_TOP
           POP_EXCEPT
           POP_TOP
           POP_TOP

%3d        LOAD_CONST               2 (2)
           STORE_FAST               2 (y)
           LOAD_CONST               0 (None)
           RETURN_VALUE
        >> COPY                     3
           POP_EXCEPT
           RERAISE                  1
ExceptionTable:
2 rows
""" % (_with.__code__.co_firstlineno,
       _with.__code__.co_firstlineno + 1,
       _with.__code__.co_firstlineno + 2,
       _with.__code__.co_firstlineno + 1,
       _with.__code__.co_firstlineno + 3,
       _with.__code__.co_firstlineno + 1,
       _with.__code__.co_firstlineno + 3,
       )

async def _asyncwith(c):
    async with c:
        x = 1
    y = 2

dis_asyncwith = """\
%3d        RETURN_GENERATOR
           POP_TOP
           RESUME                   0

%3d        LOAD_FAST                0 (c)
           BEFORE_ASYNC_WITH
           GET_AWAITABLE            1
           LOAD_CONST               0 (None)
        >> SEND                     3 (to 22)
           YIELD_VALUE              3
           RESUME                   3
           JUMP_BACKWARD_NO_INTERRUPT     4 (to 14)
        >> POP_TOP

%3d        LOAD_CONST               1 (1)
           STORE_FAST               1 (x)

%3d        LOAD_CONST               0 (None)
           LOAD_CONST               0 (None)
           LOAD_CONST               0 (None)
           CALL                     2
           GET_AWAITABLE            2
           LOAD_CONST               0 (None)
        >> SEND                     3 (to 56)
           YIELD_VALUE              2
           RESUME                   3
           JUMP_BACKWARD_NO_INTERRUPT     4 (to 48)
        >> POP_TOP

%3d        LOAD_CONST               2 (2)
           STORE_FAST               2 (y)
           LOAD_CONST               0 (None)
           RETURN_VALUE

%3d     >> CLEANUP_THROW
           JUMP_BACKWARD           24 (to 22)
        >> CLEANUP_THROW
           JUMP_BACKWARD            9 (to 56)
        >> PUSH_EXC_INFO
           WITH_EXCEPT_START
           GET_AWAITABLE            2
           LOAD_CONST               0 (None)
        >> SEND                     4 (to 92)
           YIELD_VALUE              6
           RESUME                   3
           JUMP_BACKWARD_NO_INTERRUPT     4 (to 82)
        >> CLEANUP_THROW
        >> POP_JUMP_IF_TRUE         1 (to 96)
           RERAISE                  2
        >> POP_TOP
           POP_EXCEPT
           POP_TOP
           POP_TOP

%3d        LOAD_CONST               2 (2)
           STORE_FAST               2 (y)
           LOAD_CONST               0 (None)
           RETURN_VALUE
        >> COPY                     3
           POP_EXCEPT
           RERAISE                  1
ExceptionTable:
6 rows
""" % (_asyncwith.__code__.co_firstlineno,
       _asyncwith.__code__.co_firstlineno + 1,
       _asyncwith.__code__.co_firstlineno + 2,
       _asyncwith.__code__.co_firstlineno + 1,
       _asyncwith.__code__.co_firstlineno + 3,
       _asyncwith.__code__.co_firstlineno + 1,
       _asyncwith.__code__.co_firstlineno + 3,
       )


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
%3d        RESUME                   0

%3d        NOP

%3d        LOAD_FAST                0 (a)

%3d        PUSH_NULL
           LOAD_FAST                1 (b)
           CALL                     0
           POP_TOP
           RETURN_VALUE
        >> PUSH_EXC_INFO
           PUSH_NULL
           LOAD_FAST                1 (b)
           CALL                     0
           POP_TOP
           RERAISE                  0
        >> COPY                     3
           POP_EXCEPT
           RERAISE                  1
ExceptionTable:
2 rows
""" % (_tryfinally.__code__.co_firstlineno,
       _tryfinally.__code__.co_firstlineno + 1,
       _tryfinally.__code__.co_firstlineno + 2,
       _tryfinally.__code__.co_firstlineno + 4,
       )

dis_tryfinallyconst = """\
%3d        RESUME                   0

%3d        NOP

%3d        NOP

%3d        PUSH_NULL
           LOAD_FAST                0 (b)
           CALL                     0
           POP_TOP
           LOAD_CONST               1 (1)
           RETURN_VALUE
           PUSH_EXC_INFO
           PUSH_NULL
           LOAD_FAST                0 (b)
           CALL                     0
           POP_TOP
           RERAISE                  0
        >> COPY                     3
           POP_EXCEPT
           RERAISE                  1
ExceptionTable:
1 row
""" % (_tryfinallyconst.__code__.co_firstlineno,
       _tryfinallyconst.__code__.co_firstlineno + 1,
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
           MAKE_CELL                0 (y)

%3d        RESUME                   0

%3d        LOAD_CLOSURE             0 (y)
           BUILD_TUPLE              1
           LOAD_CONST               1 (<code object foo at 0x..., file "%s", line %d>)
           MAKE_FUNCTION            8 (closure)
           STORE_FAST               1 (foo)

%3d        LOAD_FAST                1 (foo)
           RETURN_VALUE
""" % (_h.__code__.co_firstlineno,
       _h.__code__.co_firstlineno + 1,
       __file__,
       _h.__code__.co_firstlineno + 1,
       _h.__code__.co_firstlineno + 4,
)

dis_nested_1 = """%s
Disassembly of <code object foo at 0x..., file "%s", line %d>:
           COPY_FREE_VARS           1
           MAKE_CELL                0 (x)

%3d        RESUME                   0

%3d        LOAD_CLOSURE             0 (x)
           BUILD_TUPLE              1
           LOAD_CONST               1 (<code object <listcomp> at 0x..., file "%s", line %d>)
           MAKE_FUNCTION            8 (closure)
           LOAD_DEREF               1 (y)
           GET_ITER
           CALL                     0
           RETURN_VALUE
""" % (dis_nested_0,
       __file__,
       _h.__code__.co_firstlineno + 1,
       _h.__code__.co_firstlineno + 1,
       _h.__code__.co_firstlineno + 3,
       __file__,
       _h.__code__.co_firstlineno + 3,
)

dis_nested_2 = """%s
Disassembly of <code object <listcomp> at 0x..., file "%s", line %d>:
           COPY_FREE_VARS           1

%3d        RESUME                   0
           BUILD_LIST               0
           LOAD_FAST                0 (.0)
        >> FOR_ITER                 7 (to 26)
           STORE_FAST               1 (z)
           LOAD_DEREF               2 (x)
           LOAD_FAST                1 (z)
           BINARY_OP                0 (+)
           LIST_APPEND              2
           JUMP_BACKWARD            9 (to 8)
        >> RETURN_VALUE
""" % (dis_nested_1,
       __file__,
       _h.__code__.co_firstlineno + 3,
       _h.__code__.co_firstlineno + 3,
)

def load_test(x, y=0):
    a, b = x, y
    return a, b

dis_load_test_quickened_code = """\
%3d           0 RESUME_QUICK             0

%3d           2 LOAD_FAST__LOAD_FAST     0 (x)
              4 LOAD_FAST                1 (y)
              6 STORE_FAST__STORE_FAST     3 (b)
              8 STORE_FAST__LOAD_FAST     2 (a)

%3d          10 LOAD_FAST__LOAD_FAST     2 (a)
             12 LOAD_FAST                3 (b)
             14 BUILD_TUPLE              2
             16 RETURN_VALUE
""" % (load_test.__code__.co_firstlineno,
       load_test.__code__.co_firstlineno + 1,
       load_test.__code__.co_firstlineno + 2)

def loop_test():
    for i in [1, 2, 3] * 3:
        load_test(i)

dis_loop_test_quickened_code = """\
%3d        RESUME_QUICK             0

%3d        BUILD_LIST               0
           LOAD_CONST               1 ((1, 2, 3))
           LIST_EXTEND              1
           LOAD_CONST               2 (3)
           BINARY_OP_ADAPTIVE       5 (*)
           GET_ITER
        >> FOR_ITER_LIST           15 (to 50)
           STORE_FAST               0 (i)

%3d        LOAD_GLOBAL_MODULE       1 (NULL + load_test)
           LOAD_FAST                0 (i)
           CALL_PY_WITH_DEFAULTS     1
           POP_TOP
           JUMP_BACKWARD_QUICK     17 (to 16)

%3d     >> LOAD_CONST               0 (None)
           RETURN_VALUE
""" % (loop_test.__code__.co_firstlineno,
       loop_test.__code__.co_firstlineno + 1,
       loop_test.__code__.co_firstlineno + 2,
       loop_test.__code__.co_firstlineno + 1,)

def extended_arg_quick():
    *_, _ = ...

dis_extended_arg_quick_code = """\
%3d           0 RESUME                   0

%3d           2 LOAD_CONST               1 (Ellipsis)
              4 EXTENDED_ARG             1
              6 UNPACK_EX              256
              8 STORE_FAST               0 (_)
             10 STORE_FAST               0 (_)
             12 LOAD_CONST               0 (None)
             14 RETURN_VALUE
"""% (extended_arg_quick.__code__.co_firstlineno,
      extended_arg_quick.__code__.co_firstlineno + 1,)

QUICKENING_WARMUP_DELAY = 8

class DisTestBase(unittest.TestCase):
    "Common utilities for DisTests and TestDisTraceback"

    def strip_addresses(self, text):
        return re.sub(r'\b0x[0-9A-Fa-f]+\b', '0x...', text)

    def find_offset_column(self, lines):
        for line in lines:
            if line and not line.startswith("Disassembly"):
                break
        else:
            return 0, 0
        offset = 5
        while (line[offset] == " "):
            offset += 1
        if (line[offset] == ">"):
            offset += 2
        while (line[offset] == " "):
            offset += 1
        end = offset
        while line[end] in "0123456789":
            end += 1
        return end-5, end

    def assert_offsets_increasing(self, text, delta):
        expected_offset = 0
        lines = text.splitlines()
        start, end = self.find_offset_column(lines)
        for line in lines:
            if not line:
                continue
            if line.startswith("Disassembly"):
                expected_offset = 0
                continue
            if line.startswith("Exception"):
                break
            offset = int(line[start:end])
            self.assertGreaterEqual(offset, expected_offset, line)
            expected_offset = offset + delta

    def assert_exception_table_increasing(self, lines):
        prev_start, prev_end = -1, -1
        count = 0
        for line in lines:
            m = re.match(r'  (\d+) to (\d+) -> \d+ \[\d+\]', line)
            start, end = [int(g) for g in m.groups()]
            self.assertGreaterEqual(end, start)
            self.assertGreater(start, prev_end)
            prev_start, prev_end = start, end
            count += 1
        return count

    def strip_offsets(self, text):
        lines = text.splitlines(True)
        start, end = self.find_offset_column(lines)
        res = []
        lines = iter(lines)
        for line in lines:
            if line.startswith("Exception"):
                res.append(line)
                break
            if not line or line.startswith("Disassembly"):
                res.append(line)
            else:
                res.append(line[:start] + line[end:])
        num_rows = self.assert_exception_table_increasing(lines)
        if num_rows:
            res.append(f"{num_rows} row{'s' if num_rows > 1 else ''}\n")
        return "".join(res)

    def do_disassembly_compare(self, got, expected, with_offsets=False):
        if not with_offsets:
            self.assert_offsets_increasing(got, 2)
            got = self.strip_offsets(got)
        if got != expected:
            got = self.strip_addresses(got)
        self.assertEqual(got, expected)


class DisTests(DisTestBase):

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

    def do_disassembly_test(self, func, expected, with_offsets=False):
        self.maxDiff = None
        got = self.get_disassembly(func, depth=0)
        self.do_disassembly_compare(got, expected, with_offsets)

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
        long_opcodes = set(['JUMP_BACKWARD_NO_INTERRUPT',
                           ])
        for opcode, opname in enumerate(dis.opname):
            if opname in long_opcodes:
                continue
            with self.subTest(opname=opname):
                width = dis._OPNAME_WIDTH
                if opcode in dis.hasarg:
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

    def test_bug_46724(self):
        # Test that negative operargs are handled properly
        self.do_disassembly_test(bug46724, dis_bug46724)

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
        self.maxDiff = None
        def func(count):
            namespace = {}
            func = "def foo(x):\n " + ";".join(["x = x + 1"] * count) + "\n return x"
            exec(func, namespace)
            return namespace['foo']

        def expected(count, w):
            s = ['''\
  1        %*d RESUME                   0

''' % (w, 0)]
            s += ['''\
           %*d LOAD_FAST                0 (x)
           %*d LOAD_CONST               1 (1)
           %*d BINARY_OP                0 (+)
           %*d STORE_FAST               0 (x)
''' % (w, 10*i + 2, w, 10*i + 4, w, 10*i + 6, w, 10*i + 10)
                 for i in range(count)]
            s += ['''\

  3        %*d LOAD_FAST                0 (x)
           %*d RETURN_VALUE
''' % (w, 10*count + 2, w, 10*count + 4)]
            s[1] = '  2' + s[1][3:]
            return ''.join(s)

        for i in range(1, 5):
            self.do_disassembly_test(func(i), expected(i, 4), True)
        self.do_disassembly_test(func(999), expected(999, 4), True)
        self.do_disassembly_test(func(1000), expected(1000, 5), True)

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

    def test_disassemble_with(self):
        self.do_disassembly_test(_with, dis_with)

    def test_disassemble_asyncwith(self):
        self.do_disassembly_test(_asyncwith, dis_asyncwith)

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
        self.maxDiff = None
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
        self.do_disassembly_test(None, tb_dis, True)

    def test_dis_object(self):
        self.assertRaises(TypeError, dis.dis, object())

    def test_disassemble_recursive(self):
        def check(expected, **kwargs):
            dis = self.get_disassembly(_h, **kwargs)
            dis = self.strip_addresses(dis)
            dis = self.strip_offsets(dis)
            self.assertEqual(dis, expected)

        check(dis_nested_0, depth=0)
        check(dis_nested_1, depth=1)
        check(dis_nested_2, depth=2)
        check(dis_nested_2, depth=3)
        check(dis_nested_2, depth=None)
        check(dis_nested_2)

    @staticmethod
    def code_quicken(f, times=QUICKENING_WARMUP_DELAY):
        for _ in range(times):
            f()

    @cpython_only
    def test_super_instructions(self):
        self.code_quicken(lambda: load_test(0, 0))
        got = self.get_disassembly(load_test, adaptive=True)
        self.do_disassembly_compare(got, dis_load_test_quickened_code, True)

    @cpython_only
    def test_binary_specialize(self):
        binary_op_quicken = """\
  0           0 RESUME_QUICK             0

  1           2 LOAD_NAME                0 (a)
              4 LOAD_NAME                1 (b)
              6 %s
             10 RETURN_VALUE
"""
        co_int = compile('a + b', "<int>", "eval")
        self.code_quicken(lambda: exec(co_int, {}, {'a': 1, 'b': 2}))
        got = self.get_disassembly(co_int, adaptive=True)
        self.do_disassembly_compare(got, binary_op_quicken % "BINARY_OP_ADD_INT        0 (+)", True)

        co_unicode = compile('a + b', "<unicode>", "eval")
        self.code_quicken(lambda: exec(co_unicode, {}, {'a': 'a', 'b': 'b'}))
        got = self.get_disassembly(co_unicode, adaptive=True)
        self.do_disassembly_compare(got, binary_op_quicken % "BINARY_OP_ADD_UNICODE     0 (+)", True)

        binary_subscr_quicken = """\
  0           0 RESUME_QUICK             0

  1           2 LOAD_NAME                0 (a)
              4 LOAD_CONST               0 (0)
              6 %s
             16 RETURN_VALUE
"""
        co_list = compile('a[0]', "<list>", "eval")
        self.code_quicken(lambda: exec(co_list, {}, {'a': [0]}))
        got = self.get_disassembly(co_list, adaptive=True)
        self.do_disassembly_compare(got, binary_subscr_quicken % "BINARY_SUBSCR_LIST_INT", True)

        co_dict = compile('a[0]', "<dict>", "eval")
        self.code_quicken(lambda: exec(co_dict, {}, {'a': {0: '1'}}))
        got = self.get_disassembly(co_dict, adaptive=True)
        self.do_disassembly_compare(got, binary_subscr_quicken % "BINARY_SUBSCR_DICT", True)

    @cpython_only
    def test_load_attr_specialize(self):
        load_attr_quicken = """\
  0           0 RESUME_QUICK             0

  1           2 LOAD_CONST               0 ('a')
              4 LOAD_ATTR_SLOT           0 (__class__)
             24 RETURN_VALUE
"""
        co = compile("'a'.__class__", "", "eval")
        self.code_quicken(lambda: exec(co, {}, {}))
        got = self.get_disassembly(co, adaptive=True)
        self.do_disassembly_compare(got, load_attr_quicken, True)

    @cpython_only
    def test_call_specialize(self):
        call_quicken = """\
  0        RESUME_QUICK             0

  1        PUSH_NULL
           LOAD_NAME                0 (str)
           LOAD_CONST               0 (1)
           CALL_NO_KW_STR_1         1
           RETURN_VALUE
"""
        co = compile("str(1)", "", "eval")
        self.code_quicken(lambda: exec(co, {}, {}))
        got = self.get_disassembly(co, adaptive=True)
        self.do_disassembly_compare(got, call_quicken)

    @cpython_only
    def test_loop_quicken(self):
        # Loop can trigger a quicken where the loop is located
        self.code_quicken(loop_test, 1)
        got = self.get_disassembly(loop_test, adaptive=True)
        self.do_disassembly_compare(got, dis_loop_test_quickened_code)

    @cpython_only
    def test_extended_arg_quick(self):
        got = self.get_disassembly(extended_arg_quick)
        self.do_disassembly_compare(got, dis_extended_arg_quick_code, True)

    def get_cached_values(self, quickened, adaptive):
        def f():
            l = []
            for i in range(42):
                l.append(i)
        if quickened:
            self.code_quicken(f)
        else:
            # "copy" the code to un-quicken it:
            f.__code__ = f.__code__.replace()
        for instruction in dis.get_instructions(
            f, show_caches=True, adaptive=adaptive
        ):
            if instruction.opname == "CACHE":
                yield instruction.argrepr

    @cpython_only
    def test_show_caches(self):
        for quickened in (False, True):
            for adaptive in (False, True):
                with self.subTest(f"{quickened=}, {adaptive=}"):
                    if quickened and adaptive:
                        pattern = r"^(\w+: \d+)?$"
                    else:
                        pattern = r"^(\w+: 0)?$"
                    caches = list(self.get_cached_values(quickened, adaptive))
                    for cache in caches:
                        self.assertRegex(cache, pattern)
                    total_caches = 23
                    empty_caches = 8 if adaptive and quickened else total_caches
                    self.assertEqual(caches.count(""), empty_caches)
                    self.assertEqual(len(caches), total_caches)


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
Stack size:        \\d+
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
Stack size:        \\d+
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
Stack size:        \\d+
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
Stack size:        \\d+
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
Stack size:        \\d+
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
Stack size:        \\d+
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
Stack size:        \\d+
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
        print("I can haz else")
    while i:
        print(i)
        i -= 1
        if i > 6:
            continue
        if i < 4:
            break
    else:
        print("We get lolcatz!")
    try:
        1 / 0
    except ZeroDivisionError:
        print("Here we go")
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

# Test case regeneration
# The following lines are useful to regenerate the expected results after
# either the fodder is modified or the bytecode generation changes. Delete
# everything in the 'Generated Test Cases' block below and replace with the
# output of _regenerate_expected (after removing the print() output from
# outer(), which will appear at the beginning).

def _format(op):
    arg = "_" if op.arg is None else op.arg
    line = "_" if op.starts_line is None else op.starts_line
    if isinstance(op.argval, types.CodeType):
        argval = f"code_object_{op.argval.co_name}"
        argrepr = f"repr({argval})"
    elif isinstance(op.argval, str):
        argval = repr(op.argval)
        argrepr = repr(op.argrepr)
    else:
        argval = op.argval
        argrepr = repr(op.argrepr)
    return (f"    ({op.opname!r:<28}, {op.opcode:>3}, {arg:>3}, {argval!s:>20}, "
            f"{argrepr:>24}, {op.offset:>6}, {line:>11}, {op.is_jump_target!r}),")


def _print_expected(code, name, first_line):
    print(f"expected_opinfo_{name} = [inst(*args) for args in [")
    print(f"    # {'opname':<24}, opcode, arg, {'argval':>20}, {'argrepr':>24}, offset, starts_line, is_jump_target")
    for op in dis.get_instructions(code, first_line=first_line):
        print(_format(op))
    print("]]\n")


def _regenerate_expected():
    for code, name, first_line in [
            (outer, "outer", expected_outer_line),
            (outer(), "f", expected_f_line),
            (outer()(), "inner", expected_inner_line),
            (jumpy, "jumpy", expected_jumpy_line)
    ]:
        _print_expected(code, name, first_line)

# _regenerate_expected()
# End test case regeneration

def inst(opname, opcode, arg, argval, argrepr, offset, starts_line, is_jump_target):
    return dis.Instruction(opname=opname, opcode=opcode, arg=arg, argval=argval,
                           argrepr=argrepr, offset=offset,
                           starts_line=starts_line, is_jump_target=is_jump_target,
                           positions=None)

_ = None


# Generated test cases

# fmt: off

expected_opinfo_outer = [inst(*args) for args in [
    # opname                  , opcode, arg,               argval,                  argrepr, offset, starts_line, is_jump_target
    ('MAKE_CELL'                 , 135,   0,                  'a',                      'a',      0,           _, False),
    ('MAKE_CELL'                 , 135,   1,                  'b',                      'b',      2,           _, False),
    ('RESUME'                    , 151,   0,                    0,                       '',      4,           1, False),
    ('LOAD_CONST'                , 100,   7,               (3, 4),                 '(3, 4)',      6,           2, False),
    ('LOAD_CLOSURE'              , 136,   0,                  'a',                      'a',      8,           _, False),
    ('LOAD_CLOSURE'              , 136,   1,                  'b',                      'b',     10,           _, False),
    ('BUILD_TUPLE'               , 102,   2,                    2,                       '',     12,           _, False),
    ('LOAD_CONST'                , 100,   3,        code_object_f,      repr(code_object_f),     14,           _, False),
    ('MAKE_FUNCTION'             , 132,   9,                    9,      'defaults, closure',     16,           _, False),
    ('STORE_FAST'                , 125,   2,                  'f',                      'f',     18,           _, False),
    ('LOAD_GLOBAL'               , 116,   1,              'print',           'NULL + print',     20,           7, False),
    ('LOAD_DEREF'                , 137,   0,                  'a',                      'a',     32,           _, False),
    ('LOAD_DEREF'                , 137,   1,                  'b',                      'b',     34,           _, False),
    ('LOAD_CONST'                , 100,   4,                   '',                     "''",     36,           _, False),
    ('LOAD_CONST'                , 100,   5,                    1,                      '1',     38,           _, False),
    ('BUILD_LIST'                , 103,   0,                    0,                       '',     40,           _, False),
    ('BUILD_MAP'                 , 105,   0,                    0,                       '',     42,           _, False),
    ('LOAD_CONST'                , 100,   6,       'Hello world!',         "'Hello world!'",     44,           _, False),
    ('CALL'                      , 171,   7,                    7,                       '',     46,           _, False),
    ('POP_TOP'                   ,   1,   _,                 None,                       '',     56,           _, False),
    ('LOAD_FAST'                 , 124,   2,                  'f',                      'f',     58,           8, False),
    ('RETURN_VALUE'              ,  83,   _,                 None,                       '',     60,           _, False),
]]

expected_opinfo_f = [inst(*args) for args in [
    # opname                  , opcode, arg,               argval,                  argrepr, offset, starts_line, is_jump_target
    ('COPY_FREE_VARS'            , 149,   2,                    2,                       '',      0,           _, False),
    ('MAKE_CELL'                 , 135,   0,                  'c',                      'c',      2,           _, False),
    ('MAKE_CELL'                 , 135,   1,                  'd',                      'd',      4,           _, False),
    ('RESUME'                    , 151,   0,                    0,                       '',      6,           2, False),
    ('LOAD_CONST'                , 100,   4,               (5, 6),                 '(5, 6)',      8,           3, False),
    ('LOAD_CLOSURE'              , 136,   3,                  'a',                      'a',     10,           _, False),
    ('LOAD_CLOSURE'              , 136,   4,                  'b',                      'b',     12,           _, False),
    ('LOAD_CLOSURE'              , 136,   0,                  'c',                      'c',     14,           _, False),
    ('LOAD_CLOSURE'              , 136,   1,                  'd',                      'd',     16,           _, False),
    ('BUILD_TUPLE'               , 102,   4,                    4,                       '',     18,           _, False),
    ('LOAD_CONST'                , 100,   3,    code_object_inner,  repr(code_object_inner),     20,           _, False),
    ('MAKE_FUNCTION'             , 132,   9,                    9,      'defaults, closure',     22,           _, False),
    ('STORE_FAST'                , 125,   2,              'inner',                  'inner',     24,           _, False),
    ('LOAD_GLOBAL'               , 116,   1,              'print',           'NULL + print',     26,           5, False),
    ('LOAD_DEREF'                , 137,   3,                  'a',                      'a',     38,           _, False),
    ('LOAD_DEREF'                , 137,   4,                  'b',                      'b',     40,           _, False),
    ('LOAD_DEREF'                , 137,   0,                  'c',                      'c',     42,           _, False),
    ('LOAD_DEREF'                , 137,   1,                  'd',                      'd',     44,           _, False),
    ('CALL'                      , 171,   4,                    4,                       '',     46,           _, False),
    ('POP_TOP'                   ,   1,   _,                 None,                       '',     56,           _, False),
    ('LOAD_FAST'                 , 124,   2,              'inner',                  'inner',     58,           6, False),
    ('RETURN_VALUE'              ,  83,   _,                 None,                       '',     60,           _, False),
]]

expected_opinfo_inner = [inst(*args) for args in [
    # opname                  , opcode, arg,               argval,                  argrepr, offset, starts_line, is_jump_target
    ('COPY_FREE_VARS'            , 149,   4,                    4,                       '',      0,           _, False),
    ('RESUME'                    , 151,   0,                    0,                       '',      2,           3, False),
    ('LOAD_GLOBAL'               , 116,   1,              'print',           'NULL + print',      4,           4, False),
    ('LOAD_DEREF'                , 137,   2,                  'a',                      'a',     16,           _, False),
    ('LOAD_DEREF'                , 137,   3,                  'b',                      'b',     18,           _, False),
    ('LOAD_DEREF'                , 137,   4,                  'c',                      'c',     20,           _, False),
    ('LOAD_DEREF'                , 137,   5,                  'd',                      'd',     22,           _, False),
    ('LOAD_FAST'                 , 124,   0,                  'e',                      'e',     24,           _, False),
    ('LOAD_FAST'                 , 124,   1,                  'f',                      'f',     26,           _, False),
    ('CALL'                      , 171,   6,                    6,                       '',     28,           _, False),
    ('POP_TOP'                   ,   1,   _,                 None,                       '',     38,           _, False),
    ('LOAD_CONST'                , 100,   0,                 None,                   'None',     40,           _, False),
    ('RETURN_VALUE'              ,  83,   _,                 None,                       '',     42,           _, False),
]]

expected_opinfo_jumpy = [inst(*args) for args in [
    # opname                  , opcode, arg,               argval,                  argrepr, offset, starts_line, is_jump_target
    ('RESUME'                    , 151,   0,                    0,                       '',      0,           1, False),
    ('LOAD_GLOBAL'               , 116,   1,              'range',           'NULL + range',      2,           3, False),
    ('LOAD_CONST'                , 100,   1,                   10,                     '10',     14,           _, False),
    ('CALL'                      , 171,   1,                    1,                       '',     16,           _, False),
    ('GET_ITER'                  ,  68,   _,                 None,                       '',     26,           _, False),
    ('FOR_ITER'                  ,  93,  30,                   92,                  'to 92',     28,           _, True),
    ('STORE_FAST'                , 125,   0,                  'i',                      'i',     32,           _, False),
    ('LOAD_GLOBAL'               , 116,   3,              'print',           'NULL + print',     34,           4, False),
    ('LOAD_FAST'                 , 124,   0,                  'i',                      'i',     46,           _, False),
    ('CALL'                      , 171,   1,                    1,                       '',     48,           _, False),
    ('POP_TOP'                   ,   1,   _,                 None,                       '',     58,           _, False),
    ('LOAD_FAST'                 , 124,   0,                  'i',                      'i',     60,           5, False),
    ('LOAD_CONST'                , 100,   2,                    4,                      '4',     62,           _, False),
    ('COMPARE_OP'                , 107,   0,                  '<',                      '<',     64,           _, False),
    ('POP_JUMP_IF_FALSE'         , 114,   1,                   74,                  'to 74',     70,           _, False),
    ('JUMP_BACKWARD'             , 140,  23,                   28,                  'to 28',     72,           6, False),
    ('LOAD_FAST'                 , 124,   0,                  'i',                      'i',     74,           7, True),
    ('LOAD_CONST'                , 100,   3,                    6,                      '6',     76,           _, False),
    ('COMPARE_OP'                , 107,   4,                  '>',                      '>',     78,           _, False),
    ('POP_JUMP_IF_TRUE'          , 115,   1,                   88,                  'to 88',     84,           _, False),
    ('JUMP_BACKWARD'             , 140,  30,                   28,                  'to 28',     86,           _, False),
    ('POP_TOP'                   ,   1,   _,                 None,                       '',     88,           8, True),
    ('JUMP_FORWARD'              , 110,  13,                  118,                 'to 118',     90,           _, False),
    ('LOAD_GLOBAL'               , 116,   3,              'print',           'NULL + print',     92,          10, True),
    ('LOAD_CONST'                , 100,   4,     'I can haz else',       "'I can haz else'",    104,           _, False),
    ('CALL'                      , 171,   1,                    1,                       '',    106,           _, False),
    ('POP_TOP'                   ,   1,   _,                 None,                       '',    116,           _, False),
    ('LOAD_FAST_CHECK'           , 127,   0,                  'i',                      'i',    118,          11, True),
    ('POP_JUMP_IF_FALSE'         , 114,  35,                  192,                 'to 192',    120,           _, False),
    ('LOAD_GLOBAL'               , 116,   3,              'print',           'NULL + print',    122,          12, True),
    ('LOAD_FAST'                 , 124,   0,                  'i',                      'i',    134,           _, False),
    ('CALL'                      , 171,   1,                    1,                       '',    136,           _, False),
    ('POP_TOP'                   ,   1,   _,                 None,                       '',    146,           _, False),
    ('LOAD_FAST'                 , 124,   0,                  'i',                      'i',    148,          13, False),
    ('LOAD_CONST'                , 100,   5,                    1,                      '1',    150,           _, False),
    ('BINARY_OP'                 , 122,  23,                   23,                     '-=',    152,           _, False),
    ('STORE_FAST'                , 125,   0,                  'i',                      'i',    156,           _, False),
    ('LOAD_FAST'                 , 124,   0,                  'i',                      'i',    158,          14, False),
    ('LOAD_CONST'                , 100,   3,                    6,                      '6',    160,           _, False),
    ('COMPARE_OP'                , 107,   4,                  '>',                      '>',    162,           _, False),
    ('POP_JUMP_IF_FALSE'         , 114,   1,                  172,                 'to 172',    168,           _, False),
    ('JUMP_BACKWARD'             , 140,  27,                  118,                 'to 118',    170,          15, False),
    ('LOAD_FAST'                 , 124,   0,                  'i',                      'i',    172,          16, True),
    ('LOAD_CONST'                , 100,   2,                    4,                      '4',    174,           _, False),
    ('COMPARE_OP'                , 107,   0,                  '<',                      '<',    176,           _, False),
    ('POP_JUMP_IF_FALSE'         , 114,   1,                  186,                 'to 186',    182,           _, False),
    ('JUMP_FORWARD'              , 110,  16,                  218,                 'to 218',    184,          17, False),
    ('LOAD_FAST'                 , 124,   0,                  'i',                      'i',    186,          11, True),
    ('POP_JUMP_IF_FALSE'         , 114,   1,                  192,                 'to 192',    188,           _, False),
    ('JUMP_BACKWARD'             , 140,  35,                  122,                 'to 122',    190,           _, False),
    ('LOAD_GLOBAL'               , 116,   3,              'print',           'NULL + print',    192,          19, True),
    ('LOAD_CONST'                , 100,   6,    'We get lolcatz!',      "'We get lolcatz!'",    204,           _, False),
    ('CALL'                      , 171,   1,                    1,                       '',    206,           _, False),
    ('POP_TOP'                   ,   1,   _,                 None,                       '',    216,           _, False),
    ('NOP'                       ,   9,   _,                 None,                       '',    218,          20, True),
    ('LOAD_CONST'                , 100,   5,                    1,                      '1',    220,          21, False),
    ('LOAD_CONST'                , 100,   7,                    0,                      '0',    222,           _, False),
    ('BINARY_OP'                 , 122,  11,                   11,                      '/',    224,           _, False),
    ('POP_TOP'                   ,   1,   _,                 None,                       '',    228,           _, False),
    ('LOAD_FAST'                 , 124,   0,                  'i',                      'i',    230,          25, False),
    ('BEFORE_WITH'               ,  53,   _,                 None,                       '',    232,           _, False),
    ('STORE_FAST'                , 125,   1,              'dodgy',                  'dodgy',    234,           _, False),
    ('LOAD_GLOBAL'               , 116,   3,              'print',           'NULL + print',    236,          26, False),
    ('LOAD_CONST'                , 100,   8,   'Never reach this',     "'Never reach this'",    248,           _, False),
    ('CALL'                      , 171,   1,                    1,                       '',    250,           _, False),
    ('POP_TOP'                   ,   1,   _,                 None,                       '',    260,           _, False),
    ('LOAD_CONST'                , 100,   0,                 None,                   'None',    262,          25, False),
    ('LOAD_CONST'                , 100,   0,                 None,                   'None',    264,           _, False),
    ('LOAD_CONST'                , 100,   0,                 None,                   'None',    266,           _, False),
    ('CALL'                      , 171,   2,                    2,                       '',    268,           _, False),
    ('POP_TOP'                   ,   1,   _,                 None,                       '',    278,           _, False),
    ('LOAD_GLOBAL'               , 116,   3,              'print',           'NULL + print',    280,          28, True),
    ('LOAD_CONST'                , 100,  10, "OK, now we're done",  '"OK, now we\'re done"',    292,           _, False),
    ('CALL'                      , 171,   1,                    1,                       '',    294,           _, False),
    ('POP_TOP'                   ,   1,   _,                 None,                       '',    304,           _, False),
    ('LOAD_CONST'                , 100,   0,                 None,                   'None',    306,           _, False),
    ('RETURN_VALUE'              ,  83,   _,                 None,                       '',    308,           _, False),
    ('PUSH_EXC_INFO'             ,  35,   _,                 None,                       '',    310,          25, False),
    ('WITH_EXCEPT_START'         ,  49,   _,                 None,                       '',    312,           _, False),
    ('POP_JUMP_IF_TRUE'          , 115,   1,                  318,                 'to 318',    314,           _, False),
    ('RERAISE'                   , 119,   2,                    2,                       '',    316,           _, False),
    ('POP_TOP'                   ,   1,   _,                 None,                       '',    318,           _, True),
    ('POP_EXCEPT'                ,  89,   _,                 None,                       '',    320,           _, False),
    ('POP_TOP'                   ,   1,   _,                 None,                       '',    322,           _, False),
    ('POP_TOP'                   ,   1,   _,                 None,                       '',    324,           _, False),
    ('JUMP_BACKWARD'             , 140,  24,                  280,                 'to 280',    326,           _, False),
    ('COPY'                      , 120,   3,                    3,                       '',    328,           _, False),
    ('POP_EXCEPT'                ,  89,   _,                 None,                       '',    330,           _, False),
    ('RERAISE'                   , 119,   1,                    1,                       '',    332,           _, False),
    ('PUSH_EXC_INFO'             ,  35,   _,                 None,                       '',    334,           _, False),
    ('LOAD_GLOBAL'               , 116,   4,  'ZeroDivisionError',      'ZeroDivisionError',    336,          22, False),
    ('CHECK_EXC_MATCH'           ,  36,   _,                 None,                       '',    348,           _, False),
    ('POP_JUMP_IF_FALSE'         , 114,  16,                  384,                 'to 384',    350,           _, False),
    ('POP_TOP'                   ,   1,   _,                 None,                       '',    352,           _, False),
    ('LOAD_GLOBAL'               , 116,   3,              'print',           'NULL + print',    354,          23, False),
    ('LOAD_CONST'                , 100,   9,         'Here we go',           "'Here we go'",    366,           _, False),
    ('CALL'                      , 171,   1,                    1,                       '',    368,           _, False),
    ('POP_TOP'                   ,   1,   _,                 None,                       '',    378,           _, False),
    ('POP_EXCEPT'                ,  89,   _,                 None,                       '',    380,           _, False),
    ('JUMP_BACKWARD'             , 140,  52,                  280,                 'to 280',    382,           _, False),
    ('RERAISE'                   , 119,   0,                    0,                       '',    384,          22, True),
    ('COPY'                      , 120,   3,                    3,                       '',    386,           _, False),
    ('POP_EXCEPT'                ,  89,   _,                 None,                       '',    388,           _, False),
    ('RERAISE'                   , 119,   1,                    1,                       '',    390,           _, False),
    ('PUSH_EXC_INFO'             ,  35,   _,                 None,                       '',    392,           _, False),
    ('LOAD_GLOBAL'               , 116,   3,              'print',           'NULL + print',    394,          28, False),
    ('LOAD_CONST'                , 100,  10, "OK, now we're done",  '"OK, now we\'re done"',    406,           _, False),
    ('CALL'                      , 171,   1,                    1,                       '',    408,           _, False),
    ('POP_TOP'                   ,   1,   _,                 None,                       '',    418,           _, False),
    ('RERAISE'                   , 119,   0,                    0,                       '',    420,           _, False),
    ('COPY'                      , 120,   3,                    3,                       '',    422,           _, False),
    ('POP_EXCEPT'                ,  89,   _,                 None,                       '',    424,           _, False),
    ('RERAISE'                   , 119,   1,                    1,                       '',    426,           _, False),
]]

# fmt: on
# End generated test cases

# One last piece of inspect fodder to check the default line number handling
def simple(): pass
line = simple.__code__.co_firstlineno

# fmt: off
expected_opinfo_simple = [inst(*args) for args in [
    # opname                  , opcode, arg,            argval,               argrepr, offset, starts_line, is_jump_target
    ('RESUME'                 , 151,   0,                    0,                    '',      0,        line,  False),
    ('LOAD_CONST'             , 100,   0,                 None,                'None',      2,           _,  False),
    ('RETURN_VALUE'           ,  83,   _,                 None,                    '',      4,           _,  False),
]]
# fmt: on


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
            (0, 1, 0, 0),
            (1, 1, 0, 1),
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

        named_positions = [
            (pos.lineno, pos.end_lineno, pos.col_offset, pos.end_col_offset)
            for pos in positions
        ]
        self.assertEqual(named_positions, expected)

    @requires_debug_ranges()
    def test_co_positions_missing_info(self):
        code = compile('x, y, z', '<test>', 'exec')
        code_without_location_table = code.replace(co_linetable=b'')
        actual = dis.get_instructions(code_without_location_table)
        for instruction in actual:
            with self.subTest(instruction=instruction):
                positions = instruction.positions
                self.assertEqual(len(positions), 4)
                if instruction.opname == "RESUME":
                    continue
                self.assertIsNone(positions.lineno)
                self.assertIsNone(positions.end_lineno)
                self.assertIsNone(positions.col_offset)
                self.assertIsNone(positions.end_col_offset)

    @requires_debug_ranges()
    def test_co_positions_with_lots_of_caches(self):
        def roots(a, b, c):
            d = b**2 - 4 * a * c
            yield (-b - cmath.sqrt(d)) / (2 * a)
            if d:
                yield (-b + cmath.sqrt(d)) / (2 * a)
        code = roots.__code__
        ops = code.co_code[::2]
        cache_opcode = opcode.opmap["CACHE"]
        caches = sum(op == cache_opcode for op in ops)
        non_caches = len(ops) - caches
        # Make sure we have "lots of caches". If not, roots should be changed:
        assert 1 / 3 <= caches / non_caches, "this test needs more caches!"
        for show_caches in (False, True):
            for adaptive in (False, True):
                with self.subTest(f"{adaptive=}, {show_caches=}"):
                    co_positions = [
                        positions
                        for op, positions in zip(ops, code.co_positions(), strict=True)
                        if show_caches or op != cache_opcode
                    ]
                    dis_positions = [
                        instruction.positions
                        for instruction in dis.get_instructions(
                            code, adaptive=adaptive, show_caches=show_caches
                        )
                    ]
                    self.assertEqual(co_positions, dis_positions)

# get_instructions has its own tests above, so can rely on it to validate
# the object oriented API
class BytecodeTests(InstructionTestCase, DisTestBase):

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
        self.do_disassembly_compare(actual, dis_f)

    def test_from_traceback(self):
        tb = get_tb()
        b = dis.Bytecode.from_traceback(tb)
        while tb.tb_next: tb = tb.tb_next

        self.assertEqual(b.current_offset, tb.tb_lasti)

    def test_from_traceback_dis(self):
        self.maxDiff = None
        tb = get_tb()
        b = dis.Bytecode.from_traceback(tb)
        self.assertEqual(self.strip_offsets(b.dis()), dis_traceback)

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

    def test_findlabels(self):
        labels = dis.findlabels(jumpy.__code__.co_code)
        jumps = [
            instr.offset
            for instr in expected_opinfo_jumpy
            if instr.is_jump_target
        ]

        self.assertEqual(sorted(labels), sorted(jumps))


class TestDisTraceback(DisTestBase):
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
        self.maxDiff = None
        # We need to have an existing last traceback in `sys`:
        tb = get_tb()
        sys.last_traceback = tb

        self.do_disassembly_compare(self.get_disassembly(None), dis_traceback)

    def test_distb_explicit_arg(self):
        self.maxDiff = None
        tb = get_tb()

        self.do_disassembly_compare(self.get_disassembly(tb), dis_traceback)


class TestDisTracebackWithFile(TestDisTraceback):
    # Run the `distb` tests again, using the file arg instead of print
    def get_disassembly(self, tb):
        output = io.StringIO()
        with contextlib.redirect_stdout(output):
            dis.distb(tb, file=output)
        return output.getvalue()


if __name__ == "__main__":
    unittest.main()
