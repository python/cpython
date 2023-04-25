import unittest

class TagStringTests(unittest.TestCase):

    def test_basic(self):
        def tag(*args):
            return "Spam"
        self.assertEqual(tag"ham", "Spam")
        self.assertEqual(tag"...{ham}...{eggs}...", "Spam")

    def test_tag_call(self):
        def tag(*args):
            return args
        self.assertEqual(tag"ham", ("ham",))
        res = tag"... {ham and eggs} ..."
        self.assertEqual(len(res), 3)
        self.assertEqual(res[0], "... ")
        func, string, conv, spec = res[1]
        ham = 42
        eggs = "delicious"
        self.assertEqual(func(), "delicious")
        self.assertEqual(string, "ham and eggs")
        self.assertEqual(conv, None)
        self.assertEqual(spec, None)
        self.assertEqual(res[2], " ...")

    def test_tag_call_with_conversion(self):
        def tag(*args):
            return args
        res = tag"{ham!r:spec}"
        func, string, conv, spec = res[0]
        ham = 42
        self.assertEqual(func(), 42)
        self.assertEqual(string, "ham")
        self.assertEqual(conv, "r")
        self.assertEqual(spec, "spec")

    def test_disallow_joins(self):
        examples = [
            "tag'foo' tag'bar'",
            "tag'foo' 'bar'",
            "tag'foo' f'bar'",
            "'foo' tag'bar'",
            "b'foo' tag'bar'",
            "r'foo' tag'bar' rb'baz'",
        ]
        for ex in examples:
            with self.assertRaises(SyntaxError):
                compile(ex, ex, "eval")


if __name__ == "__main__":
    unittest.main()
