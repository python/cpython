# Minimal tests for dis module

import contextlib
import dis
import functools
import io
import re
import sys
import types
import unittest
from test.support import (captured_stdout, requires_debug_ranges,
                          requires_specialization, cpython_only)
from test.support.bytecode_helper import BytecodeTestCase

import opcode

CACHE = dis.opmap["CACHE"]

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
%3d           RESUME                   0

%3d           LOAD_FAST                1 (x)
              LOAD_CONST               1 (1)
              COMPARE_OP              72 (==)
              LOAD_FAST                0 (self)
              STORE_ATTR               0 (x)
              RETURN_CONST             0 (None)
""" % (_C.__init__.__code__.co_firstlineno, _C.__init__.__code__.co_firstlineno + 1,)

dis_c_instance_method_bytes = """\
          RESUME                   0
          LOAD_FAST                1
          LOAD_CONST               1
          COMPARE_OP              72 (==)
          LOAD_FAST                0
          STORE_ATTR               0
          RETURN_CONST             0
"""

dis_c_class_method = """\
%3d           RESUME                   0

%3d           LOAD_FAST                1 (x)
              LOAD_CONST               1 (1)
              COMPARE_OP              72 (==)
              LOAD_FAST                0 (cls)
              STORE_ATTR               0 (x)
              RETURN_CONST             0 (None)
""" % (_C.cm.__code__.co_firstlineno, _C.cm.__code__.co_firstlineno + 2,)

dis_c_static_method = """\
%3d           RESUME                   0

%3d           LOAD_FAST                0 (x)
              LOAD_CONST               1 (1)
              COMPARE_OP              72 (==)
              STORE_FAST               0 (x)
              RETURN_CONST             0 (None)
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
%3d           RESUME                   0

%3d           LOAD_GLOBAL              1 (print + NULL)
              LOAD_FAST                0 (a)
              CALL                     1
              POP_TOP

%3d           RETURN_CONST             1 (1)
""" % (_f.__code__.co_firstlineno,
       _f.__code__.co_firstlineno + 1,
       _f.__code__.co_firstlineno + 2)

dis_f_with_offsets = """\
%3d          0       RESUME                   0

%3d          2       LOAD_GLOBAL              1 (print + NULL)
            12       LOAD_FAST                0 (a)
            14       CALL                     1
            22       POP_TOP

%3d         24       RETURN_CONST             1 (1)
""" % (_f.__code__.co_firstlineno,
       _f.__code__.co_firstlineno + 1,
       _f.__code__.co_firstlineno + 2)


dis_f_co_code = """\
          RESUME                   0
          LOAD_GLOBAL              1
          LOAD_FAST                0
          CALL                     1
          POP_TOP
          RETURN_CONST             1
"""

def bug708901():
    for res in range(1,
                     10):
        pass

dis_bug708901 = """\
%3d           RESUME                   0

%3d           LOAD_GLOBAL              1 (range + NULL)
              LOAD_CONST               1 (1)

%3d           LOAD_CONST               2 (10)

%3d           CALL                     2
              GET_ITER
      L1:     FOR_ITER                 3 (to L2)
              STORE_FAST               0 (res)

%3d           JUMP_BACKWARD            5 (to L1)

%3d   L2:     END_FOR
              POP_TOP
              RETURN_CONST             0 (None)
""" % (bug708901.__code__.co_firstlineno,
       bug708901.__code__.co_firstlineno + 1,
       bug708901.__code__.co_firstlineno + 2,
       bug708901.__code__.co_firstlineno + 1,
       bug708901.__code__.co_firstlineno + 3,
       bug708901.__code__.co_firstlineno + 1)


def bug1333982(x=[]):
    assert 0, ((s for s in x) +
              1)
    pass

dis_bug1333982 = """\
%3d           RESUME                   0

%3d           LOAD_ASSERTION_ERROR
              LOAD_CONST               1 (<code object <genexpr> at 0x..., file "%s", line %d>)
              MAKE_FUNCTION
              LOAD_FAST                0 (x)
              GET_ITER
              CALL                     0

%3d           LOAD_CONST               2 (1)

%3d           BINARY_OP                0 (+)
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
          RETURN_CONST             0 (None)
"""

# Extended arg followed by NOP
code_bug_45757 = bytes([
        opcode.opmap['EXTENDED_ARG'], 0x01,  # EXTENDED_ARG 0x01
        opcode.opmap['NOP'],          0xFF,  # NOP 0xFF
        opcode.opmap['EXTENDED_ARG'], 0x01,  # EXTENDED_ARG 0x01
        opcode.opmap['LOAD_CONST'],   0x29,  # LOAD_CONST 0x29
        opcode.opmap['RETURN_VALUE'], 0x00,  # RETURN_VALUE 0x00
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
  L1:     EXTENDED_ARG           255
          EXTENDED_ARG         65535
          EXTENDED_ARG         16777215
          JUMP_FORWARD            -4 (to L1)
"""

def func_w_kwargs(a, b, **c):
    pass

def wrap_func_w_kwargs():
    func_w_kwargs(1, 2, c=5)

dis_kw_names = """\
%3d           RESUME                   0

%3d           LOAD_GLOBAL              1 (func_w_kwargs + NULL)
              LOAD_CONST               1 (1)
              LOAD_CONST               2 (2)
              LOAD_CONST               3 (5)
              LOAD_CONST               4 (('c',))
              CALL_KW                  3
              POP_TOP
              RETURN_CONST             0 (None)
""" % (wrap_func_w_kwargs.__code__.co_firstlineno,
       wrap_func_w_kwargs.__code__.co_firstlineno + 1)

dis_intrinsic_1_2 = """\
  0           RESUME                   0

  1           LOAD_CONST               0 (0)
              LOAD_CONST               1 (('*',))
              IMPORT_NAME              0 (math)
              CALL_INTRINSIC_1         2 (INTRINSIC_IMPORT_STAR)
              POP_TOP
              RETURN_CONST             2 (None)
"""

dis_intrinsic_1_5 = """\
  0           RESUME                   0

  1           LOAD_NAME                0 (a)
              CALL_INTRINSIC_1         5 (INTRINSIC_UNARY_POSITIVE)
              RETURN_VALUE
"""

dis_intrinsic_1_6 = """\
  0           RESUME                   0

  1           BUILD_LIST               0
              LOAD_NAME                0 (a)
              LIST_EXTEND              1
              CALL_INTRINSIC_1         6 (INTRINSIC_LIST_TO_TUPLE)
              RETURN_VALUE
"""

_BIG_LINENO_FORMAT = """\
  1           RESUME                   0

%3d           LOAD_GLOBAL              0 (spam)
              POP_TOP
              RETURN_CONST             0 (None)
"""

_BIG_LINENO_FORMAT2 = """\
   1           RESUME                   0

%4d           LOAD_GLOBAL              0 (spam)
               POP_TOP
               RETURN_CONST             0 (None)
"""

dis_module_expected_results = """\
Disassembly of f:
  4           RESUME                   0
              RETURN_CONST             0 (None)

Disassembly of g:
  5           RESUME                   0
              RETURN_CONST             0 (None)

"""

expr_str = "x + 1"

dis_expr_str = """\
  0           RESUME                   0

  1           LOAD_NAME                0 (x)
              LOAD_CONST               0 (1)
              BINARY_OP                0 (+)
              RETURN_VALUE
"""

simple_stmt_str = "x = x + 1"

dis_simple_stmt_str = """\
  0           RESUME                   0

  1           LOAD_NAME                0 (x)
              LOAD_CONST               0 (1)
              BINARY_OP                0 (+)
              STORE_NAME               0 (x)
              RETURN_CONST             1 (None)
"""

annot_stmt_str = """\

x: int = 1
y: fun(1)
lst[fun(0)]: int = 1
"""
# leading newline is for a reason (tests lineno)

dis_annot_stmt_str = """\
  0           RESUME                   0

  2           SETUP_ANNOTATIONS
              LOAD_CONST               0 (1)
              STORE_NAME               0 (x)
              LOAD_NAME                1 (int)
              LOAD_NAME                2 (__annotations__)
              LOAD_CONST               1 ('x')
              STORE_SUBSCR

  3           LOAD_NAME                3 (fun)
              PUSH_NULL
              LOAD_CONST               0 (1)
              CALL                     1
              LOAD_NAME                2 (__annotations__)
              LOAD_CONST               2 ('y')
              STORE_SUBSCR

  4           LOAD_CONST               0 (1)
              LOAD_NAME                4 (lst)
              LOAD_NAME                3 (fun)
              PUSH_NULL
              LOAD_CONST               3 (0)
              CALL                     1
              STORE_SUBSCR
              LOAD_NAME                1 (int)
              POP_TOP
              RETURN_CONST             4 (None)
"""

compound_stmt_str = """\
x = 0
while 1:
    x += 1"""
# Trailing newline has been deliberately omitted

dis_compound_stmt_str = """\
  0           RESUME                   0

  1           LOAD_CONST               0 (0)
              STORE_NAME               0 (x)

  2           NOP

  3   L1:     LOAD_NAME                0 (x)
              LOAD_CONST               1 (1)
              BINARY_OP               13 (+=)
              STORE_NAME               0 (x)

  2           JUMP_BACKWARD            7 (to L1)
"""

dis_traceback = """\
%4d           RESUME                   0

%4d           NOP

%4d   L1:     LOAD_CONST               1 (1)
               LOAD_CONST               2 (0)
           --> BINARY_OP               11 (/)
               POP_TOP

%4d   L2:     LOAD_FAST_CHECK          1 (tb)
               RETURN_VALUE

  --   L3:     PUSH_EXC_INFO

%4d           LOAD_GLOBAL              0 (Exception)
               CHECK_EXC_MATCH
               POP_JUMP_IF_FALSE       23 (to L7)
               STORE_FAST               0 (e)

%4d   L4:     LOAD_FAST                0 (e)
               LOAD_ATTR                2 (__traceback__)
               STORE_FAST               1 (tb)
       L5:     POP_EXCEPT
               LOAD_CONST               0 (None)
               STORE_FAST               0 (e)
               DELETE_FAST              0 (e)

%4d           LOAD_FAST                1 (tb)
               RETURN_VALUE

  --   L6:     LOAD_CONST               0 (None)
               STORE_FAST               0 (e)
               DELETE_FAST              0 (e)
               RERAISE                  1

%4d   L7:     RERAISE                  0

  --   L8:     COPY                     3
               POP_EXCEPT
               RERAISE                  1
ExceptionTable:
  L1 to L2 -> L3 [0]
  L3 to L4 -> L8 [1] lasti
  L4 to L5 -> L6 [1] lasti
  L6 to L8 -> L8 [1] lasti
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
%3d           RESUME                   0

%3d           LOAD_FAST                0 (a)
              FORMAT_SIMPLE
              LOAD_CONST               1 (' ')
              LOAD_FAST                1 (b)
              LOAD_CONST               2 ('4')
              FORMAT_WITH_SPEC
              LOAD_CONST               1 (' ')
              LOAD_FAST                2 (c)
              CONVERT_VALUE            2 (repr)
              FORMAT_SIMPLE
              LOAD_CONST               1 (' ')
              LOAD_FAST                3 (d)
              CONVERT_VALUE            2 (repr)
              LOAD_CONST               2 ('4')
              FORMAT_WITH_SPEC
              BUILD_STRING             7
              RETURN_VALUE
""" % (_fstring.__code__.co_firstlineno, _fstring.__code__.co_firstlineno + 1)

def _with(c):
    with c:
        x = 1
    y = 2

dis_with = """\
%4d           RESUME                   0

%4d           LOAD_FAST                0 (c)
               BEFORE_WITH
       L1:     POP_TOP

%4d           LOAD_CONST               1 (1)
               STORE_FAST               1 (x)

%4d   L2:     LOAD_CONST               0 (None)
               LOAD_CONST               0 (None)
               LOAD_CONST               0 (None)
               CALL                     2
               POP_TOP

%4d           LOAD_CONST               2 (2)
               STORE_FAST               2 (y)
               RETURN_CONST             0 (None)

%4d   L3:     PUSH_EXC_INFO
               WITH_EXCEPT_START
               TO_BOOL
               POP_JUMP_IF_TRUE         1 (to L4)
               RERAISE                  2
       L4:     POP_TOP
       L5:     POP_EXCEPT
               POP_TOP
               POP_TOP

%4d           LOAD_CONST               2 (2)
               STORE_FAST               2 (y)
               RETURN_CONST             0 (None)

  --   L6:     COPY                     3
               POP_EXCEPT
               RERAISE                  1
ExceptionTable:
  L1 to L2 -> L3 [1] lasti
  L3 to L5 -> L6 [3] lasti
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
%4d            RETURN_GENERATOR
                POP_TOP
        L1:     RESUME                   0

%4d            LOAD_FAST                0 (c)
                BEFORE_ASYNC_WITH
                GET_AWAITABLE            1
                LOAD_CONST               0 (None)
        L2:     SEND                     3 (to L5)
        L3:     YIELD_VALUE              1
        L4:     RESUME                   3
                JUMP_BACKWARD_NO_INTERRUPT 5 (to L2)
        L5:     END_SEND
        L6:     POP_TOP

%4d            LOAD_CONST               1 (1)
                STORE_FAST               1 (x)

%4d    L7:     LOAD_CONST               0 (None)
                LOAD_CONST               0 (None)
                LOAD_CONST               0 (None)
                CALL                     2
                GET_AWAITABLE            2
                LOAD_CONST               0 (None)
        L8:     SEND                     3 (to L11)
        L9:     YIELD_VALUE              1
       L10:     RESUME                   3
                JUMP_BACKWARD_NO_INTERRUPT 5 (to L8)
       L11:     END_SEND
                POP_TOP

%4d            LOAD_CONST               2 (2)
                STORE_FAST               2 (y)
                RETURN_CONST             0 (None)

%4d   L12:     CLEANUP_THROW
       L13:     JUMP_BACKWARD_NO_INTERRUPT 25 (to L5)
       L14:     CLEANUP_THROW
       L15:     JUMP_BACKWARD_NO_INTERRUPT 9 (to L11)
       L16:     PUSH_EXC_INFO
                WITH_EXCEPT_START
                GET_AWAITABLE            2
                LOAD_CONST               0 (None)
       L17:     SEND                     4 (to L21)
       L18:     YIELD_VALUE              1
       L19:     RESUME                   3
                JUMP_BACKWARD_NO_INTERRUPT 5 (to L17)
       L20:     CLEANUP_THROW
       L21:     END_SEND
                TO_BOOL
                POP_JUMP_IF_TRUE         1 (to L22)
                RERAISE                  2
       L22:     POP_TOP
       L23:     POP_EXCEPT
                POP_TOP
                POP_TOP

%4d            LOAD_CONST               2 (2)
                STORE_FAST               2 (y)
                RETURN_CONST             0 (None)

  --   L24:     COPY                     3
                POP_EXCEPT
                RERAISE                  1
       L25:     CALL_INTRINSIC_1         3 (INTRINSIC_STOPITERATION_ERROR)
                RERAISE                  1
ExceptionTable:
  L1 to L3 -> L25 [0] lasti
  L3 to L4 -> L12 [3]
  L4 to L6 -> L25 [0] lasti
  L6 to L7 -> L16 [1] lasti
  L7 to L9 -> L25 [0] lasti
  L9 to L10 -> L14 [2]
  L10 to L13 -> L25 [0] lasti
  L14 to L15 -> L25 [0] lasti
  L16 to L18 -> L24 [3] lasti
  L18 to L19 -> L20 [6]
  L19 to L23 -> L24 [3] lasti
  L23 to L25 -> L25 [0] lasti
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
%4d           RESUME                   0

%4d           NOP

%4d   L1:     LOAD_FAST                0 (a)

%4d   L2:     LOAD_FAST                1 (b)
               PUSH_NULL
               CALL                     0
               POP_TOP
               RETURN_VALUE

  --   L3:     PUSH_EXC_INFO

%4d           LOAD_FAST                1 (b)
               PUSH_NULL
               CALL                     0
               POP_TOP
               RERAISE                  0

  --   L4:     COPY                     3
               POP_EXCEPT
               RERAISE                  1
ExceptionTable:
  L1 to L2 -> L3 [0]
  L3 to L4 -> L4 [1] lasti
""" % (_tryfinally.__code__.co_firstlineno,
       _tryfinally.__code__.co_firstlineno + 1,
       _tryfinally.__code__.co_firstlineno + 2,
       _tryfinally.__code__.co_firstlineno + 4,
       _tryfinally.__code__.co_firstlineno + 4,
       )

dis_tryfinallyconst = """\
%4d           RESUME                   0

%4d           NOP

%4d           NOP

%4d           LOAD_FAST                0 (b)
               PUSH_NULL
               CALL                     0
               POP_TOP
               RETURN_CONST             1 (1)

  --   L1:     PUSH_EXC_INFO

%4d           LOAD_FAST                0 (b)
               PUSH_NULL
               CALL                     0
               POP_TOP
               RERAISE                  0

  --   L2:     COPY                     3
               POP_EXCEPT
               RERAISE                  1
ExceptionTable:
  L1 to L2 -> L2 [1] lasti
""" % (_tryfinallyconst.__code__.co_firstlineno,
       _tryfinallyconst.__code__.co_firstlineno + 1,
       _tryfinallyconst.__code__.co_firstlineno + 2,
       _tryfinallyconst.__code__.co_firstlineno + 4,
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
        return list(x + z for z in y)
    return foo

dis_nested_0 = """\
  --           MAKE_CELL                0 (y)

%4d           RESUME                   0

%4d           LOAD_FAST                0 (y)
               BUILD_TUPLE              1
               LOAD_CONST               1 (<code object foo at 0x..., file "%s", line %d>)
               MAKE_FUNCTION
               SET_FUNCTION_ATTRIBUTE   8 (closure)
               STORE_FAST               1 (foo)

%4d           LOAD_FAST                1 (foo)
               RETURN_VALUE
""" % (_h.__code__.co_firstlineno,
       _h.__code__.co_firstlineno + 1,
       __file__,
       _h.__code__.co_firstlineno + 1,
       _h.__code__.co_firstlineno + 4,
)

dis_nested_1 = """%s
Disassembly of <code object foo at 0x..., file "%s", line %d>:
  --           COPY_FREE_VARS           1
               MAKE_CELL                0 (x)

%4d           RESUME                   0

%4d           LOAD_GLOBAL              1 (list + NULL)
               LOAD_FAST                0 (x)
               BUILD_TUPLE              1
               LOAD_CONST               1 (<code object <genexpr> at 0x..., file "%s", line %d>)
               MAKE_FUNCTION
               SET_FUNCTION_ATTRIBUTE   8 (closure)
               LOAD_DEREF               1 (y)
               GET_ITER
               CALL                     0
               CALL                     1
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
Disassembly of <code object <genexpr> at 0x..., file "%s", line %d>:
  --           COPY_FREE_VARS           1

%4d           RETURN_GENERATOR
               POP_TOP
       L1:     RESUME                   0
               LOAD_FAST                0 (.0)
               GET_ITER
       L2:     FOR_ITER                10 (to L3)
               STORE_FAST               1 (z)
               LOAD_DEREF               2 (x)
               LOAD_FAST                1 (z)
               BINARY_OP                0 (+)
               YIELD_VALUE              0
               RESUME                   5
               POP_TOP
               JUMP_BACKWARD           12 (to L2)
       L3:     END_FOR
               POP_TOP
               RETURN_CONST             0 (None)

  --   L4:     CALL_INTRINSIC_1         3 (INTRINSIC_STOPITERATION_ERROR)
               RERAISE                  1
ExceptionTable:
  L1 to L4 -> L4 [0] lasti
""" % (dis_nested_1,
       __file__,
       _h.__code__.co_firstlineno + 3,
       _h.__code__.co_firstlineno + 3,
)

def load_test(x, y=0):
    a, b = x, y
    return a, b

dis_load_test_quickened_code = """\
%3d           RESUME_CHECK             0

%3d           LOAD_FAST_LOAD_FAST      1 (x, y)
              STORE_FAST_STORE_FAST   50 (b, a)

%3d           LOAD_FAST_LOAD_FAST     35 (a, b)
              BUILD_TUPLE              2
              RETURN_VALUE
""" % (load_test.__code__.co_firstlineno,
       load_test.__code__.co_firstlineno + 1,
       load_test.__code__.co_firstlineno + 2)

def loop_test():
    for i in [1, 2, 3] * 3:
        load_test(i)

dis_loop_test_quickened_code = """\
%3d           RESUME_CHECK             0

%3d           BUILD_LIST               0
              LOAD_CONST               1 ((1, 2, 3))
              LIST_EXTEND              1
              LOAD_CONST               2 (3)
              BINARY_OP                5 (*)
              GET_ITER
      L1:     FOR_ITER_LIST           14 (to L2)
              STORE_FAST               0 (i)

%3d           LOAD_GLOBAL_MODULE       1 (load_test + NULL)
              LOAD_FAST                0 (i)
              CALL_PY_GENERAL          1
              POP_TOP
              JUMP_BACKWARD           16 (to L1)

%3d   L2:     END_FOR
              POP_TOP
              RETURN_CONST             0 (None)
""" % (loop_test.__code__.co_firstlineno,
       loop_test.__code__.co_firstlineno + 1,
       loop_test.__code__.co_firstlineno + 2,
       loop_test.__code__.co_firstlineno + 1,)

def extended_arg_quick():
    *_, _ = ...

dis_extended_arg_quick_code = """\
%3d           RESUME                   0

%3d           LOAD_CONST               1 (Ellipsis)
              EXTENDED_ARG             1
              UNPACK_EX              256
              POP_TOP
              STORE_FAST               0 (_)
              RETURN_CONST             0 (None)
"""% (extended_arg_quick.__code__.co_firstlineno,
      extended_arg_quick.__code__.co_firstlineno + 1,)

ADAPTIVE_WARMUP_DELAY = 2

class DisTestBase(unittest.TestCase):
    "Common utilities for DisTests and TestDisTraceback"

    def strip_addresses(self, text):
        return re.sub(r'\b0x[0-9A-Fa-f]+\b', '0x...', text)

    def assert_exception_table_increasing(self, lines):
        prev_start, prev_end = -1, -1
        count = 0
        for line in lines:
            m = re.match(r'  L(\d+) to L(\d+) -> L\d+ \[\d+\]', line)
            start, end = [int(g) for g in m.groups()]
            self.assertGreaterEqual(end, start)
            self.assertGreaterEqual(start, prev_end)
            prev_start, prev_end = start, end
            count += 1
        return count

    def do_disassembly_compare(self, got, expected):
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

    def do_disassembly_test(self, func, expected, **kwargs):
        self.maxDiff = None
        got = self.get_disassembly(func, depth=0, **kwargs)
        self.do_disassembly_compare(got, expected)
        # Add checks for dis.disco
        if hasattr(func, '__code__'):
            got_disco = io.StringIO()
            with contextlib.redirect_stdout(got_disco):
                dis.disco(func.__code__, **kwargs)
            self.do_disassembly_compare(got_disco.getvalue(), expected)

    def test_opmap(self):
        self.assertEqual(dis.opmap["CACHE"], 0)
        self.assertIn(dis.opmap["LOAD_CONST"], dis.hasconst)
        self.assertIn(dis.opmap["STORE_NAME"], dis.hasname)

    def test_opname(self):
        self.assertEqual(dis.opname[dis.opmap["LOAD_FAST"]], "LOAD_FAST")

    def test_boundaries(self):
        self.assertEqual(dis.opmap["EXTENDED_ARG"], dis.EXTENDED_ARG)

    def test_widths(self):
        long_opcodes = set(['JUMP_BACKWARD_NO_INTERRUPT',
                            'INSTRUMENTED_CALL_FUNCTION_EX'])
        for opcode, opname in enumerate(dis.opname):
            if opname in long_opcodes or opname.startswith("INSTRUMENTED"):
                continue
            with self.subTest(opname=opname):
                width = dis._OPNAME_WIDTH
                if opcode in dis.hasarg:
                    width += 1 + dis._OPARG_WIDTH
                self.assertLessEqual(len(opname), width)

    def test_dis(self):
        self.do_disassembly_test(_f, dis_f)

    def test_dis_with_offsets(self):
        self.do_disassembly_test(_f, dis_f_with_offsets, show_offsets=True)

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

    def test_kw_names(self):
        # Test that value is displayed for keyword argument names:
        self.do_disassembly_test(wrap_func_w_kwargs, dis_kw_names)

    def test_intrinsic_1(self):
        # Test that argrepr is displayed for CALL_INTRINSIC_1
        self.do_disassembly_test("from math import *", dis_intrinsic_1_2)
        self.do_disassembly_test("+a", dis_intrinsic_1_5)
        self.do_disassembly_test("(*a,)", dis_intrinsic_1_6)

    def test_intrinsic_2(self):
        self.assertIn("CALL_INTRINSIC_2         1 (INTRINSIC_PREP_RERAISE_STAR)",
                      self.get_disassembly("try: pass\nexcept* Exception: x"))

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
            del sys.last_exc
        except AttributeError:
            pass
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
            sys.last_exc = e

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

    def test__try_compile_no_context_exc_on_error(self):
        # see gh-102114
        try:
            dis._try_compile(")", "")
        except Exception as e:
            self.assertIsNone(e.__context__)

    @staticmethod
    def code_quicken(f, times=ADAPTIVE_WARMUP_DELAY):
        for _ in range(times):
            f()

    @cpython_only
    @requires_specialization
    def test_super_instructions(self):
        self.code_quicken(lambda: load_test(0, 0))
        got = self.get_disassembly(load_test, adaptive=True)
        self.do_disassembly_compare(got, dis_load_test_quickened_code)

    @cpython_only
    @requires_specialization
    def test_binary_specialize(self):
        binary_op_quicken = """\
  0           RESUME_CHECK             0

  1           LOAD_NAME                0 (a)
              LOAD_NAME                1 (b)
              %s
              RETURN_VALUE
"""
        co_int = compile('a + b', "<int>", "eval")
        self.code_quicken(lambda: exec(co_int, {}, {'a': 1, 'b': 2}))
        got = self.get_disassembly(co_int, adaptive=True)
        self.do_disassembly_compare(got, binary_op_quicken % "BINARY_OP_ADD_INT        0 (+)")

        co_unicode = compile('a + b', "<unicode>", "eval")
        self.code_quicken(lambda: exec(co_unicode, {}, {'a': 'a', 'b': 'b'}))
        got = self.get_disassembly(co_unicode, adaptive=True)
        self.do_disassembly_compare(got, binary_op_quicken % "BINARY_OP_ADD_UNICODE    0 (+)")

        binary_subscr_quicken = """\
  0           RESUME_CHECK             0

  1           LOAD_NAME                0 (a)
              LOAD_CONST               0 (0)
              %s
              RETURN_VALUE
"""
        co_list = compile('a[0]', "<list>", "eval")
        self.code_quicken(lambda: exec(co_list, {}, {'a': [0]}))
        got = self.get_disassembly(co_list, adaptive=True)
        self.do_disassembly_compare(got, binary_subscr_quicken % "BINARY_SUBSCR_LIST_INT")

        co_dict = compile('a[0]', "<dict>", "eval")
        self.code_quicken(lambda: exec(co_dict, {}, {'a': {0: '1'}}))
        got = self.get_disassembly(co_dict, adaptive=True)
        self.do_disassembly_compare(got, binary_subscr_quicken % "BINARY_SUBSCR_DICT")

    @cpython_only
    @requires_specialization
    def test_load_attr_specialize(self):
        load_attr_quicken = """\
  0           RESUME_CHECK             0

  1           LOAD_CONST               0 ('a')
              LOAD_ATTR_SLOT           0 (__class__)
              RETURN_VALUE
"""
        co = compile("'a'.__class__", "", "eval")
        self.code_quicken(lambda: exec(co, {}, {}))
        got = self.get_disassembly(co, adaptive=True)
        self.do_disassembly_compare(got, load_attr_quicken)

    @cpython_only
    @requires_specialization
    def test_call_specialize(self):
        call_quicken = """\
  0           RESUME_CHECK             0

  1           LOAD_NAME                0 (str)
              PUSH_NULL
              LOAD_CONST               0 (1)
              CALL_STR_1               1
              RETURN_VALUE
"""
        co = compile("str(1)", "", "eval")
        self.code_quicken(lambda: exec(co, {}, {}))
        got = self.get_disassembly(co, adaptive=True)
        self.do_disassembly_compare(got, call_quicken)

    @cpython_only
    @requires_specialization
    def test_loop_quicken(self):
        # Loop can trigger a quicken where the loop is located
        self.code_quicken(loop_test, 4)
        got = self.get_disassembly(loop_test, adaptive=True)
        expected = dis_loop_test_quickened_code
        self.do_disassembly_compare(got, expected)

    @cpython_only
    def test_extended_arg_quick(self):
        got = self.get_disassembly(extended_arg_quick)
        self.do_disassembly_compare(got, dis_extended_arg_quick_code)

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
        for instruction in _unroll_caches_as_Instructions(dis.get_instructions(
            f, show_caches=True, adaptive=adaptive
        ), show_caches=True):
            if instruction.opname == "CACHE":
                yield instruction.argrepr

    @cpython_only
    def test_show_caches(self):
        for quickened in (False, True):
            for adaptive in (False, True):
                with self.subTest(f"{quickened=}, {adaptive=}"):
                    if adaptive:
                        pattern = r"^(\w+: \d+)?$"
                    else:
                        pattern = r"^(\w+: 0)?$"
                    caches = list(self.get_cached_values(quickened, adaptive))
                    for cache in caches:
                        self.assertRegex(cache, pattern)
                    total_caches = 21
                    empty_caches = 7
                    self.assertEqual(caches.count(""), empty_caches)
                    self.assertEqual(len(caches), total_caches)

    @cpython_only
    def test_show_currinstr_with_cache(self):
        """
        Make sure that with lasti pointing to CACHE, it still shows the current
        line correctly
        """
        def f():
            print(a)
        # The code above should generate a LOAD_GLOBAL which has CACHE instr after
        # However, this might change in the future. So we explicitly try to find
        # a CACHE entry in the instructions. If we can't do that, fail the test

        for inst in _unroll_caches_as_Instructions(
                dis.get_instructions(f, show_caches=True), show_caches=True):
            if inst.opname == "CACHE":
                op_offset = inst.offset - 2
                cache_offset = inst.offset
                break
            else:
                opname = inst.opname
        else:
            self.fail("Can't find a CACHE entry in the function provided to do the test")

        assem_op = self.get_disassembly(f.__code__, lasti=op_offset, wrapper=False)
        assem_cache = self.get_disassembly(f.__code__, lasti=cache_offset, wrapper=False)

        # Make sure --> exists and points to the correct op
        self.assertRegex(assem_op, fr"--> {opname}")
        # Make sure when lasti points to cache, it shows the same disassembly
        self.assertEqual(assem_op, assem_cache)


class DisWithFileTests(DisTests):

    # Run the tests again, using the file arg instead of print
    def get_disassembly(self, func, lasti=-1, wrapper=True, **kwargs):
        output = io.StringIO()
        if wrapper:
            dis.dis(func, file=output, **kwargs)
        else:
            dis.disassemble(func, lasti, file=output, **kwargs)
        return output.getvalue()


if dis.code_info.__doc__ is None:
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
code_object_f = outer.__code__.co_consts[1]
expected_f_line = code_object_f.co_firstlineno - _line_offset
code_object_inner = code_object_f.co_consts[1]
expected_inner_line = code_object_inner.co_firstlineno - _line_offset
expected_jumpy_line = 1

# The following lines are useful to regenerate the expected results after
# either the fodder is modified or the bytecode generation changes
# After regeneration, update the references to code_object_f and
# code_object_inner before rerunning the tests

def _stringify_instruction(instr):
    # Since line numbers and other offsets change a lot for these
    # test cases, ignore them.
    return f"  {instr._replace(positions=None)!r},"

def _prepare_test_cases():
    ignore = io.StringIO()
    with contextlib.redirect_stdout(ignore):
        f = outer()
        inner = f()
    _instructions_outer = dis.get_instructions(outer, first_line=expected_outer_line)
    _instructions_f = dis.get_instructions(f, first_line=expected_f_line)
    _instructions_inner = dis.get_instructions(inner, first_line=expected_inner_line)
    _instructions_jumpy = dis.get_instructions(jumpy, first_line=expected_jumpy_line)
    result = "\n".join(
        [
            "expected_opinfo_outer = [",
            *map(_stringify_instruction, _instructions_outer),
            "]",
            "",
            "expected_opinfo_f = [",
            *map(_stringify_instruction, _instructions_f),
            "]",
            "",
            "expected_opinfo_inner = [",
            *map(_stringify_instruction, _instructions_inner),
            "]",
            "",
            "expected_opinfo_jumpy = [",
            *map(_stringify_instruction, _instructions_jumpy),
            "]",
        ]
    )
    result = result.replace(repr(repr(code_object_f)), "repr(code_object_f)")
    result = result.replace(repr(code_object_f), "code_object_f")
    result = result.replace(repr(repr(code_object_inner)), "repr(code_object_inner)")
    result = result.replace(repr(code_object_inner), "code_object_inner")
    print(result)

# from test.test_dis import _prepare_test_cases; _prepare_test_cases()

Instruction = dis.Instruction

expected_opinfo_outer = [
  Instruction(opname='MAKE_CELL', opcode=94, arg=0, argval='a', argrepr='a', offset=0, start_offset=0, starts_line=True, line_number=None, label=None, positions=None),
  Instruction(opname='MAKE_CELL', opcode=94, arg=1, argval='b', argrepr='b', offset=2, start_offset=2, starts_line=False, line_number=None, label=None, positions=None),
  Instruction(opname='RESUME', opcode=149, arg=0, argval=0, argrepr='', offset=4, start_offset=4, starts_line=True, line_number=1, label=None, positions=None),
  Instruction(opname='LOAD_CONST', opcode=83, arg=5, argval=(3, 4), argrepr='(3, 4)', offset=6, start_offset=6, starts_line=True, line_number=2, label=None, positions=None),
  Instruction(opname='LOAD_FAST', opcode=85, arg=0, argval='a', argrepr='a', offset=8, start_offset=8, starts_line=False, line_number=2, label=None, positions=None),
  Instruction(opname='LOAD_FAST', opcode=85, arg=1, argval='b', argrepr='b', offset=10, start_offset=10, starts_line=False, line_number=2, label=None, positions=None),
  Instruction(opname='BUILD_TUPLE', opcode=52, arg=2, argval=2, argrepr='', offset=12, start_offset=12, starts_line=False, line_number=2, label=None, positions=None),
  Instruction(opname='LOAD_CONST', opcode=83, arg=1, argval=code_object_f, argrepr=repr(code_object_f), offset=14, start_offset=14, starts_line=False, line_number=2, label=None, positions=None),
  Instruction(opname='MAKE_FUNCTION', opcode=26, arg=None, argval=None, argrepr='', offset=16, start_offset=16, starts_line=False, line_number=2, label=None, positions=None),
  Instruction(opname='SET_FUNCTION_ATTRIBUTE', opcode=106, arg=8, argval=8, argrepr='closure', offset=18, start_offset=18, starts_line=False, line_number=2, label=None, positions=None),
  Instruction(opname='SET_FUNCTION_ATTRIBUTE', opcode=106, arg=1, argval=1, argrepr='defaults', offset=20, start_offset=20, starts_line=False, line_number=2, label=None, positions=None),
  Instruction(opname='STORE_FAST', opcode=110, arg=2, argval='f', argrepr='f', offset=22, start_offset=22, starts_line=False, line_number=2, label=None, positions=None),
  Instruction(opname='LOAD_GLOBAL', opcode=91, arg=1, argval='print', argrepr='print + NULL', offset=24, start_offset=24, starts_line=True, line_number=7, label=None, positions=None),
  Instruction(opname='LOAD_DEREF', opcode=84, arg=0, argval='a', argrepr='a', offset=34, start_offset=34, starts_line=False, line_number=7, label=None, positions=None),
  Instruction(opname='LOAD_DEREF', opcode=84, arg=1, argval='b', argrepr='b', offset=36, start_offset=36, starts_line=False, line_number=7, label=None, positions=None),
  Instruction(opname='LOAD_CONST', opcode=83, arg=2, argval='', argrepr="''", offset=38, start_offset=38, starts_line=False, line_number=7, label=None, positions=None),
  Instruction(opname='LOAD_CONST', opcode=83, arg=3, argval=1, argrepr='1', offset=40, start_offset=40, starts_line=False, line_number=7, label=None, positions=None),
  Instruction(opname='BUILD_LIST', opcode=47, arg=0, argval=0, argrepr='', offset=42, start_offset=42, starts_line=False, line_number=7, label=None, positions=None),
  Instruction(opname='BUILD_MAP', opcode=48, arg=0, argval=0, argrepr='', offset=44, start_offset=44, starts_line=False, line_number=7, label=None, positions=None),
  Instruction(opname='LOAD_CONST', opcode=83, arg=4, argval='Hello world!', argrepr="'Hello world!'", offset=46, start_offset=46, starts_line=False, line_number=7, label=None, positions=None),
  Instruction(opname='CALL', opcode=53, arg=7, argval=7, argrepr='', offset=48, start_offset=48, starts_line=False, line_number=7, label=None, positions=None),
  Instruction(opname='POP_TOP', opcode=32, arg=None, argval=None, argrepr='', offset=56, start_offset=56, starts_line=False, line_number=7, label=None, positions=None),
  Instruction(opname='LOAD_FAST', opcode=85, arg=2, argval='f', argrepr='f', offset=58, start_offset=58, starts_line=True, line_number=8, label=None, positions=None),
  Instruction(opname='RETURN_VALUE', opcode=36, arg=None, argval=None, argrepr='', offset=60, start_offset=60, starts_line=False, line_number=8, label=None, positions=None),
]

expected_opinfo_f = [
  Instruction(opname='COPY_FREE_VARS', opcode=62, arg=2, argval=2, argrepr='', offset=0, start_offset=0, starts_line=True, line_number=None, label=None, positions=None),
  Instruction(opname='MAKE_CELL', opcode=94, arg=0, argval='c', argrepr='c', offset=2, start_offset=2, starts_line=False, line_number=None, label=None, positions=None),
  Instruction(opname='MAKE_CELL', opcode=94, arg=1, argval='d', argrepr='d', offset=4, start_offset=4, starts_line=False, line_number=None, label=None, positions=None),
  Instruction(opname='RESUME', opcode=149, arg=0, argval=0, argrepr='', offset=6, start_offset=6, starts_line=True, line_number=2, label=None, positions=None),
  Instruction(opname='LOAD_CONST', opcode=83, arg=2, argval=(5, 6), argrepr='(5, 6)', offset=8, start_offset=8, starts_line=True, line_number=3, label=None, positions=None),
  Instruction(opname='LOAD_FAST', opcode=85, arg=3, argval='a', argrepr='a', offset=10, start_offset=10, starts_line=False, line_number=3, label=None, positions=None),
  Instruction(opname='LOAD_FAST', opcode=85, arg=4, argval='b', argrepr='b', offset=12, start_offset=12, starts_line=False, line_number=3, label=None, positions=None),
  Instruction(opname='LOAD_FAST', opcode=85, arg=0, argval='c', argrepr='c', offset=14, start_offset=14, starts_line=False, line_number=3, label=None, positions=None),
  Instruction(opname='LOAD_FAST', opcode=85, arg=1, argval='d', argrepr='d', offset=16, start_offset=16, starts_line=False, line_number=3, label=None, positions=None),
  Instruction(opname='BUILD_TUPLE', opcode=52, arg=4, argval=4, argrepr='', offset=18, start_offset=18, starts_line=False, line_number=3, label=None, positions=None),
  Instruction(opname='LOAD_CONST', opcode=83, arg=1, argval=code_object_inner, argrepr=repr(code_object_inner), offset=20, start_offset=20, starts_line=False, line_number=3, label=None, positions=None),
  Instruction(opname='MAKE_FUNCTION', opcode=26, arg=None, argval=None, argrepr='', offset=22, start_offset=22, starts_line=False, line_number=3, label=None, positions=None),
  Instruction(opname='SET_FUNCTION_ATTRIBUTE', opcode=106, arg=8, argval=8, argrepr='closure', offset=24, start_offset=24, starts_line=False, line_number=3, label=None, positions=None),
  Instruction(opname='SET_FUNCTION_ATTRIBUTE', opcode=106, arg=1, argval=1, argrepr='defaults', offset=26, start_offset=26, starts_line=False, line_number=3, label=None, positions=None),
  Instruction(opname='STORE_FAST', opcode=110, arg=2, argval='inner', argrepr='inner', offset=28, start_offset=28, starts_line=False, line_number=3, label=None, positions=None),
  Instruction(opname='LOAD_GLOBAL', opcode=91, arg=1, argval='print', argrepr='print + NULL', offset=30, start_offset=30, starts_line=True, line_number=5, label=None, positions=None),
  Instruction(opname='LOAD_DEREF', opcode=84, arg=3, argval='a', argrepr='a', offset=40, start_offset=40, starts_line=False, line_number=5, label=None, positions=None),
  Instruction(opname='LOAD_DEREF', opcode=84, arg=4, argval='b', argrepr='b', offset=42, start_offset=42, starts_line=False, line_number=5, label=None, positions=None),
  Instruction(opname='LOAD_DEREF', opcode=84, arg=0, argval='c', argrepr='c', offset=44, start_offset=44, starts_line=False, line_number=5, label=None, positions=None),
  Instruction(opname='LOAD_DEREF', opcode=84, arg=1, argval='d', argrepr='d', offset=46, start_offset=46, starts_line=False, line_number=5, label=None, positions=None),
  Instruction(opname='CALL', opcode=53, arg=4, argval=4, argrepr='', offset=48, start_offset=48, starts_line=False, line_number=5, label=None, positions=None),
  Instruction(opname='POP_TOP', opcode=32, arg=None, argval=None, argrepr='', offset=56, start_offset=56, starts_line=False, line_number=5, label=None, positions=None),
  Instruction(opname='LOAD_FAST', opcode=85, arg=2, argval='inner', argrepr='inner', offset=58, start_offset=58, starts_line=True, line_number=6, label=None, positions=None),
  Instruction(opname='RETURN_VALUE', opcode=36, arg=None, argval=None, argrepr='', offset=60, start_offset=60, starts_line=False, line_number=6, label=None, positions=None),
]

expected_opinfo_inner = [
  Instruction(opname='COPY_FREE_VARS', opcode=62, arg=4, argval=4, argrepr='', offset=0, start_offset=0, starts_line=True, line_number=None, label=None, positions=None),
  Instruction(opname='RESUME', opcode=149, arg=0, argval=0, argrepr='', offset=2, start_offset=2, starts_line=True, line_number=3, label=None, positions=None),
  Instruction(opname='LOAD_GLOBAL', opcode=91, arg=1, argval='print', argrepr='print + NULL', offset=4, start_offset=4, starts_line=True, line_number=4, label=None, positions=None),
  Instruction(opname='LOAD_DEREF', opcode=84, arg=2, argval='a', argrepr='a', offset=14, start_offset=14, starts_line=False, line_number=4, label=None, positions=None),
  Instruction(opname='LOAD_DEREF', opcode=84, arg=3, argval='b', argrepr='b', offset=16, start_offset=16, starts_line=False, line_number=4, label=None, positions=None),
  Instruction(opname='LOAD_DEREF', opcode=84, arg=4, argval='c', argrepr='c', offset=18, start_offset=18, starts_line=False, line_number=4, label=None, positions=None),
  Instruction(opname='LOAD_DEREF', opcode=84, arg=5, argval='d', argrepr='d', offset=20, start_offset=20, starts_line=False, line_number=4, label=None, positions=None),
  Instruction(opname='LOAD_FAST_LOAD_FAST', opcode=88, arg=1, argval=('e', 'f'), argrepr='e, f', offset=22, start_offset=22, starts_line=False, line_number=4, label=None, positions=None),
  Instruction(opname='CALL', opcode=53, arg=6, argval=6, argrepr='', offset=24, start_offset=24, starts_line=False, line_number=4, label=None, positions=None),
  Instruction(opname='POP_TOP', opcode=32, arg=None, argval=None, argrepr='', offset=32, start_offset=32, starts_line=False, line_number=4, label=None, positions=None),
  Instruction(opname='RETURN_CONST', opcode=103, arg=0, argval=None, argrepr='None', offset=34, start_offset=34, starts_line=False, line_number=4, label=None, positions=None),
]

expected_opinfo_jumpy = [
  Instruction(opname='RESUME', opcode=149, arg=0, argval=0, argrepr='', offset=0, start_offset=0, starts_line=True, line_number=1, label=None, positions=None, cache_info=None),
  Instruction(opname='LOAD_GLOBAL', opcode=91, arg=1, argval='range', argrepr='range + NULL', offset=2, start_offset=2, starts_line=True, line_number=3, label=None, positions=None, cache_info=[('counter', 1, b'\x00\x00'), ('index', 1, b'\x00\x00'), ('module_keys_version', 1, b'\x00\x00'), ('builtin_keys_version', 1, b'\x00\x00')]),
  Instruction(opname='LOAD_CONST', opcode=83, arg=1, argval=10, argrepr='10', offset=12, start_offset=12, starts_line=False, line_number=3, label=None, positions=None, cache_info=None),
  Instruction(opname='CALL', opcode=53, arg=1, argval=1, argrepr='', offset=14, start_offset=14, starts_line=False, line_number=3, label=None, positions=None, cache_info=[('counter', 1, b'\x00\x00'), ('func_version', 2, b'\x00\x00\x00\x00')]),
  Instruction(opname='GET_ITER', opcode=19, arg=None, argval=None, argrepr='', offset=22, start_offset=22, starts_line=False, line_number=3, label=None, positions=None, cache_info=None),
  Instruction(opname='FOR_ITER', opcode=72, arg=30, argval=88, argrepr='to L4', offset=24, start_offset=24, starts_line=False, line_number=3, label=1, positions=None, cache_info=[('counter', 1, b'\x00\x00')]),
  Instruction(opname='STORE_FAST', opcode=110, arg=0, argval='i', argrepr='i', offset=28, start_offset=28, starts_line=False, line_number=3, label=None, positions=None, cache_info=None),
  Instruction(opname='LOAD_GLOBAL', opcode=91, arg=3, argval='print', argrepr='print + NULL', offset=30, start_offset=30, starts_line=True, line_number=4, label=None, positions=None, cache_info=[('counter', 1, b'\x00\x00'), ('index', 1, b'\x00\x00'), ('module_keys_version', 1, b'\x00\x00'), ('builtin_keys_version', 1, b'\x00\x00')]),
  Instruction(opname='LOAD_FAST', opcode=85, arg=0, argval='i', argrepr='i', offset=40, start_offset=40, starts_line=False, line_number=4, label=None, positions=None, cache_info=None),
  Instruction(opname='CALL', opcode=53, arg=1, argval=1, argrepr='', offset=42, start_offset=42, starts_line=False, line_number=4, label=None, positions=None, cache_info=[('counter', 1, b'\x00\x00'), ('func_version', 2, b'\x00\x00\x00\x00')]),
  Instruction(opname='POP_TOP', opcode=32, arg=None, argval=None, argrepr='', offset=50, start_offset=50, starts_line=False, line_number=4, label=None, positions=None, cache_info=None),
  Instruction(opname='LOAD_FAST', opcode=85, arg=0, argval='i', argrepr='i', offset=52, start_offset=52, starts_line=True, line_number=5, label=None, positions=None, cache_info=None),
  Instruction(opname='LOAD_CONST', opcode=83, arg=2, argval=4, argrepr='4', offset=54, start_offset=54, starts_line=False, line_number=5, label=None, positions=None, cache_info=None),
  Instruction(opname='COMPARE_OP', opcode=58, arg=18, argval='<', argrepr='bool(<)', offset=56, start_offset=56, starts_line=False, line_number=5, label=None, positions=None, cache_info=[('counter', 1, b'\x00\x00')]),
  Instruction(opname='POP_JUMP_IF_FALSE', opcode=97, arg=2, argval=68, argrepr='to L2', offset=60, start_offset=60, starts_line=False, line_number=5, label=None, positions=None, cache_info=[('counter', 1, b'\x00\x00')]),
  Instruction(opname='JUMP_BACKWARD', opcode=77, arg=22, argval=24, argrepr='to L1', offset=64, start_offset=64, starts_line=True, line_number=6, label=None, positions=None, cache_info=[('counter', 1, b'\x00\x00')]),
  Instruction(opname='LOAD_FAST', opcode=85, arg=0, argval='i', argrepr='i', offset=68, start_offset=68, starts_line=True, line_number=7, label=2, positions=None, cache_info=None),
  Instruction(opname='LOAD_CONST', opcode=83, arg=3, argval=6, argrepr='6', offset=70, start_offset=70, starts_line=False, line_number=7, label=None, positions=None, cache_info=None),
  Instruction(opname='COMPARE_OP', opcode=58, arg=148, argval='>', argrepr='bool(>)', offset=72, start_offset=72, starts_line=False, line_number=7, label=None, positions=None, cache_info=[('counter', 1, b'\x00\x00')]),
  Instruction(opname='POP_JUMP_IF_TRUE', opcode=100, arg=2, argval=84, argrepr='to L3', offset=76, start_offset=76, starts_line=False, line_number=7, label=None, positions=None, cache_info=[('counter', 1, b'\x00\x00')]),
  Instruction(opname='JUMP_BACKWARD', opcode=77, arg=30, argval=24, argrepr='to L1', offset=80, start_offset=80, starts_line=False, line_number=7, label=None, positions=None, cache_info=[('counter', 1, b'\x00\x00')]),
  Instruction(opname='POP_TOP', opcode=32, arg=None, argval=None, argrepr='', offset=84, start_offset=84, starts_line=True, line_number=8, label=3, positions=None, cache_info=None),
  Instruction(opname='JUMP_FORWARD', opcode=79, arg=13, argval=114, argrepr='to L5', offset=86, start_offset=86, starts_line=False, line_number=8, label=None, positions=None, cache_info=None),
  Instruction(opname='END_FOR', opcode=11, arg=None, argval=None, argrepr='', offset=88, start_offset=88, starts_line=True, line_number=3, label=4, positions=None, cache_info=None),
  Instruction(opname='POP_TOP', opcode=32, arg=None, argval=None, argrepr='', offset=90, start_offset=90, starts_line=False, line_number=3, label=None, positions=None, cache_info=None),
  Instruction(opname='LOAD_GLOBAL', opcode=91, arg=3, argval='print', argrepr='print + NULL', offset=92, start_offset=92, starts_line=True, line_number=10, label=None, positions=None, cache_info=[('counter', 1, b'\x00\x00'), ('index', 1, b'\x00\x00'), ('module_keys_version', 1, b'\x00\x00'), ('builtin_keys_version', 1, b'\x00\x00')]),
  Instruction(opname='LOAD_CONST', opcode=83, arg=4, argval='I can haz else clause?', argrepr="'I can haz else clause?'", offset=102, start_offset=102, starts_line=False, line_number=10, label=None, positions=None, cache_info=None),
  Instruction(opname='CALL', opcode=53, arg=1, argval=1, argrepr='', offset=104, start_offset=104, starts_line=False, line_number=10, label=None, positions=None, cache_info=[('counter', 1, b'\x00\x00'), ('func_version', 2, b'\x00\x00\x00\x00')]),
  Instruction(opname='POP_TOP', opcode=32, arg=None, argval=None, argrepr='', offset=112, start_offset=112, starts_line=False, line_number=10, label=None, positions=None, cache_info=None),
  Instruction(opname='LOAD_FAST_CHECK', opcode=87, arg=0, argval='i', argrepr='i', offset=114, start_offset=114, starts_line=True, line_number=11, label=5, positions=None, cache_info=None),
  Instruction(opname='TO_BOOL', opcode=40, arg=None, argval=None, argrepr='', offset=116, start_offset=116, starts_line=False, line_number=11, label=None, positions=None, cache_info=[('counter', 1, b'\x00\x00'), ('version', 2, b'\x00\x00\x00\x00')]),
  Instruction(opname='POP_JUMP_IF_FALSE', opcode=97, arg=40, argval=208, argrepr='to L9', offset=124, start_offset=124, starts_line=False, line_number=11, label=None, positions=None, cache_info=[('counter', 1, b'\x00\x00')]),
  Instruction(opname='LOAD_GLOBAL', opcode=91, arg=3, argval='print', argrepr='print + NULL', offset=128, start_offset=128, starts_line=True, line_number=12, label=6, positions=None, cache_info=[('counter', 1, b'\x00\x00'), ('index', 1, b'\x00\x00'), ('module_keys_version', 1, b'\x00\x00'), ('builtin_keys_version', 1, b'\x00\x00')]),
  Instruction(opname='LOAD_FAST', opcode=85, arg=0, argval='i', argrepr='i', offset=138, start_offset=138, starts_line=False, line_number=12, label=None, positions=None, cache_info=None),
  Instruction(opname='CALL', opcode=53, arg=1, argval=1, argrepr='', offset=140, start_offset=140, starts_line=False, line_number=12, label=None, positions=None, cache_info=[('counter', 1, b'\x00\x00'), ('func_version', 2, b'\x00\x00\x00\x00')]),
  Instruction(opname='POP_TOP', opcode=32, arg=None, argval=None, argrepr='', offset=148, start_offset=148, starts_line=False, line_number=12, label=None, positions=None, cache_info=None),
  Instruction(opname='LOAD_FAST', opcode=85, arg=0, argval='i', argrepr='i', offset=150, start_offset=150, starts_line=True, line_number=13, label=None, positions=None, cache_info=None),
  Instruction(opname='LOAD_CONST', opcode=83, arg=5, argval=1, argrepr='1', offset=152, start_offset=152, starts_line=False, line_number=13, label=None, positions=None, cache_info=None),
  Instruction(opname='BINARY_OP', opcode=45, arg=23, argval=23, argrepr='-=', offset=154, start_offset=154, starts_line=False, line_number=13, label=None, positions=None, cache_info=[('counter', 1, b'\x00\x00')]),
  Instruction(opname='STORE_FAST', opcode=110, arg=0, argval='i', argrepr='i', offset=158, start_offset=158, starts_line=False, line_number=13, label=None, positions=None, cache_info=None),
  Instruction(opname='LOAD_FAST', opcode=85, arg=0, argval='i', argrepr='i', offset=160, start_offset=160, starts_line=True, line_number=14, label=None, positions=None, cache_info=None),
  Instruction(opname='LOAD_CONST', opcode=83, arg=3, argval=6, argrepr='6', offset=162, start_offset=162, starts_line=False, line_number=14, label=None, positions=None, cache_info=None),
  Instruction(opname='COMPARE_OP', opcode=58, arg=148, argval='>', argrepr='bool(>)', offset=164, start_offset=164, starts_line=False, line_number=14, label=None, positions=None, cache_info=[('counter', 1, b'\x00\x00')]),
  Instruction(opname='POP_JUMP_IF_FALSE', opcode=97, arg=2, argval=176, argrepr='to L7', offset=168, start_offset=168, starts_line=False, line_number=14, label=None, positions=None, cache_info=[('counter', 1, b'\x00\x00')]),
  Instruction(opname='JUMP_BACKWARD', opcode=77, arg=31, argval=114, argrepr='to L5', offset=172, start_offset=172, starts_line=True, line_number=15, label=None, positions=None, cache_info=[('counter', 1, b'\x00\x00')]),
  Instruction(opname='LOAD_FAST', opcode=85, arg=0, argval='i', argrepr='i', offset=176, start_offset=176, starts_line=True, line_number=16, label=7, positions=None, cache_info=None),
  Instruction(opname='LOAD_CONST', opcode=83, arg=2, argval=4, argrepr='4', offset=178, start_offset=178, starts_line=False, line_number=16, label=None, positions=None, cache_info=None),
  Instruction(opname='COMPARE_OP', opcode=58, arg=18, argval='<', argrepr='bool(<)', offset=180, start_offset=180, starts_line=False, line_number=16, label=None, positions=None, cache_info=[('counter', 1, b'\x00\x00')]),
  Instruction(opname='POP_JUMP_IF_FALSE', opcode=97, arg=1, argval=190, argrepr='to L8', offset=184, start_offset=184, starts_line=False, line_number=16, label=None, positions=None, cache_info=[('counter', 1, b'\x00\x00')]),
  Instruction(opname='JUMP_FORWARD', opcode=79, arg=20, argval=230, argrepr='to L10', offset=188, start_offset=188, starts_line=True, line_number=17, label=None, positions=None, cache_info=None),
  Instruction(opname='LOAD_FAST', opcode=85, arg=0, argval='i', argrepr='i', offset=190, start_offset=190, starts_line=True, line_number=11, label=8, positions=None, cache_info=None),
  Instruction(opname='TO_BOOL', opcode=40, arg=None, argval=None, argrepr='', offset=192, start_offset=192, starts_line=False, line_number=11, label=None, positions=None, cache_info=[('counter', 1, b'\x00\x00'), ('version', 2, b'\x00\x00\x00\x00')]),
  Instruction(opname='POP_JUMP_IF_FALSE', opcode=97, arg=2, argval=208, argrepr='to L9', offset=200, start_offset=200, starts_line=False, line_number=11, label=None, positions=None, cache_info=[('counter', 1, b'\x00\x00')]),
  Instruction(opname='JUMP_BACKWARD', opcode=77, arg=40, argval=128, argrepr='to L6', offset=204, start_offset=204, starts_line=False, line_number=11, label=None, positions=None, cache_info=[('counter', 1, b'\x00\x00')]),
  Instruction(opname='LOAD_GLOBAL', opcode=91, arg=3, argval='print', argrepr='print + NULL', offset=208, start_offset=208, starts_line=True, line_number=19, label=9, positions=None, cache_info=[('counter', 1, b'\x00\x00'), ('index', 1, b'\x00\x00'), ('module_keys_version', 1, b'\x00\x00'), ('builtin_keys_version', 1, b'\x00\x00')]),
  Instruction(opname='LOAD_CONST', opcode=83, arg=6, argval='Who let lolcatz into this test suite?', argrepr="'Who let lolcatz into this test suite?'", offset=218, start_offset=218, starts_line=False, line_number=19, label=None, positions=None, cache_info=None),
  Instruction(opname='CALL', opcode=53, arg=1, argval=1, argrepr='', offset=220, start_offset=220, starts_line=False, line_number=19, label=None, positions=None, cache_info=[('counter', 1, b'\x00\x00'), ('func_version', 2, b'\x00\x00\x00\x00')]),
  Instruction(opname='POP_TOP', opcode=32, arg=None, argval=None, argrepr='', offset=228, start_offset=228, starts_line=False, line_number=19, label=None, positions=None, cache_info=None),
  Instruction(opname='NOP', opcode=30, arg=None, argval=None, argrepr='', offset=230, start_offset=230, starts_line=True, line_number=20, label=10, positions=None, cache_info=None),
  Instruction(opname='LOAD_CONST', opcode=83, arg=5, argval=1, argrepr='1', offset=232, start_offset=232, starts_line=True, line_number=21, label=None, positions=None, cache_info=None),
  Instruction(opname='LOAD_CONST', opcode=83, arg=7, argval=0, argrepr='0', offset=234, start_offset=234, starts_line=False, line_number=21, label=None, positions=None, cache_info=None),
  Instruction(opname='BINARY_OP', opcode=45, arg=11, argval=11, argrepr='/', offset=236, start_offset=236, starts_line=False, line_number=21, label=None, positions=None, cache_info=[('counter', 1, b'\x00\x00')]),
  Instruction(opname='POP_TOP', opcode=32, arg=None, argval=None, argrepr='', offset=240, start_offset=240, starts_line=False, line_number=21, label=None, positions=None, cache_info=None),
  Instruction(opname='LOAD_FAST', opcode=85, arg=0, argval='i', argrepr='i', offset=242, start_offset=242, starts_line=True, line_number=25, label=None, positions=None, cache_info=None),
  Instruction(opname='BEFORE_WITH', opcode=2, arg=None, argval=None, argrepr='', offset=244, start_offset=244, starts_line=False, line_number=25, label=None, positions=None, cache_info=None),
  Instruction(opname='STORE_FAST', opcode=110, arg=1, argval='dodgy', argrepr='dodgy', offset=246, start_offset=246, starts_line=False, line_number=25, label=None, positions=None, cache_info=None),
  Instruction(opname='LOAD_GLOBAL', opcode=91, arg=3, argval='print', argrepr='print + NULL', offset=248, start_offset=248, starts_line=True, line_number=26, label=None, positions=None, cache_info=[('counter', 1, b'\x00\x00'), ('index', 1, b'\x00\x00'), ('module_keys_version', 1, b'\x00\x00'), ('builtin_keys_version', 1, b'\x00\x00')]),
  Instruction(opname='LOAD_CONST', opcode=83, arg=8, argval='Never reach this', argrepr="'Never reach this'", offset=258, start_offset=258, starts_line=False, line_number=26, label=None, positions=None, cache_info=None),
  Instruction(opname='CALL', opcode=53, arg=1, argval=1, argrepr='', offset=260, start_offset=260, starts_line=False, line_number=26, label=None, positions=None, cache_info=[('counter', 1, b'\x00\x00'), ('func_version', 2, b'\x00\x00\x00\x00')]),
  Instruction(opname='POP_TOP', opcode=32, arg=None, argval=None, argrepr='', offset=268, start_offset=268, starts_line=False, line_number=26, label=None, positions=None, cache_info=None),
  Instruction(opname='LOAD_CONST', opcode=83, arg=0, argval=None, argrepr='None', offset=270, start_offset=270, starts_line=True, line_number=25, label=None, positions=None, cache_info=None),
  Instruction(opname='LOAD_CONST', opcode=83, arg=0, argval=None, argrepr='None', offset=272, start_offset=272, starts_line=False, line_number=25, label=None, positions=None, cache_info=None),
  Instruction(opname='LOAD_CONST', opcode=83, arg=0, argval=None, argrepr='None', offset=274, start_offset=274, starts_line=False, line_number=25, label=None, positions=None, cache_info=None),
  Instruction(opname='CALL', opcode=53, arg=2, argval=2, argrepr='', offset=276, start_offset=276, starts_line=False, line_number=25, label=None, positions=None, cache_info=[('counter', 1, b'\x00\x00'), ('func_version', 2, b'\x00\x00\x00\x00')]),
  Instruction(opname='POP_TOP', opcode=32, arg=None, argval=None, argrepr='', offset=284, start_offset=284, starts_line=False, line_number=25, label=None, positions=None, cache_info=None),
  Instruction(opname='LOAD_GLOBAL', opcode=91, arg=3, argval='print', argrepr='print + NULL', offset=286, start_offset=286, starts_line=True, line_number=28, label=11, positions=None, cache_info=[('counter', 1, b'\x00\x00'), ('index', 1, b'\x00\x00'), ('module_keys_version', 1, b'\x00\x00'), ('builtin_keys_version', 1, b'\x00\x00')]),
  Instruction(opname='LOAD_CONST', opcode=83, arg=10, argval="OK, now we're done", argrepr='"OK, now we\'re done"', offset=296, start_offset=296, starts_line=False, line_number=28, label=None, positions=None, cache_info=None),
  Instruction(opname='CALL', opcode=53, arg=1, argval=1, argrepr='', offset=298, start_offset=298, starts_line=False, line_number=28, label=None, positions=None, cache_info=[('counter', 1, b'\x00\x00'), ('func_version', 2, b'\x00\x00\x00\x00')]),
  Instruction(opname='POP_TOP', opcode=32, arg=None, argval=None, argrepr='', offset=306, start_offset=306, starts_line=False, line_number=28, label=None, positions=None, cache_info=None),
  Instruction(opname='RETURN_CONST', opcode=103, arg=0, argval=None, argrepr='None', offset=308, start_offset=308, starts_line=False, line_number=28, label=None, positions=None, cache_info=None),
  Instruction(opname='PUSH_EXC_INFO', opcode=33, arg=None, argval=None, argrepr='', offset=310, start_offset=310, starts_line=True, line_number=25, label=None, positions=None, cache_info=None),
  Instruction(opname='WITH_EXCEPT_START', opcode=44, arg=None, argval=None, argrepr='', offset=312, start_offset=312, starts_line=False, line_number=25, label=None, positions=None, cache_info=None),
  Instruction(opname='TO_BOOL', opcode=40, arg=None, argval=None, argrepr='', offset=314, start_offset=314, starts_line=False, line_number=25, label=None, positions=None, cache_info=[('counter', 1, b'\x00\x00'), ('version', 2, b'\x00\x00\x00\x00')]),
  Instruction(opname='POP_JUMP_IF_TRUE', opcode=100, arg=1, argval=328, argrepr='to L12', offset=322, start_offset=322, starts_line=False, line_number=25, label=None, positions=None, cache_info=[('counter', 1, b'\x00\x00')]),
  Instruction(opname='RERAISE', opcode=102, arg=2, argval=2, argrepr='', offset=326, start_offset=326, starts_line=False, line_number=25, label=None, positions=None, cache_info=None),
  Instruction(opname='POP_TOP', opcode=32, arg=None, argval=None, argrepr='', offset=328, start_offset=328, starts_line=False, line_number=25, label=12, positions=None, cache_info=None),
  Instruction(opname='POP_EXCEPT', opcode=31, arg=None, argval=None, argrepr='', offset=330, start_offset=330, starts_line=False, line_number=25, label=None, positions=None, cache_info=None),
  Instruction(opname='POP_TOP', opcode=32, arg=None, argval=None, argrepr='', offset=332, start_offset=332, starts_line=False, line_number=25, label=None, positions=None, cache_info=None),
  Instruction(opname='POP_TOP', opcode=32, arg=None, argval=None, argrepr='', offset=334, start_offset=334, starts_line=False, line_number=25, label=None, positions=None, cache_info=None),
  Instruction(opname='JUMP_BACKWARD_NO_INTERRUPT', opcode=78, arg=26, argval=286, argrepr='to L11', offset=336, start_offset=336, starts_line=False, line_number=25, label=None, positions=None, cache_info=None),
  Instruction(opname='COPY', opcode=61, arg=3, argval=3, argrepr='', offset=338, start_offset=338, starts_line=True, line_number=None, label=None, positions=None, cache_info=None),
  Instruction(opname='POP_EXCEPT', opcode=31, arg=None, argval=None, argrepr='', offset=340, start_offset=340, starts_line=False, line_number=None, label=None, positions=None, cache_info=None),
  Instruction(opname='RERAISE', opcode=102, arg=1, argval=1, argrepr='', offset=342, start_offset=342, starts_line=False, line_number=None, label=None, positions=None, cache_info=None),
  Instruction(opname='PUSH_EXC_INFO', opcode=33, arg=None, argval=None, argrepr='', offset=344, start_offset=344, starts_line=False, line_number=None, label=None, positions=None, cache_info=None),
  Instruction(opname='LOAD_GLOBAL', opcode=91, arg=4, argval='ZeroDivisionError', argrepr='ZeroDivisionError', offset=346, start_offset=346, starts_line=True, line_number=22, label=None, positions=None, cache_info=[('counter', 1, b'\x00\x00'), ('index', 1, b'\x00\x00'), ('module_keys_version', 1, b'\x00\x00'), ('builtin_keys_version', 1, b'\x00\x00')]),
  Instruction(opname='CHECK_EXC_MATCH', opcode=7, arg=None, argval=None, argrepr='', offset=356, start_offset=356, starts_line=False, line_number=22, label=None, positions=None, cache_info=None),
  Instruction(opname='POP_JUMP_IF_FALSE', opcode=97, arg=14, argval=390, argrepr='to L13', offset=358, start_offset=358, starts_line=False, line_number=22, label=None, positions=None, cache_info=[('counter', 1, b'\x00\x00')]),
  Instruction(opname='POP_TOP', opcode=32, arg=None, argval=None, argrepr='', offset=362, start_offset=362, starts_line=False, line_number=22, label=None, positions=None, cache_info=None),
  Instruction(opname='LOAD_GLOBAL', opcode=91, arg=3, argval='print', argrepr='print + NULL', offset=364, start_offset=364, starts_line=True, line_number=23, label=None, positions=None, cache_info=[('counter', 1, b'\x00\x00'), ('index', 1, b'\x00\x00'), ('module_keys_version', 1, b'\x00\x00'), ('builtin_keys_version', 1, b'\x00\x00')]),
  Instruction(opname='LOAD_CONST', opcode=83, arg=9, argval='Here we go, here we go, here we go...', argrepr="'Here we go, here we go, here we go...'", offset=374, start_offset=374, starts_line=False, line_number=23, label=None, positions=None, cache_info=None),
  Instruction(opname='CALL', opcode=53, arg=1, argval=1, argrepr='', offset=376, start_offset=376, starts_line=False, line_number=23, label=None, positions=None, cache_info=[('counter', 1, b'\x00\x00'), ('func_version', 2, b'\x00\x00\x00\x00')]),
  Instruction(opname='POP_TOP', opcode=32, arg=None, argval=None, argrepr='', offset=384, start_offset=384, starts_line=False, line_number=23, label=None, positions=None, cache_info=None),
  Instruction(opname='POP_EXCEPT', opcode=31, arg=None, argval=None, argrepr='', offset=386, start_offset=386, starts_line=False, line_number=23, label=None, positions=None, cache_info=None),
  Instruction(opname='JUMP_BACKWARD_NO_INTERRUPT', opcode=78, arg=52, argval=286, argrepr='to L11', offset=388, start_offset=388, starts_line=False, line_number=23, label=None, positions=None, cache_info=None),
  Instruction(opname='RERAISE', opcode=102, arg=0, argval=0, argrepr='', offset=390, start_offset=390, starts_line=True, line_number=22, label=13, positions=None, cache_info=None),
  Instruction(opname='COPY', opcode=61, arg=3, argval=3, argrepr='', offset=392, start_offset=392, starts_line=True, line_number=None, label=None, positions=None, cache_info=None),
  Instruction(opname='POP_EXCEPT', opcode=31, arg=None, argval=None, argrepr='', offset=394, start_offset=394, starts_line=False, line_number=None, label=None, positions=None, cache_info=None),
  Instruction(opname='RERAISE', opcode=102, arg=1, argval=1, argrepr='', offset=396, start_offset=396, starts_line=False, line_number=None, label=None, positions=None, cache_info=None),
  Instruction(opname='PUSH_EXC_INFO', opcode=33, arg=None, argval=None, argrepr='', offset=398, start_offset=398, starts_line=False, line_number=None, label=None, positions=None, cache_info=None),
  Instruction(opname='LOAD_GLOBAL', opcode=91, arg=3, argval='print', argrepr='print + NULL', offset=400, start_offset=400, starts_line=True, line_number=28, label=None, positions=None, cache_info=[('counter', 1, b'\x00\x00'), ('index', 1, b'\x00\x00'), ('module_keys_version', 1, b'\x00\x00'), ('builtin_keys_version', 1, b'\x00\x00')]),
  Instruction(opname='LOAD_CONST', opcode=83, arg=10, argval="OK, now we're done", argrepr='"OK, now we\'re done"', offset=410, start_offset=410, starts_line=False, line_number=28, label=None, positions=None, cache_info=None),
  Instruction(opname='CALL', opcode=53, arg=1, argval=1, argrepr='', offset=412, start_offset=412, starts_line=False, line_number=28, label=None, positions=None, cache_info=[('counter', 1, b'\x00\x00'), ('func_version', 2, b'\x00\x00\x00\x00')]),
  Instruction(opname='POP_TOP', opcode=32, arg=None, argval=None, argrepr='', offset=420, start_offset=420, starts_line=False, line_number=28, label=None, positions=None, cache_info=None),
  Instruction(opname='RERAISE', opcode=102, arg=0, argval=0, argrepr='', offset=422, start_offset=422, starts_line=False, line_number=28, label=None, positions=None, cache_info=None),
  Instruction(opname='COPY', opcode=61, arg=3, argval=3, argrepr='', offset=424, start_offset=424, starts_line=True, line_number=None, label=None, positions=None, cache_info=None),
  Instruction(opname='POP_EXCEPT', opcode=31, arg=None, argval=None, argrepr='', offset=426, start_offset=426, starts_line=False, line_number=None, label=None, positions=None, cache_info=None),
  Instruction(opname='RERAISE', opcode=102, arg=1, argval=1, argrepr='', offset=428, start_offset=428, starts_line=False, line_number=None, label=None, positions=None, cache_info=None),
]

# One last piece of inspect fodder to check the default line number handling
def simple(): pass
expected_opinfo_simple = [
  Instruction(opname='RESUME', opcode=149, arg=0, argval=0, argrepr='', offset=0, start_offset=0, starts_line=True, line_number=simple.__code__.co_firstlineno, label=None, positions=None),
  Instruction(opname='RETURN_CONST', opcode=103, arg=0, argval=None, argrepr='None', offset=2, start_offset=2, starts_line=False, line_number=simple.__code__.co_firstlineno, label=None),
]


class InstructionTestCase(BytecodeTestCase):

    def assertInstructionsEqual(self, instrs_1, instrs_2, /):
        instrs_1 = [instr_1._replace(positions=None, cache_info=None) for instr_1 in instrs_1]
        instrs_2 = [instr_2._replace(positions=None, cache_info=None) for instr_2 in instrs_2]
        self.assertEqual(instrs_1, instrs_2)

class InstructionTests(InstructionTestCase):

    def __init__(self, *args):
        super().__init__(*args)
        self.maxDiff = None

    def test_instruction_str(self):
        # smoke test for __str__
        instrs = dis.get_instructions(simple)
        for instr in instrs:
            str(instr)

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
                        None if instruction.positions is None else (
                            instruction.positions.lineno,
                            instruction.positions.end_lineno,
                            instruction.positions.col_offset,
                            instruction.positions.end_col_offset,
                        )
                        for instruction in _unroll_caches_as_Instructions(dis.get_instructions(
                            code, adaptive=adaptive, show_caches=show_caches
                        ), show_caches=show_caches)
                    ]
                    self.assertEqual(co_positions, dis_positions)

    def test_oparg_alias(self):
        instruction = Instruction(opname="NOP", opcode=dis.opmap["NOP"], arg=None, argval=None,
                                  argrepr='', offset=10, start_offset=10, starts_line=True, line_number=1, label=None,
                                  positions=None)
        self.assertEqual(instruction.arg, instruction.oparg)

    def test_show_caches_with_label(self):
        def f(x, y, z):
            if x:
                res = y
            else:
                res = z
            return res

        output = io.StringIO()
        dis.dis(f.__code__, file=output, show_caches=True)
        self.assertIn("L1:", output.getvalue())

    def test_baseopname_and_baseopcode(self):
        # Standard instructions
        for name, code in dis.opmap.items():
            instruction = Instruction(opname=name, opcode=code, arg=None, argval=None, argrepr='', offset=0,
                                      start_offset=0, starts_line=True, line_number=1, label=None, positions=None)
            baseopname = instruction.baseopname
            baseopcode = instruction.baseopcode
            self.assertIsNotNone(baseopname)
            self.assertIsNotNone(baseopcode)
            self.assertEqual(name, baseopname)
            self.assertEqual(code, baseopcode)

        # Specialized instructions
        for name in opcode._specialized_opmap:
            instruction = Instruction(opname=name, opcode=dis._all_opmap[name], arg=None, argval=None, argrepr='',
                                      offset=0, start_offset=0, starts_line=True, line_number=1, label=None, positions=None)
            baseopname = instruction.baseopname
            baseopcode = instruction.baseopcode
            self.assertIn(name, opcode._specializations[baseopname])
            self.assertEqual(opcode.opmap[baseopname], baseopcode)

    def test_jump_target(self):
        # Non-jump instructions should return None
        instruction = Instruction(opname="NOP", opcode=dis.opmap["NOP"], arg=None, argval=None,
                                  argrepr='', offset=10, start_offset=10, starts_line=True, line_number=1, label=None,
                                  positions=None)
        self.assertIsNone(instruction.jump_target)

        delta = 100
        instruction = Instruction(opname="JUMP_FORWARD", opcode=dis.opmap["JUMP_FORWARD"], arg=delta, argval=delta,
                                  argrepr='', offset=10, start_offset=10, starts_line=True, line_number=1, label=None,
                                  positions=None)
        self.assertEqual(10 + 2 + 100*2, instruction.jump_target)

        # Test negative deltas
        instruction = Instruction(opname="JUMP_BACKWARD", opcode=dis.opmap["JUMP_BACKWARD"], arg=delta, argval=delta,
                                  argrepr='', offset=200, start_offset=200, starts_line=True, line_number=1, label=None,
                                  positions=None)
        self.assertEqual(200 + 2 - 100*2 + 2*1, instruction.jump_target)

        # Make sure cache entries are handled
        instruction = Instruction(opname="SEND", opcode=dis.opmap["SEND"], arg=delta, argval=delta,
                                  argrepr='', offset=10, start_offset=10, starts_line=True, line_number=1, label=None,
                                  positions=None)
        self.assertEqual(10 + 2 + 1*2 + 100*2, instruction.jump_target)

    def test_argval_argrepr(self):
        def f(opcode, oparg, offset, *init_args):
            arg_resolver = dis.ArgResolver(*init_args)
            return arg_resolver.get_argval_argrepr(opcode, oparg, offset)

        offset = 42
        co_consts = (0, 1, 2, 3)
        names = {1: 'a', 2: 'b'}
        varname_from_oparg = lambda i : names[i]
        labels_map = {24: 1}
        args = (offset, co_consts, names, varname_from_oparg, labels_map)
        self.assertEqual(f(opcode.opmap["POP_TOP"], None, *args), (None, ''))
        self.assertEqual(f(opcode.opmap["LOAD_CONST"], 1, *args), (1, '1'))
        self.assertEqual(f(opcode.opmap["LOAD_GLOBAL"], 2, *args), ('a', 'a'))
        self.assertEqual(f(opcode.opmap["JUMP_BACKWARD"], 11, *args), (24, 'to L1'))
        self.assertEqual(f(opcode.opmap["COMPARE_OP"], 3, *args), ('<', '<'))
        self.assertEqual(f(opcode.opmap["SET_FUNCTION_ATTRIBUTE"], 2, *args), (2, 'kwdefaults'))
        self.assertEqual(f(opcode.opmap["BINARY_OP"], 3, *args), (3, '<<'))
        self.assertEqual(f(opcode.opmap["CALL_INTRINSIC_1"], 2, *args), (2, 'INTRINSIC_IMPORT_STAR'))

    def test_custom_arg_resolver(self):
        class MyArgResolver(dis.ArgResolver):
            def offset_from_jump_arg(self, op, arg, offset):
                return arg + 1

            def get_label_for_offset(self, offset):
                return 2 * offset

        def f(opcode, oparg, offset, *init_args):
            arg_resolver = MyArgResolver(*init_args)
            return arg_resolver.get_argval_argrepr(opcode, oparg, offset)
        offset = 42
        self.assertEqual(f(opcode.opmap["JUMP_BACKWARD"], 1, offset), (2, 'to L4'))
        self.assertEqual(f(opcode.opmap["SETUP_FINALLY"], 2, offset), (3, 'to L6'))


    def get_instructions(self, code):
        return dis._get_instructions_bytes(code)

    def test_start_offset(self):
        # When no extended args are present,
        # start_offset should be equal to offset

        instructions = list(dis.Bytecode(_f))
        for instruction in instructions:
            self.assertEqual(instruction.offset, instruction.start_offset)

        def last_item(iterable):
            return functools.reduce(lambda a, b : b, iterable)

        code = bytes([
            opcode.opmap["LOAD_FAST"], 0x00,
            opcode.opmap["EXTENDED_ARG"], 0x01,
            opcode.opmap["POP_JUMP_IF_TRUE"], 0xFF,
        ])
        labels_map = dis._make_labels_map(code)
        jump = last_item(self.get_instructions(code))
        self.assertEqual(4, jump.offset)
        self.assertEqual(2, jump.start_offset)

        code = bytes([
            opcode.opmap["LOAD_FAST"], 0x00,
            opcode.opmap["EXTENDED_ARG"], 0x01,
            opcode.opmap["EXTENDED_ARG"], 0x01,
            opcode.opmap["EXTENDED_ARG"], 0x01,
            opcode.opmap["POP_JUMP_IF_TRUE"], 0xFF,
            opcode.opmap["CACHE"], 0x00,
        ])
        jump = last_item(self.get_instructions(code))
        self.assertEqual(8, jump.offset)
        self.assertEqual(2, jump.start_offset)

        code = bytes([
            opcode.opmap["LOAD_FAST"], 0x00,
            opcode.opmap["EXTENDED_ARG"], 0x01,
            opcode.opmap["POP_JUMP_IF_TRUE"], 0xFF,
            opcode.opmap["CACHE"], 0x00,
            opcode.opmap["EXTENDED_ARG"], 0x01,
            opcode.opmap["EXTENDED_ARG"], 0x01,
            opcode.opmap["EXTENDED_ARG"], 0x01,
            opcode.opmap["POP_JUMP_IF_TRUE"], 0xFF,
            opcode.opmap["CACHE"], 0x00,
        ])
        instructions = list(self.get_instructions(code))
        # 1st jump
        self.assertEqual(4, instructions[2].offset)
        self.assertEqual(2, instructions[2].start_offset)
        # 2nd jump
        self.assertEqual(14, instructions[6].offset)
        self.assertEqual(8, instructions[6].start_offset)

    def test_cache_offset_and_end_offset(self):
        code = bytes([
            opcode.opmap["LOAD_GLOBAL"], 0x01,
            opcode.opmap["CACHE"], 0x00,
            opcode.opmap["CACHE"], 0x00,
            opcode.opmap["CACHE"], 0x00,
            opcode.opmap["CACHE"], 0x00,
            opcode.opmap["LOAD_FAST"], 0x00,
            opcode.opmap["CALL"], 0x01,
            opcode.opmap["CACHE"], 0x00,
            opcode.opmap["CACHE"], 0x00,
            opcode.opmap["CACHE"], 0x00
        ])
        instructions = list(self.get_instructions(code))
        self.assertEqual(2, instructions[0].cache_offset)
        self.assertEqual(10, instructions[0].end_offset)
        self.assertEqual(12, instructions[1].cache_offset)
        self.assertEqual(12, instructions[1].end_offset)
        self.assertEqual(14, instructions[2].cache_offset)
        self.assertEqual(20, instructions[2].end_offset)

        # end_offset of the previous instruction should be equal to the
        # start_offset of the following instruction
        instructions = list(dis.Bytecode(self.test_cache_offset_and_end_offset))
        for prev, curr in zip(instructions, instructions[1:]):
            self.assertEqual(prev.end_offset, curr.start_offset)


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

    def test_findlabels(self):
        labels = dis.findlabels(jumpy.__code__.co_code)
        jumps = [
            instr.offset
            for instr in expected_opinfo_jumpy
            if instr.is_jump_target
        ]

        self.assertEqual(sorted(labels), sorted(jumps))

    def test_findlinestarts(self):
        def func():
            pass

        code = func.__code__
        offsets = [linestart[0] for linestart in dis.findlinestarts(code)]
        self.assertEqual(offsets, [0, 2])


class TestDisTraceback(DisTestBase):
    def setUp(self) -> None:
        try:  # We need to clean up existing tracebacks
            del sys.last_exc
        except AttributeError:
            pass
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

def _unroll_caches_as_Instructions(instrs, show_caches=False):
    # Cache entries are no longer reported by dis as fake instructions,
    # but some tests assume that do. We should rewrite the tests to assume
    # the new API, but it will be clearer to keep the tests working as
    # before and do that in a separate PR.

    for instr in instrs:
        yield instr
        if not show_caches:
            continue

        offset = instr.offset
        for name, size, data in (instr.cache_info or ()):
            for i in range(size):
                offset += 2
                # Only show the fancy argrepr for a CACHE instruction when it's
                # the first entry for a particular cache value:
                if i == 0:
                    argrepr = f"{name}: {int.from_bytes(data, sys.byteorder)}"
                else:
                    argrepr = ""

                yield Instruction("CACHE", CACHE, 0, None, argrepr, offset, offset,
                                  False, None, None, instr.positions)


if __name__ == "__main__":
    unittest.main()
