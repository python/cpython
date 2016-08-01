import unittest
from collections import namedtuple

Pair = namedtuple('Pair', ['big', 'small'])


def combos(op1, op2):
    return (
        (op1.big, op2.small),
        (op2.small, op1.big),
        (op2.big, op1.small),
        (op1.small, op2.big)
    )


class Generator:
    def __init__(self, big, small):
        self._big = big
        self._small = small

    @property
    def big(self):
        return (i for i in self._big)

    @property
    def small(self):
        return (i for i in self._small)


# noinspection PyTypeChecker
class TestDictViewIntersection(unittest.TestCase):

    def intersectionAssertRaises(self, op1, op2, exception):
        for left, right in combos(op1, op2):
            with self.assertRaises(exception):
                val = left & right

    def intersectionAssertEqual(self, op1, op2, expected):
        for left, right in combos(op1, op2):
            self.assertEqual(left & right, expected)



    def setUp(self):
        print("\nIn method", self._testMethodName)

        self.dict = Pair(
            big={1: 1, 2: 2, 3: 3},
            small={1: 1}
        )

        self.dict_keys = Pair(
            big=self.dict.big.keys(),
            small=self.dict.small.keys()
        )

        self.dict_values = Pair(
            big=self.dict.big.values(),
            small=self.dict.small.values()
        )

        self.dict_items = Pair(
            big=self.dict.big.items(),
            small=self.dict.small.items()
        )

        self.set = Pair(
            big={1, 2, 3},
            small={1}
        )

        self.list = Pair(
            big=[1, 2, 3],
            small=[1]
        )

        self.generator = Generator(self.list.big, self.list.small)


    def test001_dict_dict(self):
        """Should raise exception"""
        self.intersectionAssertRaises(self.dict, self.dict, TypeError)

    def test002_dict_dict_keys(self):
        """Should be slow. Instead should use dict.keys() & dict.keys()"""
        self.intersectionAssertEqual(self.dict, self.dict_keys, {1})

    def test003_dict_dict_values(self):
        """Should raise exception"""
        self.intersectionAssertRaises(self.dict, self.dict_values, TypeError)

    def test004_dict_dict_items(self):
        """Should be slow. Instead should use dict.keys() & dict.items()"""
        self.intersectionAssertEqual(Pair(
            big={(1, 1): 1, (2, 2): 2, (3, 3): 3},
            small={(1, 1): 1}
        ), self.dict_items, {(1, 1)})

    def test005_dict_set(self):
        """Should raise exception"""
        self.intersectionAssertRaises(self.dict, self.set, TypeError)

    def test006_dict_list(self):
        """Should raise exception"""
        self.intersectionAssertRaises(self.dict, self.list, TypeError)

    def test007_dict_generator(self):
        """Should raise exception"""
        self.intersectionAssertRaises(self.dict, self.generator, TypeError)

    def test008_dict_keys_dict_keys(self):
        """Should be fast"""
        self.intersectionAssertEqual(self.dict_keys, self.dict_keys, {1})

    def test009_dict_keys_dict_values(self):
        """Should be slow"""
        self.intersectionAssertEqual(self.dict_keys, self.dict_values, {1})

    def test010_dict_keys_dict_items(self):
        """Should be fast"""
        self.intersectionAssertEqual(Pair(
            big={(1, 1): 1, (2, 2): 2, (3, 3): 3}.keys(),
            small={(1, 1): 1}.keys()
        ), self.dict_items, {(1, 1)})

    def test011_dict_keys_set(self):
        """Should be fast"""
        self.intersectionAssertEqual(self.dict_keys, self.set, {1})

    def test012_dict_keys_list(self):
        """Should be slow"""
        self.intersectionAssertEqual(self.dict_keys, self.list, {1})

    def test013_dict_keys_generator(self):
        """Should be slow"""
        self.intersectionAssertEqual(self.dict_keys, self.generator, {1})

    def test014_dict_values_dict_values(self):
        """Should raise exception"""
        self.intersectionAssertRaises(self.dict_values, self.dict_values, TypeError)

    def test015_dict_values_dict_items(self):
        """Should be slow"""
        self.intersectionAssertEqual(Pair(
            big={1: (1, 1), 2: (2, 2), 3: (3, 3)}.values(),
            small={1: (1, 1)}.values()
        ), self.dict_items, {(1, 1)})

    def test016_dict_values_set(self):
        """Should raise exception"""
        self.intersectionAssertRaises(self.dict_values, self.set, TypeError)

    def test017_dict_values_list(self):
        """Should be raise exception"""
        self.intersectionAssertRaises(self.dict_values, self.list, TypeError)

    def test018_dict_values_generator(self):
        """Should raise exception"""
        self.intersectionAssertRaises(self.dict_values, self.generator, TypeError)

    def test019_dict_items_dict_items(self):
        """Should be fast"""
        self.intersectionAssertEqual(self.dict_items, self.dict_items, {(1, 1)})

    def test020_dict_items_set(self):
        """Should be fast"""
        self.intersectionAssertEqual(
            self.dict_items,
            Pair(
                big={(1, 1), (2, 2), (3, 3)},
                small={(1, 1)}
            ),
            {(1, 1)}
        )

    def test021_dict_items_list(self):
        """Should be slow"""
        self.intersectionAssertEqual(
            self.dict_items,
            Pair(
                big=[(1, 1), (2, 2), (3, 3)],
                small=[(1, 1)]
            ),
            {(1, 1)}
        )

    def test022_dict_items_generator(self):
        """Should be slow"""
        self.intersectionAssertEqual(
            self.dict_items,
            Generator(
                big=[(1, 1), (2, 2), (3, 3)],
                small=[(1, 1)]
            ),
            {(1, 1)}
        )

    def test023_set_set(self):
        """Should be fast. Outside of scope of fix."""
        self.intersectionAssertEqual(self.set, self.set, {1})

    def test024_set_list(self):
        """Should raise exception. Outside of scope of fix."""
        self.intersectionAssertRaises(self.set, self.list, TypeError)

    def test025_set_generator(self):
        """Should raise exception. Outside of scope of fix."""
        self.intersectionAssertRaises(self.set, self.generator, TypeError)

    def test026_list_list(self):
        """Should raise exception. Outside of scope of fix."""
        self.intersectionAssertRaises(self.list, self.list, TypeError)

    def test027_list_generator(self):
        """Should raise exception. Outside of scope of fix."""
        self.intersectionAssertRaises(self.list, self.generator, TypeError)

    def test028_generator_generator(self):
        """Should raise exception. Outside of scope of fix."""
        self.intersectionAssertRaises(self.generator, self.generator, TypeError)


if __name__ == '__main__':
    unittest.main()

