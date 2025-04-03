import dis
from itertools import combinations, product
import opcode
import sys
import textwrap
import unittest
try:
    import _testinternalcapi
except ImportError:
    _testinternalcapi = None

from test import support
from test.support.bytecode_helper import (
    BytecodeTestCase, CfgOptimizationTestCase, CompilationStepTestCase)


def compile_pattern_with_fast_locals(pattern):
    source = textwrap.dedent(
        f"""
        def f(x):
            match x:
                case {pattern}:
                    pass
        """
    )
    namespace = {}
    exec(source, namespace)
    return namespace["f"].__code__


def count_instr_recursively(f, opname):
    count = 0
    for instr in dis.get_instructions(f):
        if instr.opname == opname:
            count += 1
    if hasattr(f, '__code__'):
        f = f.__code__
    for c in f.co_consts:
        if hasattr(c, 'co_code'):
            count += count_instr_recursively(c, opname)
    return count


def get_binop_argval(arg):
    for i, nb_op in enumerate(opcode._nb_ops):
        if arg == nb_op[0]:
            return i
    assert False, f"{arg} is not a valid BINARY_OP argument."


class TestTranforms(BytecodeTestCase):

    def check_jump_targets(self, code):
        instructions = list(dis.get_instructions(code))
        targets = {instr.offset: instr for instr in instructions}
        for instr in instructions:
            if 'JUMP_' not in instr.opname:
                continue
            tgt = targets[instr.argval]
            # jump to unconditional jump
            if tgt.opname in ('JUMP_BACKWARD', 'JUMP_FORWARD'):
                self.fail(f'{instr.opname} at {instr.offset} '
                          f'jumps to {tgt.opname} at {tgt.offset}')
            # unconditional jump to RETURN_VALUE
            if (instr.opname in ('JUMP_BACKWARD', 'JUMP_FORWARD') and
                tgt.opname == 'RETURN_VALUE'):
                self.fail(f'{instr.opname} at {instr.offset} '
                          f'jumps to {tgt.opname} at {tgt.offset}')

    def check_lnotab(self, code):
        "Check that the lnotab byte offsets are sensible."
        code = dis._get_code_object(code)
        lnotab = list(dis.findlinestarts(code))
        # Don't bother checking if the line info is sensible, because
        # most of the line info we can get at comes from lnotab.
        min_bytecode = min(t[0] for t in lnotab)
        max_bytecode = max(t[0] for t in lnotab)
        self.assertGreaterEqual(min_bytecode, 0)
        self.assertLess(max_bytecode, len(code.co_code))
        # This could conceivably test more (and probably should, as there
        # aren't very many tests of lnotab), if peepholer wasn't scheduled
        # to be replaced anyway.

    def test_unot(self):
        # UNARY_NOT POP_JUMP_IF_FALSE  -->  POP_JUMP_IF_TRUE'
        def unot(x):
            if not x == 2:
                del x
        self.assertNotInBytecode(unot, 'UNARY_NOT')
        self.assertNotInBytecode(unot, 'POP_JUMP_IF_FALSE')
        self.assertInBytecode(unot, 'POP_JUMP_IF_TRUE')
        self.check_lnotab(unot)

    def test_elim_inversion_of_is_or_in(self):
        for line, cmp_op, invert in (
            ('not a is b', 'IS_OP', 1,),
            ('not a is not b', 'IS_OP', 0,),
            ('not a in b', 'CONTAINS_OP', 1,),
            ('not a not in b', 'CONTAINS_OP', 0,),
            ):
            with self.subTest(line=line):
                code = compile(line, '', 'single')
                self.assertInBytecode(code, cmp_op, invert)
                self.check_lnotab(code)

    def test_global_as_constant(self):
        # LOAD_GLOBAL None/True/False  -->  LOAD_CONST None/True/False
        def f():
            x = None
            x = None
            return x
        def g():
            x = True
            return x
        def h():
            x = False
            return x

        for func, elem in ((f, None), (g, True), (h, False)):
            with self.subTest(func=func):
                self.assertNotInBytecode(func, 'LOAD_GLOBAL')
                self.assertInBytecode(func, 'LOAD_CONST', elem)
                self.check_lnotab(func)

        def f():
            'Adding a docstring made this test fail in Py2.5.0'
            return None

        self.assertNotInBytecode(f, 'LOAD_GLOBAL')
        self.assertInBytecode(f, 'LOAD_CONST', None)
        self.check_lnotab(f)

    def test_while_one(self):
        # Skip over:  LOAD_CONST trueconst  POP_JUMP_IF_FALSE xx
        def f():
            while 1:
                pass
            return list
        for elem in ('LOAD_CONST', 'POP_JUMP_IF_FALSE'):
            self.assertNotInBytecode(f, elem)
        for elem in ('JUMP_BACKWARD',):
            self.assertInBytecode(f, elem)
        self.check_lnotab(f)

    def test_pack_unpack(self):
        for line, elem in (
            ('a, = a,', 'LOAD_CONST',),
            ('a, b = a, b', 'SWAP',),
            ('a, b, c = a, b, c', 'SWAP',),
            ):
            with self.subTest(line=line):
                code = compile(line,'','single')
                self.assertInBytecode(code, elem)
                self.assertNotInBytecode(code, 'BUILD_TUPLE')
                self.assertNotInBytecode(code, 'UNPACK_SEQUENCE')
                self.check_lnotab(code)

    def test_constant_folding_tuples_of_constants(self):
        for line, elem in (
            ('a = 1,2,3', (1, 2, 3)),
            ('("a","b","c")', ('a', 'b', 'c')),
            ('a,b,c,d = 1,2,3,4', (1, 2, 3, 4)),
            ('(None, 1, None)', (None, 1, None)),
            ('((1, 2), 3, 4)', ((1, 2), 3, 4)),
            ):
            with self.subTest(line=line):
                code = compile(line,'','single')
                self.assertInBytecode(code, 'LOAD_CONST', elem)
                self.assertNotInBytecode(code, 'BUILD_TUPLE')
                self.check_lnotab(code)

        # Long tuples should be folded too.
        code = compile(repr(tuple(range(10000))),'','single')
        self.assertNotInBytecode(code, 'BUILD_TUPLE')
        # One LOAD_CONST for the tuple, one for the None return value
        load_consts = [instr for instr in dis.get_instructions(code)
                              if instr.opname == 'LOAD_CONST']
        self.assertEqual(len(load_consts), 2)
        self.check_lnotab(code)

        # Bug 1053819:  Tuple of constants misidentified when presented with:
        # . . . opcode_with_arg 100   unary_opcode   BUILD_TUPLE 1  . . .
        # The following would segfault upon compilation
        def crater():
            (~[
                0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
                0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
                0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
                0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
                0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
                0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
                0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
                0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
                0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
                0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
            ],)
        self.check_lnotab(crater)

    def test_constant_folding_lists_of_constants(self):
        for line, elem in (
            # in/not in constants with BUILD_LIST should be folded to a tuple:
            ('a in [1,2,3]', (1, 2, 3)),
            ('a not in ["a","b","c"]', ('a', 'b', 'c')),
            ('a in [None, 1, None]', (None, 1, None)),
            ('a not in [(1, 2), 3, 4]', ((1, 2), 3, 4)),
            ):
            with self.subTest(line=line):
                code = compile(line, '', 'single')
                self.assertInBytecode(code, 'LOAD_CONST', elem)
                self.assertNotInBytecode(code, 'BUILD_LIST')
                self.check_lnotab(code)

    def test_constant_folding_sets_of_constants(self):
        for line, elem in (
            # in/not in constants with BUILD_SET should be folded to a frozenset:
            ('a in {1,2,3}', frozenset({1, 2, 3})),
            ('a not in {"a","b","c"}', frozenset({'a', 'c', 'b'})),
            ('a in {None, 1, None}', frozenset({1, None})),
            ('a not in {(1, 2), 3, 4}', frozenset({(1, 2), 3, 4})),
            ('a in {1, 2, 3, 3, 2, 1}', frozenset({1, 2, 3})),
            ):
            with self.subTest(line=line):
                code = compile(line, '', 'single')
                self.assertNotInBytecode(code, 'BUILD_SET')
                self.assertInBytecode(code, 'LOAD_CONST', elem)
                self.check_lnotab(code)

        # Ensure that the resulting code actually works:
        def f(a):
            return a in {1, 2, 3}

        def g(a):
            return a not in {1, 2, 3}

        self.assertTrue(f(3))
        self.assertTrue(not f(4))
        self.check_lnotab(f)

        self.assertTrue(not g(3))
        self.assertTrue(g(4))
        self.check_lnotab(g)

    def test_constant_folding_small_int(self):
        tests = [
            ('(0, )[0]', 0),
            ('(1 + 2, )[0]', 3),
            ('(2 + 2 * 2, )[0]', 6),
            ('(1, (1 + 2 + 3, ))[1][0]', 6),
            ('1 + 2', 3),
            ('2 + 2 * 2 // 2 - 2', 2),
            ('(255, )[0]', 255),
            ('(256, )[0]', None),
            ('(1000, )[0]', None),
            ('(1 - 2, )[0]', None),
            ('255 + 0', 255),
            ('255 + 1', None),
            ('-1', None),
            ('--1', 1),
            ('--255', 255),
            ('--256', None),
            ('~1', None),
            ('~~1', 1),
            ('~~255', 255),
            ('~~256', None),
            ('++255', 255),
            ('++256', None),
        ]
        for expr, oparg in tests:
            with self.subTest(expr=expr, oparg=oparg):
                code = compile(expr, '', 'single')
                if oparg is not None:
                    self.assertInBytecode(code, 'LOAD_SMALL_INT', oparg)
                else:
                    self.assertNotInBytecode(code, 'LOAD_SMALL_INT')
                self.check_lnotab(code)

    def test_constant_folding_unaryop(self):
        intrinsic_positive = 5
        tests = [
            ('-0', 'UNARY_NEGATIVE', None, True, 'LOAD_SMALL_INT', 0),
            ('-0.0', 'UNARY_NEGATIVE', None, True, 'LOAD_CONST', -0.0),
            ('-(1.0-1.0)', 'UNARY_NEGATIVE', None, True, 'LOAD_CONST', -0.0),
            ('-0.5', 'UNARY_NEGATIVE', None, True, 'LOAD_CONST', -0.5),
            ('---1', 'UNARY_NEGATIVE', None, True, 'LOAD_CONST', -1),
            ('---""', 'UNARY_NEGATIVE', None, False, None, None),
            ('~~~1', 'UNARY_INVERT', None, True, 'LOAD_CONST', -2),
            ('~~~""', 'UNARY_INVERT', None, False, None, None),
            ('not not True', 'UNARY_NOT', None, True, 'LOAD_CONST', True),
            ('not not x', 'UNARY_NOT', None, True, 'LOAD_NAME', 'x'),  # this should be optimized regardless of constant or not
            ('+++1', 'CALL_INTRINSIC_1', intrinsic_positive, True, 'LOAD_SMALL_INT', 1),
            ('---x', 'UNARY_NEGATIVE', None, False, None, None),
            ('~~~x', 'UNARY_INVERT', None, False, None, None),
            ('+++x', 'CALL_INTRINSIC_1', intrinsic_positive, False, None, None),
        ]

        for (
            expr,
            original_opcode,
            original_argval,
            is_optimized,
            optimized_opcode,
            optimized_argval,
        ) in tests:
            with self.subTest(expr=expr, is_optimized=is_optimized):
                code = compile(expr, "", "single")
                if is_optimized:
                    self.assertNotInBytecode(code, original_opcode, argval=original_argval)
                    self.assertInBytecode(code, optimized_opcode, argval=optimized_argval)
                else:
                    self.assertInBytecode(code, original_opcode, argval=original_argval)
                self.check_lnotab(code)

        # Check that -0.0 works after marshaling
        def negzero():
            return -(1.0-1.0)

        for instr in dis.get_instructions(negzero):
            self.assertFalse(instr.opname.startswith('UNARY_'))
        self.check_lnotab(negzero)

    def test_constant_folding_binop(self):
        tests = [
            ('1 + 2', 'NB_ADD', True, 'LOAD_SMALL_INT', 3),
            ('1 + 2 + 3', 'NB_ADD', True, 'LOAD_SMALL_INT', 6),
            ('1 + ""', 'NB_ADD', False, None, None),
            ('1 - 2', 'NB_SUBTRACT', True, 'LOAD_CONST', -1),
            ('1 - 2 - 3', 'NB_SUBTRACT', True, 'LOAD_CONST', -4),
            ('1 - ""', 'NB_SUBTRACT', False, None, None),
            ('2 * 2', 'NB_MULTIPLY', True, 'LOAD_SMALL_INT', 4),
            ('2 * 2 * 2', 'NB_MULTIPLY', True, 'LOAD_SMALL_INT', 8),
            ('2 / 2', 'NB_TRUE_DIVIDE', True, 'LOAD_CONST', 1.0),
            ('2 / 2 / 2', 'NB_TRUE_DIVIDE', True, 'LOAD_CONST', 0.5),
            ('2 / ""', 'NB_TRUE_DIVIDE', False, None, None),
            ('2 // 2', 'NB_FLOOR_DIVIDE', True, 'LOAD_SMALL_INT', 1),
            ('2 // 2 // 2', 'NB_FLOOR_DIVIDE', True, 'LOAD_SMALL_INT', 0),
            ('2 // ""', 'NB_FLOOR_DIVIDE', False, None, None),
            ('2 % 2', 'NB_REMAINDER', True, 'LOAD_SMALL_INT', 0),
            ('2 % 2 % 2', 'NB_REMAINDER', True, 'LOAD_SMALL_INT', 0),
            ('2 % ()', 'NB_REMAINDER', False, None, None),
            ('2 ** 2', 'NB_POWER', True, 'LOAD_SMALL_INT', 4),
            ('2 ** 2 ** 2', 'NB_POWER', True, 'LOAD_SMALL_INT', 16),
            ('2 ** ""', 'NB_POWER', False, None, None),
            ('2 << 2', 'NB_LSHIFT', True, 'LOAD_SMALL_INT', 8),
            ('2 << 2 << 2', 'NB_LSHIFT', True, 'LOAD_SMALL_INT', 32),
            ('2 << ""', 'NB_LSHIFT', False, None, None),
            ('2 >> 2', 'NB_RSHIFT', True, 'LOAD_SMALL_INT', 0),
            ('2 >> 2 >> 2', 'NB_RSHIFT', True, 'LOAD_SMALL_INT', 0),
            ('2 >> ""', 'NB_RSHIFT', False, None, None),
            ('2 | 2', 'NB_OR', True, 'LOAD_SMALL_INT', 2),
            ('2 | 2 | 2', 'NB_OR', True, 'LOAD_SMALL_INT', 2),
            ('2 | ""', 'NB_OR', False, None, None),
            ('2 & 2', 'NB_AND', True, 'LOAD_SMALL_INT', 2),
            ('2 & 2 & 2', 'NB_AND', True, 'LOAD_SMALL_INT', 2),
            ('2 & ""', 'NB_AND', False, None, None),
            ('2 ^ 2', 'NB_XOR', True, 'LOAD_SMALL_INT', 0),
            ('2 ^ 2 ^ 2', 'NB_XOR', True, 'LOAD_SMALL_INT', 2),
            ('2 ^ ""', 'NB_XOR', False, None, None),
            ('(1, )[0]', 'NB_SUBSCR', True, 'LOAD_SMALL_INT', 1),
            ('(1, )[-1]', 'NB_SUBSCR', True, 'LOAD_SMALL_INT', 1),
            ('(1 + 2, )[0]', 'NB_SUBSCR', True, 'LOAD_SMALL_INT', 3),
            ('(1, (1, 2))[1][1]', 'NB_SUBSCR', True, 'LOAD_SMALL_INT', 2),
            ('(1, 2)[2-1]', 'NB_SUBSCR', True, 'LOAD_SMALL_INT', 2),
            ('(1, (1, 2))[1][2-1]', 'NB_SUBSCR', True, 'LOAD_SMALL_INT', 2),
            ('(1, (1, 2))[1:6][0][2-1]', 'NB_SUBSCR', True, 'LOAD_SMALL_INT', 2),
            ('"a"[0]', 'NB_SUBSCR', True, 'LOAD_CONST', 'a'),
            ('("a" + "b")[1]', 'NB_SUBSCR', True, 'LOAD_CONST', 'b'),
            ('("a" + "b", )[0][1]', 'NB_SUBSCR', True, 'LOAD_CONST', 'b'),
            ('("a" * 10)[9]', 'NB_SUBSCR', True, 'LOAD_CONST', 'a'),
            ('(1, )[1]', 'NB_SUBSCR', False, None, None),
            ('(1, )[-2]', 'NB_SUBSCR', False, None, None),
            ('"a"[1]', 'NB_SUBSCR', False, None, None),
            ('"a"[-2]', 'NB_SUBSCR', False, None, None),
            ('("a" + "b")[2]', 'NB_SUBSCR', False, None, None),
            ('("a" + "b", )[0][2]', 'NB_SUBSCR', False, None, None),
            ('("a" + "b", )[1][0]', 'NB_SUBSCR', False, None, None),
            ('("a" * 10)[10]', 'NB_SUBSCR', False, None, None),
            ('(1, (1, 2))[2:6][0][2-1]', 'NB_SUBSCR', False, None, None),
        ]

        for (
            expr,
            nb_op,
            is_optimized,
            optimized_opcode,
            optimized_argval
        ) in tests:
            with self.subTest(expr=expr, is_optimized=is_optimized):
                code = compile(expr, '', 'single')
                nb_op_val = get_binop_argval(nb_op)
                if is_optimized:
                    self.assertNotInBytecode(code, 'BINARY_OP', argval=nb_op_val)
                    self.assertInBytecode(code, optimized_opcode, argval=optimized_argval)
                else:
                    self.assertInBytecode(code, 'BINARY_OP', argval=nb_op_val)
                self.check_lnotab(code)

        # Verify that large sequences do not result from folding
        code = compile('"x"*10000', '', 'single')
        self.assertInBytecode(code, 'LOAD_CONST', 10000)
        self.assertNotIn("x"*10000, code.co_consts)
        self.check_lnotab(code)
        code = compile('1<<1000', '', 'single')
        self.assertInBytecode(code, 'LOAD_CONST', 1000)
        self.assertNotIn(1<<1000, code.co_consts)
        self.check_lnotab(code)
        code = compile('2**1000', '', 'single')
        self.assertInBytecode(code, 'LOAD_CONST', 1000)
        self.assertNotIn(2**1000, code.co_consts)
        self.check_lnotab(code)

        # Test binary subscript on unicode
        # valid code get optimized
        code = compile('"foo"[0]', '', 'single')
        self.assertInBytecode(code, 'LOAD_CONST', 'f')
        self.assertNotInBytecode(code, 'BINARY_OP')
        self.check_lnotab(code)
        code = compile('"\u0061\uffff"[1]', '', 'single')
        self.assertInBytecode(code, 'LOAD_CONST', '\uffff')
        self.assertNotInBytecode(code,'BINARY_OP')
        self.check_lnotab(code)

        # With PEP 393, non-BMP char get optimized
        code = compile('"\U00012345"[0]', '', 'single')
        self.assertInBytecode(code, 'LOAD_CONST', '\U00012345')
        self.assertNotInBytecode(code, 'BINARY_OP')
        self.check_lnotab(code)

        # invalid code doesn't get optimized
        # out of range
        code = compile('"fuu"[10]', '', 'single')
        self.assertInBytecode(code, 'BINARY_OP')
        self.check_lnotab(code)


    def test_constant_folding_remove_nop_location(self):
        sources = [
            """
            (-
             -
             -
             1)
            """,

            """
            (1
             +
             2
             +
             3)
            """,

            """
            (1,
             2,
             3)[0]
            """,

            """
            [1,
             2,
             3]
            """,

            """
            {1,
             2,
             3}
            """,

            """
            1 in [
               1,
               2,
               3
            ]
            """,

            """
            1 in {
               1,
               2,
               3
            }
            """,

            """
            for _ in [1,
                      2,
                      3]:
                pass
            """,

            """
            for _ in [1,
                      2,
                      x]:
                pass
            """,

            """
            for _ in {1,
                      2,
                      3}:
                pass
            """
        ]

        for source in sources:
            code = compile(textwrap.dedent(source), '', 'single')
            self.assertNotInBytecode(code, 'NOP')

    def test_elim_extra_return(self):
        # RETURN LOAD_CONST None RETURN  -->  RETURN
        def f(x):
            return x
        self.assertNotInBytecode(f, 'LOAD_CONST', None)
        returns = [instr for instr in dis.get_instructions(f)
                          if instr.opname == 'RETURN_VALUE']
        self.assertEqual(len(returns), 1)
        self.check_lnotab(f)

    def test_elim_jump_to_return(self):
        # JUMP_FORWARD to RETURN -->  RETURN
        def f(cond, true_value, false_value):
            # Intentionally use two-line expression to test issue37213.
            return (true_value if cond
                    else false_value)
        self.check_jump_targets(f)
        self.assertNotInBytecode(f, 'JUMP_FORWARD')
        self.assertNotInBytecode(f, 'JUMP_BACKWARD')
        returns = [instr for instr in dis.get_instructions(f)
                          if instr.opname == 'RETURN_VALUE']
        self.assertEqual(len(returns), 2)
        self.check_lnotab(f)

    def test_elim_jump_to_uncond_jump(self):
        # POP_JUMP_IF_FALSE to JUMP_FORWARD --> POP_JUMP_IF_FALSE to non-jump
        def f():
            if a:
                # Intentionally use two-line expression to test issue37213.
                if (c
                    or d):
                    foo()
            else:
                baz()
        self.check_jump_targets(f)
        self.check_lnotab(f)

    def test_elim_jump_to_uncond_jump2(self):
        # POP_JUMP_IF_FALSE to JUMP_BACKWARD --> POP_JUMP_IF_FALSE to non-jump
        def f():
            while a:
                # Intentionally use two-line expression to test issue37213.
                if (c
                    or d):
                    a = foo()
        self.check_jump_targets(f)
        self.check_lnotab(f)

    def test_elim_jump_to_uncond_jump3(self):
        # Intentionally use two-line expressions to test issue37213.
        # POP_JUMP_IF_FALSE to POP_JUMP_IF_FALSE --> POP_JUMP_IF_FALSE to non-jump
        def f(a, b, c):
            return ((a and b)
                    and c)
        self.check_jump_targets(f)
        self.check_lnotab(f)
        self.assertEqual(count_instr_recursively(f, 'POP_JUMP_IF_FALSE'), 2)
        # POP_JUMP_IF_TRUE to POP_JUMP_IF_TRUE --> POP_JUMP_IF_TRUE to non-jump
        def f(a, b, c):
            return ((a or b)
                    or c)
        self.check_jump_targets(f)
        self.check_lnotab(f)
        self.assertEqual(count_instr_recursively(f, 'POP_JUMP_IF_TRUE'), 2)
        # JUMP_IF_FALSE_OR_POP to JUMP_IF_TRUE_OR_POP --> POP_JUMP_IF_FALSE to non-jump
        def f(a, b, c):
            return ((a and b)
                    or c)
        self.check_jump_targets(f)
        self.check_lnotab(f)
        self.assertEqual(count_instr_recursively(f, 'POP_JUMP_IF_FALSE'), 1)
        self.assertEqual(count_instr_recursively(f, 'POP_JUMP_IF_TRUE'), 1)
        # POP_JUMP_IF_TRUE to POP_JUMP_IF_FALSE --> POP_JUMP_IF_TRUE to non-jump
        def f(a, b, c):
            return ((a or b)
                    and c)
        self.check_jump_targets(f)
        self.check_lnotab(f)
        self.assertEqual(count_instr_recursively(f, 'POP_JUMP_IF_FALSE'), 1)
        self.assertEqual(count_instr_recursively(f, 'POP_JUMP_IF_TRUE'), 1)

    def test_elim_jump_to_uncond_jump4(self):
        def f():
            for i in range(5):
                if i > 3:
                    print(i)
        self.check_jump_targets(f)

    def test_elim_jump_after_return1(self):
        # Eliminate dead code: jumps immediately after returns can't be reached
        def f(cond1, cond2):
            if cond1: return 1
            if cond2: return 2
            while 1:
                return 3
            while 1:
                if cond1: return 4
                return 5
            return 6
        self.assertNotInBytecode(f, 'JUMP_FORWARD')
        self.assertNotInBytecode(f, 'JUMP_BACKWARD')
        returns = [instr for instr in dis.get_instructions(f)
                          if instr.opname == 'RETURN_VALUE']
        self.assertLessEqual(len(returns), 6)
        self.check_lnotab(f)

    def test_make_function_doesnt_bail(self):
        def f():
            def g()->1+1:
                pass
            return g
        self.assertNotInBytecode(f, 'BINARY_OP')
        self.check_lnotab(f)

    def test_in_literal_list(self):
        def containtest():
            return x in [a, b]
        self.assertEqual(count_instr_recursively(containtest, 'BUILD_LIST'), 0)
        self.check_lnotab(containtest)

    def test_iterate_literal_list(self):
        def forloop():
            for x in [a, b]:
                pass
        self.assertEqual(count_instr_recursively(forloop, 'BUILD_LIST'), 0)
        self.check_lnotab(forloop)

    def test_condition_with_binop_with_bools(self):
        def f():
            if True or False:
                return 1
            return 0
        self.assertEqual(f(), 1)
        self.check_lnotab(f)

    def test_if_with_if_expression(self):
        # Check bpo-37289
        def f(x):
            if (True if x else False):
                return True
            return False
        self.assertTrue(f(True))
        self.check_lnotab(f)

    def test_trailing_nops(self):
        # Check the lnotab of a function that even after trivial
        # optimization has trailing nops, which the lnotab adjustment has to
        # handle properly (bpo-38115).
        def f(x):
            while 1:
                return 3
            while 1:
                return 5
            return 6
        self.check_lnotab(f)

    def test_assignment_idiom_in_comprehensions(self):
        def listcomp():
            return [y for x in a for y in [f(x)]]
        self.assertEqual(count_instr_recursively(listcomp, 'FOR_ITER'), 1)
        def setcomp():
            return {y for x in a for y in [f(x)]}
        self.assertEqual(count_instr_recursively(setcomp, 'FOR_ITER'), 1)
        def dictcomp():
            return {y: y for x in a for y in [f(x)]}
        self.assertEqual(count_instr_recursively(dictcomp, 'FOR_ITER'), 1)
        def genexpr():
            return (y for x in a for y in [f(x)])
        self.assertEqual(count_instr_recursively(genexpr, 'FOR_ITER'), 1)

    @support.requires_resource('cpu')
    def test_format_combinations(self):
        flags = '-+ #0'
        testcases = [
            *product(('', '1234', 'абвг'), 'sra'),
            *product((1234, -1234), 'duioxX'),
            *product((1234.5678901, -1234.5678901), 'duifegFEG'),
            *product((float('inf'), -float('inf')), 'fegFEG'),
        ]
        width_precs = [
            *product(('', '1', '30'), ('', '.', '.0', '.2')),
            ('', '.40'),
            ('30', '.40'),
        ]
        for value, suffix in testcases:
            for width, prec in width_precs:
                for r in range(len(flags) + 1):
                    for spec in combinations(flags, r):
                        fmt = '%' + ''.join(spec) + width + prec + suffix
                        with self.subTest(fmt=fmt, value=value):
                            s1 = fmt % value
                            s2 = eval(f'{fmt!r} % (x,)', {'x': value})
                            self.assertEqual(s2, s1, f'{fmt = }')

    def test_format_misc(self):
        def format(fmt, *values):
            vars = [f'x{i+1}' for i in range(len(values))]
            if len(vars) == 1:
                args = '(' + vars[0] + ',)'
            else:
                args = '(' + ', '.join(vars) + ')'
            return eval(f'{fmt!r} % {args}', dict(zip(vars, values)))

        self.assertEqual(format('string'), 'string')
        self.assertEqual(format('x = %s!', 1234), 'x = 1234!')
        self.assertEqual(format('x = %d!', 1234), 'x = 1234!')
        self.assertEqual(format('x = %x!', 1234), 'x = 4d2!')
        self.assertEqual(format('x = %f!', 1234), 'x = 1234.000000!')
        self.assertEqual(format('x = %s!', 1234.5678901), 'x = 1234.5678901!')
        self.assertEqual(format('x = %f!', 1234.5678901), 'x = 1234.567890!')
        self.assertEqual(format('x = %d!', 1234.5678901), 'x = 1234!')
        self.assertEqual(format('x = %s%% %%%%', 1234), 'x = 1234% %%')
        self.assertEqual(format('x = %s!', '%% %s'), 'x = %% %s!')
        self.assertEqual(format('x = %s, y = %d', 12, 34), 'x = 12, y = 34')

    def test_format_errors(self):
        with self.assertRaisesRegex(TypeError,
                    'not enough arguments for format string'):
            eval("'%s' % ()")
        with self.assertRaisesRegex(TypeError,
                    'not all arguments converted during string formatting'):
            eval("'%s' % (x, y)", {'x': 1, 'y': 2})
        with self.assertRaisesRegex(ValueError, 'incomplete format'):
            eval("'%s%' % (x,)", {'x': 1234})
        with self.assertRaisesRegex(ValueError, 'incomplete format'):
            eval("'%s%%%' % (x,)", {'x': 1234})
        with self.assertRaisesRegex(TypeError,
                    'not enough arguments for format string'):
            eval("'%s%z' % (x,)", {'x': 1234})
        with self.assertRaisesRegex(ValueError, 'unsupported format character'):
            eval("'%s%z' % (x, 5)", {'x': 1234})
        with self.assertRaisesRegex(TypeError, 'a real number is required, not str'):
            eval("'%d' % (x,)", {'x': '1234'})
        with self.assertRaisesRegex(TypeError, 'an integer is required, not float'):
            eval("'%x' % (x,)", {'x': 1234.56})
        with self.assertRaisesRegex(TypeError, 'an integer is required, not str'):
            eval("'%x' % (x,)", {'x': '1234'})
        with self.assertRaisesRegex(TypeError, 'must be real number, not str'):
            eval("'%f' % (x,)", {'x': '1234'})
        with self.assertRaisesRegex(TypeError,
                    'not enough arguments for format string'):
            eval("'%s, %s' % (x, *y)", {'x': 1, 'y': []})
        with self.assertRaisesRegex(TypeError,
                    'not all arguments converted during string formatting'):
            eval("'%s, %s' % (x, *y)", {'x': 1, 'y': [2, 3]})

    def test_static_swaps_unpack_two(self):
        def f(a, b):
            a, b = a, b
            b, a = a, b
        self.assertNotInBytecode(f, "SWAP")

    def test_static_swaps_unpack_three(self):
        def f(a, b, c):
            a, b, c = a, b, c
            a, c, b = a, b, c
            b, a, c = a, b, c
            b, c, a = a, b, c
            c, a, b = a, b, c
            c, b, a = a, b, c
        self.assertNotInBytecode(f, "SWAP")

    def test_static_swaps_match_mapping(self):
        for a, b, c in product("_a", "_b", "_c"):
            pattern = f"{{'a': {a}, 'b': {b}, 'c': {c}}}"
            with self.subTest(pattern):
                code = compile_pattern_with_fast_locals(pattern)
                self.assertNotInBytecode(code, "SWAP")

    def test_static_swaps_match_class(self):
        forms = [
            "C({}, {}, {})",
            "C({}, {}, c={})",
            "C({}, b={}, c={})",
            "C(a={}, b={}, c={})"
        ]
        for a, b, c in product("_a", "_b", "_c"):
            for form in forms:
                pattern = form.format(a, b, c)
                with self.subTest(pattern):
                    code = compile_pattern_with_fast_locals(pattern)
                    self.assertNotInBytecode(code, "SWAP")

    def test_static_swaps_match_sequence(self):
        swaps = {"*_, b, c", "a, *_, c", "a, b, *_"}
        forms = ["{}, {}, {}", "{}, {}, *{}", "{}, *{}, {}", "*{}, {}, {}"]
        for a, b, c in product("_a", "_b", "_c"):
            for form in forms:
                pattern = form.format(a, b, c)
                with self.subTest(pattern):
                    code = compile_pattern_with_fast_locals(pattern)
                    if pattern in swaps:
                        # If this fails... great! Remove this pattern from swaps
                        # to prevent regressing on any improvement:
                        self.assertInBytecode(code, "SWAP")
                    else:
                        self.assertNotInBytecode(code, "SWAP")


class TestBuglets(unittest.TestCase):

    def test_bug_11510(self):
        # folded constant set optimization was commingled with the tuple
        # unpacking optimization which would fail if the set had duplicate
        # elements so that the set length was unexpected
        def f():
            x, y = {1, 1}
            return x, y
        with self.assertRaises(ValueError):
            f()

    def test_bpo_42057(self):
        for i in range(10):
            try:
                raise Exception
            except Exception or Exception:
                pass

    def test_bpo_45773_pop_jump_if_true(self):
        compile("while True or spam: pass", "<test>", "exec")

    def test_bpo_45773_pop_jump_if_false(self):
        compile("while True or not spam: pass", "<test>", "exec")


class TestMarkingVariablesAsUnKnown(BytecodeTestCase):

    def setUp(self):
        self.addCleanup(sys.settrace, sys.gettrace())
        sys.settrace(None)

    def test_load_fast_known_simple(self):
        def f():
            x = 1
            y = x + x
        self.assertInBytecode(f, 'LOAD_FAST_BORROW_LOAD_FAST_BORROW')

    def test_load_fast_unknown_simple(self):
        def f():
            if condition():
                x = 1
            print(x)
        self.assertInBytecode(f, 'LOAD_FAST_CHECK')
        self.assertNotInBytecode(f, 'LOAD_FAST')

    def test_load_fast_unknown_because_del(self):
        def f():
            x = 1
            del x
            print(x)
        self.assertInBytecode(f, 'LOAD_FAST_CHECK')
        self.assertNotInBytecode(f, 'LOAD_FAST')

    def test_load_fast_known_because_parameter(self):
        def f1(x):
            print(x)
        self.assertInBytecode(f1, 'LOAD_FAST_BORROW')
        self.assertNotInBytecode(f1, 'LOAD_FAST_CHECK')

        def f2(*, x):
            print(x)
        self.assertInBytecode(f2, 'LOAD_FAST_BORROW')
        self.assertNotInBytecode(f2, 'LOAD_FAST_CHECK')

        def f3(*args):
            print(args)
        self.assertInBytecode(f3, 'LOAD_FAST_BORROW')
        self.assertNotInBytecode(f3, 'LOAD_FAST_CHECK')

        def f4(**kwargs):
            print(kwargs)
        self.assertInBytecode(f4, 'LOAD_FAST_BORROW')
        self.assertNotInBytecode(f4, 'LOAD_FAST_CHECK')

        def f5(x=0):
            print(x)
        self.assertInBytecode(f5, 'LOAD_FAST_BORROW')
        self.assertNotInBytecode(f5, 'LOAD_FAST_CHECK')

    def test_load_fast_known_because_already_loaded(self):
        def f():
            if condition():
                x = 1
            print(x)
            print(x)
        self.assertInBytecode(f, 'LOAD_FAST_CHECK')
        self.assertInBytecode(f, 'LOAD_FAST_BORROW')

    def test_load_fast_known_multiple_branches(self):
        def f():
            if condition():
                x = 1
            else:
                x = 2
            print(x)
        self.assertInBytecode(f, 'LOAD_FAST_BORROW')
        self.assertNotInBytecode(f, 'LOAD_FAST_CHECK')

    def test_load_fast_unknown_after_error(self):
        def f():
            try:
                res = 1 / 0
            except ZeroDivisionError:
                pass
            return res
        # LOAD_FAST (known) still occurs in the no-exception branch.
        # Assert that it doesn't occur in the LOAD_FAST_CHECK branch.
        self.assertInBytecode(f, 'LOAD_FAST_CHECK')

    def test_load_fast_unknown_after_error_2(self):
        def f():
            try:
                1 / 0
            except ZeroDivisionError:
                print(a, b, c, d, e, f, g)
            a = b = c = d = e = f = g = 1
        self.assertInBytecode(f, 'LOAD_FAST_CHECK')
        self.assertNotInBytecode(f, 'LOAD_FAST')

    def test_load_fast_too_many_locals(self):
        # When there get to be too many locals to analyze completely,
        # later locals are all converted to LOAD_FAST_CHECK, except
        # when a store or prior load occurred in the same basicblock.
        def f():
            a00 = a01 = a02 = a03 = a04 = a05 = a06 = a07 = a08 = a09 = 1
            a10 = a11 = a12 = a13 = a14 = a15 = a16 = a17 = a18 = a19 = 1
            a20 = a21 = a22 = a23 = a24 = a25 = a26 = a27 = a28 = a29 = 1
            a30 = a31 = a32 = a33 = a34 = a35 = a36 = a37 = a38 = a39 = 1
            a40 = a41 = a42 = a43 = a44 = a45 = a46 = a47 = a48 = a49 = 1
            a50 = a51 = a52 = a53 = a54 = a55 = a56 = a57 = a58 = a59 = 1
            a60 = a61 = a62 = a63 = a64 = a65 = a66 = a67 = a68 = a69 = 1
            a70 = a71 = a72 = a73 = a74 = a75 = a76 = a77 = a78 = a79 = 1
            del a72, a73
            print(a73)
            print(a70, a71, a72, a73)
            while True:
                print(a00, a01, a62, a63)
                print(a64, a65, a78, a79)

        self.assertInBytecode(f, 'LOAD_FAST_BORROW_LOAD_FAST_BORROW', ("a00", "a01"))
        self.assertNotInBytecode(f, 'LOAD_FAST_CHECK', "a00")
        self.assertNotInBytecode(f, 'LOAD_FAST_CHECK', "a01")
        for i in 62, 63:
            # First 64 locals: analyze completely
            self.assertInBytecode(f, 'LOAD_FAST_BORROW', f"a{i:02}")
            self.assertNotInBytecode(f, 'LOAD_FAST_CHECK', f"a{i:02}")
        for i in 64, 65, 78, 79:
            # Locals >=64 not in the same basicblock
            self.assertInBytecode(f, 'LOAD_FAST_CHECK', f"a{i:02}")
            self.assertNotInBytecode(f, 'LOAD_FAST', f"a{i:02}")
        for i in 70, 71:
            # Locals >=64 in the same basicblock
            self.assertInBytecode(f, 'LOAD_FAST_BORROW', f"a{i:02}")
            self.assertNotInBytecode(f, 'LOAD_FAST_CHECK', f"a{i:02}")
        # del statements should invalidate within basicblocks.
        self.assertInBytecode(f, 'LOAD_FAST_CHECK', "a72")
        self.assertNotInBytecode(f, 'LOAD_FAST', "a72")
        # previous checked loads within a basicblock enable unchecked loads
        self.assertInBytecode(f, 'LOAD_FAST_CHECK', "a73")
        self.assertInBytecode(f, 'LOAD_FAST_BORROW', "a73")

    def test_setting_lineno_no_undefined(self):
        code = textwrap.dedent("""\
            def f():
                x = y = 2
                if not x:
                    return 4
                for i in range(55):
                    x + 6
                L = 7
                L = 8
                L = 9
                L = 10
        """)
        ns = {}
        exec(code, ns)
        f = ns['f']
        self.assertInBytecode(f, "LOAD_FAST_BORROW")
        self.assertNotInBytecode(f, "LOAD_FAST_CHECK")
        co_code = f.__code__.co_code
        def trace(frame, event, arg):
            if event == 'line' and frame.f_lineno == 9:
                frame.f_lineno = 3
                sys.settrace(None)
                return None
            return trace
        sys.settrace(trace)
        result = f()
        self.assertIsNone(result)
        self.assertInBytecode(f, "LOAD_FAST_BORROW")
        self.assertNotInBytecode(f, "LOAD_FAST_CHECK")
        self.assertEqual(f.__code__.co_code, co_code)

    def test_setting_lineno_one_undefined(self):
        code = textwrap.dedent("""\
            def f():
                x = y = 2
                if not x:
                    return 4
                for i in range(55):
                    x + 6
                del x
                L = 8
                L = 9
                L = 10
        """)
        ns = {}
        exec(code, ns)
        f = ns['f']
        self.assertInBytecode(f, "LOAD_FAST_BORROW")
        self.assertNotInBytecode(f, "LOAD_FAST_CHECK")
        co_code = f.__code__.co_code
        def trace(frame, event, arg):
            if event == 'line' and frame.f_lineno == 9:
                frame.f_lineno = 3
                sys.settrace(None)
                return None
            return trace
        e = r"assigning None to 1 unbound local"
        with self.assertWarnsRegex(RuntimeWarning, e):
            sys.settrace(trace)
            result = f()
        self.assertEqual(result, 4)
        self.assertInBytecode(f, "LOAD_FAST_BORROW")
        self.assertNotInBytecode(f, "LOAD_FAST_CHECK")
        self.assertEqual(f.__code__.co_code, co_code)

    def test_setting_lineno_two_undefined(self):
        code = textwrap.dedent("""\
            def f():
                x = y = 2
                if not x:
                    return 4
                for i in range(55):
                    x + 6
                del x, y
                L = 8
                L = 9
                L = 10
        """)
        ns = {}
        exec(code, ns)
        f = ns['f']
        self.assertInBytecode(f, "LOAD_FAST_BORROW")
        self.assertNotInBytecode(f, "LOAD_FAST_CHECK")
        co_code = f.__code__.co_code
        def trace(frame, event, arg):
            if event == 'line' and frame.f_lineno == 9:
                frame.f_lineno = 3
                sys.settrace(None)
                return None
            return trace
        e = r"assigning None to 2 unbound locals"
        with self.assertWarnsRegex(RuntimeWarning, e):
            sys.settrace(trace)
            result = f()
        self.assertEqual(result, 4)
        self.assertInBytecode(f, "LOAD_FAST_BORROW")
        self.assertNotInBytecode(f, "LOAD_FAST_CHECK")
        self.assertEqual(f.__code__.co_code, co_code)

    def make_function_with_no_checks(self):
        code = textwrap.dedent("""\
            def f():
                x = 2
                L = 3
                L = 4
                L = 5
                if not L:
                    x + 7
                    y = 2
        """)
        ns = {}
        exec(code, ns)
        f = ns['f']
        self.assertInBytecode(f, "LOAD_FAST_BORROW")
        self.assertNotInBytecode(f, "LOAD_FAST_CHECK")
        return f

    def test_modifying_local_does_not_add_check(self):
        f = self.make_function_with_no_checks()
        def trace(frame, event, arg):
            if event == 'line' and frame.f_lineno == 4:
                frame.f_locals["x"] = 42
                sys.settrace(None)
                return None
            return trace
        sys.settrace(trace)
        f()
        self.assertInBytecode(f, "LOAD_FAST_BORROW")
        self.assertNotInBytecode(f, "LOAD_FAST_CHECK")

    def test_initializing_local_does_not_add_check(self):
        f = self.make_function_with_no_checks()
        def trace(frame, event, arg):
            if event == 'line' and frame.f_lineno == 4:
                frame.f_locals["y"] = 42
                sys.settrace(None)
                return None
            return trace
        sys.settrace(trace)
        f()
        self.assertInBytecode(f, "LOAD_FAST_BORROW")
        self.assertNotInBytecode(f, "LOAD_FAST_CHECK")


class DirectCfgOptimizerTests(CfgOptimizationTestCase):

    def cfg_optimization_test(self, insts, expected_insts,
                              consts=None, expected_consts=None,
                              nlocals=0):

        self.check_instructions(insts)
        self.check_instructions(expected_insts)

        if expected_consts is None:
            expected_consts = consts
        seq = self.seq_from_insts(insts)
        opt_insts, opt_consts = self.get_optimized(seq, consts, nlocals)
        expected_insts = self.seq_from_insts(expected_insts).get_instructions()
        self.assertInstructionsMatch(opt_insts, expected_insts)
        self.assertEqual(opt_consts, expected_consts)

    def test_conditional_jump_forward_non_const_condition(self):
        insts = [
            ('LOAD_NAME', 1, 11),
            ('POP_JUMP_IF_TRUE', lbl := self.Label(), 12),
            ('LOAD_CONST', 2, 13),
            ('RETURN_VALUE', None, 13),
            lbl,
            ('LOAD_CONST', 3, 14),
            ('RETURN_VALUE', None, 14),
        ]
        expected_insts = [
            ('LOAD_NAME', 1, 11),
            ('POP_JUMP_IF_TRUE', lbl := self.Label(), 12),
            ('LOAD_SMALL_INT', 2, 13),
            ('RETURN_VALUE', None, 13),
            lbl,
            ('LOAD_SMALL_INT', 3, 14),
            ('RETURN_VALUE', None, 14),
        ]
        self.cfg_optimization_test(insts,
                                   expected_insts,
                                   consts=[0, 1, 2, 3, 4],
                                   expected_consts=[0])

    def test_list_exceeding_stack_use_guideline(self):
        def f():
            return [
                0, 1, 2, 3, 4,
                5, 6, 7, 8, 9,
                10, 11, 12, 13, 14,
                15, 16, 17, 18, 19,
                20, 21, 22, 23, 24,
                25, 26, 27, 28, 29,
                30, 31, 32, 33, 34,
                35, 36, 37, 38, 39
            ]
        self.assertEqual(f(), list(range(40)))

    def test_set_exceeding_stack_use_guideline(self):
        def f():
            return {
                0, 1, 2, 3, 4,
                5, 6, 7, 8, 9,
                10, 11, 12, 13, 14,
                15, 16, 17, 18, 19,
                20, 21, 22, 23, 24,
                25, 26, 27, 28, 29,
                30, 31, 32, 33, 34,
                35, 36, 37, 38, 39
            }
        self.assertEqual(f(), frozenset(range(40)))

    def test_nested_const_foldings(self):
        # (1, (--2 + ++2 * 2 // 2 - 2, )[0], ~~3, not not True)  ==>  (1, 2, 3, True)
        intrinsic_positive = 5
        before = [
            ('LOAD_SMALL_INT', 1, 0),
            ('NOP', None, 0),
            ('LOAD_SMALL_INT', 2, 0),
            ('UNARY_NEGATIVE', None, 0),
            ('NOP', None, 0),
            ('UNARY_NEGATIVE', None, 0),
            ('NOP', None, 0),
            ('NOP', None, 0),
            ('LOAD_SMALL_INT', 2, 0),
            ('CALL_INTRINSIC_1', intrinsic_positive, 0),
            ('NOP', None, 0),
            ('CALL_INTRINSIC_1', intrinsic_positive, 0),
            ('BINARY_OP', get_binop_argval('NB_MULTIPLY')),
            ('LOAD_SMALL_INT', 2, 0),
            ('NOP', None, 0),
            ('BINARY_OP', get_binop_argval('NB_FLOOR_DIVIDE')),
            ('NOP', None, 0),
            ('LOAD_SMALL_INT', 2, 0),
            ('BINARY_OP', get_binop_argval('NB_ADD')),
            ('NOP', None, 0),
            ('LOAD_SMALL_INT', 2, 0),
            ('NOP', None, 0),
            ('BINARY_OP', get_binop_argval('NB_SUBTRACT')),
            ('NOP', None, 0),
            ('BUILD_TUPLE', 1, 0),
            ('LOAD_SMALL_INT', 0, 0),
            ('BINARY_OP', get_binop_argval('NB_SUBSCR'), 0),
            ('NOP', None, 0),
            ('LOAD_SMALL_INT', 3, 0),
            ('NOP', None, 0),
            ('UNARY_INVERT', None, 0),
            ('NOP', None, 0),
            ('UNARY_INVERT', None, 0),
            ('NOP', None, 0),
            ('LOAD_SMALL_INT', 3, 0),
            ('NOP', None, 0),
            ('UNARY_NOT', None, 0),
            ('NOP', None, 0),
            ('UNARY_NOT', None, 0),
            ('NOP', None, 0),
            ('BUILD_TUPLE', 4, 0),
            ('NOP', None, 0),
            ('RETURN_VALUE', None, 0)
        ]
        after = [
            ('LOAD_CONST', 1, 0),
            ('RETURN_VALUE', None, 0)
        ]
        self.cfg_optimization_test(before, after, consts=[], expected_consts=[-2, (1, 2, 3, True)])

    def test_build_empty_tuple(self):
        before = [
            ('BUILD_TUPLE', 0, 0),
            ('RETURN_VALUE', None, 0),
        ]
        after = [
            ('LOAD_CONST', 0, 0),
            ('RETURN_VALUE', None, 0),
        ]
        self.cfg_optimization_test(before, after, consts=[], expected_consts=[()])

    def test_fold_tuple_of_constants(self):
        before = [
            ('NOP', None, 0),
            ('LOAD_SMALL_INT', 1, 0),
            ('NOP', None, 0),
            ('LOAD_SMALL_INT', 2, 0),
            ('NOP', None, 0),
            ('NOP', None, 0),
            ('LOAD_SMALL_INT', 3, 0),
            ('NOP', None, 0),
            ('BUILD_TUPLE', 3, 0),
            ('RETURN_VALUE', None, 0),
        ]
        after = [
            ('LOAD_CONST', 0, 0),
            ('RETURN_VALUE', None, 0),
        ]
        self.cfg_optimization_test(before, after, consts=[], expected_consts=[(1, 2, 3)])

        # not all consts
        same = [
            ('LOAD_SMALL_INT', 1, 0),
            ('LOAD_NAME', 0, 0),
            ('LOAD_SMALL_INT', 2, 0),
            ('BUILD_TUPLE', 3, 0),
            ('RETURN_VALUE', None, 0)
        ]
        self.cfg_optimization_test(same, same, consts=[])

    def test_fold_constant_intrinsic_list_to_tuple(self):
        INTRINSIC_LIST_TO_TUPLE = 6

        # long tuple
        consts = 1000
        before = (
            [('BUILD_LIST', 0, 0)] +
            [('LOAD_CONST', 0, 0), ('LIST_APPEND', 1, 0)] * consts +
            [('CALL_INTRINSIC_1', INTRINSIC_LIST_TO_TUPLE, 0), ('RETURN_VALUE', None, 0)]
        )
        after = [
            ('LOAD_CONST', 1, 0),
            ('RETURN_VALUE', None, 0)
        ]
        result_const = tuple(["test"] * consts)
        self.cfg_optimization_test(before, after, consts=["test"], expected_consts=["test", result_const])

        # empty list
        before = [
            ('BUILD_LIST', 0, 0),
            ('CALL_INTRINSIC_1', INTRINSIC_LIST_TO_TUPLE, 0),
            ('RETURN_VALUE', None, 0)
        ]
        after = [
            ('LOAD_CONST', 0, 0),
            ('RETURN_VALUE', None, 0)
        ]
        self.cfg_optimization_test(before, after, consts=[], expected_consts=[()])

        # multiple BUILD_LIST 0: ([], 1, [], 2)
        same = [
            ('BUILD_LIST', 0, 0),
            ('BUILD_LIST', 0, 0),
            ('LIST_APPEND', 1, 0),
            ('LOAD_SMALL_INT', 1, 0),
            ('LIST_APPEND', 1, 0),
            ('BUILD_LIST', 0, 0),
            ('LIST_APPEND', 1, 0),
            ('LOAD_SMALL_INT', 2, 0),
            ('LIST_APPEND', 1, 0),
            ('CALL_INTRINSIC_1', INTRINSIC_LIST_TO_TUPLE, 0),
            ('RETURN_VALUE', None, 0)
        ]
        self.cfg_optimization_test(same, same, consts=[])

        # nested folding: (1, 1+1, 3)
        before = [
            ('BUILD_LIST', 0, 0),
            ('LOAD_SMALL_INT', 1, 0),
            ('LIST_APPEND', 1, 0),
            ('LOAD_SMALL_INT', 1, 0),
            ('LOAD_SMALL_INT', 1, 0),
            ('BINARY_OP', get_binop_argval('NB_ADD'), 0),
            ('LIST_APPEND', 1, 0),
            ('LOAD_SMALL_INT', 3, 0),
            ('LIST_APPEND', 1, 0),
            ('CALL_INTRINSIC_1', INTRINSIC_LIST_TO_TUPLE, 0),
            ('RETURN_VALUE', None, 0)
        ]
        after = [
            ('LOAD_CONST', 0, 0),
            ('RETURN_VALUE', None, 0)
        ]
        self.cfg_optimization_test(before, after, consts=[], expected_consts=[(1, 2, 3)])

        # NOP's in between: (1, 2, 3)
        before = [
            ('BUILD_LIST', 0, 0),
            ('NOP', None, 0),
            ('LOAD_SMALL_INT', 1, 0),
            ('NOP', None, 0),
            ('NOP', None, 0),
            ('LIST_APPEND', 1, 0),
            ('NOP', None, 0),
            ('LOAD_SMALL_INT', 2, 0),
            ('NOP', None, 0),
            ('NOP', None, 0),
            ('LIST_APPEND', 1, 0),
            ('NOP', None, 0),
            ('LOAD_SMALL_INT', 3, 0),
            ('NOP', None, 0),
            ('LIST_APPEND', 1, 0),
            ('NOP', None, 0),
            ('CALL_INTRINSIC_1', INTRINSIC_LIST_TO_TUPLE, 0),
            ('RETURN_VALUE', None, 0)
        ]
        after = [
            ('LOAD_CONST', 0, 0),
            ('RETURN_VALUE', None, 0)
        ]
        self.cfg_optimization_test(before, after, consts=[], expected_consts=[(1, 2, 3)])

    def test_optimize_if_const_list(self):
        before = [
            ('NOP', None, 0),
            ('LOAD_SMALL_INT', 1, 0),
            ('NOP', None, 0),
            ('LOAD_SMALL_INT', 2, 0),
            ('NOP', None, 0),
            ('NOP', None, 0),
            ('LOAD_SMALL_INT', 3, 0),
            ('NOP', None, 0),
            ('BUILD_LIST', 3, 0),
            ('RETURN_VALUE', None, 0),
        ]
        after = [
            ('BUILD_LIST', 0, 0),
            ('LOAD_CONST', 0, 0),
            ('LIST_EXTEND', 1, 0),
            ('RETURN_VALUE', None, 0),
        ]
        self.cfg_optimization_test(before, after, consts=[], expected_consts=[(1, 2, 3)])

        # need minimum 3 consts to optimize
        same = [
            ('LOAD_SMALL_INT', 1, 0),
            ('LOAD_SMALL_INT', 2, 0),
            ('BUILD_LIST', 2, 0),
            ('RETURN_VALUE', None, 0),
        ]
        self.cfg_optimization_test(same, same, consts=[])

        # not all consts
        same = [
            ('LOAD_SMALL_INT', 1, 0),
            ('LOAD_NAME', 0, 0),
            ('LOAD_SMALL_INT', 3, 0),
            ('BUILD_LIST', 3, 0),
            ('RETURN_VALUE', None, 0),
        ]
        self.cfg_optimization_test(same, same, consts=[])

    def test_optimize_if_const_set(self):
        before = [
            ('NOP', None, 0),
            ('LOAD_SMALL_INT', 1, 0),
            ('NOP', None, 0),
            ('LOAD_SMALL_INT', 2, 0),
            ('NOP', None, 0),
            ('NOP', None, 0),
            ('LOAD_SMALL_INT', 3, 0),
            ('NOP', None, 0),
            ('BUILD_SET', 3, 0),
            ('RETURN_VALUE', None, 0),
        ]
        after = [
            ('BUILD_SET', 0, 0),
            ('LOAD_CONST', 0, 0),
            ('SET_UPDATE', 1, 0),
            ('RETURN_VALUE', None, 0),
        ]
        self.cfg_optimization_test(before, after, consts=[], expected_consts=[frozenset({1, 2, 3})])

        # need minimum 3 consts to optimize
        same = [
            ('LOAD_SMALL_INT', 1, 0),
            ('LOAD_SMALL_INT', 2, 0),
            ('BUILD_SET', 2, 0),
            ('RETURN_VALUE', None, 0),
        ]
        self.cfg_optimization_test(same, same, consts=[])

        # not all consts
        same = [
            ('LOAD_SMALL_INT', 1, 0),
            ('LOAD_NAME', 0, 0),
            ('LOAD_SMALL_INT', 3, 0),
            ('BUILD_SET', 3, 0),
            ('RETURN_VALUE', None, 0),
        ]
        self.cfg_optimization_test(same, same, consts=[])

    def test_optimize_literal_list_for_iter(self):
        # for _ in [1, 2]: pass  ==>  for _ in (1, 2): pass
        before = [
            ('LOAD_SMALL_INT', 1, 0),
            ('LOAD_SMALL_INT', 2, 0),
            ('BUILD_LIST', 2, 0),
            ('GET_ITER', None, 0),
            start := self.Label(),
            ('FOR_ITER', end := self.Label(), 0),
            ('STORE_FAST', 0, 0),
            ('JUMP', start, 0),
            end,
            ('END_FOR', None, 0),
            ('POP_ITER', None, 0),
            ('LOAD_CONST', 0, 0),
            ('RETURN_VALUE', None, 0),
        ]
        after = [
            ('LOAD_CONST', 1, 0),
            ('GET_ITER', None, 0),
            start := self.Label(),
            ('FOR_ITER', end := self.Label(), 0),
            ('STORE_FAST', 0, 0),
            ('JUMP', start, 0),
            end,
            ('END_FOR', None, 0),
            ('POP_ITER', None, 0),
            ('LOAD_CONST', 0, 0),
            ('RETURN_VALUE', None, 0),
        ]
        self.cfg_optimization_test(before, after, consts=[None], expected_consts=[None, (1, 2)])

        # for _ in [1, x]: pass  ==>  for _ in (1, x): pass
        before = [
            ('LOAD_SMALL_INT', 1, 0),
            ('LOAD_NAME', 0, 0),
            ('BUILD_LIST', 2, 0),
            ('GET_ITER', None, 0),
            start := self.Label(),
            ('FOR_ITER', end := self.Label(), 0),
            ('STORE_FAST', 0, 0),
            ('JUMP', start, 0),
            end,
            ('END_FOR', None, 0),
            ('POP_ITER', None, 0),
            ('LOAD_CONST', 0, 0),
            ('RETURN_VALUE', None, 0),
        ]
        after = [
            ('LOAD_SMALL_INT', 1, 0),
            ('LOAD_NAME', 0, 0),
            ('BUILD_TUPLE', 2, 0),
            ('GET_ITER', None, 0),
            start := self.Label(),
            ('FOR_ITER', end := self.Label(), 0),
            ('STORE_FAST', 0, 0),
            ('JUMP', start, 0),
            end,
            ('END_FOR', None, 0),
            ('POP_ITER', None, 0),
            ('LOAD_CONST', 0, 0),
            ('RETURN_VALUE', None, 0),
        ]
        self.cfg_optimization_test(before, after, consts=[None], expected_consts=[None])

    def test_optimize_literal_set_for_iter(self):
        # for _ in {1, 2}: pass  ==>  for _ in (1, 2): pass
        before = [
            ('LOAD_SMALL_INT', 1, 0),
            ('LOAD_SMALL_INT', 2, 0),
            ('BUILD_SET', 2, 0),
            ('GET_ITER', None, 0),
            start := self.Label(),
            ('FOR_ITER', end := self.Label(), 0),
            ('STORE_FAST', 0, 0),
            ('JUMP', start, 0),
            end,
            ('END_FOR', None, 0),
            ('POP_ITER', None, 0),
            ('LOAD_CONST', 0, 0),
            ('RETURN_VALUE', None, 0),
        ]
        after = [
            ('LOAD_CONST', 1, 0),
            ('GET_ITER', None, 0),
            start := self.Label(),
            ('FOR_ITER', end := self.Label(), 0),
            ('STORE_FAST', 0, 0),
            ('JUMP', start, 0),
            end,
            ('END_FOR', None, 0),
            ('POP_ITER', None, 0),
            ('LOAD_CONST', 0, 0),
            ('RETURN_VALUE', None, 0),
        ]
        self.cfg_optimization_test(before, after, consts=[None], expected_consts=[None, frozenset({1, 2})])

        # non constant literal set is not changed
        # for _ in {1, x}: pass  ==>  for _ in {1, x}: pass
        same = [
            ('LOAD_SMALL_INT', 1, 0),
            ('LOAD_NAME', 0, 0),
            ('BUILD_SET', 2, 0),
            ('GET_ITER', None, 0),
            start := self.Label(),
            ('FOR_ITER', end := self.Label(), 0),
            ('STORE_FAST', 0, 0),
            ('JUMP', start, 0),
            end,
            ('END_FOR', None, 0),
            ('POP_ITER', None, 0),
            ('LOAD_CONST', 0, 0),
            ('RETURN_VALUE', None, 0),
        ]
        self.cfg_optimization_test(same, same, consts=[None], expected_consts=[None])

    def test_optimize_literal_list_contains(self):
        # x in [1, 2]  ==>  x in (1, 2)
        before = [
            ('LOAD_NAME', 0, 0),
            ('LOAD_SMALL_INT', 1, 0),
            ('LOAD_SMALL_INT', 2, 0),
            ('BUILD_LIST', 2, 0),
            ('CONTAINS_OP', 0, 0),
            ('POP_TOP', None, 0),
            ('LOAD_CONST', 0, 0),
            ('RETURN_VALUE', None, 0),
        ]
        after = [
            ('LOAD_NAME', 0, 0),
            ('LOAD_CONST', 1, 0),
            ('CONTAINS_OP', 0, 0),
            ('POP_TOP', None, 0),
            ('LOAD_CONST', 0, 0),
            ('RETURN_VALUE', None, 0),
        ]
        self.cfg_optimization_test(before, after, consts=[None], expected_consts=[None, (1, 2)])

        # x in [1, y]  ==>  x in (1, y)
        before = [
            ('LOAD_NAME', 0, 0),
            ('LOAD_SMALL_INT', 1, 0),
            ('LOAD_NAME', 1, 0),
            ('BUILD_LIST', 2, 0),
            ('CONTAINS_OP', 0, 0),
            ('POP_TOP', None, 0),
            ('LOAD_CONST', 0, 0),
            ('RETURN_VALUE', None, 0),
        ]
        after = [
            ('LOAD_NAME', 0, 0),
            ('LOAD_SMALL_INT', 1, 0),
            ('LOAD_NAME', 1, 0),
            ('BUILD_TUPLE', 2, 0),
            ('CONTAINS_OP', 0, 0),
            ('POP_TOP', None, 0),
            ('LOAD_CONST', 0, 0),
            ('RETURN_VALUE', None, 0),
        ]
        self.cfg_optimization_test(before, after, consts=[None], expected_consts=[None])

    def test_optimize_literal_set_contains(self):
        # x in {1, 2}  ==>  x in (1, 2)
        before = [
            ('LOAD_NAME', 0, 0),
            ('LOAD_SMALL_INT', 1, 0),
            ('LOAD_SMALL_INT', 2, 0),
            ('BUILD_SET', 2, 0),
            ('CONTAINS_OP', 0, 0),
            ('POP_TOP', None, 0),
            ('LOAD_CONST', 0, 0),
            ('RETURN_VALUE', None, 0),
        ]
        after = [
            ('LOAD_NAME', 0, 0),
            ('LOAD_CONST', 1, 0),
            ('CONTAINS_OP', 0, 0),
            ('POP_TOP', None, 0),
            ('LOAD_CONST', 0, 0),
            ('RETURN_VALUE', None, 0),
        ]
        self.cfg_optimization_test(before, after, consts=[None], expected_consts=[None, frozenset({1, 2})])

        # non constant literal set is not changed
        # x in {1, y}  ==>  x in {1, y}
        same = [
            ('LOAD_NAME', 0, 0),
            ('LOAD_SMALL_INT', 1, 0),
            ('LOAD_NAME', 1, 0),
            ('BUILD_SET', 2, 0),
            ('CONTAINS_OP', 0, 0),
            ('POP_TOP', None, 0),
            ('LOAD_CONST', 0, 0),
            ('RETURN_VALUE', None, 0),
        ]
        self.cfg_optimization_test(same, same, consts=[None], expected_consts=[None])

    def test_optimize_unary_not(self):
        # test folding
        before = [
            ('LOAD_SMALL_INT', 1, 0),
            ('TO_BOOL', None, 0),
            ('UNARY_NOT', None, 0),
            ('RETURN_VALUE', None, 0),
        ]
        after = [
            ('LOAD_CONST', 1, 0),
            ('RETURN_VALUE', None, 0),
        ]
        self.cfg_optimization_test(before, after, consts=[], expected_consts=[True, False])

        # test cancel out
        before = [
            ('LOAD_NAME', 0, 0),
            ('TO_BOOL', None, 0),
            ('UNARY_NOT', None, 0),
            ('UNARY_NOT', None, 0),
            ('UNARY_NOT', None, 0),
            ('UNARY_NOT', None, 0),
            ('RETURN_VALUE', None, 0),
        ]
        after = [
            ('LOAD_NAME', 0, 0),
            ('TO_BOOL', None, 0),
            ('RETURN_VALUE', None, 0),
        ]
        self.cfg_optimization_test(before, after, consts=[], expected_consts=[])

        # test eliminate to bool
        before = [
            ('LOAD_NAME', 0, 0),
            ('TO_BOOL', None, 0),
            ('UNARY_NOT', None, 0),
            ('TO_BOOL', None, 0),
            ('TO_BOOL', None, 0),
            ('TO_BOOL', None, 0),
            ('RETURN_VALUE', None, 0),
        ]
        after = [
            ('LOAD_NAME', 0, 0),
            ('TO_BOOL', None, 0),
            ('UNARY_NOT', None, 0),
            ('RETURN_VALUE', None, 0),
        ]
        self.cfg_optimization_test(before, after, consts=[], expected_consts=[])

        # test folding & cancel out
        before = [
            ('LOAD_SMALL_INT', 1, 0),
            ('TO_BOOL', None, 0),
            ('UNARY_NOT', None, 0),
            ('UNARY_NOT', None, 0),
            ('UNARY_NOT', None, 0),
            ('UNARY_NOT', None, 0),
            ('RETURN_VALUE', None, 0),
        ]
        after = [
            ('LOAD_CONST', 0, 0),
            ('RETURN_VALUE', None, 0),
        ]
        self.cfg_optimization_test(before, after, consts=[], expected_consts=[True])

        # test folding & eliminate to bool
        before = [
            ('LOAD_SMALL_INT', 1, 0),
            ('TO_BOOL', None, 0),
            ('UNARY_NOT', None, 0),
            ('TO_BOOL', None, 0),
            ('TO_BOOL', None, 0),
            ('TO_BOOL', None, 0),
            ('RETURN_VALUE', None, 0),
        ]
        after = [
            ('LOAD_CONST', 1, 0),
            ('RETURN_VALUE', None, 0),
        ]
        self.cfg_optimization_test(before, after, consts=[], expected_consts=[True, False])

        # test cancel out & eliminate to bool (to bool stays as we are not iterating to a fixed point)
        before = [
            ('LOAD_NAME', 0, 0),
            ('TO_BOOL', None, 0),
            ('UNARY_NOT', None, 0),
            ('UNARY_NOT', None, 0),
            ('UNARY_NOT', None, 0),
            ('UNARY_NOT', None, 0),
            ('TO_BOOL', None, 0),
            ('RETURN_VALUE', None, 0),
        ]
        after = [
            ('LOAD_NAME', 0, 0),
            ('TO_BOOL', None, 0),
            ('TO_BOOL', None, 0),
            ('RETURN_VALUE', None, 0),
        ]
        self.cfg_optimization_test(before, after, consts=[], expected_consts=[])

        is_ = in_ = 0
        isnot = notin = 1

        # test is/isnot
        before = [
            ('LOAD_NAME', 0, 0),
            ('LOAD_NAME', 1, 0),
            ('IS_OP', is_, 0),
            ('UNARY_NOT', None, 0),
            ('RETURN_VALUE', None, 0),
        ]
        after = [
            ('LOAD_NAME', 0, 0),
            ('LOAD_NAME', 1, 0),
            ('IS_OP', isnot, 0),
            ('RETURN_VALUE', None, 0),
        ]
        self.cfg_optimization_test(before, after, consts=[], expected_consts=[])

        # test is/isnot cancel out
        before = [
            ('LOAD_NAME', 0, 0),
            ('LOAD_NAME', 1, 0),
            ('IS_OP', is_, 0),
            ('UNARY_NOT', None, 0),
            ('UNARY_NOT', None, 0),
            ('RETURN_VALUE', None, 0),
        ]
        after = [
            ('LOAD_NAME', 0, 0),
            ('LOAD_NAME', 1, 0),
            ('IS_OP', is_, 0),
            ('RETURN_VALUE', None, 0),
        ]
        self.cfg_optimization_test(before, after, consts=[], expected_consts=[])

        # test is/isnot eliminate to bool
        before = [
            ('LOAD_NAME', 0, 0),
            ('LOAD_NAME', 1, 0),
            ('IS_OP', is_, 0),
            ('UNARY_NOT', None, 0),
            ('TO_BOOL', None, 0),
            ('TO_BOOL', None, 0),
            ('TO_BOOL', None, 0),
            ('RETURN_VALUE', None, 0),
        ]
        after = [
            ('LOAD_NAME', 0, 0),
            ('LOAD_NAME', 1, 0),
            ('IS_OP', isnot, 0),
            ('RETURN_VALUE', None, 0),
        ]
        self.cfg_optimization_test(before, after, consts=[], expected_consts=[])

        # test is/isnot cancel out & eliminate to bool
        before = [
            ('LOAD_NAME', 0, 0),
            ('LOAD_NAME', 1, 0),
            ('IS_OP', is_, 0),
            ('UNARY_NOT', None, 0),
            ('UNARY_NOT', None, 0),
            ('TO_BOOL', None, 0),
            ('TO_BOOL', None, 0),
            ('TO_BOOL', None, 0),
            ('RETURN_VALUE', None, 0),
        ]
        after = [
            ('LOAD_NAME', 0, 0),
            ('LOAD_NAME', 1, 0),
            ('IS_OP', is_, 0),
            ('RETURN_VALUE', None, 0),
        ]
        self.cfg_optimization_test(before, after, consts=[], expected_consts=[])

        # test in/notin
        before = [
            ('LOAD_NAME', 0, 0),
            ('LOAD_NAME', 1, 0),
            ('CONTAINS_OP', in_, 0),
            ('UNARY_NOT', None, 0),
            ('RETURN_VALUE', None, 0),
        ]
        after = [
            ('LOAD_NAME', 0, 0),
            ('LOAD_NAME', 1, 0),
            ('CONTAINS_OP', notin, 0),
            ('RETURN_VALUE', None, 0),
        ]
        self.cfg_optimization_test(before, after, consts=[], expected_consts=[])

        # test in/notin cancel out
        before = [
            ('LOAD_NAME', 0, 0),
            ('LOAD_NAME', 1, 0),
            ('CONTAINS_OP', in_, 0),
            ('UNARY_NOT', None, 0),
            ('UNARY_NOT', None, 0),
            ('RETURN_VALUE', None, 0),
        ]
        after = [
            ('LOAD_NAME', 0, 0),
            ('LOAD_NAME', 1, 0),
            ('CONTAINS_OP', in_, 0),
            ('RETURN_VALUE', None, 0),
        ]
        self.cfg_optimization_test(before, after, consts=[], expected_consts=[])

        # test is/isnot & eliminate to bool
        before = [
            ('LOAD_NAME', 0, 0),
            ('LOAD_NAME', 1, 0),
            ('CONTAINS_OP', in_, 0),
            ('UNARY_NOT', None, 0),
            ('TO_BOOL', None, 0),
            ('TO_BOOL', None, 0),
            ('TO_BOOL', None, 0),
            ('RETURN_VALUE', None, 0),
        ]
        after = [
            ('LOAD_NAME', 0, 0),
            ('LOAD_NAME', 1, 0),
            ('CONTAINS_OP', notin, 0),
            ('RETURN_VALUE', None, 0),
        ]
        self.cfg_optimization_test(before, after, consts=[], expected_consts=[])

        # test in/notin cancel out & eliminate to bool
        before = [
            ('LOAD_NAME', 0, 0),
            ('LOAD_NAME', 1, 0),
            ('CONTAINS_OP', in_, 0),
            ('UNARY_NOT', None, 0),
            ('UNARY_NOT', None, 0),
            ('TO_BOOL', None, 0),
            ('RETURN_VALUE', None, 0),
        ]
        after = [
            ('LOAD_NAME', 0, 0),
            ('LOAD_NAME', 1, 0),
            ('CONTAINS_OP', in_, 0),
            ('RETURN_VALUE', None, 0),
        ]
        self.cfg_optimization_test(before, after, consts=[], expected_consts=[])

    def test_optimize_if_const_unaryop(self):
        # test unary negative
        before = [
            ('LOAD_SMALL_INT', 2, 0),
            ('UNARY_NEGATIVE', None, 0),
            ('UNARY_NEGATIVE', None, 0),
            ('RETURN_VALUE', None, 0)
        ]
        after = [
            ('LOAD_SMALL_INT', 2, 0),
            ('RETURN_VALUE', None, 0),
        ]
        self.cfg_optimization_test(before, after, consts=[], expected_consts=[-2])

        # test unary invert
        before = [
            ('LOAD_SMALL_INT', 2, 0),
            ('UNARY_INVERT', None, 0),
            ('UNARY_INVERT', None, 0),
            ('RETURN_VALUE', None, 0)
        ]
        after = [
            ('LOAD_SMALL_INT', 2, 0),
            ('RETURN_VALUE', None, 0),
        ]
        self.cfg_optimization_test(before, after, consts=[], expected_consts=[-3])

        # test unary positive
        before = [
            ('LOAD_SMALL_INT', 2, 0),
            ('CALL_INTRINSIC_1', 5, 0),
            ('CALL_INTRINSIC_1', 5, 0),
            ('RETURN_VALUE', None, 0)
        ]
        after = [
            ('LOAD_SMALL_INT', 2, 0),
            ('RETURN_VALUE', None, 0),
        ]
        self.cfg_optimization_test(before, after, consts=[], expected_consts=[])

    def test_optimize_if_const_binop(self):
        add = get_binop_argval('NB_ADD')
        sub = get_binop_argval('NB_SUBTRACT')
        mul = get_binop_argval('NB_MULTIPLY')
        div = get_binop_argval('NB_TRUE_DIVIDE')
        floor = get_binop_argval('NB_FLOOR_DIVIDE')
        rem = get_binop_argval('NB_REMAINDER')
        pow = get_binop_argval('NB_POWER')
        lshift = get_binop_argval('NB_LSHIFT')
        rshift = get_binop_argval('NB_RSHIFT')
        or_ = get_binop_argval('NB_OR')
        and_ = get_binop_argval('NB_AND')
        xor = get_binop_argval('NB_XOR')
        subscr = get_binop_argval('NB_SUBSCR')

        # test add
        before = [
            ('LOAD_SMALL_INT', 2, 0),
            ('LOAD_SMALL_INT', 2, 0),
            ('BINARY_OP', add, 0),
            ('LOAD_SMALL_INT', 2, 0),
            ('BINARY_OP', add, 0),
            ('RETURN_VALUE', None, 0)
        ]
        after = [
            ('LOAD_SMALL_INT', 6, 0),
            ('RETURN_VALUE', None, 0)
        ]
        self.cfg_optimization_test(before, after, consts=[], expected_consts=[])

        # test sub
        before = [
            ('LOAD_SMALL_INT', 2, 0),
            ('LOAD_SMALL_INT', 2, 0),
            ('BINARY_OP', sub, 0),
            ('LOAD_SMALL_INT', 2, 0),
            ('BINARY_OP', sub, 0),
            ('RETURN_VALUE', None, 0)
        ]
        after = [
            ('LOAD_CONST', 0, 0),
            ('RETURN_VALUE', None, 0)
        ]
        self.cfg_optimization_test(before, after, consts=[], expected_consts=[-2])

        # test mul
        before = [
            ('LOAD_SMALL_INT', 2, 0),
            ('LOAD_SMALL_INT', 2, 0),
            ('BINARY_OP', mul, 0),
            ('LOAD_SMALL_INT', 2, 0),
            ('BINARY_OP', mul, 0),
            ('RETURN_VALUE', None, 0)
        ]
        after = [
            ('LOAD_SMALL_INT', 8, 0),
            ('RETURN_VALUE', None, 0)
        ]
        self.cfg_optimization_test(before, after, consts=[], expected_consts=[])

        # test div
        before = [
            ('LOAD_SMALL_INT', 2, 0),
            ('LOAD_SMALL_INT', 2, 0),
            ('BINARY_OP', div, 0),
            ('LOAD_SMALL_INT', 2, 0),
            ('BINARY_OP', div, 0),
            ('RETURN_VALUE', None, 0)
        ]
        after = [
            ('LOAD_CONST', 1, 0),
            ('RETURN_VALUE', None, 0)
        ]
        self.cfg_optimization_test(before, after, consts=[], expected_consts=[1.0, 0.5])

        # test floor
        before = [
            ('LOAD_SMALL_INT', 2, 0),
            ('LOAD_SMALL_INT', 2, 0),
            ('BINARY_OP', floor, 0),
            ('LOAD_SMALL_INT', 2, 0),
            ('BINARY_OP', floor, 0),
            ('RETURN_VALUE', None, 0)
        ]
        after = [
            ('LOAD_SMALL_INT', 0, 0),
            ('RETURN_VALUE', None, 0)
        ]
        self.cfg_optimization_test(before, after, consts=[], expected_consts=[])

        # test rem
        before = [
            ('LOAD_SMALL_INT', 2, 0),
            ('LOAD_SMALL_INT', 2, 0),
            ('BINARY_OP', rem, 0),
            ('LOAD_SMALL_INT', 2, 0),
            ('BINARY_OP', rem, 0),
            ('RETURN_VALUE', None, 0)
        ]
        after = [
            ('LOAD_SMALL_INT', 0, 0),
            ('RETURN_VALUE', None, 0)
        ]
        self.cfg_optimization_test(before, after, consts=[], expected_consts=[])

        # test pow
        before = [
            ('LOAD_SMALL_INT', 2, 0),
            ('LOAD_SMALL_INT', 2, 0),
            ('BINARY_OP', pow, 0),
            ('LOAD_SMALL_INT', 2, 0),
            ('BINARY_OP', pow, 0),
            ('RETURN_VALUE', None, 0)
        ]
        after = [
            ('LOAD_SMALL_INT', 16, 0),
            ('RETURN_VALUE', None, 0)
        ]
        self.cfg_optimization_test(before, after, consts=[], expected_consts=[])

        # test lshift
        before = [
            ('LOAD_SMALL_INT', 1, 0),
            ('LOAD_SMALL_INT', 1, 0),
            ('BINARY_OP', lshift, 0),
            ('LOAD_SMALL_INT', 1, 0),
            ('BINARY_OP', lshift, 0),
            ('RETURN_VALUE', None, 0)
        ]
        after = [
            ('LOAD_SMALL_INT', 4, 0),
            ('RETURN_VALUE', None, 0)
        ]
        self.cfg_optimization_test(before, after, consts=[], expected_consts=[])

        # test rshift
        before = [
            ('LOAD_SMALL_INT', 4, 0),
            ('LOAD_SMALL_INT', 1, 0),
            ('BINARY_OP', rshift, 0),
            ('LOAD_SMALL_INT', 1, 0),
            ('BINARY_OP', rshift, 0),
            ('RETURN_VALUE', None, 0)
        ]
        after = [
            ('LOAD_SMALL_INT', 1, 0),
            ('RETURN_VALUE', None, 0)
        ]
        self.cfg_optimization_test(before, after, consts=[], expected_consts=[])

        # test or
        before = [
            ('LOAD_SMALL_INT', 1, 0),
            ('LOAD_SMALL_INT', 2, 0),
            ('BINARY_OP', or_, 0),
            ('LOAD_SMALL_INT', 4, 0),
            ('BINARY_OP', or_, 0),
            ('RETURN_VALUE', None, 0)
        ]
        after = [
            ('LOAD_SMALL_INT', 7, 0),
            ('RETURN_VALUE', None, 0)
        ]
        self.cfg_optimization_test(before, after, consts=[], expected_consts=[])

        # test and
        before = [
            ('LOAD_SMALL_INT', 1, 0),
            ('LOAD_SMALL_INT', 1, 0),
            ('BINARY_OP', and_, 0),
            ('LOAD_SMALL_INT', 1, 0),
            ('BINARY_OP', and_, 0),
            ('RETURN_VALUE', None, 0)
        ]
        after = [
            ('LOAD_SMALL_INT', 1, 0),
            ('RETURN_VALUE', None, 0)
        ]
        self.cfg_optimization_test(before, after, consts=[], expected_consts=[])

        # test xor
        before = [
            ('LOAD_SMALL_INT', 2, 0),
            ('LOAD_SMALL_INT', 2, 0),
            ('BINARY_OP', xor, 0),
            ('LOAD_SMALL_INT', 2, 0),
            ('BINARY_OP', xor, 0),
            ('RETURN_VALUE', None, 0)
        ]
        after = [
            ('LOAD_SMALL_INT', 2, 0),
            ('RETURN_VALUE', None, 0)
        ]
        self.cfg_optimization_test(before, after, consts=[], expected_consts=[])

        # test subscr
        before = [
            ('LOAD_CONST', 0, 0),
            ('LOAD_SMALL_INT', 1, 0),
            ('BINARY_OP', subscr, 0),
            ('LOAD_SMALL_INT', 2, 0),
            ('BINARY_OP', subscr, 0),
            ('RETURN_VALUE', None, 0)
        ]
        after = [
            ('LOAD_SMALL_INT', 3, 0),
            ('RETURN_VALUE', None, 0)
        ]
        self.cfg_optimization_test(before, after, consts=[(1, (1, 2, 3))], expected_consts=[(1, (1, 2, 3))])


    def test_conditional_jump_forward_const_condition(self):
        # The unreachable branch of the jump is removed, the jump
        # becomes redundant and is replaced by a NOP (for the lineno)

        insts = [
            ('LOAD_CONST', 3, 11),
            ('POP_JUMP_IF_TRUE', lbl := self.Label(), 12),
            ('LOAD_CONST', 2, 13),
            lbl,
            ('LOAD_CONST', 3, 14),
            ('RETURN_VALUE', None, 14),
        ]
        expected_insts = [
            ('NOP', None, 11),
            ('NOP', None, 12),
            ('LOAD_SMALL_INT', 3, 14),
            ('RETURN_VALUE', None, 14),
        ]
        self.cfg_optimization_test(insts,
                                   expected_insts,
                                   consts=[0, 1, 2, 3, 4],
                                   expected_consts=[0])

    def test_conditional_jump_backward_non_const_condition(self):
        insts = [
            lbl1 := self.Label(),
            ('LOAD_NAME', 1, 11),
            ('POP_JUMP_IF_TRUE', lbl1, 12),
            ('LOAD_NAME', 2, 13),
            ('RETURN_VALUE', None, 13),
        ]
        expected = [
            lbl := self.Label(),
            ('LOAD_NAME', 1, 11),
            ('POP_JUMP_IF_TRUE', lbl, 12),
            ('LOAD_NAME', 2, 13),
            ('RETURN_VALUE', None, 13),
        ]
        self.cfg_optimization_test(insts, expected, consts=list(range(5)))

    def test_conditional_jump_backward_const_condition(self):
        # The unreachable branch of the jump is removed
        insts = [
            lbl1 := self.Label(),
            ('LOAD_CONST', 3, 11),
            ('POP_JUMP_IF_TRUE', lbl1, 12),
            ('LOAD_CONST', 2, 13),
            ('RETURN_VALUE', None, 13),
        ]
        expected_insts = [
            lbl := self.Label(),
            ('NOP', None, 11),
            ('JUMP', lbl, 12),
        ]
        self.cfg_optimization_test(insts, expected_insts, consts=list(range(5)))

    def test_except_handler_label(self):
        insts = [
            ('SETUP_FINALLY', handler := self.Label(), 10),
            ('POP_BLOCK', None, -1),
            ('LOAD_CONST', 1, 11),
            ('RETURN_VALUE', None, 11),
            handler,
            ('LOAD_CONST', 2, 12),
            ('RETURN_VALUE', None, 12),
        ]
        expected_insts = [
            ('SETUP_FINALLY', handler := self.Label(), 10),
            ('LOAD_SMALL_INT', 1, 11),
            ('RETURN_VALUE', None, 11),
            handler,
            ('LOAD_SMALL_INT', 2, 12),
            ('RETURN_VALUE', None, 12),
        ]
        self.cfg_optimization_test(insts, expected_insts, consts=list(range(5)))

    def test_no_unsafe_static_swap(self):
        # We can't change order of two stores to the same location
        insts = [
            ('LOAD_CONST', 0, 1),
            ('LOAD_CONST', 1, 2),
            ('LOAD_CONST', 2, 3),
            ('SWAP', 3, 4),
            ('STORE_FAST', 1, 4),
            ('STORE_FAST', 1, 4),
            ('POP_TOP', None, 4),
            ('LOAD_CONST', 0, 5),
            ('RETURN_VALUE', None, 5)
        ]
        expected_insts = [
            ('LOAD_SMALL_INT', 0, 1),
            ('LOAD_SMALL_INT', 1, 2),
            ('NOP', None, 3),
            ('STORE_FAST', 1, 4),
            ('POP_TOP', None, 4),
            ('LOAD_SMALL_INT', 0, 5),
            ('RETURN_VALUE', None, 5)
        ]
        self.cfg_optimization_test(insts, expected_insts, consts=list(range(3)), nlocals=1)

    def test_dead_store_elimination_in_same_lineno(self):
        insts = [
            ('LOAD_CONST', 0, 1),
            ('LOAD_CONST', 1, 2),
            ('LOAD_CONST', 2, 3),
            ('STORE_FAST', 1, 4),
            ('STORE_FAST', 1, 4),
            ('STORE_FAST', 1, 4),
            ('LOAD_CONST', 0, 5),
            ('RETURN_VALUE', None, 5)
        ]
        expected_insts = [
            ('LOAD_SMALL_INT', 0, 1),
            ('LOAD_SMALL_INT', 1, 2),
            ('NOP', None, 3),
            ('POP_TOP', None, 4),
            ('STORE_FAST', 1, 4),
            ('LOAD_SMALL_INT', 0, 5),
            ('RETURN_VALUE', None, 5)
        ]
        self.cfg_optimization_test(insts, expected_insts, consts=list(range(3)), nlocals=1)

    def test_no_dead_store_elimination_in_different_lineno(self):
        insts = [
            ('LOAD_CONST', 0, 1),
            ('LOAD_CONST', 1, 2),
            ('LOAD_CONST', 2, 3),
            ('STORE_FAST', 1, 4),
            ('STORE_FAST', 1, 5),
            ('STORE_FAST', 1, 6),
            ('LOAD_CONST', 0, 5),
            ('RETURN_VALUE', None, 5)
        ]
        expected_insts = [
            ('LOAD_SMALL_INT', 0, 1),
            ('LOAD_SMALL_INT', 1, 2),
            ('LOAD_SMALL_INT', 2, 3),
            ('STORE_FAST', 1, 4),
            ('STORE_FAST', 1, 5),
            ('STORE_FAST', 1, 6),
            ('LOAD_SMALL_INT', 0, 5),
            ('RETURN_VALUE', None, 5)
        ]
        self.cfg_optimization_test(insts, expected_insts, consts=list(range(3)), nlocals=1)

    def test_unconditional_jump_threading(self):

        def get_insts(lno1, lno2, op1, op2):
            return [
                       lbl2 := self.Label(),
                       ('LOAD_NAME', 0, 10),
                       ('POP_TOP', None, 10),
                       (op1, lbl1 := self.Label(), lno1),
                       ('LOAD_NAME', 1, 20),
                       lbl1,
                       (op2, lbl2, lno2),
                   ]

        for op1 in ('JUMP', 'JUMP_NO_INTERRUPT'):
            for op2 in ('JUMP', 'JUMP_NO_INTERRUPT'):
                # different lines
                lno1, lno2 = (4, 5)
                with self.subTest(lno = (lno1, lno2), ops = (op1, op2)):
                    insts = get_insts(lno1, lno2, op1, op2)
                    op = 'JUMP' if 'JUMP' in (op1, op2) else 'JUMP_NO_INTERRUPT'
                    expected_insts = [
                        ('LOAD_NAME', 0, 10),
                        ('POP_TOP', None, 10),
                        ('NOP', None, 4),
                        (op, 0, 5),
                    ]
                    self.cfg_optimization_test(insts, expected_insts, consts=list(range(5)))

                # Threading
                for lno1, lno2 in [(-1, -1), (-1, 5), (6, -1), (7, 7)]:
                    with self.subTest(lno = (lno1, lno2), ops = (op1, op2)):
                        insts = get_insts(lno1, lno2, op1, op2)
                        lno = lno1 if lno1 != -1 else lno2
                        if lno == -1:
                            lno = 10  # Propagated from the line before

                        op = 'JUMP' if 'JUMP' in (op1, op2) else 'JUMP_NO_INTERRUPT'
                        expected_insts = [
                            ('LOAD_NAME', 0, 10),
                            ('POP_TOP', None, 10),
                            (op, 0, lno),
                        ]
                        self.cfg_optimization_test(insts, expected_insts, consts=list(range(5)))

    def test_list_to_tuple_get_iter(self):
        # for _ in (*foo, *bar) -> for _ in [*foo, *bar]
        INTRINSIC_LIST_TO_TUPLE = 6
        insts = [
            ("BUILD_LIST", 0, 1),
            ("LOAD_FAST", 0, 2),
            ("LIST_EXTEND", 1, 3),
            ("LOAD_FAST", 1, 4),
            ("LIST_EXTEND", 1, 5),
            ("CALL_INTRINSIC_1", INTRINSIC_LIST_TO_TUPLE, 6),
            ("GET_ITER", None, 7),
            top := self.Label(),
            ("FOR_ITER", end := self.Label(), 8),
            ("STORE_FAST", 2, 9),
            ("JUMP", top, 10),
            end,
            ("END_FOR", None, 11),
            ("POP_TOP", None, 12),
            ("LOAD_CONST", 0, 13),
            ("RETURN_VALUE", None, 14),
        ]
        expected_insts = [
            ("BUILD_LIST", 0, 1),
            ("LOAD_FAST_BORROW", 0, 2),
            ("LIST_EXTEND", 1, 3),
            ("LOAD_FAST_BORROW", 1, 4),
            ("LIST_EXTEND", 1, 5),
            ("NOP", None, 6),  # ("CALL_INTRINSIC_1", INTRINSIC_LIST_TO_TUPLE, 6),
            ("GET_ITER", None, 7),
            top := self.Label(),
            ("FOR_ITER", end := self.Label(), 8),
            ("STORE_FAST", 2, 9),
            ("JUMP", top, 10),
            end,
            ("END_FOR", None, 11),
            ("POP_TOP", None, 12),
            ("LOAD_CONST", 0, 13),
            ("RETURN_VALUE", None, 14),
        ]
        self.cfg_optimization_test(insts, expected_insts, consts=[None])

    def test_list_to_tuple_get_iter_is_safe(self):
        a, b = [], []
        for item in (*(items := [0, 1, 2, 3]),):
            a.append(item)
            b.append(items.pop())
        self.assertEqual(a, [0, 1, 2, 3])
        self.assertEqual(b, [3, 2, 1, 0])
        self.assertEqual(items, [])


class OptimizeLoadFastTestCase(DirectCfgOptimizerTests):
    def make_bb(self, insts):
        last_loc = insts[-1][2]
        maxconst = 0
        for op, arg, _ in insts:
            if op == "LOAD_CONST":
                maxconst = max(maxconst, arg)
        consts = [None for _ in range(maxconst + 1)]
        return insts + [
            ("LOAD_CONST", 0, last_loc + 1),
            ("RETURN_VALUE", None, last_loc + 2),
        ], consts

    def check(self, insts, expected_insts, consts=None):
        insts_bb, insts_consts = self.make_bb(insts)
        expected_insts_bb, exp_consts = self.make_bb(expected_insts)
        self.cfg_optimization_test(insts_bb, expected_insts_bb,
                                   consts=insts_consts, expected_consts=exp_consts)

    def test_optimized(self):
        insts = [
            ("LOAD_FAST", 0, 1),
            ("LOAD_FAST", 1, 2),
            ("BINARY_OP", 2, 3),
        ]
        expected = [
            ("LOAD_FAST_BORROW", 0, 1),
            ("LOAD_FAST_BORROW", 1, 2),
            ("BINARY_OP", 2, 3),
        ]
        self.check(insts, expected)

        insts = [
            ("LOAD_FAST", 0, 1),
            ("LOAD_CONST", 1, 2),
            ("SWAP", 2, 3),
            ("POP_TOP", None, 4),
        ]
        expected = [
            ("LOAD_FAST_BORROW", 0, 1),
            ("LOAD_CONST", 1, 2),
            ("SWAP", 2, 3),
            ("POP_TOP", None, 4),
        ]
        self.check(insts, expected)

    def test_unoptimized_if_unconsumed(self):
        insts = [
            ("LOAD_FAST", 0, 1),
            ("LOAD_FAST", 1, 2),
            ("POP_TOP", None, 3),
        ]
        expected = [
            ("LOAD_FAST", 0, 1),
            ("LOAD_FAST_BORROW", 1, 2),
            ("POP_TOP", None, 3),
        ]
        self.check(insts, expected)

        insts = [
            ("LOAD_FAST", 0, 1),
            ("COPY", 1, 2),
            ("POP_TOP", None, 3),
        ]
        expected = [
            ("LOAD_FAST", 0, 1),
            ("NOP", None, 2),
            ("NOP", None, 3),
        ]
        self.check(insts, expected)

    def test_unoptimized_if_support_killed(self):
        insts = [
            ("LOAD_FAST", 0, 1),
            ("LOAD_CONST", 0, 2),
            ("STORE_FAST", 0, 3),
            ("POP_TOP", None, 4),
        ]
        self.check(insts, insts)

        insts = [
            ("LOAD_FAST", 0, 1),
            ("LOAD_CONST", 0, 2),
            ("LOAD_CONST", 0, 3),
            ("STORE_FAST_STORE_FAST", ((0 << 4) | 1), 4),
            ("POP_TOP", None, 5),
        ]
        self.check(insts, insts)

    def test_unoptimized_if_aliased(self):
        insts = [
            ("LOAD_FAST", 0, 1),
            ("STORE_FAST", 1, 2),
        ]
        self.check(insts, insts)

        insts = [
            ("LOAD_FAST", 0, 1),
            ("LOAD_CONST", 0, 3),
            ("STORE_FAST_STORE_FAST", ((0 << 4) | 1), 4),
        ]
        self.check(insts, insts)

    def test_consume_no_inputs(self):
        insts = [
            ("LOAD_FAST", 0, 1),
            ("GET_LEN", None, 2),
            ("STORE_FAST", 1 , 3),
            ("STORE_FAST", 2, 4),
        ]
        self.check(insts, insts)

    def test_consume_some_inputs_no_outputs(self):
        insts = [
            ("LOAD_FAST", 0, 1),
            ("GET_LEN", None, 2),
            ("LIST_APPEND", 0, 3),
        ]
        self.check(insts, insts)

    def test_check_exc_match(self):
        insts = [
            ("LOAD_FAST", 0, 1),
            ("LOAD_FAST", 1, 2),
            ("CHECK_EXC_MATCH", None, 3)
        ]
        expected = [
            ("LOAD_FAST", 0, 1),
            ("LOAD_FAST_BORROW", 1, 2),
            ("CHECK_EXC_MATCH", None, 3)
        ]
        self.check(insts, expected)

    def test_for_iter(self):
        insts = [
            ("LOAD_FAST", 0, 1),
            top := self.Label(),
            ("FOR_ITER", end := self.Label(), 2),
            ("STORE_FAST", 2, 3),
            ("JUMP", top, 4),
            end,
            ("END_FOR", None, 5),
            ("POP_TOP", None, 6),
            ("LOAD_CONST", 0, 7),
            ("RETURN_VALUE", None, 8),
        ]
        self.cfg_optimization_test(insts, insts, consts=[None])

    def test_load_attr(self):
        insts = [
            ("LOAD_FAST", 0, 1),
            ("LOAD_ATTR", 0, 2),
        ]
        expected = [
            ("LOAD_FAST_BORROW", 0, 1),
            ("LOAD_ATTR", 0, 2),
        ]
        self.check(insts, expected)

        # Method call, leaves self on stack unconsumed
        insts = [
            ("LOAD_FAST", 0, 1),
            ("LOAD_ATTR", 1, 2),
        ]
        expected = [
            ("LOAD_FAST", 0, 1),
            ("LOAD_ATTR", 1, 2),
        ]
        self.check(insts, expected)

    def test_super_attr(self):
        insts = [
            ("LOAD_FAST", 0, 1),
            ("LOAD_FAST", 1, 2),
            ("LOAD_FAST", 2, 3),
            ("LOAD_SUPER_ATTR", 0, 4),
        ]
        expected = [
            ("LOAD_FAST_BORROW", 0, 1),
            ("LOAD_FAST_BORROW", 1, 2),
            ("LOAD_FAST_BORROW", 2, 3),
            ("LOAD_SUPER_ATTR", 0, 4),
        ]
        self.check(insts, expected)

        # Method call, leaves self on stack unconsumed
        insts = [
            ("LOAD_FAST", 0, 1),
            ("LOAD_FAST", 1, 2),
            ("LOAD_FAST", 2, 3),
            ("LOAD_SUPER_ATTR", 1, 4),
        ]
        expected = [
            ("LOAD_FAST_BORROW", 0, 1),
            ("LOAD_FAST_BORROW", 1, 2),
            ("LOAD_FAST", 2, 3),
            ("LOAD_SUPER_ATTR", 1, 4),
        ]
        self.check(insts, expected)

    def test_send(self):
        insts = [
            ("LOAD_FAST", 0, 1),
            ("LOAD_FAST", 1, 2),
            ("SEND", end := self.Label(), 3),
            ("LOAD_CONST", 0, 4),
            ("RETURN_VALUE", None, 5),
            end,
            ("LOAD_CONST", 0, 6),
            ("RETURN_VALUE", None, 7)
        ]
        expected = [
            ("LOAD_FAST", 0, 1),
            ("LOAD_FAST_BORROW", 1, 2),
            ("SEND", end := self.Label(), 3),
            ("LOAD_CONST", 0, 4),
            ("RETURN_VALUE", None, 5),
            end,
            ("LOAD_CONST", 0, 6),
            ("RETURN_VALUE", None, 7)
        ]
        self.cfg_optimization_test(insts, expected, consts=[None])



if __name__ == "__main__":
    unittest.main()
