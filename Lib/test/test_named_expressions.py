import os
import unittest


class NamedExpressionInvalidTest(unittest.TestCase):
    def test_named_expression_invalid_01(self):
        code = """x := 0"""

        with self.assertRaisesRegex(SyntaxError, "invalid syntax"):
            exec(code, {}, {})

    def test_named_expression_invalid_02(self):
        code = """x = y := 0"""

        with self.assertRaisesRegex(SyntaxError, "invalid syntax"):
            exec(code, {}, {})

    def test_named_expression_invalid_03(self):
        code = """foo(cat=category := 'vector')"""

        with self.assertRaisesRegex(SyntaxError, "invalid syntax"):
            exec(code, {}, {})

    def test_named_expression_invalid_04(self):
        code = """y := f(x)"""

        with self.assertRaisesRegex(SyntaxError, "invalid syntax"):
            exec(code, {}, {})

    def test_named_expression_invalid_05(self):
        code = """y0 = y1 := f(x)"""

        with self.assertRaisesRegex(SyntaxError, "invalid syntax"):
            exec(code, {}, {})

    def test_named_expression_invalid_06(self):
        code = """foo(x = y := f(x))"""

        with self.assertRaisesRegex(SyntaxError, "invalid syntax"):
            exec(code, {}, {})

    def test_named_expression_invalid_07(self):
        code = """def foo(answer = p := 42): pass"""

        with self.assertRaisesRegex(SyntaxError, "invalid syntax"):
            exec(code, {}, {})

    def test_named_expression_invalid_08(self):
        code = """def foo(answer: p := 42 = 5): pass"""

        with self.assertRaisesRegex(SyntaxError, "invalid syntax"):
            exec(code, {}, {})

    def test_named_expression_invalid_09(self):
        code = """(lambda: x := 1)"""

        with self.assertRaisesRegex(SyntaxError, "can't assign to lambda"):
            exec(code, {}, {})

    def test_named_expression_invalid_10(self):
        code = """z := y := x := 0")  # not in P"""

        with self.assertRaisesRegex(SyntaxError, "invalid syntax"):
            exec(code, {}, {})

    def test_named_expression_invalid_11(self):
        code = """info := (name, phone, *rest)"""

        with self.assertRaisesRegex(SyntaxError, "invalid syntax"):
            exec(code, {}, {})

    def test_named_expression_invalid_12(self):
        code = """[[(j := j) for i in range(5)] for j in range(5)]"""

        with self.assertRaisesRegex(UnboundLocalError, "local variable 'j' referenced before assignment"):
            exec(code, {}, {})

    def test_named_expression_invalid_13(self):
        code = "[i + 1 for i in i := [1,2]]"

        with self.assertRaisesRegex(SyntaxError, "invalid syntax"):
            exec(code, {}, {})


class NamedExpressionAssignmentTest(unittest.TestCase):
    def test_named_expression_assignment_01(self):
        (a := 10)
        self.assertEqual(a, 10)

    def test_named_expression_assignment_02(self):
        a = 20
        (a := a)
        self.assertEqual(a, 20)

    def test_named_expression_assignment_03(self):
        (total := 1 + 2)
        self.assertEqual(total, 3)

    def test_named_expression_assignment_04(self):
        if spam := "eggs":
            self.assertEqual(spam, "eggs")
        else: self.fail("variable was not assigned using named expression")

    def test_named_expression_assignment_05(self):
        if True and (spam := True):
            self.assertTrue(spam)
        else: self.fail("variable was not assigned using named expression")

    def test_named_expression_assignment_06(self):
        if (match := 10) is 10:
            pass
        else: self.fail("variable was not assigned using named expression")

    def test_named_expression_assignment_07(self):
        def spam(a):
            return a
        input_data = [1, 2, 3]
        results = [(x, y, x/y) for x in input_data if (y := spam(x)) > 0]
        self.assertEqual(results, [(1, 1, 1.0), (2, 2, 1.0), (3, 3, 1.0)])

    def test_named_expression_assignment_08(self):
        def spam(a):
            return a
        results = [[y := spam(x), x/y] for x in range(1, 5)]
        self.assertEqual(results, [[1, 1.0], [2, 1.0], [3, 1.0], [4, 1.0]])

    def test_named_expression_assignment_09(self):
        (x := 1, 2)
        self.assertEqual(x, 1)

    def test_named_expression_assignment_10(self):
        (z := (y := (x := 0)))
        self.assertEqual(x, 0)
        self.assertEqual(y, 0)
        self.assertEqual(z, 0)

    def test_named_expression_assignment_11(self):
        (loc := (1, 2))
        self.assertEqual(loc, (1, 2))

    def test_named_expression_assignment_12(self):
        """
        Where all variables are positive integers, and a is at least as large
        as the n'th root of x, this algorithm returns the floor of the n'th
        root of x (and roughly doubling the number of accurate bits per
        iteration)::
        """
        a = 9
        n = 2
        x = 3

        while a > (d := x // a**(n-1)):
            a = ((n-1)*a + d) // n

        self.assertEqual(a, 1)

    @unittest.skip("Not implemented -- No keyword args")
    def test_named_expression_assignment_13(self):
        f = open("../../Lib/test/data/blake2b.txt", "r")
        self.assertEqual(len(lines := f.readlines()), 259)


class NamedExpressionScopeTest(unittest.TestCase):
    def test_named_expression_scope_01(self):
        code = 'def foo():\n\n    (a := 5)\nprint(a)\n'
        with self.assertRaisesRegex(NameError, "name 'a' is not defined"):
            exec(code, {}, {})

    @unittest.skip("Not implemented -- comprehension")
    def test_named_expression_scope_02(self):
        total = 0
        partial_sums = [total := total + v for v in range(5)]
        self.assertEqual(total, 15)

    @unittest.skip("Not implemented -- comprehension")
    def test_named_expression_scope_03(self):
        """
        This test uses an example data file, Lib/test/data/blake2b.txt,
        which starts with a comment followed by a bunch of lines of data.
        Thus, this test relies on the existance of said comment in order to
        pass.
        """
        f = open("../../Lib/test/data/blake2b.txt", "r")

        containsComment = any((comment := line).startswith('#') for line in f.readlines())
        self.assertTrue(containsComment)
        self.assertEqual(comment, "# EOF")

    @unittest.skip("Test WIP, Not implemented -- comprehension")
    def test_named_expression_scope_04(self):
        def spam(a):
            return a
        results = [[y := spam(x), x/y] for x in range(1, 5)]
        self.assertEqual(y, 'something')

    @unittest.skip("Test WIP, Not implemented -- comprehension")
    def test_named_expression_scope_05(self):
        def spam(a):
            return a
        input_data = [1, 2, 3]
        results = [(x, y, x/y) for x in input_data if (y := spam(x)) > 0]
        self.assertEqual(y, 'something')


if __name__ == "__main__":
    unittest.main()
