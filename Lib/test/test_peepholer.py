import dis
import re
import sys
from io import StringIO
import unittest

def disassemble(func):
    f = StringIO()
    tmp = sys.stdout
    sys.stdout = f
    dis.dis(func)
    sys.stdout = tmp
    result = f.getvalue()
    f.close()
    return result

def dis_single(line):
    return disassemble(compile(line, '', 'single'))

class TestTranforms(unittest.TestCase):

    def test_unot(self):
        # UNARY_NOT POP_JUMP_IF_FALSE  -->  POP_JUMP_IF_TRUE'
        def unot(x):
            if not x == 2:
                del x
        asm = disassemble(unot)
        for elem in ('UNARY_NOT', 'POP_JUMP_IF_FALSE'):
            self.assertNotIn(elem, asm)
        for elem in ('POP_JUMP_IF_TRUE',):
            self.assertIn(elem, asm)

    def test_elim_inversion_of_is_or_in(self):
        for line, elem in (
            ('not a is b', '(is not)',),
            ('not a in b', '(not in)',),
            ('not a is not b', '(is)',),
            ('not a not in b', '(in)',),
            ):
            asm = dis_single(line)
            self.assertIn(elem, asm)

    def test_global_as_constant(self):
        # LOAD_GLOBAL None/True/False  -->  LOAD_CONST None/True/False
        def f(x):
            None
            None
            return x
        def g(x):
            True
            return x
        def h(x):
            False
            return x
        for func, name in ((f, 'None'), (g, 'True'), (h, 'False')):
            asm = disassemble(func)
            for elem in ('LOAD_GLOBAL',):
                self.assertNotIn(elem, asm)
            for elem in ('LOAD_CONST', '('+name+')'):
                self.assertIn(elem, asm)
        def f():
            'Adding a docstring made this test fail in Py2.5.0'
            return None
        self.assertIn('LOAD_CONST', disassemble(f))
        self.assertNotIn('LOAD_GLOBAL', disassemble(f))

    def test_while_one(self):
        # Skip over:  LOAD_CONST trueconst  POP_JUMP_IF_FALSE xx
        def f():
            while 1:
                pass
            return list
        asm = disassemble(f)
        for elem in ('LOAD_CONST', 'POP_JUMP_IF_FALSE'):
            self.assertNotIn(elem, asm)
        for elem in ('JUMP_ABSOLUTE',):
            self.assertIn(elem, asm)

    def test_pack_unpack(self):
        for line, elem in (
            ('a, = a,', 'LOAD_CONST',),
            ('a, b = a, b', 'ROT_TWO',),
            ('a, b, c = a, b, c', 'ROT_THREE',),
            ):
            asm = dis_single(line)
            self.assertIn(elem, asm)
            self.assertNotIn('BUILD_TUPLE', asm)
            self.assertNotIn('UNPACK_TUPLE', asm)

    def test_folding_of_tuples_of_constants(self):
        for line, elem in (
            ('a = 1,2,3', '((1, 2, 3))'),
            ('("a","b","c")', "(('a', 'b', 'c'))"),
            ('a,b,c = 1,2,3', '((1, 2, 3))'),
            ('(None, 1, None)', '((None, 1, None))'),
            ('((1, 2), 3, 4)', '(((1, 2), 3, 4))'),
            ):
            asm = dis_single(line)
            self.assertIn(elem, asm)
            self.assertNotIn('BUILD_TUPLE', asm)

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
            ('a in [1,2,3]', '(1, 2, 3)'),
            ('a not in ["a","b","c"]', "(('a', 'b', 'c'))"),
            ('a in [None, 1, None]', '((None, 1, None))'),
            ('a not in [(1, 2), 3, 4]', '(((1, 2), 3, 4))'),
            ):
            asm = dis_single(line)
            self.assertIn(elem, asm)
            self.assertNotIn('BUILD_LIST', asm)

    def test_folding_of_sets_of_constants(self):
        for line, elem in (
            # in/not in constants with BUILD_SET should be folded to a frozenset:
            ('a in {1,2,3}', frozenset({1, 2, 3})),
            ('a not in {"a","b","c"}', frozenset({'a', 'c', 'b'})),
            ('a in {None, 1, None}', frozenset({1, None})),
            ('a not in {(1, 2), 3, 4}', frozenset({(1, 2), 3, 4})),
            ('a in {1, 2, 3, 3, 2, 1}', frozenset({1, 2, 3})),
            ):
            asm = dis_single(line)
            self.assertNotIn('BUILD_SET', asm)

            # Verify that the frozenset 'elem' is in the disassembly
            # The ordering of the elements in repr( frozenset ) isn't
            # guaranteed, so we jump through some hoops to ensure that we have
            # the frozenset we expect:
            self.assertIn('frozenset', asm)
            # Extract the frozenset literal from the disassembly:
            m = re.match(r'.*(frozenset\({.*}\)).*', asm, re.DOTALL)
            self.assertTrue(m)
            self.assertEqual(eval(m.group(1)), elem)

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
            ('a = 2+3+4', '(9)'),                   # chained fold
            ('"@"*4', "('@@@@')"),                  # check string ops
            ('a="abc" + "def"', "('abcdef')"),      # check string ops
            ('a = 3**4', '(81)'),                   # binary power
            ('a = 3*4', '(12)'),                    # binary multiply
            ('a = 13//4', '(3)'),                   # binary floor divide
            ('a = 14%4', '(2)'),                    # binary modulo
            ('a = 2+3', '(5)'),                     # binary add
            ('a = 13-4', '(9)'),                    # binary subtract
            ('a = (12,13)[1]', '(13)'),             # binary subscr
            ('a = 13 << 2', '(52)'),                # binary lshift
            ('a = 13 >> 2', '(3)'),                 # binary rshift
            ('a = 13 & 7', '(5)'),                  # binary and
            ('a = 13 ^ 7', '(10)'),                 # binary xor
            ('a = 13 | 7', '(15)'),                 # binary or
            ):
            asm = dis_single(line)
            self.assertIn(elem, asm, asm)
            self.assertNotIn('BINARY_', asm)

        # Verify that unfoldables are skipped
        asm = dis_single('a=2+"b"')
        self.assertIn('(2)', asm)
        self.assertIn("('b')", asm)

        # Verify that large sequences do not result from folding
        asm = dis_single('a="x"*1000')
        self.assertIn('(1000)', asm)

    def test_folding_of_unaryops_on_constants(self):
        for line, elem in (
            ('-0.5', '(-0.5)'),                     # unary negative
            ('~-2', '(1)'),                         # unary invert
            ('+1', '(1)'),                          # unary positive
        ):
            asm = dis_single(line)
            self.assertIn(elem, asm, asm)
            self.assertNotIn('UNARY_', asm)

        # Verify that unfoldables are skipped
        for line, elem in (
            ('-"abc"', "('abc')"),                  # unary negative
            ('~"abc"', "('abc')"),                  # unary invert
        ):
            asm = dis_single(line)
            self.assertIn(elem, asm, asm)
            self.assertIn('UNARY_', asm)

    def test_elim_extra_return(self):
        # RETURN LOAD_CONST None RETURN  -->  RETURN
        def f(x):
            return x
        asm = disassemble(f)
        self.assertNotIn('LOAD_CONST', asm)
        self.assertNotIn('(None)', asm)
        self.assertEqual(asm.split().count('RETURN_VALUE'), 1)

    def test_elim_jump_to_return(self):
        # JUMP_FORWARD to RETURN -->  RETURN
        def f(cond, true_value, false_value):
            return true_value if cond else false_value
        asm = disassemble(f)
        self.assertNotIn('JUMP_FORWARD', asm)
        self.assertNotIn('JUMP_ABSOLUTE', asm)
        self.assertEqual(asm.split().count('RETURN_VALUE'), 2)

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
        asm = disassemble(f)
        self.assertNotIn('JUMP_FORWARD', asm)
        self.assertNotIn('JUMP_ABSOLUTE', asm)
        self.assertEqual(asm.split().count('RETURN_VALUE'), 6)

    def test_elim_jump_after_return2(self):
        # Eliminate dead code: jumps immediately after returns can't be reached
        def f(cond1, cond2):
            while 1:
                if cond1: return 4
        asm = disassemble(f)
        self.assertNotIn('JUMP_FORWARD', asm)
        # There should be one jump for the while loop.
        self.assertEqual(asm.split().count('JUMP_ABSOLUTE'), 1)
        self.assertEqual(asm.split().count('RETURN_VALUE'), 2)

    def test_make_function_doesnt_bail(self):
        def f():
            def g()->1+1:
                pass
            return g
        asm = disassemble(f)
        self.assertNotIn('BINARY_ADD', asm)

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


def test_main(verbose=None):
    import sys
    from test import support
    test_classes = (TestTranforms, TestBuglets)
    support.run_unittest(*test_classes)

    # verify reference counting
    if verbose and hasattr(sys, "gettotalrefcount"):
        import gc
        counts = [None] * 5
        for i in range(len(counts)):
            support.run_unittest(*test_classes)
            gc.collect()
            counts[i] = sys.gettotalrefcount()
        print(counts)

if __name__ == "__main__":
    test_main(verbose=True)
