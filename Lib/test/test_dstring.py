import unittest


class DStringTestCase(unittest.TestCase):
    def assertAllRaise(self, exception_type, regex, error_strings):
        for str in error_strings:
            with self.subTest(str=str):
                with self.assertRaisesRegex(exception_type, regex) as cm:
                    eval(str)
                # print("Testing expression:", repr(str))
                # print(repr(cm.exception))
                # print(repr(cm.exception.text))

    def test_single_quote(self):
        exprs = [
            "d'hello'",
            'D"hello"',
            "d'hello\\nworld'",
        ]
        self.assertAllRaise(SyntaxError, "d-string must be triple-quoted", exprs)

    def test_empty_dstring(self):
        exprs = [
            "d''''''",
            'D""""""',
        ]
        self.assertAllRaise(SyntaxError, "d-string must start with a newline", exprs)

    def test_no_last_newline(self):
        exprs = [
            "d'''\nhello world'''",
            'D"""\nhello world"""',
            "df'''\nhello {42}'''",
        ]
        self.assertAllRaise(SyntaxError, "d-string must end with an indent line", exprs)

    def test_simple_dstring(self):
        self.assertEqual(eval('d"""\n  hello world\n  """'), "hello world\n")
        self.assertEqual(eval('d"""\n  hello world\n """'), " hello world\n")
        self.assertEqual(eval('d"""\n  hello world\n"""'), "  hello world\n")
        self.assertEqual(eval('d"""\n  hello world\\\n """'), " hello world")
        self.assertEqual(eval('dr"""\n  hello world\\\n """'), " hello world\\\n")



if __name__ == '__main__':
    unittest.main()
