import unittest
from test.test_tools import skip_if_missing, imports_under_tool
from test import support
from test.support.hypothesis_helper import hypothesis

st = hypothesis.strategies
given = hypothesis.given
example = hypothesis.example


skip_if_missing("unicode")
with imports_under_tool("unicode"):
    from dawg import Dawg, build_compression_dawg, lookup, inverse_lookup


@st.composite
def char_name_db(draw, min_length=1, max_length=30):
    m = draw(st.integers(min_value=min_length, max_value=max_length))
    names = draw(
        st.sets(st.text("abcd", min_size=1, max_size=10), min_size=m, max_size=m)
    )
    characters = draw(st.sets(st.characters(), min_size=m, max_size=m))
    return list(zip(names, characters))


class TestDawg(unittest.TestCase):
    """Tests for the directed acyclic word graph data structure that is used
    to store the unicode character names in unicodedata. Tests ported from PyPy
    """

    def test_dawg_direct_simple(self):
        dawg = Dawg()
        dawg.insert("a", -4)
        dawg.insert("c", -2)
        dawg.insert("cat", -1)
        dawg.insert("catarr", 0)
        dawg.insert("catnip", 1)
        dawg.insert("zcatnip", 5)
        packed, data, inverse = dawg.finish()

        self.assertEqual(lookup(packed, data, b"a"), -4)
        self.assertEqual(lookup(packed, data, b"c"), -2)
        self.assertEqual(lookup(packed, data, b"cat"), -1)
        self.assertEqual(lookup(packed, data, b"catarr"), 0)
        self.assertEqual(lookup(packed, data, b"catnip"), 1)
        self.assertEqual(lookup(packed, data, b"zcatnip"), 5)
        self.assertRaises(KeyError, lookup, packed, data, b"b")
        self.assertRaises(KeyError, lookup, packed, data, b"catni")
        self.assertRaises(KeyError, lookup, packed, data, b"catnipp")

        self.assertEqual(inverse_lookup(packed, inverse, -4), b"a")
        self.assertEqual(inverse_lookup(packed, inverse, -2), b"c")
        self.assertEqual(inverse_lookup(packed, inverse, -1), b"cat")
        self.assertEqual(inverse_lookup(packed, inverse, 0), b"catarr")
        self.assertEqual(inverse_lookup(packed, inverse, 1), b"catnip")
        self.assertEqual(inverse_lookup(packed, inverse, 5), b"zcatnip")
        self.assertRaises(KeyError, inverse_lookup, packed, inverse, 12)

    def test_forbid_empty_dawg(self):
        dawg = Dawg()
        self.assertRaises(ValueError, dawg.finish)

    @given(char_name_db())
    @example([("abc", "a"), ("abd", "b")])
    @example(
        [
            ("bab", "1"),
            ("a", ":"),
            ("ad", "@"),
            ("b", "<"),
            ("aacc", "?"),
            ("dab", "D"),
            ("aa", "0"),
            ("ab", "F"),
            ("aaa", "7"),
            ("cbd", "="),
            ("abad", ";"),
            ("ac", "B"),
            ("abb", "4"),
            ("bb", "2"),
            ("aab", "9"),
            ("caaaaba", "E"),
            ("ca", ">"),
            ("bbaaa", "5"),
            ("d", "3"),
            ("baac", "8"),
            ("c", "6"),
            ("ba", "A"),
        ]
    )
    @example(
        [
            ("bcdac", "9"),
            ("acc", "g"),
            ("d", "d"),
            ("daabdda", "0"),
            ("aba", ";"),
            ("c", "6"),
            ("aa", "7"),
            ("abbd", "c"),
            ("badbd", "?"),
            ("bbd", "f"),
            ("cc", "@"),
            ("bb", "8"),
            ("daca", ">"),
            ("ba", ":"),
            ("baac", "3"),
            ("dbdddac", "a"),
            ("a", "2"),
            ("cabd", "b"),
            ("b", "="),
            ("abd", "4"),
            ("adcbd", "5"),
            ("abc", "e"),
            ("ab", "1"),
        ]
    )
    def test_dawg(self, data):
        # suppress debug prints
        with support.captured_stdout() as output:
            # it's enough to build it, building will also check the result
            build_compression_dawg(data)
