import unittest

class PEP3120Test(unittest.TestCase):

    def test_pep3120(self):
        self.assertEqual(
            "Питон".encode("utf-8"),
            b'\xd0\x9f\xd0\xb8\xd1\x82\xd0\xbe\xd0\xbd'
        )
        self.assertEqual(
            "\П".encode("utf-8"),
            b'\\\xd0\x9f'
        )

    def test_badsyntax(self):
        try:
            import test.tokenizedata.badsyntax_pep3120  # noqa: F401
        except SyntaxError as msg:
            msg = str(msg).lower()
            self.assertTrue('utf-8' in msg)
        else:
            self.fail("expected exception didn't occur")


class BuiltinCompileTests(unittest.TestCase):

    # Issue 3574.
    def test_latin1(self):
        # Allow compile() to read Latin-1 source.
        source_code = '# coding: Latin-1\nu = "Ç"\n'.encode("Latin-1")
        try:
            code = compile(source_code, '<dummy>', 'exec')
        except SyntaxError:
            self.fail("compile() cannot handle Latin-1 source")
        ns = {}
        exec(code, ns)
        self.assertEqual('Ç', ns['u'])


if __name__ == "__main__":
    unittest.main()
