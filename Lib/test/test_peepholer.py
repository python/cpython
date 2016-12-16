import dis
import unittest

from test.bytecode_helper import BytecodeTestCase

class TestTranforms(BytecodeTestCase):

    def test_unot(self):
        # UNARY_NOT POP_JUMP_IF_FALSE  -->  POP_JUMP_IF_TRUE'
        def unot(x):
            if not x == 2:
                del x
        self.assertNotInBytecode(unot, 'UNARY_NOT')
        self.assertNotInBytecode(unot, 'POP_JUMP_IF_FALSE')
        self.assertInBytecode(unot, 'POP_JUMP_IF_TRUE')

    def test_elim_inversion_of_is_or_in(self):
        for line, cmp_op in (
            ('not a is b', 'is not',),
            ('not a in b', 'not in',),
            ('not a is not b', 'is',),
            ('not a not in b', 'in',),
            ):
            code = compile(line, '', 'single')
            self.assertInBytecode(code, 'COMPARE_OP', cmp_op)

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
            self.assertNotInBytecode(func, 'LOAD_GLOBAL')
            self.assertInBytecode(func, 'LOAD_CONST', elem)

        def f():
            'Adding a docstring made this test fail in Py2.5.0'
            return None

        self.assertNotInBytecode(f, 'LOAD_GLOBAL')
        self.assertInBytecode(f, 'LOAD_CONST', None)

    def test_while_one(self):
        # Skip over:  LOAD_CONST trueconst  POP_JUMP_IF_FALSE xx
        def f():
            while 1:
                pass
            return list
        for elem in ('LOAD_CONST', 'POP_JUMP_IF_FALSE'):
            self.assertNotInBytecode(f, elem)
        for elem in ('JUMP_ABSOLUTE',):
            self.assertInBytecode(f, elem)

    def test_pack_unpack(self):
        for line, elem in (
            ('a, = a,', 'LOAD_CONST',),
            ('a, b = a, b', 'ROT_TWO',),
            ('a, b, c = a, b, c', 'ROT_THREE',),
            ):
            code = compile(line,'','single')
            self.assertInBytecode(code, elem)
            self.assertNotInBytecode(code, 'BUILD_TUPLE')
            self.assertNotInBytecode(code, 'UNPACK_TUPLE')

    def test_folding_of_tuples_of_constants(self):
        for line, elem in (
            ('a = 1,2,3', (1, 2, 3)),
            ('("a","b","c")', ('a', 'b', 'c')),
            ('a,b,c = 1,2,3', (1, 2, 3)),
            ('(None, 1, None)', (None, 1, None)),
            ('((1, 2), 3, 4)', ((1, 2), 3, 4)),
            ):
            code = compile(line,'','single')
            self.assertInBytecode(code, 'LOAD_CONST', elem)
            self.assertNotInBytecode(code, 'BUILD_TUPLE')

        # Long tuples should be folded too.
        code = compile(repr(tuple(range(10000))),'','single')
        self.assertNotInBytecode(code, 'BUILD_TUPLE')
        # One LOAD_CONST for the tuple, one for the None return value
        load_consts = [instr for instr in dis.get_instructions(code)
                              if instr.opname == 'LOAD_CONST']
        self.assertEqual(len(load_consts), 2)

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

    def test_folding_of_lists_of_constants(self):
        for line, elem in (
            # in/not in constants with BUILD_LIST should be folded to a tuple:
            ('a in [1,2,3]', (1, 2, 3)),
            ('a not in ["a","b","c"]', ('a', 'b', 'c')),
            ('a in [None, 1, None]', (None, 1, None)),
            ('a not in [(1, 2), 3, 4]', ((1, 2), 3, 4)),
            ):
            code = compile(line, '', 'single')
            self.assertInBytecode(code, 'LOAD_CONST', elem)
            self.assertNotInBytecode(code, 'BUILD_LIST')

    def test_folding_of_sets_of_constants(self):
        for line, elem in (
            # in/not in constants with BUILD_SET should be folded to a frozenset:
            ('a in {1,2,3}', frozenset({1, 2, 3})),
            ('a not in {"a","b","c"}', frozenset({'a', 'c', 'b'})),
            ('a in {None, 1, None}', frozenset({1, None})),
            ('a not in {(1, 2), 3, 4}', frozenset({(1, 2), 3, 4})),
            ('a in {1, 2, 3, 3, 2, 1}', frozenset({1, 2, 3})),
            ):
            code = compile(line, '', 'single')
            self.assertNotInBytecode(code, 'BUILD_SET')
            self.assertInBytecode(code, 'LOAD_CONST', elem)

        # Ensure that the resulting code actually works:
        def f(a):
            return a in {1, 2, 3}

        def g(a):
            return a not in {1, 2, 3}

        self.assertTrue(f(3))
        self.assertTrue(not f(4))

        self.assertTrue(not g(3))
        self.assertTrue(g(4))


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
            code = compile(line, '', 'single')
            self.assertInBytecode(code, 'LOAD_CONST', elem)
            for instr in dis.get_instructions(code):
                self.assertFalse(instr.opname.startswith('BINARY_'))

        # Verify that unfoldables are skipped
        code = compile('a=2+"b"', '', 'single')
        self.assertInBytecode(code, 'LOAD_CONST', 2)
        self.assertInBytecode(code, 'LOAD_CONST', 'b')

        # Verify that large sequences do not result from folding
        code = compile('a="x"*1000', '', 'single')
        self.assertInBytecode(code, 'LOAD_CONST', 1000)

    def test_binary_subscr_on_unicode(self):
        # valid code get optimized
        code = compile('"foo"[0]', '', 'single')
        self.assertInBytecode(code, 'LOAD_CONST', 'f')
        self.assertNotInBytecode(code, 'BINARY_SUBSCR')
        code = compile('"\u0061\uffff"[1]', '', 'single')
        self.assertInBytecode(code, 'LOAD_CONST', '\uffff')
        self.assertNotInBytecode(code,'BINARY_SUBSCR')

        # With PEP 393, non-BMP char get optimized
        code = compile('"\U00012345"[0]', '', 'single')
        self.assertInBytecode(code, 'LOAD_CONST', '\U00012345')
        self.assertNotInBytecode(code, 'BINARY_SUBSCR')

        # invalid code doesn't get optimized
        # out of range
        code = compile('"fuu"[10]', '', 'single')
        self.assertInBytecode(code, 'BINARY_SUBSCR')

    def test_folding_of_unaryops_on_constants(self):
        for line, elem in (
            ('-0.5', -0.5),                     # unary negative
            ('-0.0', -0.0),                     # -0.0
            ('-(1.0-1.0)', -0.0),               # -0.0 after folding
            ('-0', 0),                          # -0
            ('~-2', 1),                         # unary invert
            ('+1', 1),                          # unary positive
        ):
            code = compile(line, '', 'single')
            self.assertInBytecode(code, 'LOAD_CONST', elem)
            for instr in dis.get_instructions(code):
                self.assertFalse(instr.opname.startswith('UNARY_'))

        # Check that -0.0 works after marshaling
        def negzero():
            return -(1.0-1.0)

        for instr in dis.get_instructions(code):
            self.assertFalse(instr.opname.startswith('UNARY_'))

        # Verify that unfoldables are skipped
        for line, elem, opname in (
            ('-"abc"', 'abc', 'UNARY_NEGATIVE'),
            ('~"abc"', 'abc', 'UNARY_INVERT'),
        ):
            code = compile(line, '', 'single')
            self.assertInBytecode(code, 'LOAD_CONST', elem)
            self.assertInBytecode(code, opname)

    def test_elim_extra_return(self):
        # RETURN LOAD_CONST None RETURN  -->  RETURN
        def f(x):
            return x
        self.assertNotInBytecode(f, 'LOAD_CONST', None)
        returns = [instr for instr in dis.get_instructions(f)
                          if instr.opname == 'RETURN_VALUE']
        self.assertEqual(len(returns), 1)

    def test_elim_jump_to_return(self):
        # JUMP_FORWARD to RETURN -->  RETURN
        def f(cond, true_value, false_value):
            return true_value if cond else false_value
        self.assertNotInBytecode(f, 'JUMP_FORWARD')
        self.assertNotInBytecode(f, 'JUMP_ABSOLUTE')
        returns = [instr for instr in dis.get_instructions(f)
                          if instr.opname == 'RETURN_VALUE']
        self.assertEqual(len(returns), 2)

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
        self.assertNotInBytecode(f, 'JUMP_ABSOLUTE')
        returns = [instr for instr in dis.get_instructions(f)
                          if instr.opname == 'RETURN_VALUE']
        self.assertEqual(len(returns), 6)

    def test_elim_jump_after_return2(self):
        # Eliminate dead code: jumps immediately after returns can't be reached
        def f(cond1, cond2):
            while 1:
                if cond1: return 4
        self.assertNotInBytecode(f, 'JUMP_FORWARD')
        # There should be one jump for the while loop.
        returns = [instr for instr in dis.get_instructions(f)
                          if instr.opname == 'JUMP_ABSOLUTE']
        self.assertEqual(len(returns), 1)
        returns = [instr for instr in dis.get_instructions(f)
                          if instr.opname == 'RETURN_VALUE']
        self.assertEqual(len(returns), 2)

    def test_make_function_doesnt_bail(self):
        def f():
            def g()->1+1:
                pass
            return g
        self.assertNotInBytecode(f, 'BINARY_ADD')

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
            code = compile(e, '', 'single')
            for instr in dis.get_instructions(code):
                self.assertFalse(instr.opname.startswith('UNARY_'))
                self.assertFalse(instr.opname.startswith('BINARY_'))
                self.assertFalse(instr.opname.startswith('BUILD_'))


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


if __name__ == "__main__":
    unittest.main()
