import keyword
import unittest


class Test_iskeyword(unittest.TestCase):
    def test_true_is_a_keyword(self):
        self.assertTrue(keyword.iskeyword('True'))

    def test_uppercase_true_is_not_a_keyword(self):
        self.assertFalse(keyword.iskeyword('TRUE'))

    def test_none_value_is_not_a_keyword(self):
        self.assertFalse(keyword.iskeyword(None))

    # This is probably an accident of the current implementation, but should be
    # preserved for backward compatibility.
    def test_changing_the_kwlist_does_not_affect_iskeyword(self):
        oldlist = keyword.kwlist
        self.addCleanup(setattr, keyword, 'kwlist', oldlist)
        keyword.kwlist = ['its', 'all', 'eggs', 'beans', 'and', 'a', 'slice']
        self.assertFalse(keyword.iskeyword('eggs'))

    def test_changing_the_softkwlist_does_not_affect_issoftkeyword(self):
        oldlist = keyword.softkwlist
        self.addCleanup(setattr, keyword, "softkwlist", oldlist)
        keyword.softkwlist = ["foo", "bar", "spam", "egs", "case"]
        self.assertFalse(keyword.issoftkeyword("spam"))

    def test_all_keywords_fail_to_be_used_as_names(self):
        for key in keyword.kwlist:
            with self.assertRaises(SyntaxError):
                exec(f"{key} = 42")

    def test_all_soft_keywords_can_be_used_as_names(self):
        for key in keyword.softkwlist:
            exec(f"{key} = 42")

    def test_async_and_await_are_keywords(self):
        self.assertIn("async", keyword.kwlist)
        self.assertIn("await", keyword.kwlist)

    def test_soft_keywords(self):
        self.assertIn("type", keyword.softkwlist)
        self.assertIn("match", keyword.softkwlist)
        self.assertIn("case", keyword.softkwlist)
        self.assertIn("_", keyword.softkwlist)

    def test_keywords_are_sorted(self):
        self.assertListEqual(sorted(keyword.kwlist), keyword.kwlist)

    def test_softkeywords_are_sorted(self):
        self.assertListEqual(sorted(keyword.softkwlist), keyword.softkwlist)


if __name__ == "__main__":
    unittest.main()
