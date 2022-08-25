import dis
from itertools import combinations, product
import sys
import textwrap
import unittest

from test.support.bytecode_helper import BytecodeTestCase, CfgOptimizationTestCase


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
            # JUMP_IF_*_OR_POP jump to conditional jump
            if '_OR_POP' in instr.opname and 'JUMP_IF_' in tgt.opname:
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
        self.assertNotInBytecode(unot, 'POP_JUMP_FORWARD_IF_FALSE')
        self.assertNotInBytecode(unot, 'POP_JUMP_BACKWARD_IF_FALSE')
        self.assertInBytecode(unot, 'POP_JUMP_FORWARD_IF_TRUE')
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

    def test_folding_of_tuples_of_constants(self):
        for line, elem in (
            ('a = 1,2,3', (1, 2, 3)),
            ('("a","b","c")', ('a', 'b', 'c')),
            ('a,b,c = 1,2,3', (1, 2, 3)),
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

    def test_folding_of_lists_of_constants(self):
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

    def test_folding_of_sets_of_constants(self):
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


    def test_folding_of_binops_on_constants(self):
        for line, elem in (
            ('a = 2+3+4', 9),                   # chained fold
            ('"@"*4', '@@@@'),                  # check string ops
            ('a="abc" + "def"', 'abcdef'),      # check string ops
            ('a = 3**4', 81),                   # binary power
            ('a = 3*4', 12),                    # binary multiply
            ('a = 13//4', 3),                   # binary floor divide
            ('a = 14%4', 2),                    # binary modulo
            ('a = 2+3', 5),                     # binary add
            ('a = 13-4', 9),                    # binary subtract
            ('a = (12,13)[1]', 13),             # binary subscr
            ('a = 13 << 2', 52),                # binary lshift
            ('a = 13 >> 2', 3),                 # binary rshift
            ('a = 13 & 7', 5),                  # binary and
            ('a = 13 ^ 7', 10),                 # binary xor
            ('a = 13 | 7', 15),                 # binary or
            ):
            with self.subTest(line=line):
                code = compile(line, '', 'single')
                self.assertInBytecode(code, 'LOAD_CONST', elem)
                for instr in dis.get_instructions(code):
                    self.assertFalse(instr.opname.startswith('BINARY_'))
                self.check_lnotab(code)

        # Verify that unfoldables are skipped
        code = compile('a=2+"b"', '', 'single')
        self.assertInBytecode(code, 'LOAD_CONST', 2)
        self.assertInBytecode(code, 'LOAD_CONST', 'b')
        self.check_lnotab(code)

        # Verify that large sequences do not result from folding
        code = compile('a="x"*10000', '', 'single')
        self.assertInBytecode(code, 'LOAD_CONST', 10000)
        self.assertNotIn("x"*10000, code.co_consts)
        self.check_lnotab(code)
        code = compile('a=1<<1000', '', 'single')
        self.assertInBytecode(code, 'LOAD_CONST', 1000)
        self.assertNotIn(1<<1000, code.co_consts)
        self.check_lnotab(code)
        code = compile('a=2**1000', '', 'single')
        self.assertInBytecode(code, 'LOAD_CONST', 1000)
        self.assertNotIn(2**1000, code.co_consts)
        self.check_lnotab(code)

    def test_binary_subscr_on_unicode(self):
        # valid code get optimized
        code = compile('"foo"[0]', '', 'single')
        self.assertInBytecode(code, 'LOAD_CONST', 'f')
        self.assertNotInBytecode(code, 'BINARY_SUBSCR')
        self.check_lnotab(code)
        code = compile('"\u0061\uffff"[1]', '', 'single')
        self.assertInBytecode(code, 'LOAD_CONST', '\uffff')
        self.assertNotInBytecode(code,'BINARY_SUBSCR')
        self.check_lnotab(code)

        # With PEP 393, non-BMP char get optimized
        code = compile('"\U00012345"[0]', '', 'single')
        self.assertInBytecode(code, 'LOAD_CONST', '\U00012345')
        self.assertNotInBytecode(code, 'BINARY_SUBSCR')
        self.check_lnotab(code)

        # invalid code doesn't get optimized
        # out of range
        code = compile('"fuu"[10]', '', 'single')
        self.assertInBytecode(code, 'BINARY_SUBSCR')
        self.check_lnotab(code)

    def test_folding_of_unaryops_on_constants(self):
        for line, elem in (
            ('-0.5', -0.5),                     # unary negative
            ('-0.0', -0.0),                     # -0.0
            ('-(1.0-1.0)', -0.0),               # -0.0 after folding
            ('-0', 0),                          # -0
            ('~-2', 1),                         # unary invert
            ('+1', 1),                          # unary positive
        ):
            with self.subTest(line=line):
                code = compile(line, '', 'single')
                self.assertInBytecode(code, 'LOAD_CONST', elem)
                for instr in dis.get_instructions(code):
                    self.assertFalse(instr.opname.startswith('UNARY_'))
                self.check_lnotab(code)

        # Check that -0.0 works after marshaling
        def negzero():
            return -(1.0-1.0)

        for instr in dis.get_instructions(negzero):
            self.assertFalse(instr.opname.startswith('UNARY_'))
        self.check_lnotab(negzero)

        # Verify that unfoldables are skipped
        for line, elem, opname in (
            ('-"abc"', 'abc', 'UNARY_NEGATIVE'),
            ('~"abc"', 'abc', 'UNARY_INVERT'),
        ):
            with self.subTest(line=line):
                code = compile(line, '', 'single')
                self.assertInBytecode(code, 'LOAD_CONST', elem)
                self.assertInBytecode(code, opname)
                self.check_lnotab(code)

    def test_elim_extra_return(self):
        # RETURN LOAD_CONST None RETURN  -->  RETURN
        def f(x):
            return x
        self.assertNotInBytecode(f, 'LOAD_CONST', None)
        returns = [instr for instr in dis.get_instructions(f)
                          if instr.opname == 'RETURN_VALUE']
        self.assertEqual(len(returns), 1)
        self.check_lnotab(f)

    @unittest.skip("Following gh-92228 the return has two predecessors "
                   "and that prevents jump elimination.")
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
        # JUMP_IF_FALSE_OR_POP to JUMP_IF_FALSE_OR_POP --> JUMP_IF_FALSE_OR_POP to non-jump
        def f(a, b, c):
            return ((a and b)
                    and c)
        self.check_jump_targets(f)
        self.check_lnotab(f)
        self.assertEqual(count_instr_recursively(f, 'JUMP_IF_FALSE_OR_POP'), 2)
        # JUMP_IF_TRUE_OR_POP to JUMP_IF_TRUE_OR_POP --> JUMP_IF_TRUE_OR_POP to non-jump
        def f(a, b, c):
            return ((a or b)
                    or c)
        self.check_jump_targets(f)
        self.check_lnotab(f)
        self.assertEqual(count_instr_recursively(f, 'JUMP_IF_TRUE_OR_POP'), 2)
        # JUMP_IF_FALSE_OR_POP to JUMP_IF_TRUE_OR_POP --> POP_JUMP_IF_FALSE to non-jump
        def f(a, b, c):
            return ((a and b)
                    or c)
        self.check_jump_targets(f)
        self.check_lnotab(f)
        self.assertNotInBytecode(f, 'JUMP_IF_FALSE_OR_POP')
        self.assertInBytecode(f, 'JUMP_IF_TRUE_OR_POP')
        self.assertInBytecode(f, 'POP_JUMP_FORWARD_IF_FALSE')
        # JUMP_IF_TRUE_OR_POP to JUMP_IF_FALSE_OR_POP --> POP_JUMP_IF_TRUE to non-jump
        def f(a, b, c):
            return ((a or b)
                    and c)
        self.check_jump_targets(f)
        self.check_lnotab(f)
        self.assertNotInBytecode(f, 'JUMP_IF_TRUE_OR_POP')
        self.assertInBytecode(f, 'JUMP_IF_FALSE_OR_POP')
        self.assertInBytecode(f, 'POP_JUMP_FORWARD_IF_TRUE')

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

    def test_constant_folding(self):
        # Issue #11244: aggressive constant folding.
        exprs = [
            '3 * -5',
            '-3 * 5',
            '2 * (3 * 4)',
            '(2 * 3) * 4',
            '(-1, 2, 3)',
            '(1, -2, 3)',
            '(1, 2, -3)',
            '(1, 2, -3) * 6',
            'lambda x: x in {(3 * -5) + (-1 - 6), (1, -2, 3) * 2, None}',
        ]
        for e in exprs:
            with self.subTest(e=e):
                code = compile(e, '', 'single')
                for instr in dis.get_instructions(code):
                    self.assertFalse(instr.opname.startswith('UNARY_'))
                    self.assertFalse(instr.opname.startswith('BINARY_'))
                    self.assertFalse(instr.opname.startswith('BUILD_'))
                self.check_lnotab(code)

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
        self.assertInBytecode(f, 'LOAD_FAST')

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
        self.assertInBytecode(f1, 'LOAD_FAST')
        self.assertNotInBytecode(f1, 'LOAD_FAST_CHECK')

        def f2(*, x):
            print(x)
        self.assertInBytecode(f2, 'LOAD_FAST')
        self.assertNotInBytecode(f2, 'LOAD_FAST_CHECK')

        def f3(*args):
            print(args)
        self.assertInBytecode(f3, 'LOAD_FAST')
        self.assertNotInBytecode(f3, 'LOAD_FAST_CHECK')

        def f4(**kwargs):
            print(kwargs)
        self.assertInBytecode(f4, 'LOAD_FAST')
        self.assertNotInBytecode(f4, 'LOAD_FAST_CHECK')

        def f5(x=0):
            print(x)
        self.assertInBytecode(f5, 'LOAD_FAST')
        self.assertNotInBytecode(f5, 'LOAD_FAST_CHECK')

    def test_load_fast_known_because_already_loaded(self):
        def f():
            if condition():
                x = 1
            print(x)
            print(x)
        self.assertInBytecode(f, 'LOAD_FAST_CHECK')
        self.assertInBytecode(f, 'LOAD_FAST')

    def test_load_fast_known_multiple_branches(self):
        def f():
            if condition():
                x = 1
            else:
                x = 2
            print(x)
        self.assertInBytecode(f, 'LOAD_FAST')
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
            except:
                print(a, b, c, d, e, f, g)
            a = b = c = d = e = f = g = 1
        self.assertInBytecode(f, 'LOAD_FAST_CHECK')
        self.assertNotInBytecode(f, 'LOAD_FAST')

    def test_setting_lineno_adds_check(self):
        code = textwrap.dedent("""\
            def f():
                x = 2
                L = 3
                L = 4
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
        self.assertInBytecode(f, "LOAD_FAST")
        def trace(frame, event, arg):
            if event == 'line' and frame.f_lineno == 9:
                frame.f_lineno = 2
                sys.settrace(None)
                return None
            return trace
        sys.settrace(trace)
        f()
        self.assertNotInBytecode(f, "LOAD_FAST")

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
        self.assertInBytecode(f, "LOAD_FAST")
        self.assertNotInBytecode(f, "LOAD_FAST_CHECK")
        return f

    def test_deleting_local_adds_check(self):
        f = self.make_function_with_no_checks()
        def trace(frame, event, arg):
            if event == 'line' and frame.f_lineno == 4:
                del frame.f_locals["x"]
                sys.settrace(None)
                return None
            return trace
        sys.settrace(trace)
        f()
        self.assertNotInBytecode(f, "LOAD_FAST")
        self.assertInBytecode(f, "LOAD_FAST_CHECK")

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
        self.assertInBytecode(f, "LOAD_FAST")
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
        self.assertInBytecode(f, "LOAD_FAST")
        self.assertNotInBytecode(f, "LOAD_FAST_CHECK")


class DirectiCfgOptimizerTests(CfgOptimizationTestCase):

    def cfg_optimization_test(self, insts, expected_insts,
                              consts=None, expected_consts=None):
        if expected_consts is None:
            expected_consts = consts
        opt_insts, opt_consts = self.get_optimized(insts, consts)
        self.compareInstructions(opt_insts, expected_insts)
        self.assertEqual(opt_consts, expected_consts)

    def test_conditional_jump_forward_non_const_condition(self):
        insts = [
            ('LOAD_NAME', 1, 11),
            ('POP_JUMP_IF_TRUE', lbl := self.Label(), 12),
            ('LOAD_CONST', 2, 13),
            lbl,
            ('LOAD_CONST', 3, 14),
        ]
        expected = [
            ('LOAD_NAME', '1', 11),
            ('POP_JUMP_IF_TRUE', lbl := self.Label(), 12),
            ('LOAD_CONST', '2', 13),
            lbl,
            ('LOAD_CONST', '3', 14)
        ]
        self.cfg_optimization_test(insts, expected, consts=list(range(5)))

    def test_conditional_jump_forward_const_condition(self):
        # The unreachable branch of the jump is removed, the jump
        # becomes redundant and is replaced by a NOP (for the lineno)

        insts = [
            ('LOAD_CONST', 3, 11),
            ('POP_JUMP_IF_TRUE', lbl := self.Label(), 12),
            ('LOAD_CONST', 2, 13),
            lbl,
            ('LOAD_CONST', 3, 14),
        ]
        expected = [
            ('NOP', None, 11),
            ('NOP', None, 12),
            ('LOAD_CONST', '3', 14)
        ]
        self.cfg_optimization_test(insts, expected, consts=list(range(5)))

    def test_conditional_jump_backward_non_const_condition(self):
        insts = [
            lbl1 := self.Label(),
            ('LOAD_NAME', 1, 11),
            ('POP_JUMP_IF_TRUE', lbl1, 12),
            ('LOAD_CONST', 2, 13),
        ]
        expected = [
            lbl := self.Label(),
            ('LOAD_NAME', '1', 11),
            ('POP_JUMP_IF_TRUE', lbl, 12),
            ('LOAD_CONST', '2', 13)
        ]
        self.cfg_optimization_test(insts, expected, consts=list(range(5)))

    def test_conditional_jump_backward_const_condition(self):
        # The unreachable branch of the jump is removed
        insts = [
            lbl1 := self.Label(),
            ('LOAD_CONST', 1, 11),
            ('POP_JUMP_IF_TRUE', lbl1, 12),
            ('LOAD_CONST', 2, 13),
        ]
        expected = [
            lbl := self.Label(),
            ('NOP', None, 11),
            ('JUMP', lbl, 12)
        ]
        self.cfg_optimization_test(insts, expected, consts=list(range(5)))


if __name__ == "__main__":
    unittest.main()
