
import unittest

class NamedExpressionSyntaxTest(unittest.TestCase):

    def test_named_expression_syntax_01(self):
        code = '''x = y := 0'''

        with self.assertRaisesRegex(SyntaxError, 'invalid syntax'):
            exec(code, {}, {})

    def test_named_expression_syntax_02(self):
        code = '''foo(cat=category := 'vector')'''

        with self.assertRaisesRegex(SyntaxError, 'invalid syntax'):
            exec(code, {}, {})

    def test_named_expression_syntax_03(self):
        code = '''y := f(x)'''

        with self.assertRaisesRegex(SyntaxError, 'invalid syntax'):
            exec(code, {}, {})

    def test_named_expression_syntax_04(self):
        code = '''y0 = y1 := f(x)'''

        with self.assertRaisesRegex(SyntaxError, 'invalid syntax'):
            exec(code, {}, {})

    def test_named_expression_syntax_05(self):
        code = '''foo(x = y := f(x))'''

        with self.assertRaisesRegex(SyntaxError, 'invalid syntax'):
            exec(code, {}, {})

    def test_named_expression_syntax_06(self):
        code = '''def foo(answer = p := 42): pass'''

        with self.assertRaisesRegex(SyntaxError, 'invalid syntax'):
            exec(code, {}, {})

    def test_named_expression_syntax_07(self):
        code = '''def foo(answer: p := 42 = 5): pass'''

        with self.assertRaisesRegex(SyntaxError, 'invalid syntax'):
            exec(code, {}, {})

    def test_named_expression_syntax_08(self):
        code = '''lambda: x := 1)'''

        with self.assertRaisesRegex(SyntaxError, 'invalid syntax'):
            exec(code, {}, {})

    def test_named_expression_syntax_09(self):
        code = '''z := y := x := 0")  # not in P'''

        with self.assertRaisesRegex(SyntaxError, 'invalid syntax'):
            exec(code, {}, {})

    def test_named_expression_syntax_10(self):
        code = '''loc:=(x,y))'''

        with self.assertRaisesRegex(SyntaxError, 'invalid syntax'):
            exec(code, {}, {})

    def test_named_expression_syntax_11(self):
        code = '''info := (name, phone, *rest)'''

        with self.assertRaisesRegex(SyntaxError, 'invalid syntax'):
            exec(code, {}, {})



if __name__ == "__main__":
    unittest.main()
