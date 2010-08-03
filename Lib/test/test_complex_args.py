
import unittest
from test import test_support
import textwrap

class ComplexArgsTestCase(unittest.TestCase):

    def check(self, func, expected, *args):
        self.assertEqual(func(*args), expected)

    # These functions are tested below as lambdas too.  If you add a
    # function test, also add a similar lambda test.

    # Functions are wrapped in "exec" statements in order to
    # silence Py3k warnings.

    def test_func_parens_no_unpacking(self):
        exec textwrap.dedent("""
        def f(((((x))))): return x
        self.check(f, 1, 1)
        # Inner parens are elided, same as: f(x,)
        def f(((x)),): return x
        self.check(f, 2, 2)
        """)

    def test_func_1(self):
        exec textwrap.dedent("""
        def f(((((x),)))): return x
        self.check(f, 3, (3,))
        def f(((((x)),))): return x
        self.check(f, 4, (4,))
        def f(((((x))),)): return x
        self.check(f, 5, (5,))
        def f(((x),)): return x
        self.check(f, 6, (6,))
        """)

    def test_func_2(self):
        exec textwrap.dedent("""
        def f(((((x)),),)): return x
        self.check(f, 2, ((2,),))
        """)

    def test_func_3(self):
        exec textwrap.dedent("""
        def f((((((x)),),),)): return x
        self.check(f, 3, (((3,),),))
        """)

    def test_func_complex(self):
        exec textwrap.dedent("""
        def f((((((x)),),),), a, b, c): return x, a, b, c
        self.check(f, (3, 9, 8, 7), (((3,),),), 9, 8, 7)

        def f(((((((x)),)),),), a, b, c): return x, a, b, c
        self.check(f, (3, 9, 8, 7), (((3,),),), 9, 8, 7)

        def f(a, b, c, ((((((x)),)),),)): return a, b, c, x
        self.check(f, (9, 8, 7, 3), 9, 8, 7, (((3,),),))
        """)

    # Duplicate the tests above, but for lambda.  If you add a lambda test,
    # also add a similar function test above.

    def test_lambda_parens_no_unpacking(self):
        exec textwrap.dedent("""
        f = lambda (((((x))))): x
        self.check(f, 1, 1)
        # Inner parens are elided, same as: f(x,)
        f = lambda ((x)),: x
        self.check(f, 2, 2)
        """)

    def test_lambda_1(self):
        exec textwrap.dedent("""
        f = lambda (((((x),)))): x
        self.check(f, 3, (3,))
        f = lambda (((((x)),))): x
        self.check(f, 4, (4,))
        f = lambda (((((x))),)): x
        self.check(f, 5, (5,))
        f = lambda (((x),)): x
        self.check(f, 6, (6,))
        """)

    def test_lambda_2(self):
        exec textwrap.dedent("""
        f = lambda (((((x)),),)): x
        self.check(f, 2, ((2,),))
        """)

    def test_lambda_3(self):
        exec textwrap.dedent("""
        f = lambda ((((((x)),),),)): x
        self.check(f, 3, (((3,),),))
        """)

    def test_lambda_complex(self):
        exec textwrap.dedent("""
        f = lambda (((((x)),),),), a, b, c: (x, a, b, c)
        self.check(f, (3, 9, 8, 7), (((3,),),), 9, 8, 7)

        f = lambda ((((((x)),)),),), a, b, c: (x, a, b, c)
        self.check(f, (3, 9, 8, 7), (((3,),),), 9, 8, 7)

        f = lambda a, b, c, ((((((x)),)),),): (a, b, c, x)
        self.check(f, (9, 8, 7, 3), 9, 8, 7, (((3,),),))
        """)


def test_main():
    with test_support._check_py3k_warnings(
            ("tuple parameter unpacking has been removed", SyntaxWarning),
            ("parenthesized argument names are invalid", SyntaxWarning)):
        test_support.run_unittest(ComplexArgsTestCase)

if __name__ == "__main__":
    test_main()
