import dis
import sys
from cStringIO import StringIO
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
        # UNARY_NOT JUMP_IF_FALSE POP_TOP  -->  JUMP_IF_TRUE POP_TOP'
        def unot(x):
            if not x == 2:
                del x
        asm = disassemble(unot)
        for elem in ('UNARY_NOT', 'JUMP_IF_FALSE'):
            self.assert_(elem not in asm)
        for elem in ('JUMP_IF_TRUE', 'POP_TOP'):
            self.assert_(elem in asm)

    def test_elim_inversion_of_is_or_in(self):
        for line, elem in (
            ('not a is b', '(is not)',),
            ('not a in b', '(not in)',),
            ('not a is not b', '(is)',),
            ('not a not in b', '(in)',),
            ):
            asm = dis_single(line)
            self.assert_(elem in asm)

    def test_none_as_constant(self):
        # LOAD_GLOBAL None  -->  LOAD_CONST None
        def f(x):
            None
            return x
        asm = disassemble(f)
        for elem in ('LOAD_GLOBAL',):
            self.assert_(elem not in asm)
        for elem in ('LOAD_CONST', '(None)'):
            self.assert_(elem in asm)
        def f():
            'Adding a docstring made this test fail in Py2.5.0'
            return None
        self.assert_('LOAD_CONST' in disassemble(f))
        self.assert_('LOAD_GLOBAL' not in disassemble(f))

    def test_while_one(self):
        # Skip over:  LOAD_CONST trueconst  JUMP_IF_FALSE xx  POP_TOP
        def f():
            while 1:
                pass
            return list
        asm = disassemble(f)
        for elem in ('LOAD_CONST', 'JUMP_IF_FALSE'):
            self.assert_(elem not in asm)
        for elem in ('JUMP_ABSOLUTE',):
            self.assert_(elem in asm)

    def test_pack_unpack(self):
        for line, elem in (
            ('a, = a,', 'LOAD_CONST',),
            ('a, b = a, b', 'ROT_TWO',),
            ('a, b, c = a, b, c', 'ROT_THREE',),
            ):
            asm = dis_single(line)
            self.assert_(elem in asm)
            self.assert_('BUILD_TUPLE' not in asm)
            self.assert_('UNPACK_TUPLE' not in asm)

    def test_folding_of_tuples_of_constants(self):
        for line, elem in (
            ('a = 1,2,3', '((1, 2, 3))'),
            ('("a","b","c")', "(('a', 'b', 'c'))"),
            ('a,b,c = 1,2,3', '((1, 2, 3))'),
            ('(None, 1, None)', '((None, 1, None))'),
            ('((1, 2), 3, 4)', '(((1, 2), 3, 4))'),
            ):
            asm = dis_single(line)
            self.assert_(elem in asm)
            self.assert_('BUILD_TUPLE' not in asm)

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
            self.assert_(elem in asm, asm)
            self.assert_('BINARY_' not in asm)

        # Verify that unfoldables are skipped
        asm = dis_single('a=2+"b"')
        self.assert_('(2)' in asm)
        self.assert_("('b')" in asm)

        # Verify that large sequences do not result from folding
        asm = dis_single('a="x"*1000')
        self.assert_('(1000)' in asm)

    def test_folding_of_unaryops_on_constants(self):
        for line, elem in (
            ('`1`', "('1')"),                       # unary convert
            ('-0.5', '(-0.5)'),                     # unary negative
            ('~-2', '(1)'),                         # unary invert
        ):
            asm = dis_single(line)
            self.assert_(elem in asm, asm)
            self.assert_('UNARY_' not in asm)

        # Verify that unfoldables are skipped
        for line, elem in (
            ('-"abc"', "('abc')"),                  # unary negative
            ('~"abc"', "('abc')"),                  # unary invert
        ):
            asm = dis_single(line)
            self.assert_(elem in asm, asm)
            self.assert_('UNARY_' in asm)

    def test_elim_extra_return(self):
        # RETURN LOAD_CONST None RETURN  -->  RETURN
        def f(x):
            return x
        asm = disassemble(f)
        self.assert_('LOAD_CONST' not in asm)
        self.assert_('(None)' not in asm)
        self.assertEqual(asm.split().count('RETURN_VALUE'), 1)



def test_main(verbose=None):
    import sys
    from test import test_support
    test_classes = (TestTranforms,)
    test_support.run_unittest(*test_classes)

    # verify reference counting
    if verbose and hasattr(sys, "gettotalrefcount"):
        import gc
        counts = [None] * 5
        for i in xrange(len(counts)):
            test_support.run_unittest(*test_classes)
            gc.collect()
            counts[i] = sys.gettotalrefcount()
        print counts

if __name__ == "__main__":
    test_main(verbose=True)
